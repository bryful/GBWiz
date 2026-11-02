#include <gambatte.h>
#include <iostream>
#include <stdexcept>
void cartridge_set_rumble(unsigned) {}
int main() try {
    gambatte::GB gb;
    gb.setColorCorrection(false);
    gb.setDarkFilterLevel(0);
    // All 32768 CGB colours must cover the full RGB32 range, with channels
    // in RGB order and 5-bit endpoint replication (31 -> 255).
    for(unsigned color=0;color<32768;++color) {
        unsigned r=color&31, g=(color>>5)&31, b=(color>>10)&31;
        unsigned expected=(((r<<3)|(r>>2))<<16)|(((g<<3)|(g>>2))<<8)|((b<<3)|(b>>2));
        if(gb.gbcToRgb32(color)!=expected) throw std::runtime_error("CGB RGB32 colour range or channel order failed");
    }
    gb.setColorCorrection(true);
    auto white=gb.gbcToRgb32(0x7fff);
    if(((white>>16)&255)<230 || ((white>>8)&255)<230 || (white&255)<230)
        throw std::runtime_error("Corrected CGB white is too dark");
    if(gb.gbcToRgb32(0)!=0) throw std::runtime_error("CGB black is not black");
    gb.setColorCorrectionMode(1);
    if(gb.gbcToRgb32(0x7fff)!=0xffffff) throw std::runtime_error("Fast correction white is too dark");
    std::cout<<"PASS: all CGB colours, channel order, corrected white and black\n";
    return 0;
} catch(const std::exception& e) { std::cerr<<e.what()<<"\n"; return 1; }
