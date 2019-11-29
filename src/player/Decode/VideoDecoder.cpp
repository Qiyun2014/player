//
// Created by qiyun on 2019/11/20.
//

#include "VideoDecoder.h"
#include <pthread.h>
#include <SDL2/SDL.h>
#include "AudioDecoder.h"

VideoDecoder::VideoDecoder(XQMediaDecoder decoder1, XQMediaDecoder decoder) : MediaDecoder(decoder1) {

}

VideoDecoder::VideoDecoder() : MediaDecoder({}){

}

VideoDecoder::~VideoDecoder() {

}

auto convert_yuv2rgb(AVFrame *src_frame, uint8_t **bgr_buffer) {
    SwsContext *swsContext = sws_getContext(src_frame->width, src_frame->height,
                                            AV_PIX_FMT_YUV420P, src_frame->width, src_frame->height,
                                            AV_PIX_FMT_BGR24, 0, NULL, NULL, NULL);
    int linesize[8] = {src_frame->linesize[0] * 3};
    int num_bytes = av_image_get_buffer_size(AV_PIX_FMT_BGR24, src_frame->width, src_frame->height, 1);
    *bgr_buffer = (uint8_t *) malloc(num_bytes * sizeof(uint8_t));
    sws_scale(swsContext, src_frame->data, src_frame->linesize, 0, src_frame->height, bgr_buffer, linesize);
    sws_freeContext(swsContext);
    free(bgr_buffer);
}


// TS
static struct timeval _timeval;
void *VideoDecoder::VideoQueue_Read_Thread(void *ptr) {
    auto *decoder = static_cast<MediaDecoder *> (ptr);
    if (decoder == nullptr) return nullptr;
    while (1) {
        int packet_count = decoder->packet_queue->xq_queue_count(AVMEDIA_TYPE_VIDEO);
        if (!decoder->_isDecoding && (packet_count == 0)) {
            printf("---------------------------------  视频停止解码... \n");
            break;
        } else if (packet_count < decoder->_max_limit) {
            printf("---------------------------------  视频继续解码... \n");
            //pthread_mutex_lock(&decoder->_mutex);
            //pthread_cond_signal(&decoder->_cond);
            //pthread_mutex_unlock(&decoder->_mutex);
            av_usleep(5 * 1000);
        } else if (decoder->_isPause) {
            //printf("==========================  音频暂停解码 ..... \n");
            av_usleep(200 * 1000);
            continue;
        }

        if (packet_count > 0) {
            gettimeofday(&_timeval, NULL);
            double ts = _timeval.tv_sec * 1000 + _timeval.tv_usec / 1000;

            uint8_t  *data;
            int size;
            double pts;

            // 视频第一个缓存包
            if (decoder->videoDecoder->pre_pts <= 0) {
                decoder->videoDecoder->pre_pts = ts;
                decoder->packet_queue->xq_queue_out((void **) &data, &size, &pts, AVMEDIA_TYPE_VIDEO);
                continue;
            } else if (decoder->audioDecoder->pre_pts <= 0) {
                continue;
            }

            // 当前音频时戳与视频的视频相差在50s内
            double interval = decoder->audioDecoder->pre_pts - ts;
            if (fabs(interval) <= 50) {
                printf("播放视频 1 ...%.5f \n", interval);
                decoder->packet_queue->xq_queue_out((void **) &data, &size, &pts, AVMEDIA_TYPE_VIDEO);
                if (decoder->mediaDecoder->decode_video_frame_callback != NULL) {
                    decoder->mediaDecoder->decode_video_frame_callback(data, size, decoder->v_decoderStruct->codecContext->width, decoder->v_decoderStruct->codecContext->height);
                }
                // 记录本次时间戳
                decoder->videoDecoder->pre_pts = ts;
            } else {
                if (interval > 0) {
                    printf("播放视频 2 ...%.5f \n", interval);
                    decoder->packet_queue->xq_queue_out((void **) &data, &size, &pts, AVMEDIA_TYPE_VIDEO);
                    // 记录本次时间戳
                    decoder->videoDecoder->pre_pts = ts;

                } else {
                    // 音频时戳小于视频时戳，视频解码太快，需要等待
                    av_usleep(abs(interval) * 1000);
                    printf("视频延时 .... %.5f \n", fabs(interval));
                }
            }
        }
    }
    return nullptr;
}