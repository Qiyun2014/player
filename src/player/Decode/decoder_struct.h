//
// Created by qiyun on 2019/11/25.
//

#ifndef LLPLAYER_DECODER_STRUCT_H
#define LLPLAYER_DECODER_STRUCT_H

extern "C" {
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/timestamp.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <libavutil/time.h>
}
#include <semaphore.h>
#import "../MessageQueue/message_queue.h"

typedef enum __VideoDecoderType {
    None = 0,       // 软解吗
    H264_Decoder,
    HEVC_Decoder,
    VP8,
    VP9,
    AVS_2
}VideoDecoderType;


// Sturct
typedef struct __XQMediaDecoder {
    // Format I/O context.
    AVFormatContext *fmt_ctx;
    // Audio stream on packet of position
    const int audio_index;
    // Video Stream on packet of position
    const int video_index;
    // Callback for decoder success
    void (*decode_frame) (AVFrame *v_frame, uint8_t *audio_buffer, int size);
    // Callback for video frame data
    void (*decode_video_frame_callback) (uint8_t *data, int size, int width, int height);
    // Async lock
    sem_t frame_get;
} XQMediaDecoder;


typedef struct __VideoDecoderStruct {
    int ret_val;
    void *in_user_ptr;
    XQMediaDecoder *decoder;
    MQ::MessageQueue **_msg_queue;
    AVPacket **packet;
    AVFrame **src_frame;
    AVFrame **dst_frame;
    AVCodecContext *codecContext;
    SwsContext *swsContext;
}VideoDecoderStruct;



typedef struct __AudioDecoderStruct {
    int ret_val;
    void *in_user_ptr;
    XQMediaDecoder *decoder;
    AVPacket **packet;
    AVFrame **frame;
    AVCodecContext *codecContext;
    SwrContext* swrContext;
    MQ::MessageQueue **_msg_queue;
    uint8_t *audio_buffer;
    int64_t buffer_size;
} AudioDecoderStruct;



typedef struct __XQPacket {
    uint8_t         *data;
    size_t          size;
    double          pts;
} XQPacket;



typedef struct __XQQueuePacket {
    AVPacket            *pkt;
    AVCodecContext      *codec_ctx;
    XQPacket            *packet;
    AVMediaType         type;
    int                 serial;
    __XQQueuePacket     *next;
} XQQueuePacket;



#endif //LLPLAYER_DECODER_STRUCT_H
