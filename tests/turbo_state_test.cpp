#include "turbo_runner.h"
#include <iostream>
void cartridge_set_rumble(unsigned){}
void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
int wmain(int argc,wchar_t** argv)try{
    require(argc==2,"Expected supported ROM");auto rom=read(argv[1]);auto profile=identifyGame(rom);
    auto game=loadCoreBytes(rom,nullptr);
    std::array<gambatte::video_pixel_t,160*144> pixels{};
    std::array<gambatte::uint_least32_t,40000> sound{};unsigned input=0;
    skipBootLogos(game,profile,rom,nullptr,pixels);
    TurboRunner turbo;turbo.attach(*game,rom,profile);require(turbo.speed(3),"1.5x rejected");
    for(int f=0;f<121;f++)turbo.run(0,input,pixels,sound);
    auto state=turbo.saveState();require(std::memcmp(state.data(),"GBWZST02",8)==0,"Music state was not stored");
    auto truncated=state;truncated.pop_back();require(!turbo.loadState(truncated),"Truncated state accepted");
    auto wrongGame=state;wrongGame[28]=0;require(!turbo.loadState(wrongGame),"Wrong game profile accepted");
    auto wrongLength=state;for(int i=12;i<16;i++)wrongLength[i]=static_cast<char>(0xff);
    require(!turbo.loadState(wrongLength),"Overflowing state length accepted");
    std::vector<std::vector<gambatte::uint_least32_t>> expected;
    std::vector<decltype(pixels)> expectedVideo;
    for(int f=0;f<60;f++){
        unsigned count=turbo.run(0,input,pixels,sound);expected.emplace_back(sound.begin(),sound.begin()+count);expectedVideo.push_back(pixels);
    }
    require(turbo.speed(8),"4x rejected");for(int f=0;f<30;f++)turbo.run(0,input,pixels,sound);
    require(turbo.loadState(state)&&turbo.speed()==3,"Restore lost speed or rejected state");
    auto badMusic=state;unsigned gameLength=0;for(unsigned i=0;i<4;i++)gameLength|=unsigned(static_cast<unsigned char>(state[12+i]))<<(8*i);
    std::fill(badMusic.begin()+32+gameLength,badMusic.end(),0);
    require(!turbo.loadState(badMusic),"Corrupt music accepted");
    auto check=[&](TurboRunner& runner){
        for(int f=0;f<60;f++){
            unsigned count=runner.run(0,input,pixels,sound);
            require(count==expected[f].size(),"Restore changed the audio clock");
            require(std::equal(sound.begin(),sound.begin()+count,expected[f].begin()),"Restore changed BGM waveform/position");
            require(pixels==expectedVideo[f],"Restore changed fractional-speed game position");
        }
    };
    check(turbo);
    auto otherGame=loadCoreBytes(rom,nullptr);TurboRunner other;other.attach(*otherGame,rom,profile);
    require(other.loadState(state)&&other.speed()==3,"Fresh session could not restore audio state");check(other);
    std::vector<char> legacy(state.begin()+32,state.begin()+32+gameLength);
    require(other.loadState(legacy),"Legacy game-only state no longer loads");
    std::cout<<"PASS: music/game/speed restore, identical next 60 audio/video frames, fresh session, legacy and corrupt states\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}
