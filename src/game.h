#pragma once
#include <gambatte.h>
#include <filesystem>
#include <fstream>
#include <memory>
#include <vector>
#include <cstring>
#include <stdexcept>
#include <limits>
namespace fs = std::filesystem;
inline std::vector<char> read(const fs::path& path) {
    std::ifstream f(path,std::ios::binary);
    return {std::istreambuf_iterator<char>(f),{}};
}
inline std::unique_ptr<gambatte::GB> loadCoreBytes(const std::vector<char>& bytes,gambatte::InputGetter* input) {
    if(bytes.size()<0x150 || bytes.size()>std::numeric_limits<unsigned>::max())
        throw std::runtime_error("ROM is missing or has an invalid size");
    auto core=std::make_unique<gambatte::GB>(); core->setInputGetter(input);
    if(core->load(bytes.data(),static_cast<unsigned>(bytes.size()))!=0)
        throw std::runtime_error("Unsupported ROM");
    for(unsigned p=0;p<3;++p) for(unsigned c=0;c<4;++c)
        core->setDmgPaletteColor(p,c,0xffffffu-(0x555555u*c));
    return core;
}
inline std::unique_ptr<gambatte::GB> loadCore(const fs::path& rom,gambatte::InputGetter* input) {
    return loadCoreBytes(read(rom),input);
}
inline void restoreBattery(gambatte::GB& core,const fs::path& save) {
    if(!fs::exists(save)) return;
    auto ram=read(save);
    if(ram.size()!=core.savedata_size())
        throw std::runtime_error("Existing save has unexpected size; refusing to overwrite");
    if(!ram.empty()) std::memcpy(core.savedata_ptr(),ram.data(),ram.size());
}
