//
// Created by Engin Manap on 14.02.2016.
//

#ifndef UBERGAME_INPUTHANDLER_H
#define UBERGAME_INPUTHANDLER_H

#include <map>
#include <SDL2/SDL.h>


class InputHandler {
public:
    //FIXME access modifiers should not be like this
    enum states {QUIT, MOUSE_MOVE};
private:
    int height, width;
    SDL_Event event;
    std::map<states,bool> inputStatus;
    Sint16 xRel, yRel;
    float xPos, yPos;
public:
    InputHandler(int, int);

    void mapInput();

    bool getInputStatus(states input){
        return inputStatus[input];
    }

    void getMousePosition(float&, float&);

};

#endif //UBERGAME_INPUTHANDLER_H