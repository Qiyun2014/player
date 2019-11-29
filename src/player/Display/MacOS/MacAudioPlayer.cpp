//
// Created by qiyun on 2019/11/20.
//

#include "MacAudioPlayer.h"
#include <pthread.h>

#define PRINTERROR(LABEL)	printf("%s err %4.4s %ld\n", LABEL, (char *)&err, err)

namespace MAC_OS_AudioPlayer {

    int FindQueueBuffer(AudioQueueBufferRef audioQueueBuffer[kNumAQBufs], AudioQueueBufferRef inBuffer) {
        for (unsigned int i = 0; i < kNumAQBufs; i ++) {
            if (inBuffer == audioQueueBuffer[i]) {
                return i;
            }
        }
        return -1;
    }

    void AudioQueueOutputCallback(
            void * __nullable       inUserData,
            AudioQueueRef           inAQ,
            AudioQueueBufferRef     inBuffer ) {

        // this is called by the audio queue when it has finished decoding our data.
        // The buffer is now free to be reused.
        MacAudioPlayer *audioPlayer = (MacAudioPlayer *)inUserData;
        unsigned int bufIndex = FindQueueBuffer(audioPlayer->audioQueueBuffer, inBuffer);
        // signal waiting thread that the buffer is free.
        pthread_mutex_lock(&audioPlayer->mutex);
        audioPlayer->inuse[bufIndex] = false;
        pthread_cond_signal(&audioPlayer->cond);
        pthread_mutex_unlock(&audioPlayer->mutex);
    }

    void AudioQueueIsRunningCallback(
            void * __nullable       inUserData,
            AudioQueueRef           inAQ,
            AudioQueuePropertyID    inID ) {

        MacAudioPlayer *audioPlayer = (MacAudioPlayer *)inUserData;
        UInt32 running;
        UInt32 size;
        OSStatus err = AudioQueueGetProperty(inAQ, kAudioQueueProperty_IsRunning, &running, &size);
        if (err) {
            PRINTERROR("get kAudioQueueProperty_IsRunning");
            audioPlayer->faied = true;
            return;
        }

        if (!running) {
            pthread_mutex_lock(&audioPlayer->mutex);
            pthread_cond_signal(&audioPlayer->done);
            pthread_mutex_unlock(&audioPlayer->mutex);
        }
    }

