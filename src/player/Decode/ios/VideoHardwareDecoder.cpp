//
// Created by qiyun on 2019/11/19.
//

#include "VideoHardwareDecoder.h"

namespace VideoDecoder {

    // 构造函数
    VideoHardwareDecoder::VideoHardwareDecoder(int width, int height) {
        this->width = width;
        this->height = height;
        // 检错锁，如果同一个线程请求同一个锁，则返回EDEADLK，否则与PTHREAD_MUTEX_TIMED_NP类型动作相同
        pthread_mutexattr_init(&mutexattr);
        pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_ERRORCHECK);
        pthread_mutex_init(&_mutex, &mutexattr);
    }

    // 析构函数
    VideoHardwareDecoder::~VideoHardwareDecoder() {
        pthread_mutexattr_destroy(&mutexattr);
        pthread_mutex_destroy(&_mutex);
        if (decompressionSession != NULL) {
            VTDecompressionSessionInvalidate(decompressionSession);
        }
    }

    // Create video format description and decode parameters
    auto VideoFormatDescriptionForDecodeParameters(VideoDecoderType type, std::string sps, std::string pps, std::string vps) -> CMFormatDescriptionRef {
        OSStatus status;
        CMFormatDescriptionRef formatDescription;

        // Set sps/ps information, decoder to use parse header for packet
        const uint8_t * parameterSetPointers[3] = {
                (const uint8_t *) sps.c_str(),
                (const uint8_t *) pps.c_str(),
                (const uint8_t *) vps.c_str()
        };
        // Set sps/ps size
        const size_t parameterSetSizes[3] = {
                (size_t) sps.length(),
                (size_t) pps.length(),
                (size_t) vps.length()
        };
        // Set decoder parameters
        switch (type) {
            case H264_Decoder: {
                if (VTIsHardwareDecodeSupported(kCMVideoCodecType_H264)) {
                    status = CMVideoFormatDescriptionCreateFromH264ParameterSets(NULL, 2, parameterSetPointers, parameterSetSizes, 4, &formatDescription);
                } else {
                    status = -1;
                }
            }
                break;
            case HEVC_Decoder: {
                if (VTIsHardwareDecodeSupported(kCMVideoCodecType_HEVC)) {
                    status = CMVideoFormatDescriptionCreateFromHEVCParameterSets(NULL, 3, parameterSetPointers, parameterSetSizes, 4, NULL, &formatDescription);
                } else {
                    status = -1;
                }
            }
                break;
            default:
                break;
        }
        return (status == noErr) ? formatDescription : NULL;
    }


    auto VideoHardwareDecoder::DefaultDestinationAttributes(int width, int height) -> CFMutableDictionaryRef {
        // Set the pixel attributes for the destination buffer
        CFMutableDictionaryRef desAttrs = CFDictionaryCreateMutable(
                NULL, // CFAllocatorRef allocator
                0,    // CFIndex capacity
                &kCFTypeDictionaryKeyCallBacks,
                &kCFTypeDictionaryValueCallBacks);

        SInt32 destinationPixelType = kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange;
        CFDictionarySetValue(desAttrs, kCVPixelBufferPixelFormatTypeKey, CFNumberCreate(NULL, kCFNumberSInt32Type, &destinationPixelType));
        CFDictionarySetValue(desAttrs, kCVPixelBufferWidthKey, CFNumberCreate(NULL, kCFNumberSInt32Type, &width));
        CFDictionarySetValue(desAttrs, kCVPixelBufferHeightKey, CFNumberCreate(NULL, kCFNumberSInt32Type, &height));
        CFDictionarySetValue(desAttrs, kCVPixelBufferOpenGLCompatibilityKey, kCFBooleanTrue);
        return desAttrs;
    }


    /*
     *   uint8_t* frame = packet.data;
         int size = packet.size;
         int startIndex = 4; // 数据都从第5位开始
         int nalu_type = ((uint8_t)frame[startIndex] & 0x1F);
          // 1为IDR，5为no-IDR
          if (nalu_type == 1 || nalu_type == 5) { }
     * */
    auto VideoHardwareDecoder::DecodeMediaData(CMFormatDescriptionRef *formatDescription, void **dataPtr, int *size, CMSampleBufferRef *sampleBuffer) -> Boolean {
        CMBlockBufferRef blockBuffer;
        OSStatus status = CMBlockBufferCreateWithMemoryBlock(NULL, *dataPtr, *size, kCFAllocatorNull, NULL, 0, *size, 0, &blockBuffer);
        if (status != noErr) {
            return false;
        }
        const size_t sampleSizeArray[] = {(size_t) *size};
        status = CMSampleBufferCreateReady( NULL, blockBuffer, *formatDescription, 1, 0, NULL, 1, sampleSizeArray, sampleBuffer);
        if (status == noErr && decompressionSession) {
            VTDecodeFrameFlags decodeFrameFlags = kVTDecodeFrame_EnableAsynchronousDecompression;
            // Last decode of result
            VTDecompressionOutputHandler decompressionOutputHandler = ^(
                    OSStatus status,
                    VTDecodeInfoFlags infoFlags,
                    CM_NULLABLE CVImageBufferRef imageBuffer,
                    CMTime presentationTimeStamp,
                    CMTime presentationDuration ) {

                printf("解码成功 ...\n" );
                if (VideoDecoderHandler) {
                    CVPixelBufferLockBaseAddress(imageBuffer, 0);
                    size_t bytesPerRow      = CVPixelBufferGetBytesPerRow(imageBuffer);
                    size_t height           = CVPixelBufferGetHeight(imageBuffer);
                    u_int8_t *baseAddress   = (u_int8_t *)malloc(bytesPerRow * height);
                    memcpy(baseAddress, CVPixelBufferGetBaseAddress(imageBuffer), bytesPerRow * height);
                    CVPixelBufferUnlockBaseAddress(imageBuffer, 0);
                    // Output decoded data to callback func
                    VideoDecoderHandler(baseAddress, bytesPerRow * height, 0);
                    free(baseAddress);
                }
            };
            // Starting decode， and add callback notify
            VTDecompressionSessionDecodeFrameWithOutputHandler(decompressionSession, *sampleBuffer, decodeFrameFlags, NULL, decompressionOutputHandler);
        }
    }


    auto VideoHardwareDecoder::SetDecoderType(VideoDecoderType type) -> Boolean {
        pthread_mutex_lock(&_mutex);
        OSStatus status = noErr;
        if (_decodeType != type) {
            CMVideoFormatDescriptionRef inFormatDescription = VideoFormatDescriptionForDecodeParameters(type, "1111", "2222", "3333");
            CFMutableDictionaryRef attributeRef = DefaultDestinationAttributes(width, height);
            // 输入编码信息
            CFMutableDictionaryRef decoderParameters = CFDictionaryCreateMutable(NULL, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
            CFDictionarySetValue(decoderParameters, kVTDecompressionPropertyKey_RealTime, kCFBooleanTrue);
            // 创建解码会话实例对象
            status = VTDecompressionSessionCreate(NULL, inFormatDescription, decoderParameters, attributeRef, NULL, &decompressionSession);
            CFRelease(attributeRef);
            CFRelease(decoderParameters);
        }
        _decodeType = type;
        pthread_mutex_unlock(&_mutex);
        return (status == noErr);
    }


    /**
     * H264码流第一个NALU是SPS(Sequence Parameter Set), 后面是 PPS(Picture Parameter Set) 和 IDR(Immediately Decoder Refresh)
     * */
    auto VideoHardwareDecoder::ParserParamaterSets(const AVCodecContext *codecContext, uint8_t **sps_data, uint8_t **pps_data) {
        /*
        AVBSFContext *bsfcCtx = NULL;
        const AVBitStreamFilter *absf = av_bsf_get_by_name((type == VIDEO_HARDWARE_H264) ? "h264_mp4toannexb" : "h265_mp4toannexb");;
        av_bsf_alloc(absf, &bsfcCtx);
        avcodec_parameters_copy()
        */
        int sps_start_idx, sps_size;
        int pps_start_idx, pps_size;

        uint8_t  *extradata = codecContext->extradata;
        for (int i = 0; i < codecContext->extradata_size; i ++) {
            if (extradata[i] == 0x67 && extradata[i - 2] == 0) {
                sps_start_idx = i;
                sps_size = extradata[i - 1];
            } else if (extradata[i] == 0x68 && extradata[i-2] == 0) {
                pps_start_idx = i;
                pps_size = extradata[i - 1];
            }
        }
        //uint8_t *sps_data = (uint8_t *) malloc(sps_size * sizeof(uint8_t));
        //uint8_t *pps_data = (uint8_t *) malloc(pps_size * sizeof(uint8_t));
        memcpy(*sps_data, extradata + sps_start_idx, sps_size);
        memcpy(*pps_data, extradata + pps_start_idx, pps_size);
    }

}