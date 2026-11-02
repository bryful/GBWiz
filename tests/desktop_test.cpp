#include "desktop.h"
#include <iostream>
#include <cmath>
#include <algorithm>
void require(bool ok,const char* message) { if(!ok) throw std::runtime_error(message); }
int main() try {
    SdlRuntime runtime;
    // Real resampling path: one second of a 440 Hz stereo signal with opposite channels.
    Resampler resampler;
    std::vector<int16_t> output;
    unsigned offset=0;
    while(offset<2097152) {
        unsigned count=std::min(35112u,2097152-offset);
        std::vector<int16_t> input(count*2);
        for(unsigned i=0;i<count;++i) {
            input[2*i]=static_cast<int16_t>(12000*std::sin(2*3.141592653589793*440*(offset+i)/2097152));
            input[2*i+1]=-input[2*i];
        }
        auto pcm=resampler.convert(input.data(),count); output.insert(output.end(),pcm.begin(),pcm.end()); offset+=count;
    }
    require(output.size()/2>=32760&&output.size()/2<=32768,"Incorrect output sample rate");
    int crossings=0,peak=0;
    for(size_t i=200;i<output.size()/2;++i) {
        if(output[2*i]>0&&output[2*(i-1)]<=0) ++crossings;
        peak=std::max(peak,std::abs(static_cast<int>(output[2*i])));
        require(std::abs(output[2*i]+output[2*i+1])<=4,"Stereo channel corruption");
    }
    std::cout<<"audio measurements: frames="<<output.size()/2<<" crossings="<<crossings<<" peak="<<peak<<"\n";
    // The bundled fixed-point filter deliberately attenuates its output.
    require(crossings>=430&&crossings<=441&&peak>3500&&peak<6500,"Pitch or volume corrupted by resampler");
    resampler.reset();
    std::vector<int16_t> silence(35112*2);
    auto quiet=resampler.convert(silence.data(),35112);
    require(std::all_of(quiet.begin(),quiet.end(),[](int16_t v){return v==0;}),"Audio reset retained stale samples");
    std::cout<<"PASS: 2 MHz -> 32768 Hz, stereo, pitch, reset\n";

    // Exercise actual SDL controller polling, trigger threshold, edges, and hot unplug.
    SDL_VirtualJoystickDesc desc{}; desc.version=SDL_VIRTUAL_JOYSTICK_DESC_VERSION;
    desc.type=SDL_JOYSTICK_TYPE_GAMECONTROLLER; desc.naxes=SDL_CONTROLLER_AXIS_MAX;
    desc.nbuttons=SDL_CONTROLLER_BUTTON_MAX; desc.name="GBWiz test controller";
    desc.button_mask=(1u<<SDL_CONTROLLER_BUTTON_MAX)-1;
    desc.axis_mask=(1u<<SDL_CONTROLLER_AXIS_MAX)-1;
    int index=SDL_JoystickAttachVirtualEx(&desc); require(index>=0,"Virtual controller unavailable");
    auto joystick=SDL_JoystickOpen(index); require(joystick!=nullptr,"Virtual joystick open failed");
    Controllers pads; Buttons buttons; pads.poll(buttons);
    SDL_JoystickSetVirtualButton(joystick,SDL_CONTROLLER_BUTTON_B,1);
    SDL_JoystickSetVirtualButton(joystick,SDL_CONTROLLER_BUTTON_DPAD_RIGHT,1);
    SDL_JoystickSetVirtualButton(joystick,SDL_CONTROLLER_BUTTON_LEFTSHOULDER,1);
    SDL_JoystickSetVirtualButton(joystick,SDL_CONTROLLER_BUTTON_RIGHTSHOULDER,1);
    SDL_JoystickSetVirtualButton(joystick,SDL_CONTROLLER_BUTTON_LEFTSTICK,1);
    SDL_JoystickSetVirtualButton(joystick,SDL_CONTROLLER_BUTTON_RIGHTSTICK,1);
    SDL_JoystickSetVirtualAxis(joystick,SDL_CONTROLLER_AXIS_TRIGGERLEFT,32767);
    buttons={}; pads.poll(buttons);
    require((buttons.game&0x11)==0x11,"NES face button / D-pad mapping failed");
    require(buttons.actions[Menu]&&buttons.actions[Save1]&&buttons.actions[Load1],"Shortcut mapping failed");
    require(buttons.actions[FloorUp]&&buttons.actions[FloorDown],"LB/RB floor mapping failed");
    require(swapAB(buttons.game,true)==0x12,"AB swap changed directional input");
    Edges edges; require(edges.update(buttons)[Save1],"Missing rising edge");
    require(!edges.update(buttons)[Save1],"Held trigger repeated state save");
    SDL_JoystickClose(joystick); require(SDL_JoystickDetachVirtual(index)==0,"Virtual unplug failed");
    buttons={}; pads.poll(buttons); require(buttons.game==0,"Unplug retained held buttons");
    std::cout<<"PASS: SDL controller mapping, shortcuts, swap, edges, unplug\n";
    Audio audio;
    require(audio.available(),"Dummy audio endpoint failed");
    audio.push(silence.data(),35112); audio.clear();
    std::cout<<"PASS: SDL audio queue and clear\n";
    return 0;
} catch(const std::exception& e) { std::cerr<<e.what()<<"\n"; return 1; }
