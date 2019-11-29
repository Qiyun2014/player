//
// Created by qiyun on 2019/11/20.
//

extern "C" {
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/timestamp.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
}

#include "AudioDecoder.h"
#include <pthread.h>
#include <semaphore.h>
#import "decoder_struct.h"


AudioDecoder::AudioDecoder(XQMediaDecoder decoder1, XQMediaDecoder decoder) : MediaDecoder(decoder1) {

}

AudioDecoder::AudioDecoder() : MediaDecoder({}){

}

AudioDecoder::~AudioDecoder() {

}


// TS
static struct timeval _timeval;
void *AudioDecoder::AudioQueue_Read_Thread(void *ptr) {
    auto *decoder = static_cast<MediaDecoder *> (ptr);
    if (decoder == nullptr) return nullptr;
    /*
    * 1. 以音频时戳为标准，视频时戳pts进行同步。
    * 2. 音频以正常速率进行播放，计算当前系统时间与上次系统时间，达到音频每帧显示时长则播放。
    * 3. 视频pts比音频帧pts大，则进行等待。
    * 4. 视频pts比音频帧小，则加快视频解码或者进行丢弃帧。
    */

    while (1) {
        int packet_count = decoder->packet_queue->xq_queue_count(AVMEDIA_TYPE_AUDIO);
        if (!decoder->_isDecoding && (packet_count == 0)) {
            printf("* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *   音频停止解码... \n");
            break;
        } else if (packet_count < decoder->_max_limit) {
            pthread_mutex_lock(&decoder->_mutex);
            if (!decoder->_isPause) {
                pthread_cond_signal(&decoder->_cond);
            }
            pthread_mutex_unlock(&decoder->_mutex);
        } else if (decoder->_isPause) {
            //printf("==========================  音频暂停解码 ..... \n");
            av_usleep(200 * 1000);
            continue;
        }

        // 队列存在数据时
        if (packet_count > 0) {
            gettimeofday(&_timeval, NULL);
            double ts = _timeval.tv_sec * 1000 + _timeval.tv_usec / 1000;
            //printf("时间戳  %.5f \n", ts);

            uint8_t  *data;
            int size;
            double pts;

            // 音频首包进行缓存
            if (decoder->audioDecoder->pre_pts <= 0) {
                decoder->audioDecoder->pre_pts = ts;
                decoder->packet_queue->xq_queue_out((void **) &data, &size, &pts, AVMEDIA_TYPE_AUDIO);
                //printf("* * * * * * * * * * * * * * * * * *  音频播放数据，缓存个数 = %d , pts = %.5f\n", packet_count, pts);
                continue;
            }

            // 两帧之间间隔大于时长
            double interval = ts - (decoder->audioDecoder->pre_pts);
            if (interval >= decoder->a_duration ) {
                decoder->packet_queue->xq_queue_out((void **) &data, &size, &pts, AVMEDIA_TYPE_AUDIO);
                decoder->audioDecoder->pre_pts = ts;
                printf("* * * * * * * * * * * * * * * * * *  音频播放数据，缓存个数 = %d , pts = %.5f\n", packet_count, pts);
            } else {
                printf("音频延时    延时多少  %.5f  毫秒 ....\n", (decoder->a_duration - interval));
                av_usleep((decoder->a_duration - interval) * 1000);
            }
        }
    }
    return nullptr;
}
