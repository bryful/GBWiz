#pragma once
#include "game_support.h"
#include "gaiden_spell_support.h"
#include "wizardry_name.h"

struct SpellCharacter {unsigned slot;std::string name;};
class SpellSupport {
    GaidenSpellSupport extra_;
    bool supported_=false;
    bool color_=false;
    struct ColorProfile {
        GameId id;unsigned record,flags,counts,mp,save,sum,battle;
        std::array<unsigned,9> copies;
    };
    const ColorProfile* colorProfile_=nullptr;
    static const ColorProfile* colorProfile(const std::vector<char>& rom){
        static const ColorProfile profiles[]={
            {GameId::Wizardry1,0x2776,0xf141,0xa988,0xf5bd,0x2c03,0x2b30,0x1a0f,
                {0x19464,0x19912,0x1b211,0x1b385,0x1baa7,0x1c8ec,0x1cf1a,0x22603,0x2274b}},
            {GameId::Wizardry2,0x276e,0xf143,0xa979,0xf5bf,0x2bfe,0x2b2b,0x19f3,
                {0x194b6,0x199a3,0x1b4ea,0x1b65e,0x1bd80,0x1c8fb,0x1cf29,0x2267b,0x227c3}},
            {GameId::Wizardry3,0x273c,0xf143,0xa988,0xf5bf,0x2bc9,0x2af6,0x19c1,
                {0x1945b,0x19909,0x1b194,0x1b308,0x1ba2a,0x1c8ec,0x1cf1a,0x22693,0x227db}}
        };
        auto id=identifyGame(rom).id;for(const auto& p:profiles)if(p.id==id)return &p;return nullptr;
    }
    static unsigned copyPc(unsigned offset){return (offset&0x3fff)+0x4002;}
    bool alignmentSupported_=false;
    int workingSlot_=-1;
    bool resyncNeeded_=false;
    std::array<unsigned char,10> workingIdentity_{};
    static constexpr size_t roster=0x1186,saveRoster=0x17,mirror=0xe4a,recordSize=128;
    static unsigned checksum(const unsigned char* record){
        unsigned result=0;for(unsigned i=0;i<127;i++)result+=record[i]|(unsigned(record[i+1])<<8);return result&65535;
    }
    static std::string name(const unsigned char* bytes){
        std::u32string text;
        for(unsigned i=0;i<8&&bytes[i];i++){
            unsigned c=bytes[i];char32_t glyph=U'?';
            if(c>=0x20&&c<=0x7e)glyph=c;
            else if(c>=0x91&&c<=0x9f)glyph=U"あいうえおかきくけこさしすせそ"[c-0x91];
            else if(c>=0xe0&&c<=0xee)glyph=U"たちつてとなにぬねのはひふへほ"[c-0xe0];
            else if(c>=0xef&&c<=0xf6)glyph=U"まみむめもやゆよ"[c-0xef];
            else if(c>=0xf7&&c<=0xfd)glyph=U"らりるれろわん"[c-0xf7];
            else if(c>=0xb1&&c<=0xbf)glyph=U"アイウエオカキクケコサシスセソ"[c-0xb1];
            else if(c>=0xc0&&c<=0xce)glyph=U"タチツテトナニヌネノハヒフヘホ"[c-0xc0];
            else if(c>=0xcf&&c<=0xd6)glyph=U"マミムメモヤユヨ"[c-0xcf];
            else if(c>=0xd7&&c<=0xdd)glyph=U"ラリルレロワン"[c-0xd7];
            else if(c>=0x86&&c<=0x8f)glyph=U"をぁぃぅぇぉゃゅょっ"[c-0x86];
            else if(c>=0xa5&&c<=0xaf)glyph=U"ーヲァィゥェォャュョッ"[c-0xa5];
            else if(c==0xde)glyph=0x3099;else if(c==0xdf)glyph=0x309a;
            text.push_back(glyph);
        }
        std::string result;
        for(char32_t c:text){if(c<128)result.push_back(char(c));else if(c<2048){result.push_back(char(0xc0|(c>>6)));result.push_back(char(0x80|(c&63)));}else{result.push_back(char(0xe0|(c>>12)));result.push_back(char(0x80|((c>>6)&63)));result.push_back(char(0x80|(c&63)));}}
        return result;
    }
    static bool recognizesColor(const std::vector<char>& rom){
        const auto* p=colorProfile(rom);if(!p||rom.size()<0x24000)return false;
        auto match=[&](size_t at,std::initializer_list<unsigned char> bytes){return std::equal(bytes.begin(),bytes.end(),reinterpret_cast<const unsigned char*>(rom.data()+at));};
        for(size_t at:p->copies)
            if(!match(at,{6,0x6e,0x2a,0x12,0x13,5,0x20,0xfa}))return false;
        for(unsigned index:{1u,5u,6u,7u})if(!match(p->copies[index]-3,{0x11,0xa2,0xc9}))return false;
        return match(0x0d49,{0x2a,0x12,0x13,0x0b,0x78,0xb1,0x20,0xf8,0xc9})
            &&match(0x07d6,{0xaf,0x22,0x0b,0x79,0xb0,0x20,0xf9,0xc9})&&match(p->battle,{0x3e,3,0xea,0x11,0xca})&&match(p->record,{0xc5,0x6f,0x26,0x6e,1,0x0a,0xc1,0xcd,0x88,0x0d})
            &&match(p->flags,{0xea,0x0b,0xc0,0xc6,0x34,0x5f,0x16,0,0x19,0x56,0x3e,2,0x21,static_cast<unsigned char>(p->counts-26),static_cast<unsigned char>(((p->counts-26)&0x3fff)/256+0x40)})
            &&match(p->counts,{0,4,7,11,14,18,22,25,30,34,38,43,47,51,54})
            &&match(p->mp,{1,0x26,0,9,0x7e,0xf5,0xe6,0xf0,0xcb,0x37})
            &&match(p->save,{0x21,0xf0,0xc0,0x11,0x11,0xa0,1,0xb2,8,0xcd,0x49,0x0d})
            &&match(p->sum,{0x21,0,0xa0,1,0xfe,0x0b,0xaf,0x57,0x7a,0x86,0x57,0x23,0x0b,0x78,0xb1,0x20,0xf7,0x7a,0xc9})
            &&match(0x8079,{0xcd,0x27,0x0b,0x18,0xbb});
    }
    bool colorAvailable(gambatte::GB& core)const{
        if(!core.isCgb()||core.savedata_size()!=32768)return false;
        auto r=static_cast<const unsigned char*>(core.rambank0_ptr());
        // The roster is shared by town, facilities and dungeon. Only reject
        // battle and active transfers, including copies suspended by an IRQ.
        if(r[0xa11]==3||r[0x49]==1)return false;
        auto regs=core.cpuRegisters();
        auto transfer=[&](unsigned pc){
            if((pc>=0x0d49&&pc<=0x0d51)||(pc>=0x07d6&&pc<=0x07dd))return true;
            for(unsigned offset:colorProfile_->copies){
                unsigned start=copyPc(offset);
                if(r[0x49]==offset/0x4000&&pc>=start&&pc<=start+6)return true;
            }
            return false;
        };
        if(transfer(regs.pc))return false;
        if(regs.sp>=0xc000&&regs.sp<0xd000)
            for(unsigned a=regs.sp;a+1<0xd000;a+=2)
                if(transfer(r[a-0xc000]|(unsigned(r[a-0xc000+1])<<8)))return false;
        auto saved=static_cast<const unsigned char*>(core.savedata_ptr());
        for(unsigned slot=0;slot<20;slot++)
            if(colorRecord(r+0x10a+slot*110)&&!std::memcmp(r+0x10a+slot*110,saved+0x2b+slot*110,10))return true;
        return false;
    }
    static bool colorRecord(const unsigned char* r){return r[0]!=0&&r[0]!=0xff&&(r[0x0a]&3)<=2&&r[0x23]<=6;}
    template<class Apply> bool editColor(gambatte::GB& core,unsigned slot,Apply apply){
        if(slot>=20||!colorAvailable(core))return false;
        auto r=static_cast<unsigned char*>(core.rambank0_ptr())+0x10a+slot*110;
        auto s=static_cast<unsigned char*>(core.savedata_ptr());auto saved=s+0x2b+slot*110;
        if(!colorRecord(r)||std::memcmp(r,saved,10))return false;
        if(resyncNeeded_)resync(core);
        auto work=static_cast<unsigned char*>(core.rambank0_ptr())+0x9a2;
        if(workingSlot_>=0&&std::memcmp(work,workingIdentity_.data(),10))resync(core);
        if(workingSlot_==-2&&!std::memcmp(work,r,10))return false;
        apply(r);apply(saved);
        if(workingSlot_==static_cast<int>(slot))apply(work);
        unsigned sum=0;for(unsigned i=0;i<0xbfe;i++)sum+=s[i];
        s[0xbfe]=sum&255;s[0xbff]=static_cast<unsigned char>(~sum);
        return true;
    }
    bool learnColor(gambatte::GB& core,unsigned slot){
        auto apply=[](unsigned char* rec){
            constexpr unsigned char masks[]={15,7,15,7,15,15,7,31,15,15,31,15,15,7};
            for(unsigned i=0;i<14;i++){
                rec[0x34+i]|=masks[i];
                // MP is packed: current in high nibble, maximum in low.
                rec[0x26+i]=0x99;
            }
        };
        return editColor(core,slot,apply);
    }

public:
    static bool recognizes(const std::vector<char>& rom){
        if(GaidenSpellSupport::recognizes(rom)||recognizesColor(rom))return true;
        if(identifyGame(rom).id!=GameId::Gaiden1||rom.size()<0x10000)return false;
        const unsigned char maximum[]={0x3e,7,0xf5,0x0a,0xc5,0x0e,0,0xf5,0x21,0xc0,0xd0,0xcb,0x3e,0x23,0xcb,0x1e,0x23,0xcb,0x1e,0x23,0xcb,0x1e,0x30,1,0x0c,0xf1,0x3d,0x20,0xea,0x79,0x12,0x13,0xc1,3,0xf1,0x3d,0x20,0xdc,0xc9};
        const unsigned char counts[]={4,4,4,4,5,4,4,5,4,5,5,5,4,4};
        const unsigned char fields[]={0x21,0x3d,0xd1,0xcd,0xfb,2,0x21,0xc0,0xd0,0xcd,8,3,0x11,0x45,0xd1,1,0x44,0x26};
        const unsigned char sum[]={0x21,0,0,0x1b,0xd5,0x0a,0x5f,3,0x0a,0x57,0x19,0xd1,0x1b,0x7a,0xb3,0x20,0xf3};
        const unsigned char copy[]={0x21,0x17,0xa0,0x11,0x61,0xae,1,0x4a,0x0e,0xcd,0x85,0x21};
        const unsigned char loadWork[]={0xcd,0x35,0x1b,0x11,4,0xd1,1,0x80,0,0xcd,0x85,0x21,0xc9};
        const unsigned char saveWork[]={0xf5,0xcd,0x35,0x1b,0x54,0x5d,0x21,4,0xd1,1,0x80,0,0xcd,0x85,0x21};
        const unsigned char consume[]={0xf5,0xcd,0x18,0x13,0xfa,0xfa,0xd0,0xcd,0xc4,0x10,0x35,0xf1,0xc3,0x46,0x13};
        const unsigned char battleMode[]={0x3e,0x14,0xea,0x88,0xc5};
        auto matches=[&](size_t at,const auto& bytes){return std::equal(std::begin(bytes),std::end(bytes),reinterpret_cast<const unsigned char*>(rom.data()+at));};
        return rom.size()>=0x10088&&matches(0xb0f3,maximum)&&matches(0x2644,counts)&&matches(0xb11a,fields)&&matches(0xd05,sum)&&matches(0xce7,copy)&&matches(0x1318,loadWork)&&matches(0x1346,saveWork)&&matches(0x10083,battleMode)&&matches(0xee7a,consume);
    }
    static bool recognizesAlignment(const std::vector<char>& rom){
        if(GaidenSpellSupport::recognizesAlignment(rom))return true;
        if(recognizesColor(rom)){
            struct Profile {GameId id;unsigned creation,update,mask,lookup;};
            static const Profile profiles[]={
                {GameId::Wizardry1,0x192f6,0x17616,0x55d3,0x276b},
                {GameId::Wizardry2,0x19348,0x176a9,0x5a1e,0x2763},
                {GameId::Wizardry3,0x192ed,0x1761c,0x58aa,0x2731}
            };
            auto id=identifyGame(rom).id;
            for(const auto& p:profiles)if(p.id==id){
                auto match=[&](size_t at,std::initializer_list<unsigned char> bytes){return std::equal(bytes.begin(),bytes.end(),reinterpret_cast<const unsigned char*>(rom.data()+at));};
                auto lo=static_cast<unsigned char>(p.lookup),hi=static_cast<unsigned char>(p.lookup>>8);
                return match(p.creation,{0xfa,0x0e,0xc0,0xe6,3,0x47,0xfa,0xac,0xc9,0xb0,0xea,0xac,0xc9})
                    &&match(0xd2fe,{0x7a,0xe6,3,0x4f,6,0,0x21,0x34,0x53,9,0x2a,0xea,0x8d,0xff,0xcd})
                    &&match(p.update,{0xfa,0xb0,0xff,0xcd,lo,hi,1,0x0a,0,9,0x7e,0x47,0xe6,3,0xb7,0x28,8,0xcb,0x3f,0x4f,0x78,0xe6,0xfc,0xb1,0x77})
                    &&match(p.mask,{0xc5,0xd5,0xe5,0xaf,0x57,0x5f,0xfa,9,0xc1,0xba,0x28,0x1c,0x7a,0xcd,lo,hi,1,0x0a,0,9,0x7e,0xe6,3,0x28,8,0xfe,2});
            }
            return false;
        }
        if(identifyGame(rom).id!=GameId::Gaiden1||!recognizes(rom)||rom.size()<0x16de5)return false;
        // Creation stores the selected good/neutral/evil index in D10E;
        // the original display indexes the same three labels with that byte.
        const unsigned char creation[]={0x3e,3,0xcd,4,0x1c,0xcd,0x39,0x1d,0xea,0x0e,0xd1,0xcd,0x4b,0x1b};
        const unsigned char display[]={0x6f,0x26,0,0x29,0x11,0x61,0x1b,0x19,0x4e,0x23,0x46,0xc9};
        const unsigned char labels[]={0x9e,0xde,0xfd,0x20,0x20,0,0xe1,0x8d,0x93,0xf8,0xe2,0,0x91,0x98,0x20,0x20,0x20,0};
        const unsigned char pointers[]={0x7e,0x40,0x84,0x40,0x8a,0x40};
        const unsigned char party[]={0xfa,0x4f,0xc9,0xb7,0xc8,0xaf,0x21,0x50,0xc9,0xf5,0xe5,0xcd,0xb0,0x0e,0xfa,0x0e,0xd1,0x5f,0xcb,0x0b,0xcb,0x0b,0xe1,0x7e,0xe6,0x3f,0xb3,0x22};
        auto matches=[&](size_t at,const auto& bytes){return std::equal(std::begin(bytes),std::end(bytes),reinterpret_cast<const unsigned char*>(rom.data()+at));};
        return matches(0x16dd7,creation)&&matches(0x1b4b,display)&&matches(0x407e,labels)&&matches(0x1b61,pointers)&&matches(0x64d,party);
    }
    void attach(const std::vector<char>& rom){extra_.attach(rom);supported_=recognizes(rom);colorProfile_=supported_?colorProfile(rom):nullptr;color_=colorProfile_!=nullptr;alignmentSupported_=recognizesAlignment(rom);workingSlot_=-1;resyncNeeded_=color_;}
    bool supported()const{return supported_;}
    bool learnsMaximumMp()const{return supported_;}
    bool alignmentSupported()const{return alignmentSupported_;}
    int alignment(gambatte::GB& core,unsigned slot)const{
        if(!alignmentSupported_||slot>=20||!available(core))return -1;
        if(extra_.supported())return extra_.alignment(core,slot);
        if(color_){
            auto r=static_cast<const unsigned char*>(core.rambank0_ptr())+0x10a+slot*110;
            auto saved=static_cast<const unsigned char*>(core.savedata_ptr())+0x2b+slot*110;
            return colorRecord(r)&&!std::memcmp(r,saved,10)?r[10]&3:-1;
        }
        auto r=static_cast<const unsigned char*>(core.rambank0_ptr())+roster+slot*recordSize;
        return r[0]&&std::memcmp(r+120,"Wizardry",8)==0&&r[10]<3?r[10]:-1;
    }
    bool battle(gambatte::GB& core)const{if(extra_.supported())return extra_.battle(core);return supported_&&static_cast<const unsigned char*>(core.rambank0_ptr())[0x588]==0x14;}
    bool busy(gambatte::GB& core)const{
        if(extra_.supported())return extra_.busy(core);
        auto regs=core.cpuRegisters();auto copy=[](unsigned pc){return pc>=0x2185&&pc<=0x218d;};
        if(copy(regs.pc))return true;
        auto ram=static_cast<const unsigned char*>(core.rambank0_ptr());
        // A VBlank/timer interrupt can suspend the shared copy routine.
        if(regs.sp>=0xc000&&regs.sp<0xe000)for(unsigned at=regs.sp;at+1<0xe000;at+=2)if(copy(ram[at-0xc000]|(unsigned(ram[at-0xc000+1])<<8)))return true;
        return false;
    }
    bool available(gambatte::GB& core)const{
        if(extra_.supported())return extra_.available(core);
        if(color_)return colorAvailable(core);
        if(!supported_||core.isCgb()||core.savedata_size()!=8192||battle(core)||busy(core))return false;
        auto ram=static_cast<const unsigned char*>(core.rambank0_ptr());
        for(unsigned slot=0;slot<20;slot++){auto record=ram+roster+slot*recordSize;if(record[0]&&std::memcmp(record+120,"Wizardry",8)==0)return true;}
        return false;
    }
    void resync(gambatte::GB& core){
        if(extra_.supported()){extra_.resync(core);return;}
        workingSlot_=-1;resyncNeeded_=false;if(!supported_)return;
        if(color_){
            auto r=static_cast<const unsigned char*>(core.rambank0_ptr());auto work=r+0x9a2;
            if(!colorRecord(work))return;
            std::copy(work,work+10,workingIdentity_.begin());int found=-1;
            for(unsigned slot=0;slot<20;slot++)if(!std::memcmp(r+0x10a+slot*110,work,10)){
                if(found!=-1){workingSlot_=-2;return;}found=static_cast<int>(slot);
            }
            if(found>=0)workingSlot_=std::memcmp(r+0x10a+found*110,work,110)?-2:found;
            return;
        }
        auto ram=static_cast<const unsigned char*>(core.rambank0_ptr());auto work=ram+0x1104;
        if(!work[0]||std::memcmp(work+120,"Wizardry",8))return;
        std::copy(work,work+9,workingIdentity_.begin());
        int candidate=-1;
        for(unsigned slot=0;slot<20;slot++){auto record=ram+roster+slot*recordSize;
            if(record[0]&&std::memcmp(record,work,9)==0){if(candidate!=-1){workingSlot_=-2;return;}candidate=static_cast<int>(slot);}}
        if(candidate>=0)workingSlot_=std::memcmp(work,ram+roster+candidate*recordSize,recordSize)==0?candidate:-2;
    }
    void observe(gambatte::GB& core,unsigned pc,unsigned address,unsigned value){
        if(extra_.supported()){extra_.observe(core,pc,address,value);return;}
        if(!supported_)return;
        if(color_){
            auto r=static_cast<const unsigned char*>(core.rambank0_ptr());
            bool copying=pc==0x0d4b;
            for(unsigned index:{1u,5u,6u,7u}){
                unsigned offset=colorProfile_->copies[index];
                copying|=r[0x49]==offset/0x4000&&pc==copyPc(offset)+2;
            }
            if(address==0xc9a2&&copying){
                unsigned source=core.cpuRegisters().hl-1;
                workingSlot_=(source>=0xc10a&&source<0xc9a2&&(source-0xc10a)%110==0)?static_cast<int>((source-0xc10a)/110):-1;
                if(workingSlot_>=0)std::copy(r+source-0xc000,r+source-0xc000+10,workingIdentity_.begin());
                resyncNeeded_=false;
            }else if(address>=0xc9a2&&address<0xc9ac&&!copying&&r[address-0xc000]!=value)workingSlot_=-1;
            if(address>=0xc10a&&address<0xc9a2&&(address-0xc10a)%110==0)resyncNeeded_=true;
            return;
        }
        auto ram=static_cast<const unsigned char*>(core.rambank0_ptr());
        if(pc==0x2187&&address==0xd104){
            unsigned source=core.cpuRegisters().hl-1;
            if(source>=0xd186&&source<0xdb86&&(source-0xd186)%128==0){workingSlot_=static_cast<int>((source-0xd186)/128);std::copy(ram+source-0xc000,ram+source-0xc000+9,workingIdentity_.begin());resyncNeeded_=false;}
            else workingSlot_=-1; // New character or another unregistered workspace.
        }else if(address>=0xd104&&address<=0xd10c&&pc!=0x2187&&ram[address-0xc000]!=value)workingSlot_=-1;
        if(pc==0x2187&&address>=0xd186&&address<0xdb86&&(address-0xd186)%128==0){
            unsigned source=core.cpuRegisters().hl-1;
            if(source==0xd104){workingSlot_=static_cast<int>((address-0xd186)/128);std::copy(ram+0x1104,ram+0x110d,workingIdentity_.begin());resyncNeeded_=false;}
            else resyncNeeded_=true; // Roster reorder/load: resolve again once paused.
        }
    }
    bool town(gambatte::GB& core)const{
        if(!supported_||core.isCgb()||core.savedata_size()!=8192)return false;
        auto io=static_cast<const unsigned char*>(core.zeropage_ptr())-0x80;
        if(!(io[0x40]&0x80)||io[0x42]||io[0x43])return false;
        auto map=static_cast<const unsigned char*>(core.vram_ptr())+((io[0x40]&8)?0x1c00:0x1800);
        const unsigned char top[]={0x10,0x11,0x10,0x10,0x10,0x10,0x12,0x13,0x14,0x15,0x16};
        const unsigned char lower[]={0x10,0x5b,0x5c,0x5d,0x5d,0x5d,0x5d,0x5e,0x5f,0x60,0x5d};
        return std::equal(std::begin(top),std::end(top),map)&&std::equal(std::begin(lower),std::end(lower),map+8*32);
    }
    std::vector<SpellCharacter> characters(gambatte::GB& core)const{
        std::vector<SpellCharacter> result;if(!available(core))return result;
        if(extra_.supported()){for(unsigned slot:extra_.slots(core))result.push_back({slot,name(extra_.record(core,slot))});return result;}
        if(color_){
            auto r=static_cast<const unsigned char*>(core.rambank0_ptr());
            for(unsigned slot=0;slot<20;slot++)if(colorRecord(r+0x10a+slot*110)){
                std::string label=wizardryCharacterName(r+0x10a+slot*110);
                result.push_back({slot,label});
            }
            return result;
        }
        auto ram=static_cast<const unsigned char*>(core.rambank0_ptr());
        for(unsigned slot=0;slot<20;slot++){auto record=ram+roster+slot*recordSize;if(record[0]&&std::memcmp(record+120,"Wizardry",8)==0)result.push_back({slot,name(record)});}
        return result;
    }
private:
    template<class Apply> bool edit(gambatte::GB& core,unsigned slot,Apply apply){
        if(!available(core)||slot>=20)return false;
        if(resyncNeeded_)resync(core);
        auto record=static_cast<unsigned char*>(core.rambank0_ptr())+roster+slot*recordSize;
        auto save=static_cast<unsigned char*>(core.savedata_ptr());auto primary=save+saveRoster+slot*recordSize;
        if(!record[0]||std::memcmp(record+120,"Wizardry",8))return false;
        auto work=static_cast<unsigned char*>(core.rambank0_ptr())+0x1104;
        if(workingSlot_>=0&&std::memcmp(work,workingIdentity_.data(),9))resync(core);
        if(workingSlot_==-2&&std::memcmp(record,workingIdentity_.data(),9)==0)return false;
        if(!primary[0]||std::memcmp(primary,record,9)||std::memcmp(primary+120,"Wizardry",8))return false;
        apply(record);
        if(workingSlot_==static_cast<int>(slot))apply(work);
        // Merge only the selected fields; preserve other live/saved stats.
        apply(primary);
        std::copy(primary,primary+recordSize,primary+mirror);
        unsigned sum=checksum(primary);
        for(size_t base:{size_t(0xe07),size_t(0x1c51)}){save[base+slot*2]=sum&255;save[base+slot*2+1]=sum>>8;}
        return true;
    }
public:
    bool learn(gambatte::GB& core,unsigned slot){if(extra_.supported())return extra_.learn(core,slot);if(color_)return learnColor(core,slot);return edit(core,slot,[](unsigned char* data){data[0x39]|=0x1f;std::fill(data+0x3a,data+0x41,0xff);std::fill(data+0x41,data+0x4f,9);});}
    bool write(gambatte::GB& core,unsigned pc,unsigned address,unsigned value)const{
        if(extra_.supported())return extra_.write(core,pc,address,value);
        if(!supported_||color_||pc!=0x7112||address<0xd145||address>=0xd153)return true;
        auto r=static_cast<unsigned char*>(core.rambank0_ptr());
        if(r[0x10ae]!=2||(r[0x113d]&31)!=31||
            !std::all_of(r+0x113e,r+0x1145,[](unsigned char v){return v==255;}))return true;
        // Gaiden I derives maximum MP into the working MP fields. Change
        // that calculation only; spell consumption remains ordinary.
        r[address-0xc000]=9;return false;
    }
    bool restoreMp(gambatte::GB& core,unsigned slot){if(color_||extra_.supported())return false;return edit(core,slot,[](unsigned char* data){std::fill(data+0x41,data+0x4f,9);});}
    bool setAlignment(gambatte::GB& core,unsigned slot,unsigned alignment){
        if(!alignmentSupported_||alignment>2)return false;
        if(extra_.supported())return extra_.setAlignment(core,slot,alignment);
        if(color_)return editColor(core,slot,[alignment](unsigned char* data){data[10]=static_cast<unsigned char>((data[10]&0xfc)|alignment);});
        if(!edit(core,slot,[alignment](unsigned char* data){data[0x0a]=static_cast<unsigned char>(alignment);}))return false;
        // Party entries also cache alignment in bits 6-7 (ROM 064D).
        // Preserve the member index and status bit, including unused entries.
        auto ram=static_cast<unsigned char*>(core.rambank0_ptr());
        for(unsigned i=0;i<std::min(unsigned(ram[0x94f]),6u);i++){
            auto& member=ram[0x950+i];if(member!=0xff&&(member&31)==slot)member=static_cast<unsigned char>((member&63)|(alignment<<6));
        }
        return true;
    }
};