    void AudioStream_PropertyListenerProc(
            void *							inClientData,
            AudioFileStreamID				inAudioFileStream,
            AudioFileStreamPropertyID		inPropertyID,
            AudioFileStreamPropertyFlags *	ioFlags) {

        MacAudioPlayer *audioPlayer = (MacAudioPlayer *)inClientData;
        OSStatus err = noErr;
        printf("Found property '%c%c%c%c' \n", (char)(inPropertyID>>24)&255, (char)(inPropertyID>>16)&255, (char)(inPropertyID>>8)&255,(char)inPropertyID&255);

        switch (inPropertyID) {
            case kAudioFileStreamProperty_ReadyToProducePackets:
            {
                // the file stream parser is now ready to produce audio packets.
                // get the stream format.
                AudioStreamBasicDescription asbd;
                UInt32 asbdSize = sizeof(asbd);
                err = AudioFileStreamGetProperty(inAudioFileStream, kAudioFileStreamProperty_DataFormat, &asbdSize, &asbd);
                if (err) {
                    PRINTERROR("get kAudioFileStreamProperty_DataFormat");
                    audioPlayer->faied = true;
                    break;
                }

                // create the audio queue
                err = AudioQueueNewOutput(&asbd, AudioQueueOutputCallback, audioPlayer, NULL, NULL, 0, &audioPlayer->audioQueue);
                if (err) {
                    PRINTERROR("AudioQueueNewOutput");
                    audioPlayer->faied = true;
                    break;
                }

                // allocate audio queue buffers
                for (unsigned int i = 0; i < kNumAQBufs; ++ i) {
                    err = AudioQueueAllocateBuffer(audioPlayer->audioQueue, kAQBufSize, &audioPlayer->audioQueueBuffer[i]);
                    if (err) {
                        PRINTERROR("AudioQueueAllocateBuffer");
                        audioPlayer->faied = true;
                        break;
                    }
                }

                // get the cookie size
                uint32_t cookieSize;
                Boolean writable;
                err = AudioFileStreamGetPropertyInfo(inAudioFileStream, kAudioFileStreamProperty_MagicCookieData, &cookieSize, &writable);
                if (err) {
                    PRINTERROR("info kAudioFileStreamProperty_MagicCookieData");
                    audioPlayer->faied = true;
                    break;
                }
                printf("cookieSize %d\n", (unsigned int)cookieSize);

                // get the cookie data
                void * cookieData = calloc(1, cookieSize);
                err = AudioFileStreamGetProperty(inAudioFileStream, kAudioFileStreamProperty_MagicCookieData, &cookieSize, cookieData);
                if (err) {
                    PRINTERROR("get kAudioFileStreamProperty_MagicCookieData");
                    free(cookieData);
                    audioPlayer->faied = true;
                    break;
                }

                // set the cookie on the queue
                err = AudioQueueSetProperty(audioPlayer->audioQueue, kAudioQueueProperty_MagicCookie, cookieData, cookieSize);
                if (err) {
                    PRINTERROR("set kAudioQueueProperty_MagicCookie");
                    audioPlayer->faied = true;
                    break;
                }

                // listen for kAudioQueueProperty_IsRunning
                err = AudioQueueAddPropertyListener(audioPlayer->audioQueue, kAudioQueueProperty_IsRunning, AudioQueueIsRunningCallback, audioPlayer);
                if (err) {
                    PRINTERROR("AudioQueueAddPropertyListener");
                    audioPlayer->faied = true;
                    break;
                }

                break;
            }
        }
    }

    OSStatus StartQueueIfNeeded(MacAudioPlayer *audioPlayer) {
        OSStatus err = noErr;
        if (!audioPlayer->started) {
            err = AudioQueueStart(audioPlayer->audioQueue, NULL);
            if (err) {
                PRINTERROR("AudioQueueStart0");
                audioPlayer->faied = true;
                return err;
            }
            audioPlayer->started = true;
        }
        return err;
    }

    OSStatus EnqueueBuffer(MacAudioPlayer *audioPlayer) {
        OSStatus err = noErr;
        // set in use flag
        audioPlayer->inuse[audioPlayer->fillBufferIndex] = true;
        // enqueue buffer
        AudioQueueBufferRef fillBuf = audioPlayer->audioQueueBuffer[audioPlayer->fillBufferIndex];
        fillBuf->mAudioDataByteSize = audioPlayer->bytesFilled;
        err = AudioQueueEnqueueBuffer(audioPlayer->audioQueue, fillBuf, audioPlayer->packetsFilled, audioPlayer->packetDescription);
        if (err) {
            PRINTERROR("AudioQueueEnqueueBuffer");
            audioPlayer->faied = true;
            return err;
        }
        StartQueueIfNeeded(audioPlayer);
    }


    void WaitForFreeBuffer(MacAudioPlayer *audioPlayer) {
        // go to next buffer
        if (++ audioPlayer->fillBufferIndex >= kNumAQBufs) {
            audioPlayer->fillBufferIndex = 0;
        }
        // reset bytes filled
        audioPlayer->bytesFilled = 0;
        // reset packets filled
        audioPlayer->packetsFilled = 0;
        // wait until next buffer is not in use
        pthread_mutex_lock(&audioPlayer->mutex);
        while (audioPlayer->inuse[audioPlayer->fillBufferIndex]) {
            pthread_cond_wait(&audioPlayer->cond, &audioPlayer->mutex);
        }
        pthread_mutex_unlock(&audioPlayer->mutex);
    }


