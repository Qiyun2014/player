//
// Created by qiyun on 2019/11/20.
//

#ifndef LLPLAYER_VIDEODECODER_H
#define LLPLAYER_VIDEODECODER_H


#define LL_TARGET_MACOS_AND_IOS (TARGET_OS_IPHONE || TARGET_OS_MAC || TARGET_OS_IPHONE)

#if LL_TARGET_MACOS_AND_IOS
#else
// #include "ios/VideoHardwareDecoder.h"
#endif

#include "decoder.h"

/**
 *  a 、如果派生类的函数和基类的函数同名，但是参数不同，此时，不管有无virtual，基类的函数被隐藏。
 *  b 、如果派生类的函数与基类的函数同名，并且参数也相同，但是基类函数没有vitual关键字，此时，基类的函数被隐藏（如果相同有Virtual就是重写覆盖了）。
 * */

class VideoDecoder : public MediaDecoder {
public:
    VideoDecoder(XQMediaDecoder decoder1, XQMediaDecoder decoder);
    VideoDecoder();
    ~VideoDecoder();

    // previous pts
    double pre_pts = 0;
    // now pts
    double now_pts = 0;
    // frame preview duration time
    double duration = 0;

    // 读取视频帧
    static void *VideoQueue_Read_Thread(void *ptr);
private:

#if LL_TARGET_MACOS_AND_IOS
    // iOS VideoToolBox 硬解码
        //XQVideoDecoder::VideoHardwareDecoder *hardwareDecoder;
#endif
};


#endif //LLPLAYER_VIDEODECODER_H
