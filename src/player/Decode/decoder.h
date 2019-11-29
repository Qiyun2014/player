//
// Created by qiyun on 2019/11/15.
//

#ifndef LLPLAYER_DECODER_H
#define LLPLAYER_DECODER_H

#import "decoder_struct.h"
#import "../MessageQueue/xq_packet_queue.h"
#include <time.h>
#include <sys/time.h>

/* *
 *  1.视频编码格式：H264, VC-1, MPEG-2, MPEG4-ASP (Divx/Xvid), VP8, MJPEG 等。
 *  2.音频编码格式：AAC, AC3, DTS(-HD), TrueHD, MP3/MP2, Vorbis, LPCM 等。
 *  3.字幕编码格式：VOB, DVB Subs, PGS, SRT, SSA/ASS, Text。
 * */

#define MAX_AUDIO_FRAME_SIZE  192000

class VideoDecoder;
class AudioDecoder;

//namespace MediaDecoder {
    class MediaDecoder {
    public:
        // Init and dealloc func .
        // This keyword is a declaration specifier that can only be applied to in-class constructor declarations .
        // An explicit constructor cannot take part in implicit conversions.
        // It can only be used to explicitly construct an object 。
        explicit MediaDecoder(XQMediaDecoder decoder);
        ~MediaDecoder();

        // Exchange to hevc decode, default is h264
        virtual void hevc_decoder_enable(bool open);
        // 是否开启硬解码
        bool hardwareDecode(int open);
        // 设置解码类型
        bool setDecoderType(VideoDecoderType type);
        // 获取当前开启硬解码状态
        bool getOpenHardware() {
            return openHardware;
        }
        // 获取当前解码器类型
        VideoDecoderType getCurrentDecoderType() {
            return _decoder_type;
        };

        // 暂停播放
        virtual void pause();
        // 播放
        virtual void play();

        // Message queue
        MQ::MessageQueue *_msg_queue;
        // Media decoder params
        XQMediaDecoder *mediaDecoder{};

        // Lock
        sem_t _sem_t{};
        pthread_t  v_thread{}, a_thread{}, dec_thread{};

        // Decoder params
        VideoDecoderStruct *v_decoderStruct{};
        AudioDecoderStruct *a_decoderStruct{};

        // Decode
        void* decode_thread();

        // Decoding
        bool _isDecoding = true;
        // Signl lock
        pthread_mutex_t _mutex = PTHREAD_MUTEX_INITIALIZER;
        pthread_cond_t _cond = PTHREAD_COND_INITIALIZER;

        // Frame duration
        double v_duration, a_duration;
        // Queue
        xq_packet_queue     *packet_queue;
        // Max buffer size
        int _max_limit = 30;

        // Decoder
        VideoDecoder *videoDecoder;
        AudioDecoder *audioDecoder;

        // Play status
        bool _isPause = false;

    protected:
        // 默认不开启硬解码
        bool openHardware = false;

        // Open media codec
        static AVCodecContext* CodecContextOnFormatContext(AVFormatContext *fmt_ctx, int stream_index, bool video_type, int *out_ret);
        // Start decoder
        auto media_decode(XQMediaDecoder decoder, AVCodecContext *v_codecContext, AVCodecContext *a_codecContext) -> void ;

    private:
        // Decoder type
        VideoDecoderType _decoder_type;
        //Return status
        int ret{};
        // Output after decoder of auido data
        uint8_t *out_audio_buffer;
        // Convert format context
        struct SwsContext*  sws_ctx = NULL;
        struct SwrContext*  swr_ctx_audio = NULL;
        // For media codec context
        AVCodecContext *v_codecContext;
        AVCodecContext *a_codecContext;

        AVFrame *_source_frame{};
        AVFrame *_decode_frame{};
        // Packets
        AVPacket    *_packet{};
    };

//}


#endif //LLPLAYER_DECODER_H
