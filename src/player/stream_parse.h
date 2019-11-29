//
// Created by qiyun on 2019/11/15.
//

#ifndef LLPLAYER_STREAM_PARSE_H
#define LLPLAYER_STREAM_PARSE_H

#include <string>
#include <libavformat/avformat.h>

enum {
    noError = 0,
    openFailed,
    notFound,
};

namespace stream_parse {
    class stream_parse_class {
    public:
        // 查找流格式
        auto stream_formart_parse(
                const std::string path,
                AVFormatContext **out_fmt_ctx) -> int ;

        // 查找数据流存储位置
        auto find_stream_index(
                const AVFormatContext *fmt_ctx,
                int *video_stream_index,
                int *audio_stream_index,
                float *frame_rate) -> void;

        // 读取数据信息
        auto parse_stream_info (
                const AVFormatContext *fmt_ctx,
                const int *video_stream_index,
                double *duration,
                float *frame_rate,
                int *width,
                int *height) -> void;

        // 查找解码器
        auto stream_codec_find(
                const AVFormatContext *fmt_ctx,
                const int *video_stream_index,
                const int *audio_stream_index,
                AVCodec **videoCodecID,
                AVCodec **audioCodecID) -> int;
    };
};

#endif //LLPLAYER_STREAM_PARSE_H
