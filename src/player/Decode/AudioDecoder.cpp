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


// 计算每帧音频时长
// audio_number ++;
// float duration = 1000.0 * (double)ret / 44100.0;
// double ts = duration * audio_number / 1000.0;  // ms -> s
// this->a_duration = duration;

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


        uint8_t  *data;
        int size;

        // 当前时间戳
        gettimeofday(&_timeval, NULL);
        double ts = _timeval.tv_sec * 1000 + _timeval.tv_usec / 1000;

        // 队列存在数据时
        if (packet_count > 0) {
            // 当前音频队列第一帧的pts, ms
            double pts = decoder->packet_queue->xq_first_pts(AVMEDIA_TYPE_AUDIO) * 1000;
            // 音频首包进行缓存
            if (decoder->audioDecoder->pre_pts <= 0) {
                // 确保视频第一帧已经加入队列后，开始播放音频
                if (decoder->packet_queue->xq_queue_count(AVMEDIA_TYPE_VIDEO) <= 0) {
                    continue;
                }
                decoder->audioDecoder->pre_pts = pts;
                decoder->audioDecoder->timestamp = ts;
                // 抛出队列中第一个数据
                decoder->packet_queue->xq_queue_out((void **) &data, &size, AVMEDIA_TYPE_AUDIO);
                printf("* * * * * * * * * * * * * * * * * * 第一次 音频播放数据，缓存个数 = %d , pts = %.5f\n", packet_count, pts);
            }

            // 两帧之间间隔大于时长
            double interval = ts - (decoder->audioDecoder->timestamp + decoder->a_duration);
            if (interval > 0) {
                printf(" 当前 ========================== ========================== ==========================   %f  秒\n", ts / 1000);
                decoder->packet_queue->xq_queue_out((void **) &data, &size, AVMEDIA_TYPE_AUDIO);
                decoder->audioDecoder->pre_pts = pts;
                decoder->audioDecoder->timestamp = ts;
                printf("* * * * * * * * * * * * * * * * * *延时多少  %.5f  毫秒,  音频播放数据，缓存个数 = %d , pts = %.5f\n", fabs(interval), packet_count, pts);
            } else {
                printf("音频延时    延时多少  %.5f  毫秒 ....\n", fabs(interval));
                av_usleep(fabs(interval) * 1000);

                decoder->packet_queue->xq_queue_out((void **) &data, &size, AVMEDIA_TYPE_AUDIO);
                decoder->audioDecoder->pre_pts = pts;
                decoder->audioDecoder->timestamp = ts;
            }
        }
    }
    return nullptr;
}
