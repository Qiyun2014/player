//
// Created by qiyun on 2019/11/15.
//

#include "decoder.h"
#include <pthread.h>
#include <semaphore.h>
#include <thread>
#include "VideoDecoder.h"
#include "AudioDecoder.h"
#include <unistd.h>

// 打开指定媒体解码器
AVCodecContext* MediaDecoder::CodecContextOnFormatContext(AVFormatContext *fmt_ctx, int stream_index, bool video_type, int *out_ret) {
    AVCodec *codec = avcodec_find_decoder(fmt_ctx->streams[stream_index]->codecpar->codec_id);
    AVCodecParameters *codec_params = fmt_ctx->streams[stream_index]->codecpar;
    AVCodecContext *codec_ctx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(codec_ctx, codec_params);
    if (avcodec_open2(codec_ctx, codec, nullptr) < 0) {
        printf("%s decode open failed ... \n", (video_type)?"Video" : "Audio");
        *out_ret = -1;
    }
    return codec_ctx;
}

int decode_reason(int value, AVCodecContext *codecContext) {
    if (value == AVERROR(EAGAIN)) {
        printf("must try to send new input ... \n");
        return -1;
    } else if (value == AVERROR(EINVAL)) {
        printf("codec not opened ... \n");
        return -1;
    } else if (value == AVERROR_EOF) {
        avcodec_flush_buffers(codecContext);
        printf("end of file ... \n");
        return -1;
    } else if (value == AVERROR(ENOMEM)) {
        printf("failed to add packet to internal queue ... \n");
        return -1;
    }
    return 0;
}


static void *Decoder_thread(void *ptr) {
    auto *decoder = (MediaDecoder *) ptr;
    if (decoder == nullptr) return nullptr;
    while (decoder->_isDecoding) {
        pthread_mutex_lock(&decoder->_mutex);
        pthread_cond_wait(&decoder->_cond, &decoder->_mutex);
        pthread_mutex_unlock(&decoder->_mutex);
        decoder->decode_thread();
        av_usleep(1000);
    }
    return nullptr;
}



void* MediaDecoder::decode_thread() {
    AVFormatContext *formatContext = this->mediaDecoder->fmt_ctx;
    ret = av_read_frame(formatContext, this->_packet);
    if (ret < 0) {
        printf("读取视频数据失败... \n");
        this->_isDecoding = false;
        return nullptr;
    }
    sem_wait(&this->_sem_t);
    AVPacket *orig_pkt = this->_packet;

    int num_of_pixel = this->v_codecContext->width * this->v_codecContext->height;
    auto *yuv_buffer = (uint8_t *) malloc(sizeof(uint8_t) * num_of_pixel * 1.5);
    // 解码视频帧
    if (this->_packet->stream_index == this->mediaDecoder->video_index) {
        ret = avcodec_send_packet(this->v_codecContext, this->_packet);
        if (ret != 0) {
            printf("Video send packet failed ... \n");
            decode_reason(ret, this->v_codecContext);
            this->_isDecoding = false;
            sem_post(&this->_sem_t);
            return nullptr;
        }
        ret = avcodec_receive_frame(this->v_codecContext, this->_source_frame);
        if (ret < 0) {
            printf("Video receive frame failed ... %d \n", ret);
            sem_post(&this->_sem_t);
            return nullptr;
        } else {
            if (this->_source_frame->pts == AV_NOPTS_VALUE) {
                this->_source_frame->pts = this->_source_frame->best_effort_timestamp;
            }
        }
        //printf("have video frame  ... \n");
        ret = sws_scale(sws_ctx,
                        (const uint8_t* const*) this->_source_frame->data,
                        this->_source_frame->linesize,
                        0,
                        this->v_codecContext->height,
                        this->_decode_frame->data,
                        this->_decode_frame->linesize);

        AVStream *stream = formatContext->streams[this->_packet->stream_index];
        double ts = ((double)(this->_source_frame->pts)) * av_q2d(stream->time_base);
        printf("视频时间戳 ---->>  %.7f \n", ts);

        if (ret > 0) {
            memcpy(yuv_buffer, this->_decode_frame->data[0], num_of_pixel);
            memcpy(yuv_buffer + num_of_pixel, this->_decode_frame->data[1], num_of_pixel / 4);
            memcpy(yuv_buffer + (num_of_pixel + num_of_pixel / 4), this->_decode_frame->data[2], num_of_pixel / 4);
            ret = this->packet_queue->xq_queue_put((void *)yuv_buffer, num_of_pixel * 1.5, ts, AVMEDIA_TYPE_VIDEO);
            if (ret < 0) {
                //printf("超出最大buffer限制，暂停解码 \n");
            } else {
                //printf("加入队列 (Video) .........显示时间戳 = %.5f \n", ts);
            }
        }

    } else if (this->_packet->stream_index == this->mediaDecoder->audio_index) {
        ret = avcodec_send_packet(this->a_codecContext, orig_pkt);
        if (ret != 0) {
            printf("Audio send packet failed ... \n");
            decode_reason(ret, this->a_codecContext);
            this->_isDecoding = false;
            sem_post(&this->_sem_t);
            return nullptr;
        }
        ret = avcodec_receive_frame(this->a_codecContext, this->_source_frame);
        if (ret < 0) {
            printf("Audio receive frame failed ... %d \n", ret);
            sem_post(&this->_sem_t);
            return nullptr;
        } else {
            if (this->_source_frame->pts == AV_NOPTS_VALUE) {
                this->_source_frame->pts = this->_source_frame->best_effort_timestamp;
            }
        }
        //printf("have audio frame  ... \n");
        int data_size = av_get_bytes_per_sample(this->a_codecContext->sample_fmt);
        if (data_size < 0) {
            ret = -1;
            printf("音频数据为空 ... %d \n", ret);
            sem_post(&this->_sem_t);
            return nullptr;
        }

        ret = swr_convert(
                swr_ctx_audio,
                &this->a_decoderStruct->audio_buffer,
                this->a_decoderStruct->buffer_size,
                (const uint8_t **) this->_source_frame->extended_data,
                this->_source_frame->nb_samples
        );
        //printf("音频解码成功 ... %d \n", ret);

        AVStream *stream = formatContext->streams[this->_packet->stream_index];
        double ts = ((double)(this->_source_frame->pts)) * av_q2d(stream->time_base);
        printf("音频时间戳 ---->> %.7f \n", ts);
        if (ret > 0) {
            ret = this->packet_queue->xq_queue_put((void *)this->a_decoderStruct->audio_buffer, this->a_decoderStruct->buffer_size, ts, AVMEDIA_TYPE_AUDIO);
            if (ret < 0) {
                printf("超出最大buffer限制，暂停解码 \n");
            } else {
                //printf("加入队列 (Audio) .........显示时间戳 = %.5f \n", ts);
            }
        }
    }
    av_free(yuv_buffer);
    av_packet_unref(orig_pkt);
    sem_post(&this->_sem_t);
    return nullptr;
}


