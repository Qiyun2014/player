//
// Created by qiyun on 2019/11/27.
//

#include "xq_packet_queue.h"


void xq_packet_queue::xq_queue_init(int max_numbers) {
    if (_a_head_packet != nullptr) {
        free(_a_head_packet);
    }
    if (_v_head_packet != nullptr) {
        free(_v_head_packet);
    }
    pthread_mutex_init(&mutex, nullptr);
}

int xq_packet_queue::xq_queue_put(void *data, int size, double pts, AVMediaType type){
    if (xq_queue_count(type) >= _limit_count) {
        return -1;
    }

    pthread_mutex_lock(&mutex);
    auto *packet = static_cast<XQPacket *>(malloc(sizeof(XQPacket)));
    packet->data = (uint8_t *)data;
    packet->size = size;
    packet->pts = pts;

    // Add first packets
    if (get_queue_packet(type) == NULL) {
        if (type == AVMEDIA_TYPE_VIDEO ) {
            _v_head_packet = static_cast<XQQueuePacket *>(malloc(sizeof(XQQueuePacket)));
        } else if (type == AVMEDIA_TYPE_AUDIO) {
            _a_head_packet = static_cast<XQQueuePacket *>(malloc(sizeof(XQQueuePacket)));
        }
        get_queue_packet(type)->packet = packet;
        get_queue_packet(type)->next = nullptr;
        get_queue_packet(type)->serial = 0;
        get_queue_packet(type)->type = type;
        pthread_mutex_unlock(&mutex);
        return 0;
    }

    auto *curr_pkt = static_cast<XQQueuePacket *>(malloc(sizeof(XQQueuePacket)));
    curr_pkt->packet = packet;
    curr_pkt->next = nullptr;
    curr_pkt->type = type;

    // Add last packets
    XQQueuePacket *packets = xq_last_queue(type);
    packets->next = curr_pkt;
    curr_pkt->serial = packets->serial + 1;
    pthread_mutex_unlock(&mutex);
    return 0;
}


int xq_packet_queue::xq_queue_count(XQQueuePacket *head_packet) {
    if (head_packet != nullptr) {
        int count = 0;
        XQQueuePacket *curr = head_packet;
        while (curr != nullptr) {
            count ++;
            curr = curr->next;
        }
        return count;
    } else {
        return 0;
    }
}

int xq_packet_queue::xq_queue_count(AVMediaType type) {
    if (get_queue_packet(type) != nullptr) {
        int count = 0;
        XQQueuePacket *curr = get_queue_packet(type);
        while (curr != nullptr && (curr->type == type)) {
            count ++;
            curr = curr->next;
        }
        return count;
    } else {
        return 0;
    }
}


double xq_packet_queue::xq_first_pts(AVMediaType type) {
    XQQueuePacket *curr = get_queue_packet(type);
    if (curr != nullptr) {
        if (curr->packet != nullptr) {
            return curr->packet->pts;
        }
        return 0;
    }
    return 0;
}


XQQueuePacket *xq_packet_queue::xq_last_queue(XQQueuePacket *head_packet) {
    if (head_packet == nullptr){
        return nullptr;
    }
    XQQueuePacket *curr = head_packet;
    while (curr->next != nullptr) {
        curr = curr->next;
    }
    return curr;
}


XQQueuePacket *xq_packet_queue::xq_last_queue(AVMediaType type) {
    return xq_last_queue(get_queue_packet(type));
}


int xq_packet_queue::xq_queue_out(void **out_data, int *out_size, AVMediaType type) {
    if (get_queue_packet(type) == nullptr){
        return 0;
    }
    pthread_mutex_lock(&mutex);
    XQQueuePacket *queue_packet = get_queue_packet(type);
    XQPacket *packet = queue_packet->packet;
    *out_data = packet->data;
    *out_size = packet->size;

    av_free(get_queue_packet(type)->packet);
    if (type == AVMEDIA_TYPE_VIDEO) {
        _v_head_packet = queue_packet->next;
    } else if (type == AVMEDIA_TYPE_AUDIO) {
        _a_head_packet = queue_packet->next;
    }
    av_free(queue_packet);
    pthread_mutex_unlock(&mutex);
    return 1;
}



/*
 *  xq_packet_queue *packetQueue = new xq_packet_queue(20);

    packetQueue->xq_queue_put((void *)"1", 1);
    packetQueue->xq_queue_put((void *)"12", 2);
    packetQueue->xq_queue_put((void *)"123", 3);
    packetQueue->xq_queue_put((void *)"1234", 4);
    packetQueue->xq_queue_put((void *)"12345", 5);
    packetQueue->xq_queue_put((void *)"123456", 6);

    char *aa;
    int s;
    packetQueue->xq_queue_out((void **)&aa, &s);
    packetQueue->xq_queue_out((void **)&aa, &s);
    packetQueue->xq_queue_out((void **)&aa, &s);
    packetQueue->xq_queue_out((void **)&aa, &s);
    packetQueue->xq_queue_out((void **)&aa, &s);
    packetQueue->xq_queue_out((void **)&aa, &s);
    packetQueue->xq_queue_out((void **)&aa, &s);
 * */