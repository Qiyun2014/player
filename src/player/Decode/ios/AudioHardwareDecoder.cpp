//
// Created by qiyun on 2019/11/21.
//

#include "AudioHardwareDecoder.h"
#include <AudioToolbox/AudioFormat.h>
#include <math.h>

namespace XQAudioDecoder {

    static struct fillComplexBufferInputProc_t {
        AudioBufferList *bufferList;
        UInt32 frames;
        Boolean equal_sample_rate;
    };

    static OSStatus LLAudioConverterComplexInputDataProc( AudioConverterRef               inAudioConverter, UInt32 *                        ioNumberDataPackets,
                                                          AudioBufferList *               ioData, AudioStreamPacketDescription * __nullable * __nullable outDataPacketDescription, void * __nullable               inUserData) {

        struct fillComplexBufferInputProc_t *arg = (struct fillComplexBufferInputProc_t *) inUserData;
        ioData->mBuffers[0].mData           = arg->bufferList->mBuffers[0].mData;
        ioData->mBuffers[0].mDataByteSize   = arg->bufferList->mBuffers[0].mDataByteSize;
        ioData->mBuffers[0].mNumberChannels = arg->bufferList->mBuffers[0].mNumberChannels;
        if(arg->equal_sample_rate == FALSE){
            *ioNumberDataPackets = (arg->bufferList->mBuffers[0].mDataByteSize/2);
        }else{
            *ioNumberDataPackets = ((*ioNumberDataPackets < 1024)? (*ioNumberDataPackets) : 1024);
        }
        return noErr;
    }


    AudioStreamBasicDescription OutputDefaultAudioDescription() {
        AudioStreamBasicDescription basicDescription;
        memset(&basicDescription, 0, sizeof(basicDescription));
        basicDescription.mSampleRate        = 44100;
        basicDescription.mFormatID          = kAudioFormatLinearPCM;
        basicDescription.mFormatFlags       = kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked;
        basicDescription.mBytesPerPacket    = basicDescription.mBytesPerFrame * basicDescription.mFramesPerPacket;
        basicDescription.mBytesPerFrame     = basicDescription.mBitsPerChannel / 8 * basicDescription.mChannelsPerFrame;
        basicDescription.mFramesPerPacket   = 1;
        basicDescription.mChannelsPerFrame  = 1;
        basicDescription.mBitsPerChannel    = 16;
        basicDescription.mReserved          = 0;
        return basicDescription;
    };


    AudioStreamBasicDescription InputAACDefaultAudioDescription(int sr, int fp, int channel) {
        AudioStreamBasicDescription basicDescription;
        memset(&basicDescription, 0, sizeof(basicDescription));
        basicDescription.mSampleRate        = sr;
        basicDescription.mFormatID          = kAudioFormatMPEG4AAC;
        basicDescription.mFormatFlags       = kMPEG4Object_AAC_LC;
        basicDescription.mFramesPerPacket   = 1024;
        basicDescription.mBytesPerPacket    = basicDescription.mBytesPerFrame * basicDescription.mFramesPerPacket;
        basicDescription.mBytesPerFrame     = basicDescription.mBitsPerChannel / 8 * basicDescription.mChannelsPerFrame;
        return basicDescription;
    };


    auto AudioHardwareDecoder::AudioHardWardWithCreateConverter() -> boolean_t {

        UInt32 outputBitrate = 96 * 1024; // 96 kbps
        UInt32 propSize = sizeof(outputBitrate);

        AudioClassDescription inClassDescriptions[2] = {
                {
                        kAudioEncoderComponentType,
                        kAudioFormatMPEG4AAC,
#if TARGET_OS_IPHONE
                        kAppleSoftwareAudioCodecManufacturer
#else
                        'appl'
#endif
                },
                {
                        kAudioEncoderComponentType,
                        kAudioFormatMPEG4AAC,
#if TARGET_OS_IPHONE
                        kAppleHardwareAudioCodecManufacturer
#else
                        'aphw'
#endif
                }
        };

        OSStatus err = AudioConverterNewSpecific(&inputBasicDescription, &outputBasicDescription, 2, inClassDescriptions, &_audioConverter);
        if (err || _audioConverter == NULL) {
            printf("AudioConverterNewSpecific Failed \n");
            return false;
        }

        // 不同采用率
        if (inputBasicDescription.mSampleRate != outputBasicDescription.mSampleRate) {
            UInt32 prop = kAudioConverterSampleRateConverterComplexity_Mastering;
            err = AudioConverterSetProperty(_audioConverter, kAudioConverterSampleRateConverterComplexity, sizeof(prop), &prop);
            if (err) {
                printf("AudioConverterSetProperty (kAudioConverterSampleRateConverterComplexity_Mastering) ... Failed \n");
                return false;
            }

            prop = kAudioConverterQuality_Max;
            err = AudioConverterSetProperty(_audioConverter, kAudioConverterSampleRateConverterQuality, sizeof(prop), &prop);
            if (err) {
                printf("AudioConverterSetProperty (kAudioConverterSampleRateConverterQuality) ... Failed \n");
                return false;
            }

            prop = kConverterPrimeMethod_None;
            err = AudioConverterSetProperty(_audioConverter, kAudioConverterPrimeMethod, sizeof(prop), &prop);
            if (err) {
                printf("AudioConverterSetProperty (kAudioConverterPrimeMethod) ... Failed \n");
                return false;
            }
            equal_sample_rate = false;
        } else {
            equal_sample_rate = true;
        }

        err = AudioConverterGetProperty(_audioConverter, kAudioConverterPropertyMaximumOutputPacketSize, &propSize, &_outputDataSize);
        if (err) {
            printf("AudioConverterGetProperty (kAudioConverterPropertyMaximumOutputPacketSize) ... Failed \n");
            return false;
        }
        return true;
    }


