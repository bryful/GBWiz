#pragma once
#include "game_support.h"
#include <functional>

// Wizardry: host-owned floor decoding, selection and relocation. No Malor code.
class DungeonMap {
    std::vector<char> rom_;
    bool supported_=false;
    GameId id_=GameId::Unknown;
    unsigned position_=0,previous_=0,party_=0,bankVar_=0,entry_=0,mapRam_=0;
    unsigned variantFlags_=0;
    std::vector<unsigned> floors_,waits_;
    bool color()const{return id_==GameId::Wizardry1||id_==GameId::Wizardry2||id_==GameId::Wizardry3;}
    unsigned physicalFloor(unsigned f)const{
        if(id_!=GameId::Wizardry1)return f;
        if(f<5)return f;
        if(f>7)return f+6;
        unsigned shift=(f-4)*2,variant=(variantFlags_>>shift)&3;
        return 5+(f-5)*3+std::min(variant,2u);
    }

    static size_t field(const std::vector<char>& s,const std::string& key,unsigned length){
        if(s.size()<5)return 0;
        size_t p=5+(static_cast<unsigned char>(s[2])<<16)+(static_cast<unsigned char>(s[3])<<8)+static_cast<unsigned char>(s[4]);
        while(p<s.size()){
            size_t q=p;while(q<s.size()&&s[q])++q;
            if(q+4>s.size())return 0;
            std::string label(s.data()+p,q-p);
            unsigned n=(static_cast<unsigned char>(s[q+1])<<16)|(static_cast<unsigned char>(s[q+2])<<8)|static_cast<unsigned char>(s[q+3]);
            p=q+4;if(n>s.size()-p)return 0;
            if(label==key)return n==length?p:0;p+=n;
        }return 0;
    }
    size_t base(unsigned f)const{
        if(!supported_||!validFloor(f))return 0;
        if(color())return 0xfc000+(physicalFloor(f)-1)*768;
        if(id_==GameId::Gaiden3)return f<=21?0x78000+(f-1)*768:0x7c06b+(f-22)*768;
        size_t table=id_==GameId::Gaiden1?0x1918f+((f&128)?12:0)+2*((f&127)-1):0x191ab+((f&128)?22:0)+2*((f&127)-1);
        unsigned address=static_cast<unsigned char>(rom_[table])|(static_cast<unsigned char>(rom_[table+1])<<8);
        return 0x14000+address;
    }
public:
    bool open=false;
    unsigned floor=1,x=0,y=0,stage=0;
    bool validFloor(unsigned f)const{return std::find(floors_.begin(),floors_.end(),f)!=floors_.end();}
    const std::vector<unsigned>& floors()const{return floors_;}
    std::string floorLabel()const{return (id_==GameId::Gaiden3?std::string("MAP "):std::string(floor&128?"OTHER B":"B"))+std::to_string(floor&127);}
    unsigned mapAddress()const{return mapRam_;}
    unsigned positionAddress()const{return position_;}
    bool isColor()const{return color();}
    void attach(const std::vector<char>& rom){
        open=false;rom_=rom;supported_=false;floors_.clear();waits_.clear();variantFlags_=0;id_=identifyGame(rom).id;
        if(rom.size()<0x20000)return;
        auto match=[&](size_t at,std::initializer_list<unsigned char> bytes){if(at+bytes.size()>rom.size())return false;for(auto b:bytes)if(static_cast<unsigned char>(rom[at++])!=b)return false;return true;};
        unsigned count=0;
        switch(id_){
        case GameId::Gaiden1:
            position_=0x8a5;previous_=0x8af;party_=0x94f;bankVar_=0x10ae;entry_=0x409f;mapRam_=0x590;count=6;waits_={0x4114,0x4132,0x4135,0x512c};
            supported_=match(0x1916c,{0x11,0x8f,0x51,0xcb,0x7f})&&match(0xc09f,{0x31,0,0xe0,0xcd,0xa8,0x40,0xc3,5,0x41})&&match(0xc12a,{0xcd,0xea,0x1b,0x3e,9,0xcd,0xd0,0x22,0xcd,0x6e,0x22});break;
        case GameId::Gaiden2:
            position_=0xc7a;previous_=0xc84;party_=0xd28;bankVar_=0xfbd;entry_=0x408d;mapRam_=0x965;count=11;waits_={0x4135,0x4153,0x41a5,0x532f};
            supported_=match(0x1916c,{0x11,0xab,0x51,0xcb,0x7f,0x28,3,0x11,0xc1,0x51})&&match(0xc08d,{0x31,0,0xe0,0xcd,0x96,0x40,0xc3,0x14,0x41})&&match(0xc19e,{0xaf,0xea,0x26,0xcd,0xcd,0x69,0x22});break;
        case GameId::Gaiden3:
            position_=0x95f;previous_=0x969;party_=0x9ae;bankVar_=0xac7;entry_=0x410d;mapRam_=0x64a;count=36;waits_={0x41a9,0x41ae,0x41b1,0x41b4,0x41b9,0x420d,0x5cb0};
            supported_=match(0x1334,{0x11,0x4a,0xc6,0xea,0x8f,0xc9,0xd5,6,0x1e,0x11,0,0x40})&&match(0xc10d,{0x31,0,0xe0,0xcd,0x16,0x41,0xc3,0x97,0x41})&&match(0xc20a,{0xcd,0xb7,0x20});break;
        case GameId::Wizardry1:case GameId::Wizardry2:case GameId::Wizardry3:
            position_=0xf9;previous_=0xfd;party_=0x109;bankVar_=0x49;entry_=0x4492;mapRam_=0x7100;count=id_==GameId::Wizardry2?11:10;
            waits_={0x407c,id_==GameId::Wizardry2?0x4b0au:0x4b15u};
            supported_=match(0x1357,{0xf5,0xfa,0x70,0xff,0xf5,0x3e,7,0xea,0x70,0xff,0xfa,0xfc,0xc0})&&match(0x8006,{0x3e,7,0xea,0x70,0xff,0x3e,6,0x21,0x92,0x44,0xc7})&&match(0x8000+0x79,{0xcd,0x27,0x0b,0x18,0xbb})&&match(0x000f,{0xf1,0xea,0x49,0xc0,0xea,0,0x20,0xc9})&&match(0x18492,{0x3e,1,0xcd,0x57,0x13});break;
        default:return;
        }
        for(unsigned f=1;f<=count;f++)floors_.push_back(f);
        if(id_==GameId::Gaiden1)for(unsigned f=129;f<=134;f++)floors_.push_back(f);
        if(id_==GameId::Gaiden2)floors_.push_back(129);
        if(supported_)for(unsigned f:floors_){auto b=base(f);if(b+768>rom.size()){supported_=false;break;}}
    }
    bool supported()const{return supported_;}
    bool available(gambatte::GB& core)const{
        if(!supported_||core.isCgb()!=color())return false;
        auto r=static_cast<const unsigned char*>(core.rambank0_ptr());
        unsigned f=r[position_+(color()?3:2)];
        if(!validFloor(f)||r[position_]>15||r[position_+1]>15||r[party_]<1||r[party_]>6)return false;
        if(color()){
            if(r[0xa11]!=0&&r[0xa11]!=2)return false;
            if(r[bankVar_]!=2&&r[bankVar_]!=3)return false;
        }else if(r[bankVar_]!=3)return false;
        // Only ordinary dungeon/camp input waits. Battle, messages and
        // transitions use different return addresses, even with stale positions.
        auto regs=core.cpuRegisters();unsigned end=color()?0xcfff:0xe000;
        if(regs.sp<(color()?0xcf80u:0xdfc0u)||regs.sp>=end)return false;
        for(unsigned at=regs.sp;at+1<end;at+=2){
            unsigned ret=r[at-0xc000]|(unsigned(r[at-0xc000+1])<<8);
            if(std::find(waits_.begin(),waits_.end(),ret)!=waits_.end()){
                if(!color()||(ret==0x407c?r[0xa11]==2&&r[bankVar_]==2:r[0xa11]==0))return true;
            }
        }
        return false;
    }
    bool show(gambatte::GB& core){
        if(!available(core))return false;auto r=static_cast<const unsigned char*>(core.rambank0_ptr());
        floor=r[position_+(color()?3:2)];x=r[position_];y=r[position_+1];variantFlags_=r[0xa9c];stage=0;open=true;return true;
    }
    unsigned cell(unsigned f,unsigned cx,unsigned cy)const{
        auto b=base(f);if(!b||cx>=16||cy>=16)return 0x55;
        if(!color())return static_cast<unsigned char>(rom_[b+(15-cy)*32+cx*2]);
        unsigned raw=static_cast<unsigned char>(rom_[b+cy*16+cx]);
        return ((raw&3)<<6)|((raw&0xc0)>>2)|(raw&0x0c)|((raw&0x30)>>4);
    }
    unsigned event(unsigned f,unsigned cx,unsigned cy)const{
        auto b=base(f);if(!b||cx>=16||cy>=16)return 255;
        return static_cast<unsigned char>(rom_[b+(color()?256+cy*16+cx:(15-cy)*32+cx*2+1)])&(color()?0xff:31);
    }
    bool destination(unsigned f,unsigned cx,unsigned cy)const{return validFloor(f)&&cx<16&&cy<16&&cell(f,cx,cy)!=0x55;}
    // Arrows select XY; PageUp/LB and PageDown/RB select the floor.
    // A executes the selected destination; B closes without moving.
    bool key(unsigned bits){
        if(!open)return false;
        if(bits&2){open=false;return false;}
        stage=0;
        unsigned index=static_cast<unsigned>(std::find(floors_.begin(),floors_.end(),floor)-floors_.begin());
        if(bits&256)index=(index+floors_.size()-1)%floors_.size();
        if(bits&512)index=(index+1)%floors_.size();
        floor=floors_[index];
        if(bits&16)x=(x+1)%16;if(bits&32)x=(x+15)%16;
        if(bits&64)y=(y+1)%16;if(bits&128)y=(y+15)%16;
        if((bits&1)&&destination(floor,x,y)){stage=2;return true;}
        return false;
    }
    bool move(gambatte::GB& core,const std::function<bool()>& saveSlot1){
        if(!open||stage!=2||!available(core)||!destination(floor,x,y))return false;
        std::vector<char> state(core.stateSize());core.saveState(state.data());
        auto original=state;
        auto bank=field(state,"rombank",2),sp=field(state,"sp",2);
        auto pc=field(state,"pc",2),ime=field(state,"ime",1),halted=field(state,"halt",1),ram=field(state,"wram",color()?32768:8192);
        if(!pc||!ime||!ram||(color()&&(!bank||!sp)))return false;
        // The callback must durably save the full TurboRunner state before
        // any mutation. Exceptions/failed saves leave the running core intact.
        if(!saveSlot1())return false;
        state[ram+position_]=char(x);state[ram+position_+1]=char(y);state[ram+position_+(color()?3:2)]=char(floor);
        state[ram+previous_]=char(x);state[ram+previous_+1]=char(y);state[ram+previous_+(color()?3:2)]=char(floor);
        state[pc]=char(entry_>>8);state[pc+1]=char(entry_);state[ime]=1;if(halted)state[halted]=0;
        if(color()){
            state[bank]=0;state[bank+1]=6;state[ram+bankVar_]=6;
            state[sp]=char(0xcf);state[sp+1]=char(0xf9);
            // Same return frames as RST 00: restore ROM bank 2, then
            // resume the ordinary dungeon input loop after the reload.
            const unsigned char returns[]={0x0f,0,0,2,0x39,0x40};
            for(unsigned i=0;i<sizeof(returns);i++)state[ram+0xff9+i]=char(returns[i]);
        }
        // Resume each game's ordinary dungeon reload, never its Malor path.
        if(!core.loadState(state.data(),state.size())){core.loadState(original.data(),original.size());return false;}
        open=false;return true;
    }
};
// Shared pixel renderer keeps desktop and Android map behavior identical.
inline void drawDungeonMap(const DungeonMap& map,gambatte::GB& core,std::array<gambatte::video_pixel_t,160*144>& pixels){
    pixels.fill(0x101822);
    auto dot=[&](int x,int y,unsigned c){if(x>=0&&x<160&&y>=0&&y<144)pixels[y*160+x]=c;};
    auto text=[&](int x,int y,const std::string& s,unsigned color){
        const std::string chars="0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ:-/ ";
        const unsigned glyphs[]={0x7b6f,0x2492,0x73e7,0x73cf,0x5bc9,0x79cf,0x79ef,0x7249,0x7bef,0x7bcf,0x2bed,0x6bae,0x7927,0x6b6e,0x79e7,0x79e4,0x796f,0x5bed,0x7497,0x124e,0x5bad,0x4927,0x5fed,0x5f6d,0x7b6f,0x7be4,0x7b7b,0x7bed,0x79cf,0x7492,0x5b6f,0x5b6a,0x5b7d,0x5aad,0x5a92,0x72a7,0x0410,0x01c0,0x12a4,0};
        for(char c:s){auto i=chars.find(c);unsigned g=i==std::string::npos?0:glyphs[i];for(int yy=0;yy<5;yy++)for(int xx=0;xx<3;xx++)if(g&(1u<<(14-yy*3-xx)))dot(x+xx,y+yy,color);x+=4;}
    };
    auto ram=static_cast<const unsigned char*>(core.rambank0_ptr());
    text(3,2,map.floorLabel()+" X"+std::to_string(map.x)+" Y"+std::to_string(map.y),0xffdd70);
    text(3,9,"XY:ARROWS A:MOVE B:CANCEL",0xffffff);
    for(unsigned y=0;y<16;y++)for(unsigned x=0;x<16;x++){
        unsigned cell=map.cell(map.floor,x,y);int px=24+x*7,py=19+(15-y)*7;
        unsigned bg=cell==0x55?0x303540:0x1d3040;
        for(int dy=0;dy<7;dy++)for(int dx=0;dx<7;dx++)dot(px+dx,py+dy,bg);
        // Raw wall pairs: north 6, west 4, east 2, south 0.
        for(unsigned dir=0;dir<4;dir++){
            unsigned wall=(cell>>(6-2*dir))&3;if(!wall)continue;
            unsigned color=wall==1?0xb4c5d6:wall==2?0xe2af52:0xd57ccf;
            for(int a=0;a<7;a++)if(wall==1||a<2||a>4||wall==3)
                dot(px+(dir==1?0:dir==2?6:a),py+(dir==0?0:dir==3?6:a),color);
        }
        if(ram[map.positionAddress()+(map.isColor()?3:2)]==map.floor&&ram[map.positionAddress()]==x&&ram[map.positionAddress()+1]==y){dot(px+3,py+2,0x60ff90);dot(px+2,py+3,0x60ff90);dot(px+3,py+3,0x60ff90);dot(px+4,py+3,0x60ff90);}
        if(map.x==x&&map.y==y)for(int a=1;a<6;a++){dot(px+a,py+1,0xffdd50);dot(px+a,py+5,0xffdd50);dot(px+1,py+a,0xffdd50);dot(px+5,py+a,0xffdd50);}
    }
    text(3,133,"FLOOR:PGUP/PGDN LB/RB",0xc4d0df);
    text(3,139,"MOVE SAVES SLOT1 FIRST",0xffffff);
}

