//
// Created by engin on 11.07.2018.
//

#ifndef OPENAL_ALHELPER_H
#define OPENAL_ALHELPER_H

#include "../libs/OpenAL-Soft/include/AL/alc.h"
#include "../libs/OpenAL-Soft/include/AL/al.h"

#include <memory>
#include <unordered_map>
#include <vector>
#include <iostream>

#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <SDL_atomic.h>
#include <SDL_thread.h>

class SoundAsset;

#define NUM_BUFFERS 3
#define BUFFER_ELEMENT_COUNT 8192

class ALHelper {
    friend class World;

    struct PlayingSound {
        uint32_t soundID;
        const SoundAsset *asset;
        uint64_t sampleCountToPlay;
        ALuint source = 0;
        ALenum format;
        ALuint buffers[NUM_BUFFERS];
        const int16_t *nextDataToBuffer;
        bool looped;
        glm::vec3 position = glm::vec3(0,0,0);
        bool isPositionRelative = true;
        bool isFinished();
        PlayingSound(uint32_t id): soundID(id) {};

        ~PlayingSound();
    };

    SDL_SpinLock playRequestLock;
    SDL_Thread *thread = nullptr;

    ALCdevice *dev;
    ALCcontext *ctx;

    glm::vec3 ListenerPosition;
    bool running = true;
    bool paused = false;
    bool resumed = false;
    uint32_t soundRequestID = 1;
    std::unordered_map<uint32_t, std::unique_ptr<PlayingSound>> playingSounds;
    std::vector<std::unique_ptr<PlayingSound>> playRequests;

    static inline ALenum to_al_format(short channels, short samples) {
        bool stereo = (channels > 1);

        switch (samples) {
            case 16:
                if (stereo)
                    return AL_FORMAT_STEREO16;
                else
                    return AL_FORMAT_MONO16;
            case 8:
                if (stereo)
                    return AL_FORMAT_STEREO8;
                else
                    return AL_FORMAT_MONO8;
            default:
                return -1;
        }
    }

    static int staticSoundManager(void *objectPointer) {
        return static_cast<ALHelper *>(objectPointer)->soundManager();
    }

    int soundManager();

    bool startPlay(std::unique_ptr<PlayingSound> &sound);

    bool refreshBuffers(std::unique_ptr<PlayingSound> &sound);//this method updates some of the values of parameter

    uint32_t getNextRequestID(){
        return soundRequestID++;
    }

    void pausePlay() {
        this->paused = true;
    }

    void resumePlay() {
        this->resumed = true;
    }

public:
    ALHelper();

    ~ALHelper();

    uint32_t play(const SoundAsset *soundAsset, bool looped);

    bool isPlaying(uint32_t soundID) {
        if(playingSounds.find(soundID) != playingSounds.end()) {
            return playingSounds[soundID]->looped || !playingSounds[soundID]->isFinished();
        }
        //it is possible that play is requested, but not yet started, they should be considered playing too, check it
        bool result = false;
        SDL_AtomicLock(&playRequestLock);
        for (auto request = playRequests.begin(); request != playRequests.end(); ++request) {
            if((*request)->soundID == soundID) {
                result = true;
                break;
            }
        }
        SDL_AtomicUnlock(&playRequestLock);
        return result;
    }

    uint32_t stop(uint32_t soundID);

    inline void setListenerPositionAndOrientation(const glm::vec3 &position, const glm::vec3 &front, const glm::vec3 &up) {
        glm::vec3 velocity = this->ListenerPosition - position;
        this->ListenerPosition = position;
        ALfloat listenerOri[] = {front.x, front.y, front.z,
                                 up.x, up.y, up.z};
        ALenum error;

// Position ...
        alListenerfv(AL_POSITION, glm::value_ptr(this->ListenerPosition));
        if ((error = alGetError()) != AL_NO_ERROR) {
            std::cerr << "Set listener position failed! " << alGetString(error) << std::endl;
            return;
        }
// Velocity ...
        alListenerfv(AL_VELOCITY, glm::value_ptr(velocity));
        if ((error = alGetError()) != AL_NO_ERROR) {
            std::cerr << "Set listener velocity failed! " << alGetString(error) << std::endl;
            return;
        }
// Orientation ...
        alListenerfv(AL_ORIENTATION, listenerOri);
        if ((error = alGetError()) != AL_NO_ERROR) {
            std::cerr << "Set listener oerientation failed! " << alGetString(error) << std::endl;
            return;
        }
    }

    void setSourcePosition(uint32_t soundID, bool isCameraRelative, const glm::vec3 &soundPosition) {
        if(playingSounds.find(soundID) != playingSounds.end()) {
            std::unique_ptr<PlayingSound>& sound =  playingSounds[soundID];

            if(isCameraRelative != sound->isPositionRelative) {
                if (isCameraRelative) {
                    alSourcei(sound->source, AL_SOURCE_RELATIVE, AL_TRUE);
                } else {
                    alSourcei(sound->source, AL_SOURCE_RELATIVE, AL_FALSE);
                }
                sound->isPositionRelative = isCameraRelative;
            }

            if(sound->position != soundPosition) {
                alSource3f(sound->source, AL_POSITION, soundPosition.x, soundPosition.y, soundPosition.z);

                alSource3f(sound->source, AL_VELOCITY, soundPosition.x - sound->position.x,
                           soundPosition.y - sound->position.y,
                           soundPosition.z - sound->position.z);
                sound->position = soundPosition;
            }

            ALenum error;
            if ((error = alGetError()) != AL_NO_ERROR) {
                std::cerr << "Error setting source position! " << alGetString(error) << std::endl;
                return;
            }
        }
    }

    bool setLooped(uint32_t soundID, bool looped);
};


#endif //OPENAL_ALHELPER_H
