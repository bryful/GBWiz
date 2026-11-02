#pragma once
#include "game_support.h"
#include "music_player.h"
#include "reward_support.h"
#include "creation_support.h"
#include "spell_support.h"
#include "mp_support.h"
#include "party_support.h"

class TurboRunner {
    gambatte::GB* main_=nullptr;
    std::vector<char> rom_;
    std::unique_ptr<MusicPlayer> music_;
    int halfSpeed_=2,remainder_=0;
    unsigned previous_=0,tick_=0;
    unsigned effectCommands_=0,musicCommands_=0;
    GameId gameId_=GameId::Unknown;
    SoundDriver driver_;
    RewardSupport rewards_;
    CreationSupport creation_;
    SpellSupport spells_;
    MpSupport mp_;
    PartySupport party_;
    unsigned dqSong_=255;
    unsigned walkingDirection_=0,scrollHold_=0;
    std::array<unsigned,8> repeatAt_{};
    std::array<unsigned,8> pressFrames_{};
    static std::vector<char> coreState(gambatte::GB& core){std::vector<char> data(core.stateSize());core.saveState(data.data());return data;}
    void clearRepeat(){previous_=tick_=0;walkingDirection_=scrollHold_=0;repeatAt_.fill(0);pressFrames_.fill(0);}
    static unsigned read32(const std::vector<char>& data,size_t offset){
        unsigned value=0;for(unsigned i=0;i<4;i++)value|=static_cast<unsigned>(static_cast<unsigned char>(data[offset+i]))<<(i*8);return value;
    }
    static void write32(std::vector<char>& data,size_t offset,unsigned value){for(unsigned i=0;i<4;i++)data[offset+i]=static_cast<char>(value>>(i*8));}
    static bool validCoreState(const char* data,size_t size){
        // Gambatte accepts unknown/missing labels, so validate the complete
        // labeled structure before handing a saved game/audio core to it.
        if(size<5||data[0]!=0||data[1]!=1)return false;
        auto u8=[&](size_t i){return static_cast<unsigned>(static_cast<unsigned char>(data[i]));};
        size_t snapshot=(u8(2)<<16)|(u8(3)<<8)|u8(4);if(snapshot>size-5)return false;
        size_t pos=5+snapshot;unsigned fields=0,required=0;std::string previous;
        while(pos<size){
            size_t start=pos;while(pos<size&&data[pos]!=0&&pos-start<=32)++pos;
            if(pos==size||pos==start||pos-start>32)return false;
            std::string label(data+start,pos-start);
            if(!previous.empty()&&label<=previous)return false;previous=label;++pos;
            if(size-pos<3)return false;
            size_t length=(u8(pos)<<16)|(u8(pos+1)<<8)|u8(pos+2);pos+=3;
            if(length>size-pos)return false;
            if(label=="pc"&&length==2)required|=1;
            if(label=="sp"&&length==2)required|=2;
            if(label=="vram"&&length>0)required|=4;
            if(label=="wram"&&length>0)required|=8;
            if(label=="hram"&&length>0)required|=16;
            pos+=length;++fields;
        }
        return required==31&&fields>=100;
    }
    static bool command(void* user,unsigned pc,unsigned sp,unsigned address,unsigned value){
        auto& self=*static_cast<TurboRunner*>(user);
        self.spells_.observe(*self.main_,pc,address,value);
        if(!self.party_.write(*self.main_,pc,sp,address,value))return false;
        if(!self.spells_.write(*self.main_,pc,address,value))return false;
        if(!self.rewards_.write(*self.main_,pc,sp,address,value))return false;
        if(!self.creation_.write(*self.main_,pc,sp,address,value))return false;
        if(!self.mp_.write(*self.main_,pc,address,value))return false;
        if(self.driver_.dq&&address==0xccd1&&pc==0x5e1a)self.dqSong_=value;
        if(self.music_){
            if(self.driver_.dq){
                if(address==0xccd1&&pc==0x5e1a){self.music_->command(true,value);++self.musicCommands_;}
                if(address==0xccd1&&pc==0x5e60){self.music_->effect(value);++self.effectCommands_;}
                return true;
            }
            if(self.driver_.color){
                if(address!=0xd000&&address!=0xd001&&address!=0xd004&&address!=0xd007)return true;
                if(pc>=0x4000||(static_cast<unsigned char*>(self.main_->zeropage_ptr())[-0x10]&7)!=2)return true;
                auto ram=static_cast<unsigned char*>(self.main_->rambank0_ptr())+0x2000;
                for(unsigned slot=0;slot<3;slot++){
                    unsigned operation=address==0xd001+slot*3?value:ram[1+slot*3];
                    bool flagged=address==0xd000?(value&(1u<<slot))!=0:(ram[0]&(1u<<slot))!=0;
                    if(flagged&&(address==0xd000||address==0xd001+slot*3)&&operation>=1&&operation<=6){
                        self.music_->colorCommand(slot,operation,ram[2+slot*3]);
                        if(slot==2)++self.musicCommands_;else ++self.effectCommands_;
                    }
                }
                return true;
            }
            if(address==0xfff0&&pc==self.driver_.music+0x19){self.music_->command(true,value);++self.musicCommands_;}
            if(address==0xffee&&pc==self.driver_.legacyEffect+2){self.music_->command(false,value);++self.effectCommands_;}
            if(address==self.driver_.effectState&&pc==self.driver_.effect+0x28&&value!=0xff){self.music_->effect(value);++self.effectCommands_;}
        }
        return true;
    }
public:
    ~TurboRunner(){detach();}
    void detach(){party_.detach();if(main_)main_->setWriteFilter(nullptr,nullptr);main_=nullptr;music_.reset();}
    void attach(gambatte::GB& main,const std::vector<char>& rom,GameProfile profile){
        detach();main_=&main;gameId_=profile.id;rom_.clear();halfSpeed_=2;remainder_=0;effectCommands_=musicCommands_=0;clearRepeat();
        driver_=MusicPlayer::driver(rom);dqSong_=255;
        rewards_.attach(rom);
        creation_.attach(rom);
        spells_.attach(rom);
        mp_.attach(rom);
        party_.attach(main,rom);
        spells_.resync(main);
        if(MusicPlayer::supported(rom))rom_=rom;
        if(!rom_.empty()||rewards_.supported()||creation_.supported()||spells_.supported()||mp_.supported()||party_.supported())main_->setWriteFilter(command,this);
    }
    bool supported()const{return !rom_.empty();}
    bool speed(int halfSpeed){
        if(halfSpeed!=2&&halfSpeed!=3&&halfSpeed!=4&&halfSpeed!=6&&halfSpeed!=8)return false;
        if(halfSpeed>2&&!supported())return false;
        if(halfSpeed>2&&!music_)restartAudio(true);
        halfSpeed_=halfSpeed;remainder_=0;clearRepeat();return true;
    }
    int speed()const{return halfSpeed_;}
    unsigned effectCommands()const{return effectCommands_;}
    unsigned musicCommands()const{return musicCommands_;}
    unsigned dqSong()const{return dqSong_;}
    RewardSupport& rewards(){return rewards_;}
    CreationSupport& creation(){return creation_;}
    SpellSupport& spells(){return spells_;}
    MpSupport& mp(){return mp_;}
    PartySupport& party(){return party_;}
    void resetInput(){clearRepeat();}
    void restartAudio(bool force=false){
        bool needed=force||music_!=nullptr;
        if(needed&&supported()){
            auto colorRam=static_cast<unsigned char*>(main_->rambank0_ptr())+0x2000;
            unsigned song=driver_.dq?dqSong_:driver_.color?(colorRam[9]?colorRam[8]:255):static_cast<unsigned char*>(main_->zeropage_ptr())[0x70];
            auto next=std::make_unique<MusicPlayer>(rom_,song);music_=std::move(next);
        }else music_.reset();
    }
    std::vector<char> saveState(){
        auto game=coreState(*main_);
        if(!music_)return game; // Preserve legacy format for ordinary 1x sessions.
        auto music=coreState(music_->core());
        std::vector<char> data(32+game.size()+music.size(),0);
        std::memcpy(data.data(),"GBWZST02",8);
        write32(data,8,32);write32(data,12,static_cast<unsigned>(game.size()));write32(data,16,static_cast<unsigned>(music.size()));
        write32(data,20,halfSpeed_);write32(data,24,remainder_);write32(data,28,static_cast<unsigned>(gameId_));
        std::copy(game.begin(),game.end(),data.begin()+32);std::copy(music.begin(),music.end(),data.begin()+32+game.size());
        return data;
    }
    bool loadState(const std::vector<char>& data){
        if(data.empty())return false;
        bool combined=data.size()>=8&&std::memcmp(data.data(),"GBWZST02",8)==0;
        const char* gameData=data.data();size_t gameSize=data.size();
        int restoredSpeed=halfSpeed_,restoredRemainder=0;
        std::unique_ptr<MusicPlayer> nextMusic;
        if(combined){
            if(data.size()<32||read32(data,8)!=32||!supported()||read32(data,28)!=static_cast<unsigned>(gameId_))return false;
            size_t gameLength=read32(data,12),musicLength=read32(data,16);
            unsigned speed=read32(data,20),remainder=read32(data,24);
            if((speed!=2&&speed!=3&&speed!=4&&speed!=6&&speed!=8)||remainder>1||(speed!=3&&remainder!=0))return false;
            if(gameLength==0||gameLength>data.size()-32||musicLength==0||musicLength!=data.size()-32-gameLength)return false;
            if(!validCoreState(data.data()+32,gameLength)||!validCoreState(data.data()+32+gameLength,musicLength))return false;
            nextMusic=std::make_unique<MusicPlayer>(rom_,255);
            if(!nextMusic->core().loadState(data.data()+32+gameLength,musicLength))return false;
            gameData=data.data()+32;gameSize=gameLength;restoredSpeed=static_cast<int>(speed);restoredRemainder=static_cast<int>(remainder);
        }
        if(!validCoreState(gameData,gameSize))return false;
        auto backup=coreState(*main_);
        if(!main_->loadState(gameData,gameSize)){main_->loadState(backup.data(),backup.size());return false;}
        try{
            if(combined)music_=std::move(nextMusic);
            else restartAudio(); // Legacy files do not contain an audio-core position.
        }catch(...){main_->loadState(backup.data(),backup.size());throw;}
        halfSpeed_=restoredSpeed;remainder_=restoredRemainder;clearRepeat();spells_.resync(*main_);return true;
    }
    unsigned run(unsigned held,unsigned& input,std::array<gambatte::video_pixel_t,160*144>& pixels,
            std::array<gambatte::uint_least32_t,40000>& sound){
        ++tick_;unsigned due=held&~previous_;
        for(unsigned i=0;i<8;i++){
            unsigned bit=1u<<i;
            if(!(held&bit)||!(previous_&bit))repeatAt_[i]=tick_+12;
            else if(tick_>=repeatAt_[i]){due|=bit;repeatAt_[i]=tick_+6;}
        }
        previous_=held;remainder_+=halfSpeed_;int frames=remainder_/2;remainder_%=2;
        unsigned direction=held&240;
        if(driver_.dq&&scrollHold_){
            unsigned released=walkingDirection_&~direction;
            for(unsigned b=4;b<8;b++)if(released&(1u<<b))pressFrames_[b]=0;
        }
        if(direction!=walkingDirection_||(held&15))scrollHold_=0;
        walkingDirection_=direction;
        // A single emulated frame can miss the game's input scan during screen
        // transitions. Keep each press visible for four game frames, while
        // scheduling repeats on the unchanged real-time clock above.
        for(unsigned b=0;b<8;b++)if(due&(1u<<b))pressFrames_[b]=4;
        unsigned samples=0;
        for(int i=0;i<frames;i++){
            input=0;
            for(unsigned b=0;b<8;b++)if(pressFrames_[b]){input|=1u<<b;--pressFrames_[b];}
            // DQ3 walks smoothly only while the direction remains held.
            // Continue a confirmed scrolling movement; expire after four
            // stationary game frames so menus retain the normal repeat rate.
            if(driver_.dq&&scrollHold_){input|=direction;--scrollHold_;}
            if(halfSpeed_==2)input=held;
            auto io=static_cast<unsigned char*>(main_->zeropage_ptr());
            unsigned scrollX=io[-0x3d],scrollY=io[-0x3e];
            samples=35112;
            main_->runFor(pixels.data(),160,sound.data(),sound.size(),samples);
            if(driver_.dq&&direction&&!(held&15)&&(input&direction)&&
                (scrollX!=io[-0x3d]||scrollY!=io[-0x3e]))scrollHold_=4;
        }
        if(music_)samples=music_->run(sound);
        return samples;
    }
};
