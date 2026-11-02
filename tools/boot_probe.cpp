#include "game_support.h"
#include <array>
#include <iostream>
void cartridge_set_rumble(unsigned) {}
int wmain(int argc,wchar_t** argv) {
    if(argc!=3)return 1;
    auto core=loadCore(argv[1],nullptr);fs::path output=argv[2];fs::create_directories(output);
    std::array<gambatte::video_pixel_t,160*144> pixels{};
    std::array<gambatte::uint_least32_t,40000> audio{};
    auto previous=pixels;int stable=0;
    for(int frame=1;frame<=2400;++frame){
        unsigned samples=35112;auto produced=core->runFor(pixels.data(),160,audio.data(),audio.size(),samples);
        stable=pixels==previous?stable+1:0;previous=pixels;
        auto vram=static_cast<unsigned char*>(core->vram_ptr());
        auto io=static_cast<unsigned char*>(core->zeropage_ptr())-0x80;
        if(stable==8||frame%60==0)std::cout<<"Boot frame "<<frame<<" stable="<<stable<<" LCDC="<<int(io[0x40])<<" BGP="<<int(io[0x47])<<" SCY="<<int(io[0x42])<<" SCX="<<int(io[0x43])<<" ready="<<gaiden1TitleReady(*core)<<" gaiden2Ready="<<gaiden2TitleReady(*core)<<" gaiden3Ready="<<gaiden3TitleReady(*core)<<" produced="<<produced<<"\n";
        if(frame==405||frame==360||frame==899){std::ofstream bytes(output/(std::to_wstring(frame)+L".vram"),std::ios::binary);bytes.write(static_cast<char*>(core->vram_ptr()),0x4000);}
        if(frame!=1&&frame%120!=0)continue;
        std::array<unsigned char,54> header{};
        header[0]='B';header[1]='M';header[10]=54;header[14]=40;header[18]=160;header[22]=144;header[26]=1;header[28]=32;
        unsigned size=54+pixels.size()*4;std::memcpy(header.data()+2,&size,4);
        std::ofstream image(output/(std::to_wstring(frame)+L".bmp"),std::ios::binary);image.write(reinterpret_cast<char*>(header.data()),header.size());
        for(int y=143;y>=0;--y)image.write(reinterpret_cast<const char*>(pixels.data()+y*160),160*4);
        std::ofstream dump(output/(std::to_wstring(frame)+L".vram"),std::ios::binary);dump.write(static_cast<char*>(core->vram_ptr()),0x4000);
        std::cout<<"Captured frame "<<frame<<"\n";
    }
}
