//
// Created by qiyun on 2019/11/15.
//

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

#include "stream_parse.h"

using namespace std;

namespace stream_parse {
    int stream_parse_class::stream_formart_parse(std::string path, AVFormatContext **out_fmt_ctx) {
        if (path.length() <= 0) {
            return openFailed;
        }
        int ret = avformat_open_input(out_fmt_ctx, path.c_str(), NULL, NULL);
        if (ret != 0) {
            printf("avformat_open_input() failed %d\n", ret);
            return notFound;
        }

        ret = avformat_find_stream_info(*out_fmt_ctx, NULL);
        if (ret != 0) {
            printf("avformat_find_stream_info() failed %d\n", ret);
            return notFound;
        }
        av_dump_format(*out_fmt_ctx, 0, path.c_str(), 0);
        return noError;
    }


    auto stream_parse_class::find_stream_index(const AVFormatContext *fmt_ctx, int *video_stream_index, int *audio_stream_index, float *frame_rate) -> void {
        *video_stream_index = -1;
        *audio_stream_index = -1;

        unsigned int i;
        for (i = 0; i < fmt_ctx->nb_streams; i ++) {
            if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                *video_stream_index = i;
                *frame_rate = fmt_ctx->streams[i]->avg_frame_rate.num / fmt_ctx->streams[i]->avg_frame_rate.den;
            } else if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
                *audio_stream_index = i;
            }
        }
    }


    auto stream_parse_class::parse_stream_info (const AVFormatContext *fmt_ctx, const int *video_stream_index, double *duration, float *frame_rate, int *width, int *height) -> void {
        // 当前视频时长
        *duration = fmt_ctx->duration / AV_TIME_BASE;
        // 获取视频分辨率
        if (*video_stream_index >= 0) {
            AVCodecParameters *video_codec_params = fmt_ctx->streams[*video_stream_index]->codecpar;
            *width = video_codec_params->width;
            *height = video_codec_params->height;
            AVStream *stream = fmt_ctx->streams[*video_stream_index];
            *frame_rate = stream->avg_frame_rate.num / stream->avg_frame_rate.den;
        }
    }


    auto stream_parse_class::stream_codec_find(
            const AVFormatContext *fmt_ctx,
            const int *video_stream_index,
            const int *audio_stream_index,
            AVCodec **videoCodecID,
            AVCodec **audioCodecID) -> int {

        if (*video_stream_index >= 0) {
            *videoCodecID = avcodec_find_decoder(fmt_ctx->streams[*video_stream_index]->codecpar->codec_id);
        }
        if (*audio_stream_index >= 0) {
            *audioCodecID = avcodec_find_decoder(fmt_ctx->streams[*audio_stream_index]->codecpar->codec_id);
        }

        for (int i = 0; i < 2; i ++) {
            AVDictionary *metadata = fmt_ctx->streams[*((i==0)?video_stream_index:audio_stream_index)]->metadata;
            if (metadata && (av_dict_count(metadata) > 0)) {
                AVDictionaryEntry *t = NULL;
                while (t = av_dict_get(metadata, "", t, AV_DICT_IGNORE_SUFFIX)) {
                    printf("%s Meta data --> { %s : %s} \n", (i==0)?"Video":"Audio", t->key, t->value);
                }
                av_dict_free(&metadata);
            }
        }

        if (videoCodecID || audioCodecID) {
            return noError;
        }
        return notFound;
    }
};