auto MediaDecoder::media_decode(XQMediaDecoder decoder, AVCodecContext *v_codec_ctx, AVCodecContext *a_codec_ctx) -> void
{
    if (a_codec_ctx == nullptr && v_codec_ctx == nullptr){
        return;
    }

    // 解码之后存储的帧
    _source_frame = av_frame_alloc();
    _decode_frame = av_frame_alloc();
    _packet = (AVPacket *) av_malloc(sizeof(AVPacket));

    // 设置输出帧格式和数据分配内存
    uint8_t *out_video_buffer = (uint8_t *) av_malloc(av_image_get_buffer_size(AV_PIX_FMT_YUV420P, v_codec_ctx->width, v_codec_ctx->height, 1));
    ret = av_image_fill_arrays(_decode_frame->data, _decode_frame->linesize, out_video_buffer, AV_PIX_FMT_YUV420P, v_codec_ctx->width, v_codec_ctx->height, 1);
    free(out_video_buffer);
    if (ret <= 0) {
        return;
    }

    /**
     *  创建一个信号量并初始化它的值。一个无名信号量在被使用前必须先初始化。
     *
     *  sem：信号量的地址。
     *  pshared：等于 0，信号量在线程间共享（常用）；不等于0，信号量在进程间共享。
     *  value：信号量的初始值。
     **/
    sem_init(&_sem_t, 0, 0);
    decoder.frame_get               = _sem_t;
    mediaDecoder                    = &decoder;

    // 开始读取数据包
    v_decoderStruct = (VideoDecoderStruct *) malloc(sizeof(VideoDecoderStruct));
    //v_decoderStruct->src_frame      = &_source_frame;
    //v_decoderStruct->dst_frame      = &_decode_frame;
    //v_decoderStruct->in_user_ptr    = this;
    v_decoderStruct->codecContext   = v_codec_ctx;
    //v_decoderStruct->swsContext     = sws_ctx;
    //v_decoderStruct->decoder        = (XQMediaDecoder *) &decoder;
    //v_decoderStruct->_msg_queue     = &_msg_queue;
    //videoDecoder->setDecoderType(H264_Decoder);


    a_decoderStruct = (AudioDecoderStruct *) malloc(sizeof(AudioDecoderStruct));
    //a_decoderStruct->decoder        = (XQMediaDecoder *) &decoder;
    //a_decoderStruct->in_user_ptr    = this;
    //a_decoderStruct->frame          = &_source_frame;
    //a_decoderStruct->codecContext   = a_codec_ctx;
    //a_decoderStruct->swrContext     = swr_ctx_audio;
    a_decoderStruct->audio_buffer   = out_audio_buffer;
    a_decoderStruct->buffer_size    = MAX_AUDIO_FRAME_SIZE * 2;
    //a_decoderStruct->_msg_queue     = &_msg_queue;


    // 单开一个read_frame线程（在线程开启两个异步子线程解码音频和视频）
    pthread_create(&dec_thread, nullptr, Decoder_thread, this);
    pthread_create(&v_thread, nullptr, VideoDecoder::VideoQueue_Read_Thread, this);
    pthread_create(&a_thread, nullptr, AudioDecoder::AudioQueue_Read_Thread, this);

    pthread_join(dec_thread, nullptr);
    pthread_join(v_thread, nullptr);
    pthread_join(a_thread, nullptr);

    printf("Main thread .... \n");
}


