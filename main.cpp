#include <iostream>
#include "src/player/player.h"

extern "C" {
#include <libavformat/avformat.h>
}

#include "src/player/MessageQueue/message_queue.h"
#include "src/player/MessageQueue/xq_packet_queue.h"

using namespace std;

int main() {
    std::cout << "Hello, World!" << std::endl;
    cout << avformat_configuration() << endl;

    avformat_alloc_context();
    avformat_network_init();

    string path("/Users/qiyun/Desktop/IMG_0007.mp4");
    //string path("http://clips.vorwaerts-gmbh.de/big_buck_bunny.mp4");
    //string path("http://weblive.hebtv.com/live/hbws_bq/index.m3u8");
    //string path("/Users/qiyun/Desktop/audio.mp3");

    player::playerClass *playerClass = new player::playerClass();
    playerClass->open_file(path);

    /*
    MQ::MessageQueue *mq = new MQ::MessageQueue();
    int serial = 1;
    mq->put_message(mq->message_count(), "1", &serial);
    mq->put_message(mq->message_count(), "2", &serial);
    mq->put_message(mq->message_count(), "3", &serial);
    mq->put_message(mq->message_count(), "4", &serial);
    mq->put_message(mq->message_count(), "5", &serial);
    mq->put_message(mq->message_count(), "6", &serial);
    mq->put_message(mq->message_count(), "7", &serial);

    mq->delete_message(0);
    mq->delete_message(0);
    mq->delete_message(0);
    mq->delete_message(0);

    int count = mq->message_count();
    printf("cout = %d", count);
    */


    return 0;
}