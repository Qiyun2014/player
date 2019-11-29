//
// Created by qiyun on 2019/11/19.
//

#ifndef LLPLAYER_GLRENDER_H
#define LLPLAYER_GLRENDER_H

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/core/mat.hpp>

#define GL_NUM_DATA_POINTERS 8

typedef enum GL_MEDIA_TYPE{
    GL_MEDIA_TYPE_NONE,
    GL_MEDIA_TYPE_YUV420i,
    GL_MEDIA_TYPE_YUV420p,
    GL_MEDIA_TYPE_RGB,
    GL_MEDIA_TYPE_BGR,
    GL_MEDIA_TYPE_RGBA,
    GL_MEDIA_TYPE_BGRA,
}GL_MEDIA_TYPE;

namespace XQRender {
    class GLRender {
    public:
        GLRender(int width, int height);
        ~GLRender();
        void DisplayToWindow(GL_MEDIA_TYPE mediaType, uint8_t *image_buffer[GL_NUM_DATA_POINTERS], int size[GL_NUM_DATA_POINTERS]);
        void DisplayVideoToWindow(GL_MEDIA_TYPE mediaType, uint8_t *data, int size, int width, int height);
    private:
        CvSize display_size;
        cv::Mat mat_dst;
    };
};

#endif //LLPLAYER_GLRENDER_H
