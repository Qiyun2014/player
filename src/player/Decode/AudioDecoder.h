//
// Created by qiyun on 2019/11/20.
//

#ifndef LLPLAYER_AUDIODECODER_H
#define LLPLAYER_AUDIODECODER_H

#include "decoder.h"

class AudioDecoder : public MediaDecoder {
public:
    AudioDecoder(XQMediaDecoder decoder1, XQMediaDecoder decoder);
    AudioDecoder();
    ~AudioDecoder();

    double pre_pts = 0;
    double timestamp = 0;
    double duration = 0;

    // 读取音频帧
    static void *AudioQueue_Read_Thread(void *ptr);
};



#endif //LLPLAYER_AUDIODECODER_H
