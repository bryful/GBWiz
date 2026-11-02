#pragma once
#include "game_support.h"

// Gaiden II/III keep 128-byte characters, but have different rosters,
// backup offsets and (III) a third spell school.
class GaidenSpellSupport {
    struct Profile{GameId id;unsigned roster,work,save,mirror,sum,copy,load,store,bank,battle,counts,maximum,accounting,mask;};
    const Profile* p_=nullptr;
    int working_=-1;bool resync_=true;
    std::array<unsigned char,9> identity_{};
    static const Profile* profile(const std::vector<char>& rom){
        static const Profile profiles[]={
            {GameId::Gaiden2,0x1095,0x1013,0x18,0xe5e,0xe1c,0x2180,0x1175,0x11a3,0xfbd,0x55d,0x26ae,0xacf1,0xad18,0x7f},
            {GameId::Gaiden3,0xba1,0xb1f,0x0a,0xe52,0xe04,0x1fc7,0x1021,0x105a,0xac7,0xf3,0x24f4,0xb05b,0xb082,0x3f}
        };
        auto id=identifyGame(rom).id;for(const auto& p:profiles)if(p.id==id)return &p;return nullptr;
    }
    bool third()const{return p_->id==GameId::Gaiden3;}
    bool valid(const unsigned char* r)const{return r[0]&&r[0]!=255&&(third()?r[0x0a]<=2:std::memcmp(r+120,"Wizardry",8)==0);}
    bool learned(const unsigned char* r)const{
        return (r[0x39]&p_->mask)==p_->mask&&std::all_of(r+0x3a,r+0x41,[](unsigned char v){return v==255;})
            &&(!third()||((r[0x6a]&7)==7&&std::all_of(r+0x6b,r+0x6e,[](unsigned char v){return v==255;})));
    }
    static unsigned checksum(const unsigned char* r){unsigned n=0;for(unsigned i=0;i<127;i++)n+=r[i]|(unsigned(r[i+1])<<8);return n&65535;}
public:
    static bool recognizes(const std::vector<char>& rom){
        auto p=profile(rom);if(!p||rom.size()<0x1b000)return false;
        auto match=[&](size_t a,std::initializer_list<unsigned char> b){return std::equal(b.begin(),b.end(),reinterpret_cast<const unsigned char*>(rom.data()+a));};
        auto lo=[](unsigned a){return static_cast<unsigned char>(a);};auto hi=[](unsigned a){return static_cast<unsigned char>(a>>8);};
        bool third=p->id==GameId::Gaiden3;unsigned decode=third?0x291:0x30e,unpack=third?0x29e:0x31b,scratch=third?0xcad9:0xcfcf;
        if(!match(third?0xa1a:0xb42,{0x21,0,0,0x1b,0xd5,0x0a,0x5f,3,0x0a,0x57,0x19,0xd1,0x1b,0x7a,0xb3,0x20,0xf3})
            ||!match(p->copy,{0x2a,0x12,0x13,0x0b,0x78,0xb1,0x20,0xf8,0xc9})
            ||!match(p->maximum,{0x3e,7,0xf5,0x0a,0xc5,0x0e,0,0xf5,0x21,lo(scratch),hi(scratch),0xcb,0x3e,0x23,0xcb,0x1e,0x23,0xcb,0x1e,0x23,0xcb,0x1e,0x30,1,0x0c,0xf1,0x3d,0x20,0xea,0x79,0x12,0x13,0xc1,3,0xf1,0x3d,0x20,0xdc,0xc9}))return false;
        unsigned flag=p->work+0xc000+0x39,mp=flag+8,target=(p->maximum&0x3fff)+0x4000;
        if(!match(p->accounting,{0x21,lo(flag),hi(flag),0xcd,lo(decode),hi(decode),0x21,lo(scratch),hi(scratch),0xcd,lo(unpack),hi(unpack),0x11,lo(mp),hi(mp),1,lo(p->counts),hi(p->counts),0xcd,lo(target),hi(target)}))return false;
        if(third)return match(p->counts,{4,4,4,4,5,5,4,5,4,4,5,5,5,4,4,4,3,4,4,4,4})
            &&match(0x1025,{0xcd,0xe0,0x17,0x11,0x1f,0xcb,1,0x80,0,0xc3,0xc7,0x1f})
            &&match(0x105a,{0xf5,0xcd,0xe0,0x17,0x54,0x5d,0x21,0x1f,0xcb,1,0x80,0,0xcd,0xc7,0x1f})
            &&match(0x1ae2a,{0x21,0x0a,0xa0,0x11,0x5c,0xae,1,0x52,0x0e,0xcd,0xc7,0x1f})
            &&match(0x1adc5,{0x11,4,0xae,0x19,0xe5,0xcd,0xdb,0x17,0x44,0x4d,0x11,0x80,0})
            &&match(0x100a7,{0x3e,0x14,0xea,0xf3,0xc0});
        return match(p->counts,{4,4,4,4,6,5,4,5,4,4,4,6,5,4})
            &&match(0x1175,{0xcd,0x8a,0x1a,0x11,0x13,0xd0,1,0x80,0,0xcd,0x80,0x21,0xc9})
            &&match(0x11a3,{0xf5,0xcd,0x8a,0x1a,0x54,0x5d,0x21,0x13,0xd0,1,0x80,0,0xcd,0x80,0x21})
            &&match(0xb25,{0x21,0x18,0xa0,0x11,0x76,0xae,1,0x5e,0x0e,0xcd,0x80,0x21})
            &&match(0xab4,{0x11,0x1c,0xae,0x19,0xe5,0xcd,0x85,0x1a,0x44,0x4d,0x11,0x80,0})
            &&match(0x10091,{0x3e,0x14,0xea,0x5d,0xc9});
    }
    static bool recognizesAlignment(const std::vector<char>& rom){
        auto p=profile(rom);if(!p||!recognizes(rom)||rom.size()<0x21194)return false;
        bool third=p->id==GameId::Gaiden3;
        auto lo=[](unsigned a){return static_cast<unsigned char>(a);};auto hi=[](unsigned a){return static_cast<unsigned char>(a>>8);};
        auto match=[&](size_t a,std::initializer_list<unsigned char> b){return std::equal(b.begin(),b.end(),reinterpret_cast<const unsigned char*>(rom.data()+a));};
        unsigned attribute=p->work+0xc000+10,display=third?0x17f6:0x1aa0,table=third?0x1816:0x1ab6;
        unsigned party=third?0xc9af:0xcd29,count=party-1,loader=third?0x0be8:0x0d07;
        return match(third?0x21186:0x20d13,{0x3e,3,0xcd,lo(third?0xdf:0x59),lo(third?0x18:0x1b),0xcd,lo(third?0x72:0x9b),lo(third?0x1a:0x1c),0xea,lo(attribute),hi(attribute),0xcd,lo(display),hi(display)})
            &&match(display,{0x6f,0x26,0,0x29,0x11,lo(table),hi(table),0x19,0x4e,0x23,0x46,0xc9})
            &&match(table,{lo(third?0x15:0xad),lo(third?0x43:0x40),lo(third?0x1b:0xb3),lo(third?0x43:0x40),lo(third?0x21:0xb9),lo(third?0x43:0x40)})
            &&match(third?0x4315:0x40ad,{0x9e,0xde,0xfd,0x20,0x20,0,0xe1,0x8d,0x93,0xf8,0xe2,0,0x91,0x98,0x20,0x20,0x20,0})
            &&match(third?0x71d:0x79b,{0xfa,lo(count),hi(count),0xb7,0xc8,0xaf,0x21,lo(party),hi(party),0xf5,0xe5,0xcd,lo(loader),hi(loader),0xfa,lo(attribute),hi(attribute),0x5f,0xcb,0x0b,0xcb,0x0b,0xe1,0x7e,0xe6,0x3f,0xb3,0x22});
    }
    int alignment(gambatte::GB& core,unsigned slot)const{
        if(slot>=20||!available(core))return -1;
        auto r=record(core,slot);auto saved=static_cast<const unsigned char*>(core.savedata_ptr())+p_->save+slot*128;
        return valid(r)&&r[10]<3&&!std::memcmp(r,saved,9)?r[10]:-1;
    }
    void attach(const std::vector<char>& rom){p_=recognizes(rom)?profile(rom):nullptr;working_=-1;resync_=true;}
    bool supported()const{return p_!=nullptr;}
    bool battle(gambatte::GB& core)const{return p_&&static_cast<const unsigned char*>(core.rambank0_ptr())[p_->battle]==0x14;}
    bool busy(gambatte::GB& core)const{
        auto regs=core.cpuRegisters();auto copy=[&](unsigned pc){return pc>=p_->copy&&pc<=p_->copy+8;};if(copy(regs.pc))return true;
        auto r=static_cast<const unsigned char*>(core.rambank0_ptr());
        if(regs.sp>=0xc000&&regs.sp<0xe000)for(unsigned a=regs.sp;a+1<0xe000;a+=2)if(copy(r[a-0xc000]|(unsigned(r[a-0xc000+1])<<8)))return true;
        return false;
    }
    bool available(gambatte::GB& core)const{
        if(!p_||core.isCgb()||core.savedata_size()!=(third()?32768u:8192u)||battle(core)||busy(core))return false;
        auto r=static_cast<const unsigned char*>(core.rambank0_ptr());auto s=static_cast<const unsigned char*>(core.savedata_ptr());
        for(unsigned i=0;i<20;i++)if(valid(r+p_->roster+i*128)&&!std::memcmp(r+p_->roster+i*128,s+p_->save+i*128,9))return true;
        return false;
    }
    const unsigned char* record(gambatte::GB& core,unsigned slot)const{return static_cast<const unsigned char*>(core.rambank0_ptr())+p_->roster+slot*128;}
    std::vector<unsigned> slots(gambatte::GB& core)const{
        std::vector<unsigned> result;if(!available(core))return result;for(unsigned i=0;i<20;i++)if(valid(record(core,i)))result.push_back(i);return result;
    }
    void resync(gambatte::GB& core){
        working_=-1;resync_=false;if(!p_)return;auto r=static_cast<const unsigned char*>(core.rambank0_ptr());auto work=r+p_->work;if(!valid(work))return;
        std::copy(work,work+9,identity_.begin());int found=-1;
        for(unsigned i=0;i<20;i++)if(valid(record(core,i))&&!std::memcmp(record(core,i),work,9)){if(found!=-1){working_=-2;return;}found=i;}
        if(found>=0)working_=std::memcmp(record(core,found),work,128)?-2:found;
    }
    void observe(gambatte::GB& core,unsigned pc,unsigned address,unsigned value){
        if(!p_)return;auto r=static_cast<const unsigned char*>(core.rambank0_ptr());unsigned work=p_->work+0xc000,roster=p_->roster+0xc000;
        if(pc==p_->copy+2&&address==work){unsigned src=core.cpuRegisters().hl-1;
            working_=(src>=roster&&src<roster+20*128&&(src-roster)%128==0)?int((src-roster)/128):-1;
            if(working_>=0)std::copy(r+src-0xc000,r+src-0xc000+9,identity_.begin());resync_=false;
        }else if(address>=work&&address<work+9&&pc!=p_->copy+2&&r[address-0xc000]!=value)working_=-1;
        if(pc==p_->copy+2&&address>=roster&&address<roster+20*128&&(address-roster)%128==0)resync_=true;
    }
private:
    template<class Apply> bool edit(gambatte::GB& core,unsigned slot,Apply apply){
        if(slot>=20||!available(core))return false;if(resync_)resync(core);
        auto r=static_cast<unsigned char*>(core.rambank0_ptr())+p_->roster+slot*128;auto s=static_cast<unsigned char*>(core.savedata_ptr());auto saved=s+p_->save+slot*128;
        if(!valid(r)||std::memcmp(r,saved,9))return false;auto work=static_cast<unsigned char*>(core.rambank0_ptr())+p_->work;
        if(working_>=0&&std::memcmp(work,identity_.data(),9))resync(core);
        if(working_==-2&&!std::memcmp(work,r,9))return false;
        apply(r);apply(saved);if(working_==int(slot))apply(work);
        std::copy(saved,saved+128,saved+p_->mirror);unsigned sum=checksum(saved);
        for(unsigned base:{p_->sum,p_->sum+p_->mirror}){s[base+slot*2]=sum&255;s[base+slot*2+1]=sum>>8;}
        return true;
    }
public:
    bool learn(gambatte::GB& core,unsigned slot){
        auto apply=[&](unsigned char* rec){rec[0x39]|=p_->mask;std::fill(rec+0x3a,rec+0x41,255);std::fill(rec+0x41,rec+0x4f,9);
            if(third()){rec[0x6a]|=7;std::fill(rec+0x6b,rec+0x6e,255);std::fill(rec+0x6e,rec+0x75,9);}};
        return edit(core,slot,apply);
    }
    bool setAlignment(gambatte::GB& core,unsigned slot,unsigned value){
        if(value>2||!edit(core,slot,[value](unsigned char* rec){rec[10]=static_cast<unsigned char>(value);}))return false;
        auto r=static_cast<unsigned char*>(core.rambank0_ptr());unsigned party=third()?0x9af:0xd29;
        for(unsigned i=0;i<std::min(unsigned(r[party-1]),6u);i++){
            auto& member=r[party+i];if(member!=255&&(member&31)==slot)member=static_cast<unsigned char>((member&63)|(value<<6));
        }
        return true;
    }
    bool write(gambatte::GB& core,unsigned pc,unsigned address,unsigned)const{
        if(!p_||pc!=((p_->maximum&0x3fff)+0x4000+31))return true;
        auto r=static_cast<unsigned char*>(core.rambank0_ptr());unsigned mp=p_->work+0xc000+0x41;
        if(r[p_->bank]!=2||!learned(r+p_->work)||!((address>=mp&&address<mp+14)||(third()&&address>=mp+0x2d&&address<mp+0x34)))return true;
        r[address-0xc000]=9;return false;
    }
};
