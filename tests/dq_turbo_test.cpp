#include "turbo_runner.h"
#include "audio_resampler.h"
#include <iostream>
void cartridge_set_rumble(unsigned){}
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
struct Input:gambatte::InputGetter{unsigned value=0;unsigned operator()()override{return value;}};
struct Notes{unsigned frame=0,pitch=0,begin=496,end=850;std::vector<unsigned> pitches,frames;};
bool observe(void* data,unsigned,unsigned,unsigned address,unsigned value){
    auto& n=*static_cast<Notes*>(data);
    if(address==0xff13)n.pitch=value;
    if(address==0xff14&&(value&128)&&n.frame>=n.begin&&n.frame<n.end){n.pitches.push_back(n.pitch|((value&7)<<8));n.frames.push_back(n.frame);}
    return true;
}
int wmain(int argc,wchar_t** argv)try{
    require(argc==2||argc==3,"Expected DQ3 ROM and optional walking checkpoint");auto rom=read(argv[1]);auto profile=identifyGame(rom);
    require(profile.id==GameId::Dq3&&MusicPlayer::supported(rom),"DQ3 sound driver not recognized");
    std::array<gambatte::video_pixel_t,160*144> pixels{};
    std::array<gambatte::uint_least32_t,40000> sound{},referenceSound{};
    auto regular=loadCoreBytes(rom,nullptr);Notes native,isolated;
    isolated.begin=0;isolated.end=354;
    regular->setWriteFilter(observe,&native);
    MusicPlayer music(rom,11);music.core().setWriteFilter(observe,&isolated);
    for(unsigned frame=0;frame<850;frame++){
        native.frame=frame;unsigned count=35112;regular->runFor(pixels.data(),160,sound.data(),sound.size(),count);
        // Align the captured interval with the isolated song's beginning.
        isolated.frame=frame;music.run(sound);
    }
    require(native.pitches.size()>=7&&isolated.pitches.size()>=7,"Missing DQ3 music notes");
    require(std::equal(native.pitches.begin(),native.pitches.begin()+7,isolated.pitches.begin()),"DQ3 original BGM pitches changed");
    // Compare consecutive intervals: the native title reference differs
    // from isolated playback by up to three frames for these seven notes.
    for(unsigned i=1;i<7;i++)require(std::abs(int(native.frames[i]-native.frames[i-1])-int(isolated.frames[i]-isolated.frames[i-1]))<=3,"DQ3 original BGM tempo changed");
    for(int speed:{3,4,6,8}){
        Input input;auto game=loadCoreBytes(rom,&input),reference=loadCoreBytes(rom,nullptr);
        TurboRunner turbo;turbo.attach(*game,rom,profile);require(turbo.speed(speed),"DQ3 speed rejected");
        Resampler audio;unsigned samples=0;
        for(unsigned f=0;f<120;f++)samples+=audio.convert(sound.data(),turbo.run(0,input.value,pixels,sound)).size();
        require(samples>130000&&samples<133000,"DQ3 audio clock accelerated");
        for(unsigned f=0;f<120*speed/2;f++){unsigned count=35112;reference->runFor(pixels.data(),160,referenceSound.data(),referenceSound.size(),count);}
        std::vector<char> a(game->stateSize()),b(reference->stateSize());game->saveState(a.data());reference->saveState(b.data());
        require(a==b,"DQ3 game clock has wrong multiplier");
    }
    Input input;auto game=loadCoreBytes(rom,&input);TurboRunner turbo;turbo.attach(*game,rom,profile);turbo.speed(4);
    for(unsigned f=0;f<650;f++)turbo.run(0,input.value,pixels,sound);
    require(turbo.musicCommands()>0&&turbo.effectCommands()>0,"DQ3 original music/SFX commands were not forwarded");
    MusicPlayer effect(rom,11),quiet(rom,11);bool changed=false;
    for(unsigned f=0;f<180;f++){
        if(f==60)effect.effect(16);
        unsigned count=effect.run(sound);quiet.run(referenceSound);
        if(f>=60&&!std::equal(sound.begin(),sound.begin()+count,referenceSound.begin()))changed=true;
    }
    require(changed,"DQ3 menu SFX produced no sound");
    auto unknown=rom;unknown[0x31d9]^=1;require(!MusicPlayer::supported(unknown),"Unknown DQ3 driver revision accepted");
    if(argc==3){
        auto checkpoint=read(argv[2]);
        for(int speed:{3,4,6,8}){
            Input input,referenceInput;referenceInput.value=32;
            auto walking=loadCoreBytes(rom,&input),reference=loadCoreBytes(rom,&referenceInput);
            require(walking->loadState(checkpoint.data(),checkpoint.size())&&reference->loadState(checkpoint.data(),checkpoint.size()),"Invalid walking checkpoint");
            TurboRunner runner;runner.attach(*walking,rom,profile);require(runner.speed(speed),"Walking speed rejected");
            std::array<gambatte::video_pixel_t,160*144> expected{};unsigned remainder=0;
            for(unsigned tick=0;tick<90;tick++){
                // Stay on the checkpoint's open path, then verify release.
                referenceInput.value=tick<32?32:0;
                runner.run(referenceInput.value,input.value,pixels,sound);remainder+=speed;
                for(unsigned f=0;f<remainder/2;f++){unsigned count=35112;reference->runFor(expected.data(),160,referenceSound.data(),referenceSound.size(),count);}
                remainder%=2;
                if(pixels!=expected){std::cerr<<"Walking mismatch speed="<<speed<<" tick="<<tick<<"\n";require(false,"DQ3 held walking stutters relative to continuous directional input");}
            }
        }
        std::cout<<"PASS: held walking matches continuous input at all four speeds\n";
    }
    std::cout<<"PASS: DQ3 original BGM pitches/intervals (3-frame tolerance), four game speeds, 1x audio, real command forwarding and SFX\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}