    auto AudioHardwareDecoder::ReceiveAudioBuffer(const void *audioData, size_t size, int64_t ts) -> void {
        if (audioData == NULL || size <= 0 || _audioConverter == NULL || _outputAudioBuffer == NULL) return;

        AudioBufferList outBuffer;
        outBuffer.mNumberBuffers                = 1;
        outBuffer.mBuffers[0].mDataByteSize     = _outputDataSize;
        outBuffer.mBuffers[0].mData             = _outputAudioBuffer;
        outBuffer.mBuffers[0].mNumberChannels   = outputBasicDescription.mChannelsPerFrame;

        AudioBufferList inBuffer;
        inBuffer.mNumberBuffers                 = 1;
        inBuffer.mBuffers[0].mDataByteSize      = (UInt32)size;
        inBuffer.mBuffers[0].mData              = (void*)audioData;
        inBuffer.mBuffers[0].mNumberChannels    = inputBasicDescription.mChannelsPerFrame;

        UInt32 frames = 1;
        AudioStreamPacketDescription outPacketDescription[frames];
        struct fillComplexBufferInputProc_t inputProc = {.bufferList = &inBuffer, .frames = frames, .equal_sample_rate = has_send_audio_specific};
        OSStatus err = AudioConverterFillComplexBuffer(_audioConverter, LLAudioConverterComplexInputDataProc, &inputProc, &frames, &outBuffer, outPacketDescription);
        if (err) {
            printf("AudioConverterFillComplexBuffer failed .... \n");
        } else {
            if (!has_send_audio_specific) {
                uint8_t audio_specific_config[2];
                audio_specific_config[0] = 0x10 | ((4 >> 1) & 0x3);
                audio_specific_config[1] = ((4 & 0x1) << 7) | ((outputBasicDescription.mChannelsPerFrame & 0xF) << 3);

                if (DecodeCompletedWithAudioBufferHandler) {
                    DecodeCompletedWithAudioBufferHandler(audio_specific_config, 2, ts);
                }
                // audio_specific_config  2
                printf("AudioHardware decoder send audio header ... \n");
            }

            if (DecodeCompletedWithAudioBufferHandler) {
                DecodeCompletedWithAudioBufferHandler(_outputAudioBuffer, outPacketDescription[0].mDataByteSize, ts);
            }
            //  _outputAudioBuffer  _outputAudioBuffer
            printf("AudioHardware decoder audio buffer ... \n");
        }
    }


    AudioHardwareDecoder::AudioHardwareDecoder(int sampleRate, int framePerPacket, int channel) {
        this->sampleRate        = sampleRate;
        this->framePerPacket    = framePerPacket;
        this->channel           = channel;
        this->inputBasicDescription     = InputAACDefaultAudioDescription(sampleRate, framePerPacket, channel);
        this->outputBasicDescription    = OutputDefaultAudioDescription();
        this->_outputAudioBuffer        = (uint8_t *) malloc(4096 * sizeof(uint8_t));

        boolean_t success = AudioHardWardWithCreateConverter();
        if (!success) {
            AudioConverterDispose(_audioConverter);
        }
    }

    AudioHardwareDecoder::~AudioHardwareDecoder() {

        if (_audioConverter) {
            AudioConverterDispose(_audioConverter);
        }

        if (_outputAudioBuffer) {
            free(_outputAudioBuffer);
        }
    }
}
