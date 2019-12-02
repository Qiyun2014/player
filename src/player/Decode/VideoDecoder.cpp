//
// Created by qiyun on 2019/11/20.
//

#include "VideoDecoder.h"
#include <pthread.h>
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


/*   RFC- 1359标准
 *
 *   1. 无法察觉：音频和视频的时间戳差值在：-100ms ~ +25ms 之间
 *   2. 能够察觉：音频滞后了 100ms 以上，或者超前了 25ms 以上
 *   3. 无法接受：音频滞后了 185ms 以上，或者超前了 90ms 以上
 */

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
            //printf("---------------------------------  视频继续解码... \n");
            //pthread_mutex_lock(&decoder->_mutex);
            //pthread_cond_signal(&decoder->_cond);
            //pthread_mutex_unlock(&decoder->_mutex);
            av_usleep(1 * 1000);
        } else if (decoder->_isPause) {
            //printf("==========================  音频暂停解码 ..... \n");
            av_usleep(200 * 1000);
            continue;
        }

        if (packet_count > 0) {
            // gettimeofday(&_timeval, NULL);
            // double ts = _timeval.tv_sec * 1000 + _timeval.tv_usec / 1000;

            uint8_t  *data;
            int size;
            double pts = decoder->packet_queue->xq_queue_count(AVMEDIA_TYPE_VIDEO) * 1000;

            // 视频第一个缓存包
            if (decoder->videoDecoder->pre_pts <= 0) {
                // 音频队列存在数据
                if (decoder->packet_queue->xq_queue_count(AVMEDIA_TYPE_AUDIO) > 0) {
                    decoder->videoDecoder->pre_pts = pts;
                    // 播放视频
                    decoder->packet_queue->xq_queue_out((void **) &data, &size, AVMEDIA_TYPE_VIDEO);
                }
                continue;
            }

            // 当前音频时戳与上一帧相差在25ms内
            double interval = pts - decoder->audioDecoder->pre_pts;
            if (fabs(interval) <= 25) {
                decoder->packet_queue->xq_queue_out((void **) &data, &size, AVMEDIA_TYPE_VIDEO);
                printf("* * * * * * * * * * * * * * * * * *  视频播放数据，缓存个数 = %d, 间隔时间 = %.2f , pts = %.5f\n", packet_count, interval, pts);
                if (decoder->mediaDecoder->decode_video_frame_callback != nullptr) {
                    decoder->mediaDecoder->decode_video_frame_callback(data, size, decoder->v_decoderStruct->codecContext->width, decoder->v_decoderStruct->codecContext->height);
                }
                // 记录本次时间戳
                decoder->videoDecoder->pre_pts = pts;
            } else {
                if (interval > 0) {
                    // 音频时戳小于视频时戳，视频解码太快，需要等待
                    av_usleep(abs(interval) * 1000);
                    printf("视频延时 .... %.5f \n", fabs(interval));
                } else {
                    printf("播放视频 .... %.5f \n", interval);
                    decoder->packet_queue->xq_queue_out((void **) &data, &size, AVMEDIA_TYPE_VIDEO);
                    // 记录本次时间戳
                    decoder->videoDecoder->pre_pts = pts;
                }
            }
        }
    }
    return nullptr;
}
