//
// Created by qiyun on 2019/11/26.
//

#ifndef LLPLAYER_MESSAGE_QUEUE_H
#define LLPLAYER_MESSAGE_QUEUE_H

#include <iostream>
#include <sys/types.h>
#include <sys/msg.h>

typedef struct __MSG_PACKET {
    void **data;
    int *size;
    size_t clock_time;
    int media_type;
} MSG_PACKET;


typedef struct __MESSAGE_PACKETS {
    MSG_PACKET *packet;
    struct __MESSAGE_PACKETS *next_packets;
    int serial;
} MESSAGE_PACKETS;


namespace MQ {
    class MessageQueue {
    public:
        MessageQueue() {
            create_message_queue();
        }
        ~MessageQueue() {
            pthread_mutex_destroy(&mutex);
            free(head_msg_pkts);
            head_msg_pkts = NULL;
        }

        void create_message_queue();
        void put_message(int serial, const void *data, int *size);
        void delete_message(int serial);
        int message_count();
        MESSAGE_PACKETS *packets_search_serial(int serial);

    private:
        MESSAGE_PACKETS *head_msg_pkts = NULL;
        pthread_mutex_t mutex;
    };
}

#endif //LLPLAYER_MESSAGE_QUEUE_H
