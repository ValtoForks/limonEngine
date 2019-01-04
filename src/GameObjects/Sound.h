//
// Created by engin on 16.07.2018.
//

#ifndef LIMONENGINE_SOUND_H
#define LIMONENGINE_SOUND_H


#include "GameObject.h"

class SoundAsset;
class AssetManager;

class Sound : public GameObject {
    std::string name;
    uint32_t worldID;
    uint32_t soundHandleID = 0;
    AssetManager *assetManager;

    glm::vec3 position = glm::vec3(0,0,0);
    bool listenerRelative = true; //by default plays at the listener position
    float startSecond = 0;
    float stopPosition = 0;
    bool looped = false;

public:
    Sound(uint32_t worldID, AssetManager *assetManager, const std::string &filename);
    ~Sound();

    void setLoop(bool looped);

    void setStartPosition(float startSecond);

    void setStopPosition(float stopPosition);

    void play();

    void stop();

    void stopAfterFinish();

    void setWorldPosition(glm::vec3 position, bool listenerRelative);
    void setWorldPosition(glm::vec3 position) { setWorldPosition(position, false);};

    /** Game object methods */
    GameObject::ObjectTypes getTypeID() const {
        return SOUND;
    }

    std::string getName() const {
        return name;
    }

    uint32_t getWorldObjectID() const override {
        return worldID;
    }
    /** Game object methods */
};


#endif //LIMONENGINE_SOUND_H
