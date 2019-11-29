//
// Created by qiyun on 2019/11/27.
//

#ifndef LLPLAYER_XQ_PACKET_QUEUE_H
#define LLPLAYER_XQ_PACKET_QUEUE_H

#include "../Decode/decoder_struct.h"

class xq_packet_queue {
public:
    xq_packet_queue(int max_limit_count) {
        xq_queue_init(max_limit_count);
        _limit_count = max_limit_count;
    };

    template <typename T>
    void T_Release(T &x) {
        XQQueuePacket *packet = static_cast<XQQueuePacket *>(x);
        while (packet->next != nullptr) {
            if (packet->pkt) {
                av_packet_unref(packet->pkt);
            }
            if (packet->packet != nullptr) {
                av_free(packet->packet);
            }
            packet = packet->next;
        }
    }
    ~xq_packet_queue() {
        T_Release(_v_head_packet);
        T_Release(_a_head_packet);
    };

    // Init
    void xq_queue_init(int max_numbers);

    // Add packet to last element
    int xq_queue_put(void *data, int size, double pts, AVMediaType type);

    // Read first packet
    int xq_queue_out(void **out_data, int *out_size, double *pts, AVMediaType type);

    // Number of count
    int xq_queue_count(XQQueuePacket *head_packet);
    int xq_queue_count(AVMediaType type);

    // Get first packet pts
    double xq_first_pts(AVMediaType type);

    // Get current queue
    XQQueuePacket *get_queue_packet(AVMediaType type) {
        return (type == AVMEDIA_TYPE_VIDEO) ? _v_head_packet : _a_head_packet;
    }

    // Last packet
    XQQueuePacket *xq_last_queue(XQQueuePacket *head_packet);
    XQQueuePacket *xq_last_queue(AVMediaType type);

private:
    XQQueuePacket   *_a_head_packet = nullptr;
    XQQueuePacket   *_v_head_packet = nullptr;
    pthread_mutex_t mutex;
    int _limit_count;
};


#endif //LLPLAYER_XQ_PACKET_QUEUE_H
