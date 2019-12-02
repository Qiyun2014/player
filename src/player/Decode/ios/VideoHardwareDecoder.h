//
// Created by qiyun on 2019/11/19.
//

#ifndef LLPLAYER_VIDEOHARDWAREDECODER_H
#define LLPLAYER_VIDEOHARDWAREDECODER_H

#include <MacTypes.h>
#include <iostream>
#include <string>
#include <VideoToolbox/VideoToolbox.h>
#include <pthread.h>
#include "../VideoDecoder.h"

namespace XQVideoDecoder {

    class VideoHardwareDecoder : public VideoDecoder {
        VideoHardwareDecoder(int width, int height);
        ~VideoHardwareDecoder();

    public:
        // Get default output video format description
        auto DefaultDestinationAttributes(int width, int height) -> CFMutableDictionaryRef;

        // Exchange h264 and hevc decoder
        auto SetDecoderType(VideoDecoderType type) -> Boolean ;

        // Decode media data
        auto DecodeMediaData(CMFormatDescriptionRef *formatDescription, void **dataPtr, int *size, CMSampleBufferRef *sampleBuffer) -> Boolean;

        // Get sps/pps from AVCodecContext
        auto ParserParamaterSets(const AVCodecContext *codecContext, uint8_t **sps_data, uint8_t **pps_data);

        // Cleanup memory
        auto VideoHardwareCleanup(void);

    private:
        VideoDecoderType     _decodeType;
        VTDecompressionSessionRef decompressionSession;

        // Resolution
        int width;
        int height;

        // Sync lock
        pthread_mutex_t _mutex;
        // NORMA、ERRORCHECK、RECURSIVE
        pthread_mutexattr_t mutexattr;
    };

}

#endif //LLPLAYER_VIDEOHARDWAREDECODER_H
