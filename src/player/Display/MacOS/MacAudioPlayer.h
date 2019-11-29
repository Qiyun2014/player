//
// Created by qiyun on 2019/11/20.
//

#ifndef LLPLAYER_MACAUDIOPLAYER_H
#define LLPLAYER_MACAUDIOPLAYER_H

#if __has_include("AudioToolbox/AudioToolbox.h")
#include "AudioToolbox/AudioToolbox.h"
#endif

const unsigned int kNumAQBufs = 3;			// number of audio queue buffers we allocate
const size_t kAQBufSize = 128 * 1024;		// number of bytes in each audio queue buffer
const size_t kAQMaxPacketDescs = 512;		// number of packet descriptions in our array

namespace MAC_OS_AudioPlayer {
    class MacAudioPlayer {
    public:
        MacAudioPlayer();
        ~MacAudioPlayer();

        // Receive an audio buffer to player
        auto ReceiveAudioBuffer(int type, const void *data, const int size) -> bool;

        // set volum
        auto setVolum(float volum) -> void {
            audio_volum = volum;
            if (audioQueue) {
                AudioQueueSetParameter(audioQueue, kAudioQueueParam_Volume, volum);
            }
        }
        auto getVolum() -> float {
            if (audioQueue) {
                AudioQueueParameterValue value;
                OSStatus err = AudioQueueGetParameter(audioQueue, kAudioQueueParam_Volume, &value);
                if (!err) {
                    audio_volum = (float)value;
                }
            }
            return audio_volum;
        }

        // the audio queue
        AudioQueueRef  audioQueue;
        // audio queue buffers
        AudioQueueBufferRef audioQueueBuffer[kNumAQBufs];
        // flags to indicate that a buffer is still in use
        bool inuse[kNumAQBufs];
        // the index of the audioQueueBuffer that is being filled
        unsigned int fillBufferIndex;
        // how many bytes have been filled
        size_t bytesFilled;
        // how many packets have been filled
        size_t packetsFilled;
        // packet descriptions for enqueuing audio
        AudioStreamPacketDescription packetDescription[kAQMaxPacketDescs];

        Boolean started;
        Boolean faied;

        // a mutex to protect the inuse flags
        pthread_mutex_t mutex;
        // a condition varable for handling the inuse flags
        pthread_cond_t cond;
        // a condition varable for handling the inuse flags
        pthread_cond_t done;

    private:
        // the audio file stream parser
        AudioFileStreamID  audioFileStream;
        // audio volum
        int audio_volum;
    };
}


#endif //LLPLAYER_MACAUDIOPLAYER_H
