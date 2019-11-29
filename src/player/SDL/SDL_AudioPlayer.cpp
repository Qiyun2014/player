//
// Created by qiyun on 2019/11/21.
//

#include "SDL_AudioPlayer.h"

namespace SDL_Audio {

    static void SDL_Audio_Callback(void *userdata, Uint8 * stream, int len) {

    }

    SDL_AudioPlayer::SDL_AudioPlayer () {
        if (SDL_Init(SDL_INIT_AUDIO)) {
            printf("Could not initilize SDL ... \n");
        }

        // Set audio settings from codec info
        wanted_spec.freq = 44100;
        wanted_spec.format = AUDIO_S16SYS;
        wanted_spec.channels = 2;
        wanted_spec.silence = 0;
        wanted_spec.samples = 1024;
        wanted_spec.callback = SDL_Audio_Callback;
        wanted_spec.userdata = this;

        if(SDL_OpenAudio(&wanted_spec, &spec) < 0) {
            fprintf(stderr, "SDL_OpenAudio: %s\n", SDL_GetError());
            return;
         }
    }

    SDL_AudioPlayer::~SDL_AudioPlayer () {
        SDL_Quit();
    }
}