    void AudioStream_PacketsProc(
            void *							inClientData,
            UInt32							inNumberBytes,
            UInt32							inNumberPackets,
            const void *					inInputData,
            AudioStreamPacketDescription	*inPacketDescriptions) {

        // this is called by audio file stream when it finds packets of audio
        MacAudioPlayer *audioPlayer = (MacAudioPlayer *)inClientData;
        printf("got data.  bytes: %d  packets: %d\n", (unsigned int)inNumberBytes, (unsigned int)inNumberPackets);

        // the following code assumes we're streaming VBR data. for CBR data, you'd need another code branch here.
        for (int i = 0; i < inNumberPackets; ++ i) {
            SInt64 packetOffset = inPacketDescriptions[i].mStartOffset;
            SInt64 packetSize   = inPacketDescriptions[i].mDataByteSize;
            // if the space remaining in the buffer is not enough for this packet, then enqueue the buffer.
            size_t bufSpaceRemaining = kAQBufSize - audioPlayer->bytesFilled;
            if (bufSpaceRemaining < packetSize) {
                EnqueueBuffer(audioPlayer);
                WaitForFreeBuffer(audioPlayer);
            }
            // copy data to the audio queue buffer
            AudioQueueBufferRef fillBuf = audioPlayer->audioQueueBuffer[audioPlayer->fillBufferIndex];
            memcpy((char *)fillBuf->mAudioData + audioPlayer->bytesFilled, (const char *)inInputData + packetOffset, packetSize);
            // fill out packet description
            audioPlayer->packetDescription[audioPlayer->packetsFilled] = inPacketDescriptions[i];
            audioPlayer->packetDescription[audioPlayer->packetsFilled].mStartOffset = audioPlayer->bytesFilled;
            // keep track of bytes filled and packets filled
            audioPlayer->bytesFilled += packetSize;
            audioPlayer->packetsFilled += 1;
            // if that was the last free packet description, then enqueue the buffer.
            size_t packetsDescsRemaining = kAQMaxPacketDescs - audioPlayer->packetsFilled;
            if (packetsDescsRemaining == 0) {
                EnqueueBuffer(audioPlayer);
                WaitForFreeBuffer(audioPlayer);
            }
        }
    }




    MacAudioPlayer::MacAudioPlayer() {
        pthread_mutex_init(&mutex, NULL);
        pthread_cond_init(&cond, NULL);
        pthread_cond_init(&done, NULL);

        OSStatus err = AudioFileStreamOpen(this, AudioStream_PropertyListenerProc, AudioStream_PacketsProc, kAudioFileAAC_ADTSType, &audioFileStream);
        if (err) {
            PRINTERROR("AudioFileStreamOpen");
            faied = true;
            return ;
        }
    }


    MacAudioPlayer::~MacAudioPlayer() {
        OSStatus err = AudioQueueFlush(audioQueue);
        if (err) {
            PRINTERROR("AudioQueueFlush");
        }

        err = AudioQueueStop(audioQueue, false);
        if (err) {
            PRINTERROR("AudioQueueStop");
        }

        pthread_mutex_lock(&mutex);
        pthread_cond_wait(&done, &mutex);
        pthread_mutex_unlock(&mutex);

        AudioFileStreamClose(audioFileStream);
        AudioQueueDispose(audioQueue, false);

        pthread_mutex_destroy(&mutex);
        pthread_cond_destroy(&cond);
        pthread_cond_destroy(&done);
    }


    auto MacAudioPlayer::ReceiveAudioBuffer(int type, const void *data, const int size) -> bool {
        if (audioFileStream == NULL) {
            return false;
        }
        OSStatus err = AudioFileStreamParseBytes(audioFileStream, size, data, 0);
        if (err) {
            PRINTERROR("AudioFileStreamParseBytes");
            return FALSE;
        }
        return TRUE;
    }
}