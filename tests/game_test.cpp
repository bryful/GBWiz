#include "game.h"
#include <iostream>
#include <chrono>
void cartridge_set_rumble(unsigned) {}
void require(bool ok,const char* message) { if(!ok) throw std::runtime_error(message); }
std::vector<char> battery(gambatte::GB& core) {
    auto data=static_cast<char*>(core.savedata_ptr());
    return {data,data+core.savedata_size()};
}
int wmain(int argc,wchar_t** argv) try {
    require(argc==4,"Expected GB ROM, GBC ROM, test output directory");
    fs::path output=fs::path(argv[3])/std::to_wstring(std::chrono::steady_clock::now().time_since_epoch().count());
    fs::create_directories(output);
    auto gb=loadCore(argv[1],nullptr); require(!gb->isCgb(),"GB mode mismatch");
    require(gb->savedata_size()>0,"Test ROM needs battery RAM");
    auto data=static_cast<unsigned char*>(gb->savedata_ptr());
    for(unsigned i=0;i<gb->savedata_size();++i) data[i]=static_cast<unsigned char>((i*13+7)&255);
    auto expected=battery(*gb);
    gb->reset(); require(battery(*gb)==expected,"Soft reset lost battery RAM");
    auto save=output/L"wizardrygaiden1.sav";
    { std::ofstream f(save,std::ios::binary); f.write(expected.data(),expected.size()); }
    auto reopened=loadCore(argv[1],nullptr); restoreBattery(*reopened,save);
    require(battery(*reopened)==expected,"Reopening ROM lost saved battery RAM");
    auto gbc=loadCore(argv[2],nullptr); require(gbc->isCgb(),"GBC mode mismatch");
    gbc->reset(); require(gbc->isCgb(),"Reset changed GBC mode");
    for(auto* core:{gb.get(),gbc.get()}){
        std::vector<char> first(core->stateSize());core->saveState(first.data());
        for(unsigned i=0;i<8;i++){
            std::vector<char> repeated(core->stateSize());core->saveState(repeated.data());
            require(first==repeated,"Repeated snapshot contains uninitialized mapper fields");
        }
    }
    // A failed candidate load or save validation must not mutate the running core.
    bool failed=false;
    try { auto invalid=loadCore(output/L"missing.gb",nullptr); } catch(const std::exception&) { failed=true; }
    require(failed && battery(*gb)==expected,"Failed ROM candidate affected current game");
    auto badSave=output/L"invalid.sav";
    { std::ofstream f(badSave,std::ios::binary); f<<"bad"; }
    failed=false;
    try { restoreBattery(*reopened,badSave); } catch(const std::exception&) { failed=true; }
    require(failed && battery(*reopened)==expected,"Invalid save was accepted or damaged RAM");
    std::cout<<"PASS: GB/GBC loading, reset preserves battery, reopen save, invalid ROM/save isolation\n";
    return 0;
} catch(const std::exception& e) { std::cerr<<e.what()<<"\n"; return 1; }
