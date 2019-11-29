//
// Created by qiyun on 2019/11/21.
//

#ifndef LLPLAYER_AUDIOHARDWAREDECODER_H
#define LLPLAYER_AUDIOHARDWAREDECODER_H

#include <AudioToolbox/AudioToolbox.h>
#include "../AudioDecoder.h"

namespace AudioDecoder {

    class AudioHardwareDecoder : AudioDecoderManager::AudioDecoder {
        AudioHardwareDecoder(int sampleRate, int framePerPacket, int channel);
        ~AudioHardwareDecoder();

        auto AudioHardWardWithCreateConverter() -> boolean_t;

        // Input need to decode of data
        auto ReceiveAudioBuffer(const void *audioData, size_t size, int64_t ts) -> void ;

        // Decode finished of callback
        auto (^DecodeCompletedWithAudioBufferHandler) (uint8_t *audioData, UInt32 size, int64_t ts) -> void;

    private:
        int sampleRate = 44100;
        int framePerPacket = 1024;
        int channel = 2;
        Boolean equal_sample_rate = TRUE;
        Boolean has_send_audio_specific = FALSE;

        UInt32 _outputDataSize;
        uint8_t *_outputAudioBuffer;

        AudioConverterRef   _audioConverter;
        AudioStreamBasicDescription inputBasicDescription;
        AudioStreamBasicDescription outputBasicDescription;
    };
}


#endif //LLPLAYER_AUDIOHARDWAREDECODER_H
