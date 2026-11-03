#pragma once
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <gambatte.h>
#include "audio_resampler.h"
#include <array>
#include <vector>
#include <stdexcept>
#include <cstring>

// SDL uses physical Xbox positions by default, including for Nintendo devices.
// NES A is the right face button, NES B is the bottom face button.
inline unsigned controllerButtons(SDL_GameController* pad, bool swapXY=false) {
    unsigned value=0;
    const SDL_GameControllerButton buttons[]={SDL_CONTROLLER_BUTTON_B,SDL_CONTROLLER_BUTTON_A,
        SDL_CONTROLLER_BUTTON_BACK,SDL_CONTROLLER_BUTTON_START,SDL_CONTROLLER_BUTTON_DPAD_RIGHT,
        SDL_CONTROLLER_BUTTON_DPAD_LEFT,SDL_CONTROLLER_BUTTON_DPAD_UP,SDL_CONTROLLER_BUTTON_DPAD_DOWN};
    for(int i=0;i<8;++i) if(SDL_GameControllerGetButton(pad,buttons[i])) value|=1u<<i;
    // SDL normalizes Switch Pro face positions; X/Y use the printed labels.
    if(SDL_GameControllerGetType(pad)==SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_PRO) swapXY=!swapXY;
    if(SDL_GameControllerGetButton(pad,SDL_CONTROLLER_BUTTON_X)) value|=swapXY?4u:8u;
    if(SDL_GameControllerGetButton(pad,SDL_CONTROLLER_BUTTON_Y)) value|=swapXY?8u:4u;
    return value;
}
inline unsigned swapAB(unsigned value, bool swap) {
    return swap ? ((value&~3u)|((value&1u)<<1)|((value&2u)>>1)) : value;
}
enum Action { Menu, Save1, Load1, Up, Down, Left, Right, Confirm, Cancel, Map, FloorUp, FloorDown, ActionCount };
struct Buttons {
    unsigned game=0;
    std::array<bool,ActionCount> actions{};
};
struct Edges {
    std::array<bool,ActionCount> previous{};
    std::array<bool,ActionCount> update(const Buttons& state) {
        std::array<bool,ActionCount> pressed{};
        for(size_t i=0;i<pressed.size();++i) pressed[i]=state.actions[i]&&!previous[i];
        previous=state.actions;
        return pressed;
    }
};
class Controllers {
    std::vector<SDL_GameController*> pads;
public:
    ~Controllers() { for(auto pad:pads) SDL_GameControllerClose(pad); }
    void poll(Buttons& result, bool swapXY=false) {
        SDL_Event event;
        while(SDL_PollEvent(&event)) {} // native window messages are handled by Win32
        for(auto it=pads.begin();it!=pads.end();) {
            if(!SDL_GameControllerGetAttached(*it)) { SDL_GameControllerClose(*it); it=pads.erase(it); }
            else ++it;
        }
        for(int i=0;i<SDL_NumJoysticks();++i) {
            auto id=SDL_JoystickGetDeviceInstanceID(i); bool exists=false;
            for(auto pad:pads) if(SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(pad))==id) exists=true;
            if(!exists&&SDL_IsGameController(i)) if(auto pad=SDL_GameControllerOpen(i)) pads.push_back(pad);
        }
        SDL_GameControllerUpdate();
        for(auto pad:pads) {
            unsigned game=controllerButtons(pad,swapXY); result.game|=game;
            result.actions[Menu]|=SDL_GameControllerGetButton(pad,SDL_CONTROLLER_BUTTON_LEFTSTICK)!=0;
            result.actions[Save1]|=SDL_GameControllerGetAxis(pad,SDL_CONTROLLER_AXIS_TRIGGERLEFT)>16000;
            result.actions[FloorUp]|=SDL_GameControllerGetButton(pad,SDL_CONTROLLER_BUTTON_LEFTSHOULDER)!=0;
            result.actions[FloorDown]|=SDL_GameControllerGetButton(pad,SDL_CONTROLLER_BUTTON_RIGHTSHOULDER)!=0;
            result.actions[Map]|=SDL_GameControllerGetAxis(pad,SDL_CONTROLLER_AXIS_TRIGGERRIGHT)>16000;
            result.actions[Load1]|=SDL_GameControllerGetButton(pad,SDL_CONTROLLER_BUTTON_RIGHTSTICK)!=0;
            result.actions[Up]|=(game&gambatte::InputGetter::UP)!=0;
            result.actions[Down]|=(game&gambatte::InputGetter::DOWN)!=0;
            result.actions[Left]|=(game&gambatte::InputGetter::LEFT)!=0;
            result.actions[Right]|=(game&gambatte::InputGetter::RIGHT)!=0;
            result.actions[Confirm]|=(game&gambatte::InputGetter::A)!=0;
            result.actions[Cancel]|=(game&gambatte::InputGetter::B)!=0;
        }
    }
};
class Audio {
    SDL_AudioDeviceID device=0;
    Resampler resampler;
public:
    Audio() {
        SDL_AudioSpec wanted{}; wanted.freq=32768; wanted.format=AUDIO_S16SYS;
        wanted.channels=2; wanted.samples=512;
        device=SDL_OpenAudioDevice(nullptr,0,&wanted,nullptr,0);
        if(device) SDL_PauseAudioDevice(device,0);
    }
    ~Audio() { if(device) SDL_CloseAudioDevice(device); }
    bool available() const { return device!=0; }
    void clear() { if(device) SDL_ClearQueuedAudio(device); resampler.reset(); }
    void push(const void* data,unsigned samples) {
        if(!device) return;
        auto pcm=resampler.convert(data,samples);
        // Avoid accumulating stale audio after a stalled window or device.
        if(SDL_GetQueuedAudioSize(device)>32768*4/5) SDL_ClearQueuedAudio(device);
        if(SDL_QueueAudio(device,pcm.data(),static_cast<Uint32>(pcm.size()*sizeof(int16_t)))!=0)
            throw std::runtime_error(SDL_GetError());
    }
};
struct SdlRuntime {
    SdlRuntime() {
        SDL_SetMainReady();
        SDL_SetHint(SDL_HINT_GAMECONTROLLER_USE_BUTTON_LABELS,"0");
        SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_SWITCH,"1");
        // A native Win32 window owns focus. SDL must continue polling its devices.
        SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS,"1");
        if(SDL_Init(SDL_INIT_GAMECONTROLLER|SDL_INIT_EVENTS)!=0) throw std::runtime_error(SDL_GetError());
        // A missing audio endpoint must not prevent playing the game.
        SDL_InitSubSystem(SDL_INIT_AUDIO);
    }
    ~SdlRuntime() { SDL_Quit(); }
};
