//
// Created by qiyun on 2019/11/26.
//

#import "message_queue.h"

namespace MQ {

    void MessageQueue::create_message_queue() {
        if (head_msg_pkts) {
            free(head_msg_pkts);
        }
        pthread_mutex_init(&mutex, nullptr);
    }


    void MessageQueue::put_message(int serial, const void *data, int *size) {
        pthread_mutex_lock(&mutex);
        MESSAGE_PACKETS *next = head_msg_pkts, *prev = head_msg_pkts;
        auto *packet = static_cast<MSG_PACKET *>(malloc(sizeof(MSG_PACKET)));
        packet->data = (void **) data;
        packet->size = size;

        // Add first packets
        if (head_msg_pkts == nullptr) {
            head_msg_pkts = static_cast<MESSAGE_PACKETS *>(malloc(sizeof(MESSAGE_PACKETS)));
            head_msg_pkts->packet = packet;
            head_msg_pkts->next_packets = nullptr;
            head_msg_pkts->serial = 0;
            pthread_mutex_unlock(&mutex);
            return;
        }
        MESSAGE_PACKETS *curr_pkt = static_cast<MESSAGE_PACKETS *>(malloc(sizeof(MESSAGE_PACKETS)));
        curr_pkt->packet = packet;
        curr_pkt->next_packets = NULL;

        // Add last packets
        if (serial == message_count()) {
            MESSAGE_PACKETS *packets = packets_search_serial(serial);
            packets->next_packets = curr_pkt;
            curr_pkt->serial = packets->serial + 1;
            pthread_mutex_unlock(&mutex);
            return;
        }

        while (next != NULL) {
            if (serial == next->serial) {
                curr_pkt->next_packets = next;
                curr_pkt->serial = serial;
                if (serial > 0 && prev != NULL) {
                    prev->next_packets = curr_pkt;
                }
            } else if (serial > next->serial) {
                next->serial += 1;
            }
            prev = next;
            next = next->next_packets;
        }
        pthread_mutex_unlock(&mutex);
    }


    void MessageQueue::delete_message(int serial) {
        pthread_mutex_lock(&mutex);
        MESSAGE_PACKETS *next = head_msg_pkts, *curr = head_msg_pkts;
        while (next != NULL) {
            if (next->serial == serial) {
                if (serial == 0) {
                    head_msg_pkts = head_msg_pkts->next_packets;
                    head_msg_pkts->serial = 0;
                    free(head_msg_pkts->packet);
                    next = head_msg_pkts->next_packets;
                    curr = head_msg_pkts;
                    continue;
                } else {
                    curr->next_packets = next->next_packets;
                    if (curr->next_packets != NULL) {
                        curr->next_packets->serial = next->serial;
                    }
                }
            }
            curr = next;
            next = next->next_packets;
            if (curr->serial > serial) {
                curr->serial -= 1;
            }
        }
        pthread_mutex_unlock(&mutex);
    }


    int MessageQueue::message_count() {
        if (head_msg_pkts != NULL) {
            int count = 0;
            MESSAGE_PACKETS *curr = head_msg_pkts;
            while (curr != NULL) {
                count ++;
                curr = curr->next_packets;
            }
            return count;
        } else {
            return 0;
        }
    }


    MESSAGE_PACKETS *MessageQueue::packets_search_serial(int serial) {
        if ((serial < 0 && serial > message_count()) || head_msg_pkts == NULL) {
            return NULL;
        }
        MESSAGE_PACKETS *curr = head_msg_pkts;
        while (curr->next_packets != NULL && curr->serial != serial) {
            curr = curr->next_packets;
        }
        return curr;
    }
}