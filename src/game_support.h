#pragma once
#include "game.h"
#include <array>
#include <algorithm>
#include <string>

enum class GameId { Unknown,Gaiden1,Gaiden2,Gaiden3,Wizardry1,Wizardry2,Wizardry3,Dq3,Dq12 };
struct GameProfile {GameId id;const char* name;};
inline GameProfile identifyGame(const std::vector<char>& rom){
    if(rom.size()<0x150)return {GameId::Unknown,"Unknown"};
    bool color=(static_cast<unsigned char>(rom[0x143])&0x80)!=0;
    std::string title(rom.data()+0x134,color?11:16);
    auto end=title.find('\0');if(end!=std::string::npos)title.resize(end);
    while(!title.empty()&&title.back()==' ')title.pop_back();
    if(!color){
        if(title=="WIZARDRY SOQ")return {GameId::Gaiden1,"Wizardry Gaiden 1 (GB)"};
        if(title=="WIZARDRY CAE")return {GameId::Gaiden2,"Wizardry Gaiden 2 (GB)"};
        if(title=="WIZARDRY SOD")return {GameId::Gaiden3,"Wizardry Gaiden 3 (GB)"};
    }else{
        if(title=="WIZARDRY1")return {GameId::Wizardry1,"Wizardry 1 (GBC)"};
        if(title=="WIZARDRY2")return {GameId::Wizardry2,"Wizardry 2 (GBC)"};
        if(title=="WIZARDRY3")return {GameId::Wizardry3,"Wizardry 3 (GBC)"};
        if(title=="GBDQ3")return {GameId::Dq3,"Dragon Quest 3 (GBC)"};
        if(title=="DQ1&2")return {GameId::Dq12,"Dragon Quest 1&2 (GBC)"};
    }
    return {GameId::Unknown,"Unknown"};
}
struct BootSkipResult {bool skipped=false;unsigned frames=0;};
inline bool gaiden1TitleReady(gambatte::GB& core){
    auto vram=static_cast<const unsigned char*>(core.vram_ptr());
    // Gambatte's zero-page pointer is ioamhram_ + 0x180 (FF80).
    // The preceding 0x80 bytes are the same backing array's FF00-FF7F registers.
    auto io=static_cast<const unsigned char*>(core.zeropage_ptr())-0x80;
    // The title reveal uses unsigned tiles (93), then switches to its final
    // signed-tile map (8B) while keeping the title atlas intact.
    bool reveal=io[0x40]==0x93&&io[0x42]==0;
    bool finalTitle=io[0x40]==0x8b&&io[0x42]==112;
    if((!reveal&&!finalTitle)||io[0x47]!=0xe4||io[0x43]!=0)return false;
    const unsigned char titleRow[]={0x4f,0x50,0x51,0x52,0x53,0x54,0x50};
    const unsigned char letterW[]={0xee,0xee,0x44,0x44,0x54,0x54,0x54,0x54,0x54,0x54,0x6c,0x6c,0x44,0x44,0,0};
    const unsigned char letterS[]={0x78,0x78,0x88,0x88,0x80,0x80,0x78,0x78,4,4,0x84,0x84,0xf8,0xf8,0,0};
    return std::equal(std::begin(titleRow),std::end(titleRow),vram+0x1800+14*32)&&
        std::equal(std::begin(letterW),std::end(letterW),vram+0x4f0)&&
        std::equal(std::begin(letterS),std::end(letterS),vram+0x430);
}
inline bool gaiden2TitleReady(gambatte::GB& core){
    if(core.isCgb())return false;
    auto vram=static_cast<const unsigned char*>(core.vram_ptr());
    auto io=static_cast<const unsigned char*>(core.zeropage_ptr())-0x80;
    if(io[0x40]!=0x93||io[0x42]||io[0x43]||io[0x47]!=0xe4)return false;
    // The title scroll must finish with both the subtitle and welcome text.
    // Verify the glyph atlas too: earlier scenes reuse the same tile indices.
    const unsigned char subtitle[]={0x44,0x45,0x46,0x47,0x48,0,0x49,0x4a,0,0x4b,0x4c,0x48};
    const unsigned char welcome[]={0x54,0x48,0x55,0x4f,0x49,0x52,0x48,0,0x4b,0x49,0,0x4b,0x4c,0x48,0,0x56,0x49,0x46,0x55,0x57};
    const unsigned char wizardry[]={0x49,0x4a,0,0x54,0x50,0x58,0x59,0x46,0x57,0x46,0x5a};
    const unsigned char letterC[]={0xc3,0xc3,0xbb,0xbb,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0xbb,0xbb,0xc7,0xc7,0xff,0xff};
    const unsigned char letterW[]={0x11,0x11,0xbb,0xbb,0xab,0xab,0xab,0xab,0xab,0xab,0x93,0x93,0xbb,0xbb,0xff,0xff};
    return std::equal(std::begin(subtitle),std::end(subtitle),vram+0x1800+9*32+4)
        &&std::equal(std::begin(welcome),std::end(welcome),vram+0x1800+14*32)
        &&std::equal(std::begin(wizardry),std::end(wizardry),vram+0x1800+16*32+4)
        &&std::equal(std::begin(letterC),std::end(letterC),vram+0x440)
        &&std::equal(std::begin(letterW),std::end(letterW),vram+0x540);
}
inline bool gaiden3TitleReady(gambatte::GB& core){
    if(core.isCgb())return false;
    auto vram=static_cast<const unsigned char*>(core.vram_ptr());
    auto io=static_cast<const unsigned char*>(core.zeropage_ptr())-0x80;
    // Gaiden III uses signed tiles and finishes its title scroll at -24.
    if(io[0x40]!=0x81||io[0x42]!=0xe8||io[0x43]||io[0x47]!=0xe4)return false;
    const unsigned char subtitle[]={0x51,0x4a,0x45,0x49,0x54,0x48,0x53,0x45,0x46};
    const unsigned char welcome[]={0x4e,0x46,0x4f,0x4a,0x47,0x55,0x46,0x44,0x48,0x47,0x44,0x48,0x4c,0x46,0x44,0x56,0x47,0x45,0x4f,0x50};
    const unsigned char wizardry[]={0x47,0x4b,0x44,0x4e,0x49,0x58,0x4d,0x45,0x50,0x45,0x59};
    const unsigned char letterS[]={0xc3,0xc3,0xbd,0xbd,0xbf,0xbf,0xc3,0xc3,0xfd,0xfd,0xbd,0xbd,0xc3,0xc3,0xff,0xff};
    const unsigned char letterW[]={0x11,0x11,0xbb,0xbb,0xab,0xab,0xab,0xab,0xab,0xab,0x93,0x93,0xbb,0xbb,0xff,0xff};
    return std::equal(std::begin(subtitle),std::end(subtitle),vram+0x1800+6*32+5)
        &&std::equal(std::begin(welcome),std::end(welcome),vram+0x1800+11*32)
        &&std::equal(std::begin(wizardry),std::end(wizardry),vram+0x1800+13*32+4)
        &&std::equal(std::begin(letterS),std::end(letterS),vram+0x1510)
        &&std::equal(std::begin(letterW),std::end(letterW),vram+0x14e0);
}
inline bool wizardryColorTitleReady(gambatte::GB& core){
    if(!core.isCgb())return false;
    auto vram=static_cast<const unsigned char*>(core.vram_ptr());
    auto io=static_cast<const unsigned char*>(core.zeropage_ptr())-0x80;
    if((io[0x40]&0xa8)!=0x80||io[0x42]||io[0x43])return false;
    // The three GBC games share the rendered Start game / Select switch menu.
    // Check both map rows and actual glyphs, not tile numbers alone.
    const unsigned char start[]={0xb2,0xb3,0xb4,0xb5,0xb6,0xb7,0xb8,0xb9,0xba,0xbb};
    const unsigned char select[]={0xc6,0xc7,0xc8,0xc9,0xca,0xcb,0xcc,0xcd,0xce,0xcf};
    const unsigned char startGlyph[]={0,0,1,1,1,1,0,0,0,0,1,1,1,1,0,0,0xf0,0xf0,0x91,0x91,0xc3,0xc3,0xf1,0xf1,0x39,0x39,0x19,0x19,0xf0,0xf0,0,0};
    const unsigned char selectGlyph[]={0x78,0x78,0xc8,0xc8,0xe0,0xe0,0x79,0x79,0x1d,0x1d,0x8d,0x8d,0xf8,0xf8,0,0,3,3,3,3,0xf3,0xf3,0x9b,0x9b,0xfb,0xfb,0x83,0x83,0xf3,0xf3,0,0};
    return std::equal(std::begin(start),std::end(start),vram+0x1800+14*32+5)&&
        std::equal(std::begin(select),std::end(select),vram+0x1800+16*32+5)&&
        std::equal(std::begin(startGlyph),std::end(startGlyph),vram+0xb20)&&
        std::equal(std::begin(selectGlyph),std::end(selectGlyph),vram+0xc60);
}
// No ROM/RAM patching or synthetic button presses. Only startup frames are
// emulated without audio output, stopping when the title reveal has completed.
inline BootSkipResult skipBootLogos(std::unique_ptr<gambatte::GB>& running,GameProfile profile,
        const std::vector<char>& rom,gambatte::InputGetter* input,
        std::array<gambatte::video_pixel_t,160*144>& pixels){
    bool color=profile.id==GameId::Wizardry1||profile.id==GameId::Wizardry2||profile.id==GameId::Wizardry3;
    bool gaiden2=profile.id==GameId::Gaiden2;
    bool gaiden3=profile.id==GameId::Gaiden3;
    if((profile.id!=GameId::Gaiden1&&!gaiden2&&!gaiden3&&!color)||running->isCgb()!=color)return {};
    // Probe a separate fresh core. Unknown startup variants leave the running
    // core completely untouched, avoiding save-state parser normalization.
    auto candidate=loadCoreBytes(rom,input);auto& core=*candidate;
    if(core.savedata_size()!=running->savedata_size())return {};
    if(core.savedata_size())std::memcpy(core.savedata_ptr(),running->savedata_ptr(),core.savedata_size());
    auto originalPixels=pixels;pixels.fill(0);
    std::array<gambatte::uint_least32_t,40000> sound{};
    auto previous=pixels;unsigned stable=0;bool sawReveal=false;
    for(unsigned frame=1;frame<=1200;++frame){
        unsigned samples=35112;long produced=core.runFor(pixels.data(),160,sound.data(),sound.size(),samples);
        auto io=static_cast<const unsigned char*>(core.zeropage_ptr())-0x80;
        bool ready=color?wizardryColorTitleReady(core):gaiden2?gaiden2TitleReady(core):gaiden3?gaiden3TitleReady(core):gaiden1TitleReady(core);
        if((color||io[0x40]==(gaiden3?0x81:0x93))&&!ready)sawReveal=true;
        if(produced>=0){stable=pixels==previous?stable+1:0;previous=pixels;}
        bool visible=!color||std::any_of(pixels.begin()+1,pixels.end(),[&](gambatte::video_pixel_t pixel){return pixel!=pixels[0];});
        if(sawReveal&&ready&&visible&&stable>=8){running=std::move(candidate);return {true,frame};}
    }
    pixels=originalPixels;return {};
}