MediaDecoder::MediaDecoder(XQMediaDecoder decoder) {
    _msg_queue = new MQ::MessageQueue();
    packet_queue = new xq_packet_queue(_max_limit);

    if (decoder.fmt_ctx == nullptr) {
        return;
    }

    v_codecContext = CodecContextOnFormatContext(decoder.fmt_ctx, decoder.video_index, true, &ret);
    if (ret < 0) {
        printf("Could't find or open video codec ... \n ");
    }
    a_codecContext = CodecContextOnFormatContext(decoder.fmt_ctx, decoder.audio_index, false, &ret);
    if (ret < 0) {
        if (ret < 0) {
            printf("Could't find or open audio codec ... \n ");
        }
    }

    if (swr_ctx_audio == nullptr) {
        int bit_samples                 = a_codecContext->frame_size;
        int sample_rate                 = 44100;
        uint64_t channel_layout         = AV_CH_LAYOUT_MONO;
        AVSampleFormat sample_format    = AV_SAMPLE_FMT_S16;
        int channel                     = av_get_channel_layout_nb_channels(channel_layout);
        int buffer_size                 = av_samples_get_buffer_size(nullptr, channel, bit_samples, sample_format, 1);
        out_audio_buffer                = (uint8_t *) av_malloc(MAX_AUDIO_FRAME_SIZE * 2);

        int in_channel_layout = av_get_default_channel_layout(a_codecContext->channels);
        swr_ctx_audio = swr_alloc_set_opts(swr_ctx_audio, channel_layout, sample_format,
                                           sample_rate, in_channel_layout, a_codecContext->sample_fmt,
                                           a_codecContext->sample_rate, 0, nullptr);
        swr_init(swr_ctx_audio);

        float duration = 1000.0 * (double)bit_samples / 44100.0;
        a_duration = duration;
    }

    if (sws_ctx == nullptr) {
        // 转换解码数据为yuv格式
        sws_ctx = sws_getContext(
                v_codecContext->width, v_codecContext->height, v_codecContext->pix_fmt,
                v_codecContext->width, v_codecContext->height,AV_PIX_FMT_YUV420P,
                SWS_BICUBIC, nullptr, nullptr,nullptr);
    }

    videoDecoder = new VideoDecoder({});
    audioDecoder = new AudioDecoder({});
    // starting
    media_decode(decoder, v_codecContext, a_codecContext);
}

MediaDecoder::~MediaDecoder() {
    printf("解码完成 ... \n");
    av_frame_free(&_source_frame);
    av_frame_free(&_decode_frame);
    av_packet_unref(_packet);

    avcodec_free_context(&v_codecContext);
    avcodec_free_context(&a_codecContext);
    if (sws_ctx != nullptr) {
        sws_freeContext(sws_ctx);
    }
    if (swr_ctx_audio != nullptr) {
        swr_free(&swr_ctx_audio);
    }
    free(out_audio_buffer);
    free(a_decoderStruct);
    free(v_decoderStruct);

    delete audioDecoder;
    delete videoDecoder;
    delete _msg_queue;
    sem_close(&_sem_t);
}


// 是否启用h265解码
void MediaDecoder::hevc_decoder_enable(bool open) {

}

bool MediaDecoder::hardwareDecode(int open) {
    if (open == openHardware) { return false;}
    if (open) {

#if LL_TARGET_MACOS_AND_IOS
        hardwareDecoder = new VideoDecoder::VideoHardwareDecoder(360, 640);
#endif
    }
    openHardware = open;
    return true;
}


// 设置解码类型
bool MediaDecoder::setDecoderType(VideoDecoderType type) {
    if (type == _decoder_type) { return false; }


    _decoder_type = type;
    return true;
}

// 暂停播放
void MediaDecoder::pause() {
    if (_isPause) return;
    _isPause = true;

}

// 播放
void MediaDecoder::play() {
    if (!_isPause) return;
    _isPause = false;

}

//}
