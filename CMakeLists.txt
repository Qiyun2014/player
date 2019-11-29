cmake_minimum_required(VERSION 3.15)
project(LLPlayer)

set(CMAKE_CXX_STANDARD 14)
set(SRC_DIR ./)
set(FFMPEG_DIR /usr/local/Cellar/ffmpeg/4.2.1_1)
set(OPENCV_DIR /usr/local/Cellar/opencv@2/2.4.13.7_5/)
set(SDL_DIR /usr/local/Cellar/sdl2/2.0.10/)

# 引用路径
include_directories(${FFMPEG_DIR}/include/ ${OPENCV_DIR}/include/ ${SDL_DIR}/include/)
link_directories(${FFMPEG_DIR}/lib/ ${OPENCV_DIR}/lib/ ${SDL_DIR}/lib/)

MESSAGE(STATUS "operation system is ${CMAKE_VERSION}")
MESSAGE(STATUS "operation system name is ${CMAKE_SYSTEM_NAME}")

# 编译文件
add_executable(LLPlayer main.cpp
        src/player/stream_parse.cpp
        src/player/stream_parse.h
        src/player/player.cpp
        src/player/player.h
        src/player/Decode/decoder.cpp
        src/player/Decode/decoder.h
        src/player/Display/GLRender.cpp
        src/player/Display/GLRender.h


#        src/player/Decode/android/MediaDecoder.cpp
#        src/player/Decode/android/MediaDecoder.h
#        src/player/Display/MacOS/MacAudioPlayer.cpp
#        src/player/Display/MacOS/MacAudioPlayer.h
#
#        src/player/Decode/ios/AudioHardwareDecoder.cpp
#        src/player/Decode/ios/AudioHardwareDecoder.h
#        src/player/Decode/ios/VideoHardwareDecoder.cpp
#        src/player/Decode/ios/VideoHardwareDecoder.h


        src/player/Decode/AudioDecoder.cpp
        src/player/Decode/AudioDecoder.h
        src/player/Decode/VideoDecoder.cpp
        src/player/Decode/VideoDecoder.h

        src/player/SDL/SDL_AudioPlayer.cpp
        src/player/SDL/SDL_AudioPlayer.h src/player/Decode/decoder_struct.h src/player/MessageQueue/message_queue.cpp src/player/MessageQueue/message_queue.h src/player/MessageQueue/xq_packet_queue.cpp src/player/MessageQueue/xq_packet_queue.h)

# 添加链接库
target_link_libraries(
        LLPlayer
        avcodec
        avdevice
        avfilter
        avformat
        avutil
        swresample
        swscale

        opencv_core.dylib
        opencv_imgproc.dylib
        opencv_features2d.dylib
        opencv_highgui.dylib
        opencv_ml.dylib

        SDL2.dylib
)