#include "turbo_runner.h"
#include "audio_resampler.h"
#include <iostream>
void cartridge_set_rumble(unsigned){}
void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
struct Input:gambatte::InputGetter{unsigned value=0,previous=0,edges=0;unsigned operator()()override{if(value&&!previous)++edges;previous=value;return value;}};
struct Note{unsigned frame,pitch;};
struct Notes{unsigned frame=0,pitch=0;std::vector<Note> notes;};
bool observe(void* context,unsigned,unsigned,unsigned addr,unsigned value){auto& n=*static_cast<Notes*>(context);if(addr==0xff13)n.pitch=value;if(addr==0xff14&&(value&128))n.notes.push_back({n.frame,n.pitch|((value&7)<<8)});return true;}
int wmain(int argc,wchar_t** argv)try{
    require(argc==2,"Expected Gaiden ROM");auto rom=read(argv[1]);auto profile=identifyGame(rom);
    // Compare the original title BGM against the isolated original sound driver:
    // pitches and note times must match with one constant startup offset.
    bool color=MusicPlayer::driver(rom).color;unsigned titleSong=color?0x8a:1;
    if(color){
        MusicPlayer early(rom,255);early.effect(2);
        std::array<gambatte::uint_least32_t,40000> earlySound{};
        for(unsigned f=0;f<10;f++)early.run(earlySound);
        require(static_cast<unsigned char*>(early.core().rambank0_ptr())[0x2002]==2,"GBC startup discarded an early SFX command");
    }
    auto regular=loadCoreBytes(rom,nullptr);MusicPlayer player(rom,titleSong);Notes normal,isolated;
    regular->setWriteFilter(observe,&normal);player.core().setWriteFilter(observe,&isolated);
    std::array<gambatte::video_pixel_t,160*144> pixels{};std::array<gambatte::uint_least32_t,40000> sound{};
    for(unsigned f=0;f<1200;f++){normal.frame=isolated.frame=f;unsigned samples=35112;regular->runFor(pixels.data(),160,sound.data(),sound.size(),samples);player.run(sound);}
    require(normal.notes.size()>20&&isolated.notes.size()>20,"Missing music notes");
    auto initialization=[](const Note& n){return n.pitch==0;};
    normal.notes.erase(std::remove_if(normal.notes.begin(),normal.notes.end(),initialization),normal.notes.end());
    isolated.notes.erase(std::remove_if(isolated.notes.begin(),isolated.notes.end(),initialization),isolated.notes.end());
    // Skip the original boot's hardware initialization trigger (pitch=0).
    size_t first=0;while(first<normal.notes.size()&&normal.notes[first].frame<400)++first;
    int offset=int(normal.notes[first].frame)-int(isolated.notes[0].frame);
    for(size_t i=0;i<25;i++){
        require(normal.notes[first+i].pitch==isolated.notes[i].pitch,"Original BGM pitch changed");
        require(std::abs(int(normal.notes[first+i].frame)-int(isolated.notes[i].frame)-offset)<=1,"Original BGM tempo changed");
    }
    if(profile.id==GameId::Gaiden3){
        auto d=MusicPlayer::driver(rom);
        for(unsigned song:{12u,17u,21u,28u}){
            MusicPlayer scene(rom,song);
            for(unsigned f=0;f<60;f++)scene.run(sound);
            unsigned expected=static_cast<unsigned char>(rom[d.bank*0x4000+song*3]);
            require(static_cast<unsigned char*>(scene.core().rambank0_ptr())[d.musicBank-0xc000]==expected,"Gaiden 3 selected the wrong song data bank");
        }
    }
    for(int speed:{3,4,6,8}){
        Input input;auto game=loadCoreBytes(rom,&input),reference=loadCoreBytes(rom,nullptr);
        TurboRunner turbo;turbo.attach(*game,rom,profile);require(turbo.speed(speed),"Turbo speed rejected");
        unsigned total=0;Resampler resampler;
        for(unsigned f=0;f<120;f++){unsigned samples=turbo.run(0,input.value,pixels,sound);total+=resampler.convert(sound.data(),samples).size();}
        require(total>130000&&total<133000,"Audio clock accelerated with game");
        for(unsigned f=0;f<120*speed/2;f++){unsigned samples=35112;reference->runFor(pixels.data(),160,sound.data(),sound.size(),samples);}
        std::vector<char> a(game->stateSize()),b(reference->stateSize());game->saveState(a.data());reference->saveState(b.data());
        require(a==b,"Game frame count does not match requested multiplier");
    }
    Input input;auto game=loadCoreBytes(rom,&input);auto boot=skipBootLogos(game,profile,rom,&input,pixels);
    // Advance unsupported startup paths only; a successful skip is at the title.
    if(!boot.skipped&&profile.id!=GameId::Gaiden1)for(unsigned f=0;f<800;f++){unsigned samples=35112;game->runFor(pixels.data(),160,sound.data(),sound.size(),samples);}
    TurboRunner turbo;turbo.attach(*game,rom,profile);require(turbo.speed(4),"2x not supported");
    MusicPlayer audioReference(rom,titleSong);std::array<gambatte::uint_least32_t,40000> expectedSound{};
    for(unsigned f=0;f<120;f++){
        unsigned actual=turbo.run(0,input.value,pixels,sound),expected=audioReference.run(expectedSound);
        require(actual==expected&&std::equal(sound.begin(),sound.begin()+actual,expectedSound.begin()),"2x game changed 1x audio waveform");
    }
    unsigned delivered=0,previousMask=0;
    for(unsigned f=0;f<120;f++){
        turbo.run(16,input.value,pixels,sound);
        if(input.value&&!previousMask)++delivered;previousMask=input.value;
    }
    require(delivered>=18&&delivered<=20,"2x accelerated held-key repeats");
    for(unsigned f=0;f<60;f++)turbo.run(f<2?1:0,input.value,pixels,sound);
    require(turbo.musicCommands()>0,"Scene music commands were not forwarded");
    MusicPlayer withEffect(rom,titleSong),withoutEffect(rom,titleSong);bool effectChangedAudio=false;
    for(unsigned f=0;f<120;f++){
        if(f==60)withEffect.command(false,1);
        unsigned count=withEffect.run(sound);withoutEffect.run(expectedSound);
        if(f>=60&&!std::equal(sound.begin(),sound.begin()+count,expectedSound.begin()))effectChangedAudio=true;
    }
    require(effectChangedAudio,"Original SFX driver produced no effect");
    // Ordinary movement/combat effects use 3A78/DB86 and the VBlank driver,
    // independently of the legacy timer SFX mailbox tested above.
    for(unsigned effect:{2u,3u,7u,12u,19u,23u}){
        MusicPlayer scene(rom,color?0x90:8),quiet(rom,color?0x90:8);bool audible=false,active=false;
        for(unsigned frame=0;frame<180;frame++){
            if(frame==30)scene.effect(effect);
            unsigned count=scene.run(sound);quiet.run(expectedSound);
            if(frame>=30){
                auto ram=static_cast<unsigned char*>(scene.core().rambank0_ptr());
                active|=color?ram[0x2012]!=0:ram[MusicPlayer::driver(rom).effectState-0xc000]==effect;
                audible|=!std::equal(sound.begin(),sound.begin()+count,expectedSound.begin());
            }
        }
        require(active&&audible,"Ordinary scene SFX driver did not run");
    }
    // Real menu navigation must survive short taps, rather than merely counting
    // input edges. A one-frame turbo pulse loses presses during menu transitions.
    if(profile.id==GameId::Gaiden1){
    Input menuInput,fastInput;
    auto menu=loadCoreBytes(rom,&menuInput),fastMenu=loadCoreBytes(rom,&fastInput);
    std::array<gambatte::video_pixel_t,160*144> normalMenu{},turboMenu{};
    skipBootLogos(menu,profile,rom,&menuInput,normalMenu);
    skipBootLogos(fastMenu,profile,rom,&fastInput,turboMenu);
    TurboRunner menuTurbo;menuTurbo.attach(*fastMenu,rom,profile);menuTurbo.speed(4);
    for(unsigned press=0;press<3;press++)for(unsigned frame=0;frame<128;frame++){
        unsigned held=frame<8?1:0;menuInput.value=held;
        unsigned count=35112;menu->runFor(normalMenu.data(),160,sound.data(),sound.size(),count);
        menuTurbo.run(held,fastInput.value,turboMenu,sound);
    }
    require(normalMenu==turboMenu,"2x short taps missed actual tavern menu navigation");
    }
    std::cout<<"PASS: original BGM pitch/tempo, 1.5/2/3/4x game clock, 1x audio clock\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}
