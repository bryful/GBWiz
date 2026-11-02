#define NOMINMAX
#include <windows.h>
#include <commdlg.h>
#include <shlobj.h>
#include <gambatte.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>
#include <array>
#include <chrono>
#include <thread>
#include <cstring>
#include <algorithm>
#include "desktop.h"
#include "game.h"
#include "game_support.h"
#include "turbo_runner.h"
#include "dungeon_map.h"
#include <optional>
namespace fs = std::filesystem;
std::array<bool,256> keyPulses{};
// The core's optional cartridge rumble hook. UI rumble is not enabled yet.
void cartridge_set_rumble(unsigned) {}
struct Input : gambatte::InputGetter {
    unsigned value=0;
    unsigned operator()() override { return value; }
};
Buttons keyboard() {
    Buttons result;
    auto held=[](int key) { return (GetAsyncKeyState(key)&0x8000)!=0||keyPulses[key]; };
    const int keys[]={'X','Z',VK_RSHIFT,VK_RETURN,VK_RIGHT,VK_LEFT,VK_UP,VK_DOWN};
    for(int i=0;i<8;++i) if(held(keys[i])) result.game|=1u<<i;
    result.actions={held(VK_F1)||held(VK_ESCAPE),held(VK_F5),held(VK_F8),
        held(VK_UP),held(VK_DOWN),held(VK_LEFT),held(VK_RIGHT),held(VK_RETURN)||held('X'),held('Z'),held('M'),held(VK_PRIOR),held(VK_NEXT)};
    keyPulses.fill(false);
    return result;
}
std::wstring wide(const std::string& text){int length=MultiByteToWideChar(CP_UTF8,0,text.data(),static_cast<int>(text.size()),nullptr,0);std::wstring result(length,0);if(length)MultiByteToWideChar(CP_UTF8,0,text.data(),static_cast<int>(text.size()),result.data(),length);return result;}
void render(HWND window, const std::array<gambatte::video_pixel_t,160*144>& pixels,
            const BITMAPINFO& bitmap, bool menu, int row, int slot, bool swap,
            const std::wstring& status, const fs::path& saves, const fs::path& rom,bool skipLogo,int speedHalf,const RewardSupport& rewards,const CreationSupport& creation,const MpSupport& mp,const PartySupport& party,bool selectingSpell,const std::vector<SpellCharacter>& spellCharacters,bool characterMenu,const SpellCharacter& selectedCharacter,int alignment) {
    RECT r; GetClientRect(window,&r);
    const LONG contentBottom=std::max(1L,r.bottom-44);
    int scale=std::max(1,static_cast<int>(std::min((r.right-r.left)/160,contentBottom/144)));
    HDC dc=GetDC(window);
    HDC buffer=CreateCompatibleDC(dc); HBITMAP image=CreateCompatibleBitmap(dc,std::max(1L,r.right),std::max(1L,r.bottom));
    HGDIOBJ old=SelectObject(buffer,image);
    PatBlt(buffer,0,0,r.right,r.bottom,BLACKNESS);
    StretchDIBits(buffer,(r.right-160*scale)/2,(contentBottom-144*scale)/2,160*scale,144*scale,0,0,160,144,pixels.data(),&bitmap,DIB_RGB_COLORS,SRCCOPY);
    SetBkMode(buffer,TRANSPARENT);
    if(menu) {
        RECT box{12,12,std::max(340L,r.right-12),std::min(480L,contentBottom-8)};
        HBRUSH brush=CreateSolidBrush(RGB(20,24,32)); FillRect(buffer,&box,brush); DeleteObject(brush);
        HFONT font=CreateFontW(-20,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,0,0,0,0,L"Yu Gothic UI");
        auto oldFont=SelectObject(buffer,font);
        std::vector<std::wstring> lines={L"再開",L"スロット : "+std::to_wstring(slot)+L" / 5  （左右で選択）",
            L"ステート保存",L"ステート読込",std::wstring(L"A / B 入れ替え : ")+(swap?L"ON":L"OFF"),L"ROM選択",L"リセット",std::wstring(L"ロゴスキップ : ")+(skipLogo?L"ON":L"OFF"),
            L"速度 : "+(speedHalf==3?std::wstring(L"1.5"):std::to_wstring(speedHalf/2))+L"倍 （左右で変更・音声1倍）",
            L"獲得経験値 : "+(rewards.experienceSupported()?L"×"+std::to_wstring(rewards.experience()):std::wstring(L"対象外（外伝1〜3・GBC版I〜III）")),
            L"獲得お金 : "+(rewards.goldSupported()?L"×"+std::to_wstring(rewards.gold()):std::wstring(L"対象外（外伝1〜3・GBC版I〜III）")),
            L"作成ボーナス最大 : "+(creation.supported()?std::wstring(creation.enabled()?L"ON":L"OFF"):std::wstring(L"対象外（外伝1〜3・GBC版）")),L"キャラクターサポート",L"MP消費なし : "+(mp.supported()?std::wstring(mp.enabled()?L"ON":L"OFF"):std::wstring(L"対象外（外伝1）")),L"善悪混合 : "+(party.supported()?std::wstring(party.enabled()?L"ON":L"OFF"):std::wstring(L"対象外（外伝1）")),L"迷宮マップ・移動（M / RT）"};
        if(selectingSpell){lines={L"戻る"};for(const auto& character:spellCharacters)lines.push_back(std::to_wstring(character.slot+1)+L": "+wide(character.name));}
        else if(characterMenu)lines={L"対象を選び直す",L"全呪文習得",L"属性 : "+(alignment>=0?std::wstring(L"‹ ")+std::array<const wchar_t*,3>{L"善",L"中立",L"悪"}[alignment]+L" ›":std::wstring(L"対象外"))};
        SetTextColor(buffer,RGB(235,235,245));
        const std::wstring heading=selectingSpell?L"対象キャラクター選択":characterMenu?std::to_wstring(selectedCharacter.slot+1)+L": "+wide(selectedCharacter.name):L"GBWiz 機能メニュー";
        TextOutW(buffer,28,22,heading.c_str(),static_cast<int>(heading.size()));
        int first=(row/10)*10;
        for(int i=first;i<static_cast<int>(lines.size())&&i<first+10;++i) {
            bool selected=row==i;
            std::wstring line=(selected?L">  ":L"    ")+lines[i];
            SetTextColor(buffer,selected?RGB(255,220,100):RGB(235,235,245));
            TextOutW(buffer,28,58+(i-first)*28,line.c_str(),static_cast<int>(line.size()));
        }
        auto path=saves/rom.stem(); path+=L".slot"+std::to_wstring(slot)+L".state";
        std::wstring info=fs::exists(path)?L"選択スロット : 保存済み":L"選択スロット : 空";
        SetTextColor(buffer,RGB(180,190,200)); TextOutW(buffer,28,box.bottom-90,info.c_str(),static_cast<int>(info.size()));
        std::wstring help=characterMenu?L"上下 : 選択   左右 : 属性変更   A : 実行   B : 戻る":L"上下 : 選択   Enter / A : 決定   F1 / B : 戻る";
        TextOutW(buffer,28,box.bottom-42,help.c_str(),static_cast<int>(help.size()));
        SelectObject(buffer,oldFont); DeleteObject(font);
    }
    if(!status.empty()) {
        RECT bar{8,contentBottom+4,r.right-8,r.bottom-4};
        FillRect(buffer,&bar,static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
        SetTextColor(buffer,RGB(255,230,150));
        HFONT font=CreateFontW(-15,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,0,0,0,0,L"Yu Gothic UI");
        auto oldFont=SelectObject(buffer,font);
        DrawTextW(buffer,status.c_str(),static_cast<int>(status.size()),&bar,DT_WORDBREAK|DT_END_ELLIPSIS|DT_NOPREFIX);
        SelectObject(buffer,oldFont);DeleteObject(font);
    }
    BitBlt(dc,0,0,r.right,r.bottom,buffer,0,0,SRCCOPY);
    SelectObject(buffer,old); DeleteObject(image); DeleteDC(buffer); ReleaseDC(window,dc);
}
std::optional<fs::path> selectRom(HWND owner,const fs::path& initialFolder) {
    wchar_t filename[32768]{};
    auto initial=initialFolder.wstring();
    OPENFILENAMEW dialog{sizeof(dialog)}; dialog.hwndOwner=owner;
    dialog.lpstrTitle=L"GB / GBC ROMを選択";
    dialog.lpstrFilter=L"Game Boy / Game Boy Color (*.gb;*.gbc)\0*.gb;*.gbc\0\0";
    dialog.lpstrFile=filename; dialog.nMaxFile=32768; dialog.lpstrInitialDir=initial.c_str();
    dialog.Flags=OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR;
    if(GetOpenFileNameW(&dialog)) return fs::path(filename);
    if(CommDlgExtendedError()!=0) throw std::runtime_error("ROM selection dialog failed");
    return std::nullopt;
}
void write(const fs::path& path, const void* data, size_t size) {
    auto temp = path; temp += L".tmp";
    std::ofstream f(temp, std::ios::binary|std::ios::trunc);
    f.write(static_cast<const char*>(data), size); f.close();
    if(!f) throw std::runtime_error("Save write failed");
    if(!MoveFileExW(temp.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))
        throw std::runtime_error("Save replace failed");
}
LRESULT CALLBACK windowProc(HWND w, UINT m, WPARAM a, LPARAM b) {
    if(m==WM_KEYDOWN && a<256 && !(b&(1LL<<30))) keyPulses[a]=true;
    if(m==WM_GETMINMAXINFO) {
        auto info=reinterpret_cast<MINMAXINFO*>(b); info->ptMinTrackSize={520,530}; return 0;
    }
    if(m==WM_DESTROY) { PostQuitMessage(0); return 0; }
    return DefWindowProcW(w,m,a,b);
}
int wmain(int argc, wchar_t** argv) try {
    fs::path rom;
    if(argc>1) rom=argv[1];
    else {
        wchar_t executable[32768]{};
        GetModuleFileNameW(nullptr, executable, 32768);
        auto chosen=selectRom(nullptr,fs::path(executable).parent_path()/L"Rom");
        if(!chosen) return 0;
        rom=*chosen;
    }
    rom=fs::absolute(rom);
    Input input;auto romData=read(rom);auto profile=identifyGame(romData);
    auto gb=loadCoreBytes(romData,&input);
    std::cout << "ROM: " << rom.filename().u8string() << " | "
              << (gb->isCgb() ? "GBC" : "GB") << "\n";
    std::array<gambatte::video_pixel_t,160*144> pixels{};
    std::array<gambatte::uint_least32_t,40000> sound{};
    // Headless smoke check does not touch user save files.
    if(argc>2 && std::wstring(argv[2])==L"--smoke") {
        for(int i=0;i<180;++i) { unsigned samples=35112; gb->runFor(pixels.data(),160,sound.data(),sound.size(),samples); }
        std::vector<char> state(gb->stateSize()); gb->saveState(state.data());
        if(!gb->loadState(state.data(),state.size())) throw std::runtime_error("State round trip failed");
        bool varied=false; for(auto p:pixels) if(p!=pixels[0]) varied=true;
        if(!varied) throw std::runtime_error("No varied frame produced");
        std::cout << "PASS: 180 frames, video output, state round trip; mode=" << (gb->isCgb()?"GBC":"GB") << "\n";
        return 0;
    }
    fs::path saves;
    for(int i=2;i<argc;++i) if(std::wstring(argv[i])==L"--data-dir" && i+1<argc) saves=fs::absolute(argv[++i]);
    if(saves.empty()) {
        wchar_t roaming[MAX_PATH];
        if(FAILED(SHGetFolderPathW(nullptr,CSIDL_APPDATA,nullptr,0,roaming))) throw std::runtime_error("AppData unavailable");
        saves=fs::path(roaming)/L"GBWiz";
    }
    fs::create_directories(saves);
    auto settings=saves/L"settings.ini";
    bool skipLogo=GetPrivateProfileIntW(L"support",L"skip_logo",1,settings.c_str())!=0;
    fs::path save=saves/rom.stem(); save += L".sav";
    restoreBattery(*gb,save);
    auto skipped=skipLogo?skipBootLogos(gb,profile,romData,&input,pixels):BootSkipResult{};
    TurboRunner turbo;turbo.attach(*gb,romData,profile);
    DungeonMap dungeonMap;dungeonMap.attach(romData);
    unsigned experienceMultiplier=GetPrivateProfileIntW(L"support",L"experience_multiplier",1,settings.c_str());
    unsigned goldMultiplier=GetPrivateProfileIntW(L"support",L"gold_multiplier",1,settings.c_str());
    if(!RewardSupport::valid(experienceMultiplier))experienceMultiplier=1;
    if(!RewardSupport::valid(goldMultiplier))goldMultiplier=1;
    turbo.rewards().set(turbo.rewards().experienceSupported()?experienceMultiplier:1,turbo.rewards().goldSupported()?goldMultiplier:1);
    bool maxBonus=GetPrivateProfileIntW(L"support",L"max_bonus",0,settings.c_str())!=0;
    turbo.creation().enable(maxBonus);
    bool noMpCost=GetPrivateProfileIntW(L"support",L"no_mp_cost",0,settings.c_str())!=0;
    turbo.mp().enable(noMpCost);
    bool mixedParty=GetPrivateProfileIntW(L"support",L"mixed_party",0,settings.c_str())!=0;
    turbo.party().enable(mixedParty);
    int speedHalf=GetPrivateProfileIntW(L"play",L"speed_half",2,settings.c_str());
    if(!turbo.speed(speedHalf))speedHalf=2;
    std::cout<<"Profile: "<<profile.name<<" | boot frames skipped: "<<skipped.frames<<"\n";
    WNDCLASSW wc{}; wc.lpfnWndProc=windowProc; wc.hInstance=GetModuleHandleW(nullptr); wc.lpszClassName=L"GBWiz";
    wc.hIcon=LoadIconW(wc.hInstance,MAKEINTRESOURCEW(101));
    RegisterClassW(&wc);
    HWND window=CreateWindowW(L"GBWiz",L"GBWiz | F1:Menu | X:A Z:B | F5:Save1 F8:Load1",WS_OVERLAPPEDWINDOW|WS_VISIBLE,CW_USEDEFAULT,CW_USEDEFAULT,656,615,nullptr,nullptr,wc.hInstance,nullptr);
    if(!window) throw std::runtime_error("Window creation failed");
    BITMAPINFO bitmap{}; bitmap.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
    bitmap.bmiHeader.biWidth=160; bitmap.bmiHeader.biHeight=-144;
    bitmap.bmiHeader.biPlanes=1; bitmap.bmiHeader.biBitCount=32; bitmap.bmiHeader.biCompression=BI_RGB;
    SdlRuntime runtime; Controllers controllers; Audio audio;
    bool swap=GetPrivateProfileIntW(L"controls",L"swap_ab",0,settings.c_str())!=0;
    int slot=std::clamp(static_cast<int>(GetPrivateProfileIntW(L"states",L"slot",1,settings.c_str())),1,5);
    bool running=true, menu=false, active=true, releaseGame=false;
    bool selectingSpell=false;int spellRow=0;std::vector<SpellCharacter> spellCharacters;
    bool characterMenu=false;int characterRow=1;SpellCharacter selectedCharacter{};
    int row=0; Edges edges;
    std::wstring status=audio.available()?L"":L"音声デバイスが使えません。無音で続行します。";
    auto statusUntil=std::chrono::steady_clock::now()+std::chrono::seconds(5);
    auto deadline=std::chrono::steady_clock::now(); auto lastFlush=deadline;
    auto flush=[&] { if(gb->savedata_size()) write(save,gb->savedata_ptr(),gb->savedata_size()); };
    auto notice=[&](std::wstring text) { status=std::move(text); statusUntil=std::chrono::steady_clock::now()+std::chrono::seconds(4); };
    auto persist=[&] {
        if(!WritePrivateProfileStringW(L"controls",L"swap_ab",swap?L"1":L"0",settings.c_str()) ||
           !WritePrivateProfileStringW(L"states",L"slot",std::to_wstring(slot).c_str(),settings.c_str()))
            notice(L"設定の保存に失敗しました");
        if(!WritePrivateProfileStringW(L"support",L"skip_logo",skipLogo?L"1":L"0",settings.c_str()))notice(L"設定の保存に失敗しました");
        if(!WritePrivateProfileStringW(L"play",L"speed_half",std::to_wstring(speedHalf).c_str(),settings.c_str()))notice(L"設定の保存に失敗しました");
        if(!WritePrivateProfileStringW(L"support",L"experience_multiplier",std::to_wstring(experienceMultiplier).c_str(),settings.c_str())||
           !WritePrivateProfileStringW(L"support",L"gold_multiplier",std::to_wstring(goldMultiplier).c_str(),settings.c_str()))notice(L"設定の保存に失敗しました");
        if(!WritePrivateProfileStringW(L"support",L"max_bonus",maxBonus?L"1":L"0",settings.c_str()))notice(L"設定の保存に失敗しました");
        if(!WritePrivateProfileStringW(L"support",L"no_mp_cost",noMpCost?L"1":L"0",settings.c_str()))notice(L"設定の保存に失敗しました");
        if(!WritePrivateProfileStringW(L"support",L"mixed_party",mixedParty?L"1":L"0",settings.c_str()))notice(L"設定の保存に失敗しました");
    };
    auto stateAction=[&](int chosen,bool load) {
        auto path=saves/rom.stem(); path+=L".slot"+std::to_wstring(chosen)+L".state";
        try {
            if(load) {
                if(!fs::exists(path)) { notice(L"このスロットには保存がありません"); return; }
                auto state=read(path);
                if(!turbo.loadState(state)) {
                    notice(L"ステートを読み込めません。現在の状態を保持しました"); return;
                }
                speedHalf=turbo.speed();persist();audio.clear();
            } else {
                auto state=turbo.saveState();
                write(path,state.data(),state.size()); flush();
            }

        } catch(const std::exception& e) { std::cerr<<e.what()<<"\n"; notice(L"保存・読込に失敗しました。保存先を確認してください"); }
    };
    auto updateTitle=[&] {
        auto title=L"GBWiz | "+rom.filename().wstring()+L" | F1:Menu | F5:Save1 F8:Load1";
        SetWindowTextW(window,title.c_str());
    };
    updateTitle();
    auto chooseGame=[&] {
        try {
            auto chosen=selectRom(window,rom.parent_path());
            if(chosen) {
                // Validate a separate core first. A failed load never replaces the current game.
                auto nextRomData=read(*chosen);auto next=loadCoreBytes(nextRomData,&input);
                auto nextProfile=identifyGame(nextRomData);
                auto nextSave=saves/chosen->stem(); nextSave+=L".sav";
                flush(); // also needed when reopening the same ROM
                restoreBattery(*next,nextSave);
                std::array<gambatte::video_pixel_t,160*144> nextPixels{};
                input.value=0;if(skipLogo)skipBootLogos(next,nextProfile,nextRomData,&input,nextPixels);
                turbo.detach();gb=std::move(next);rom=std::move(*chosen);save=std::move(nextSave);
                profile=nextProfile;romData=std::move(nextRomData);pixels=nextPixels;audio.clear();menu=false;row=0;
                dungeonMap.attach(romData);turbo.attach(*gb,romData,profile);if(!turbo.speed(speedHalf))speedHalf=2;
                turbo.rewards().set(turbo.rewards().experienceSupported()?experienceMultiplier:1,turbo.rewards().goldSupported()?goldMultiplier:1);
                turbo.creation().enable(maxBonus);turbo.mp().enable(noMpCost);turbo.party().enable(mixedParty);
                lastFlush=std::chrono::steady_clock::now(); updateTitle();

            }
        } catch(const std::exception& e) {
            std::cerr<<e.what()<<"\n";
            notice(L"ROMを切り替えられません。元のゲームを保持しました");
        }
        keyPulses.fill(false); edges.previous.fill(true); releaseGame=true;
        deadline=std::chrono::steady_clock::now();
    };
    while(running) {
        MSG msg; while(PeekMessageW(&msg,nullptr,0,0,PM_REMOVE)) { if(msg.message==WM_QUIT) running=false; TranslateMessage(&msg); DispatchMessageW(&msg); }
        if(!running) break;
        Buttons buttons=keyboard(); controllers.poll(buttons);
        bool focused=GetForegroundWindow()==window && !IsIconic(window);
        auto pressed=edges.update(buttons);
        if(!focused) {
            if(active) { audio.clear(); input.value=0; releaseGame=true; try { flush(); } catch(const std::exception& e) { std::cerr<<e.what()<<"\n"; notice(L"通常セーブの保存に失敗しました"); } }
            active=false;
            std::this_thread::sleep_for(std::chrono::milliseconds(20)); deadline=std::chrono::steady_clock::now(); continue;
        }
        if(!active) { active=true; releaseGame=true; pressed.fill(false); }
        if(dungeonMap.open){
            if(pressed[Menu]||pressed[Map])dungeonMap.open=false;
            else {unsigned bits=(pressed[Up]?64:0)|(pressed[Down]?128:0)|(pressed[Left]?32:0)|(pressed[Right]?16:0)|(pressed[Confirm]?1:0)|(pressed[Cancel]?2:0)|(pressed[FloorUp]?256:0)|(pressed[FloorDown]?512:0);
                if(dungeonMap.key(bits))try{
                    bool moved=dungeonMap.move(*gb,[&]{auto state=turbo.saveState();auto path=saves/rom.stem();path+=L".slot1.state";write(path,state.data(),state.size());return true;});
                    if(moved){turbo.resetInput();}else notice(L"現在の状態では移動できません");
                }catch(const std::exception& e){notice(L"スロット1の保存に失敗したため移動を中止しました");}
            }releaseGame=true;audio.clear();
        }else if(pressed[Map]){if(dungeonMap.show(*gb)){menu=false;releaseGame=true;audio.clear();}else notice(L"マップは対応するウィザードリィの迷宮・キャンプ中に使用できます（戦闘・画面更新中は不可）");
        }else if(pressed[Menu] || (menu&&pressed[Cancel]&&!selectingSpell&&!characterMenu)) {
            selectingSpell=false;
            characterMenu=false;
            menu=!menu; releaseGame=true; audio.clear();
        } else if(menu) {
          if(selectingSpell){
            int count=static_cast<int>(spellCharacters.size())+1;
            if(pressed[Up])spellRow=(spellRow+count-1)%count;
            if(pressed[Down])spellRow=(spellRow+1)%count;
            if(pressed[Cancel]||(pressed[Confirm]&&spellRow==0)){selectingSpell=false;row=12;}
            else if(pressed[Confirm]){selectedCharacter=spellCharacters[spellRow-1];selectingSpell=false;characterMenu=true;characterRow=1;}
          }else if(characterMenu){
            if(pressed[Up])characterRow=(characterRow+2)%3;
            if(pressed[Down])characterRow=(characterRow+1)%3;
            if(pressed[Cancel]||(pressed[Confirm]&&characterRow==0)){characterMenu=false;selectingSpell=true;}
            else if((pressed[Confirm]&&characterRow==1)||(characterRow==2&&(pressed[Left]||pressed[Right]))){
                bool applied=false;
                try{flush();const auto& target=selectedCharacter;
                    int current=turbo.spells().alignment(*gb,target.slot);
                    bool success=characterRow==2?(current>=0&&turbo.spells().setAlignment(*gb,target.slot,(current+(pressed[Right]?1:2))%3)):turbo.spells().learn(*gb,target.slot);
                    if(success){applied=true;flush();}
                    else notice(L"戦闘中・データ更新中は実行できません。操作を終えてから選び直してください");
                }catch(const std::exception& e){std::cerr<<e.what()<<"\n";notice(applied?L"変更は適用済みですが通常セーブを保存できません":L"通常セーブの保存に失敗したため適用を中止しました");}
            }
          }else{
            if(pressed[Up]) row=(row+15)%16;
            if(pressed[Down]) row=(row+1)%16;
            if(row==1&&(pressed[Left]||pressed[Right])) { slot=(slot-1+(pressed[Right]?1:4))%5+1; persist(); }
            if(row==4&&(pressed[Left]||pressed[Right]||pressed[Confirm])) { swap=!swap; persist(); }
            if(row==7&&(pressed[Left]||pressed[Right]||pressed[Confirm])) {skipLogo=!skipLogo;persist();}
            if(row==8&&(pressed[Left]||pressed[Right])){
                const int speeds[]={2,3,4,6,8};auto current=std::find(std::begin(speeds),std::end(speeds),speedHalf)-std::begin(speeds);
                int next=speeds[(current+(pressed[Right]?1:4))%5];
                if(turbo.speed(next)){speedHalf=next;audio.clear();persist();}else notice(L"倍速＋等速音声はGB版外伝1のみ対応しています");
            }
            if((row==9||row==10)&&(pressed[Left]||pressed[Right])){
                if(!(row==9?turbo.rewards().experienceSupported():turbo.rewards().goldSupported()))notice(row==9?L"獲得経験値の倍率は対応版の外伝1〜3・GBC版I〜III・DQ3で使用できます":L"獲得お金の倍率は対応版の外伝1〜3・GBC版I〜III・DQ3で使用できます");
                else{
                    const unsigned values[]={1,8,16,32,64,128,256};
                    auto& current=row==9?experienceMultiplier:goldMultiplier;
                    auto index=std::find(std::begin(values),std::end(values),current)-std::begin(values);
                    current=values[(index+(pressed[Right]?1:6))%7];
                    turbo.rewards().set(turbo.rewards().experienceSupported()?experienceMultiplier:1,turbo.rewards().goldSupported()?goldMultiplier:1);persist();
                }
            }
            if(row==11&&(pressed[Left]||pressed[Right])){
                if(turbo.creation().enable(!maxBonus)){maxBonus=!maxBonus;persist();}
                else notice(L"作成ボーナス最大は対応版の外伝1〜3・GBC版Wizardry1〜3で使用できます");
            }
            if(row==13&&(pressed[Left]||pressed[Right])){
                if(turbo.mp().enable(!noMpCost)){noMpCost=!noMpCost;persist();}
                else notice(L"MP消費なしは対応版のGB外伝1〜3で使用できます");
            }
            if(row==14&&(pressed[Left]||pressed[Right])){
                if(turbo.party().enable(!mixedParty)){mixedParty=!mixedParty;persist();}
                else notice(L"善悪混合は対応版の外伝1〜3・GBC版Wizardry1〜3で使用できます");
            }
            if(pressed[Confirm]) {
                if(row==15){if(dungeonMap.show(*gb)){menu=false;releaseGame=true;audio.clear();}else notice(L"マップは対応するウィザードリィの迷宮・キャンプ中に使用できます");}
                if(row==0) { menu=false; releaseGame=true; audio.clear(); }
                if(row==2) stateAction(slot,false);
                if(row==3) stateAction(slot,true);
                if(row==5) chooseGame();
                if(row==6) {
                    try {
                        flush();turbo.detach();input.value=0;gb->reset();pixels.fill(0);if(skipLogo)skipBootLogos(gb,profile,romData,&input,pixels);
                        dungeonMap.attach(romData);turbo.attach(*gb,romData,profile);if(!turbo.speed(speedHalf))speedHalf=2;audio.clear();
                        turbo.rewards().set(turbo.rewards().experienceSupported()?experienceMultiplier:1,turbo.rewards().goldSupported()?goldMultiplier:1);
                        turbo.creation().enable(maxBonus);turbo.mp().enable(noMpCost);turbo.party().enable(mixedParty);
                        menu=false; row=0; releaseGame=true;

                    } catch(const std::exception& e) {
                        std::cerr<<e.what()<<"\n";
                        notice(L"通常セーブの保存に失敗したためリセットを中止しました");
                    }
                }
                if(row==12){
                    if(!turbo.spells().supported())notice(L"全呪文習得は対応版の外伝1〜3・GBC版I〜IIIで使用できます");
                    else if(!turbo.spells().available(*gb))notice(turbo.spells().learnsMaximumMp()?L"戦闘中またはデータ更新中は使用できません":L"戦闘中またはデータ更新中は使用できません");
                    else{spellCharacters=turbo.spells().characters(*gb);if(spellCharacters.empty())notice(L"登録キャラクターがいません");else{selectingSpell=true;spellRow=0;}}
                }
            }
          }
        } else {
            if(pressed[Save1]) stateAction(1,false);
            if(pressed[Load1]) { stateAction(1,true); releaseGame=true; }
        }
        if(!buttons.game) releaseGame=false;
        input.value=(!menu&&!dungeonMap.open&&!releaseGame)?swapAB(buttons.game,swap):0;
        if(!menu&&!dungeonMap.open) {unsigned held=input.value;unsigned samples=turbo.run(held,input.value,pixels,sound);audio.push(sound.data(),samples);}
        if(std::chrono::steady_clock::now()>statusUntil) status.clear();
        auto display=pixels;if(dungeonMap.open)drawDungeonMap(dungeonMap,*gb,display);
        render(window,display,bitmap,menu,selectingSpell?spellRow:characterMenu?characterRow:row,slot,swap,status,saves,rom,skipLogo,speedHalf,turbo.rewards(),turbo.creation(),turbo.mp(),turbo.party(),selectingSpell,spellCharacters,characterMenu,selectedCharacter,characterMenu?turbo.spells().alignment(*gb,selectedCharacter.slot):-1);
        deadline+=std::chrono::nanoseconds(16742706); std::this_thread::sleep_until(deadline);
        auto now=std::chrono::steady_clock::now(); if(now-lastFlush>std::chrono::seconds(10)) {
            try { flush(); } catch(const std::exception& e) { std::cerr<<e.what()<<"\n"; notice(L"通常セーブの保存に失敗しました"); } lastFlush=now;
        }
        if(now-deadline>std::chrono::milliseconds(100)) deadline=now;
    }
    flush(); return 0;
} catch(const std::exception& e) { std::cerr<<e.what()<<"\n"; return 1; }
