//
// Created by qiyun on 2019/11/15.
//

#import "Decode/decoder_struct.h"
#include "player.h"
#include <stdlib.h>
#include "stream_parse.h"
#include "Decode/decoder.h"
#include <stdio.h>
#include "Display/GLRender.h"

// 视频存储上下文
AVFormatContext *fmt_context = NULL;
// 码流存储位置
int video_stream_index, audio_stream_index;
// 渲染
XQRender::GLRender *glRender = NULL;
// 解码器
MediaDecoder *mDecoder = NULL;

namespace player {
    playerClass::~playerClass() {
        avformat_network_deinit();
        if (mDecoder != NULL) {
            delete mDecoder;
        }
        avformat_close_input(&fmt_context);
    }

    auto decode_frame_callback(AVFrame *v_frame, uint8_t *audio_buffer, int size) -> void {
        if (size > 0) {
            // Audio
        } else {
            // Video
            if (glRender) {
                // AV_PIX_FMT_YUV420P,   ///< planar YUV 4:2:0, 12bpp, (1 Cr & Cb sample per 2x2 Y samples)
                glRender->DisplayToWindow(GL_MEDIA_TYPE_YUV420p, v_frame->data, v_frame->linesize);
            }
        }
    }

    auto decode_frame_callback2(uint8_t *data, int size, int width, int height) -> void {
        // Video
        if (glRender) {
            // AV_PIX_FMT_YUV420P,   ///< planar YUV 4:2:0, 12bpp, (1 Cr & Cb sample per 2x2 Y samples)
            glRender->DisplayVideoToWindow(GL_MEDIA_TYPE_YUV420p, data, size, width, height);
        }

#ifdef _WIN32
        /* Windows specific API calls are here */
#elif __linux__
        /* Linux specific API calls are here */
#elif __APPLE__
#ifdef TARGET_OS_MAC
        /* Mac OS specific API calls are here */
#elif TARGET_OS_IPHONE
        /* iPhone specific API calls are here */
#else
#endif

#endif
    }


    auto playerClass::open_file(std::string path) -> int {
        avformat_network_init();
        av_register_all();
        auto *parse = new stream_parse::stream_parse_class();

        // Init decoder and set default params
        fmt_context = avformat_alloc_context();
        fmt_context->probesize = 1024 * 1024;
        fmt_context->max_analyze_duration = 5 * AV_TIME_BASE;
        int ret = parse->stream_formart_parse(path, &fmt_context);
        if (ret != noError) {
            printf("Open file failed ......\n");
            return -1;
        }

        // Find stream position
        parse->find_stream_index(fmt_context, &video_stream_index, &audio_stream_index, &frame_rate);
        if (video_stream_index < 0 && audio_stream_index < 0) {
            printf("Cound't found audio and video stream ......\n");
            return -2;
        }

        // Get media info
        parse->parse_stream_info(fmt_context, &video_stream_index, &duration,&frame_rate, &width, &height);
        // Start decoder
        if (width || height) {
            XQMediaDecoder media_decoder = {
                    .fmt_ctx = fmt_context,
                    .audio_index = audio_stream_index,
                    .video_index = video_stream_index,
                    .decode_frame = decode_frame_callback,
                    .decode_video_frame_callback = decode_frame_callback2
            };
            if (mDecoder == NULL) {
                mDecoder = new MediaDecoder(media_decoder);
                mDecoder->v_duration = 1000.0 / frame_rate;
            }
        }
        delete parse;

        printf("全部结束 ... \n");
        return ret;
    }


    auto playerClass::seek_time(long position) -> bool {

        AVRational v_rational = fmt_context->streams[video_stream_index]->time_base;
        double v_pts = v_rational.den/v_rational.num;

        av_seek_frame(fmt_context,
                      video_stream_index,
                      v_pts * ((double)position / (double)1000) * AV_TIME_BASE + (double)fmt_context->start_time / (double) AV_TIME_BASE * v_pts,
                      AVSEEK_FLAG_ANY);

        AVCodec *v_codec = avcodec_find_decoder(fmt_context->streams[video_stream_index]->codecpar->codec_id);
        AVCodecContext *video_codec_ctx = avcodec_alloc_context3(v_codec);
        avcodec_flush_buffers(video_codec_ctx);
        return true;
    }

    void playerClass::pause() {
        _pause = true;
    }

    void playerClass::play() {
        _pause = false;
    }

}
