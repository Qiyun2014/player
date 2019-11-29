//
// Created by qiyun on 2019/11/21.
//

#ifndef LLPLAYER_SDL_AUDIOPLAYER_H
#define LLPLAYER_SDL_AUDIOPLAYER_H

#include <SDL2/SDL.h>

namespace SDL_Audio {
    class SDL_AudioPlayer {
        SDL_AudioPlayer();
        ~SDL_AudioPlayer();
    private:
        SDL_Event       event;
        SDL_AudioSpec   wanted_spec, spec;
    };
}


#endif //LLPLAYER_SDL_AUDIOPLAYER_H
