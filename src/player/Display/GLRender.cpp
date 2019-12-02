//
// Created by qiyun on 2019/11/19.
//

#include "GLRender.h"
#include <opencv2/imgproc.hpp>

using namespace cv;
using namespace std;

namespace XQRender {

    GLRender::GLRender(int width, int height) {
        display_size = cvSize(width, height);
        mat_dst = cv::Mat(display_size.height, display_size.width, CV_8UC3);
        cvNamedWindow("Video", WINDOW_AUTOSIZE);
        imshow("Video", mat_dst);
        cvWaitKey(1);
    }

    GLRender::~GLRender() {

    }

    auto GLRender::DisplayToWindow(GL_MEDIA_TYPE mediaType, uint8_t *image_buffer[GL_NUM_DATA_POINTERS], int size[GL_NUM_DATA_POINTERS]) -> void {

        switch (mediaType) {
            case GL_MEDIA_TYPE_YUV420p: {
                int num_of_pixel = display_size.width * display_size.height;
                uint8_t *yuv_buffer = (uint8_t *) malloc(sizeof(uint8_t) * num_of_pixel * 1.5);
                memcpy(yuv_buffer, image_buffer[0], num_of_pixel);
                memcpy(yuv_buffer + num_of_pixel, image_buffer[1], num_of_pixel / 4);
                memcpy(yuv_buffer + (num_of_pixel + num_of_pixel / 4), image_buffer[2], num_of_pixel / 4);

                cv::Mat input(display_size.height * 1.5, display_size.width, CV_8UC1, yuv_buffer);
                mat_dst = cv::Mat(display_size.height, display_size.width, CV_8UC3);
                cv::cvtColor(input, mat_dst, cv::COLOR_YUV420p2RGB);
                free(yuv_buffer);
                break;
            }
            default: {
                printf("Unsupport format ...");
            }
        }

        if (mat_dst.cols > 0 && mat_dst.rows > 0) {
            cvNamedWindow("Video", WINDOW_AUTOSIZE);
            imshow("Video", mat_dst);
            cvWaitKey(1);
        }
    }

    void GLRender::DisplayVideoToWindow(GL_MEDIA_TYPE mediaType, uint8_t *data, int size, int width, int height) {
        switch (mediaType) {
            case GL_MEDIA_TYPE_YUV420p: {
                cv::Mat input(height * 1.5, width, CV_8UC1, data);
                mat_dst = cv::Mat(height, width, CV_8UC3);
                cv::cvtColor(input, mat_dst, cv::COLOR_YUV420p2RGB);
                break;
            }
            default: {
            }
        }

        if (mat_dst.cols > 0 && mat_dst.rows > 0) {
             //cvUpdateWindow("Video");
        }
    }

};