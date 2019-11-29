//
// Created by qiyun on 2019/11/15.
//

#ifndef LLPLAYER_PLAYER_H
#define LLPLAYER_PLAYER_H

#include <string>

namespace player {
    class playerClass {
    public:
        ~playerClass();
        // 函数返回类型后置语法
        auto open_file(std::string path) -> int ;

        auto getDuration() -> double __const {
            return duration;
        };
        auto getWidth() -> double __const {
            return width;
        };
        auto getHeight() -> double __const  {
            return height;
        };
        auto getFrameRate() -> float __const {
            return frame_rate;
        }
        auto seek_time(long position) -> bool ;

        void pause();
        void play();

    private:
        // 视频时长
        double duration;
        // 帧率
        float frame_rate;
        // 视频分辨率
        int width;
        int height;
        // 暂停播放
        bool _pause = false;
    };
};


#endif //LLPLAYER_PLAYER_H
