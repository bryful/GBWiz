#include "game_support.h"
#include "music_player.h"
#include "audio_resampler.h"
#include <iostream>
#include <sstream>
#include <map>
void cartridge_set_rumble(unsigned){}
struct Stats{unsigned frame=0,pitch=0;std::vector<std::pair<unsigned,unsigned>> notes;};
bool observeNote(void* data,unsigned,unsigned,unsigned addr,unsigned value){auto& s=*static_cast<Stats*>(data);if(addr==0xff13)s.pitch=value;if(addr==0xff14&&(value&128))s.notes.push_back({s.frame,s.pitch|((value&7)<<8)});return true;}
int verifyScenes(const std::vector<char>& rom,const fs::path& trace){
    bool dq=MusicPlayer::driver(rom).dq;
    std::ifstream file(trace);std::string line;unsigned song=0,pitch=0;
    std::map<unsigned,std::vector<std::pair<unsigned,unsigned>>> notes;
    std::map<unsigned,std::vector<std::pair<unsigned,unsigned>>> effects;
    std::map<unsigned,unsigned> starts;
    while(std::getline(file,line)){
        std::replace(line.begin(),line.end(),',',' ');std::istringstream row(line);
        unsigned frame,pc,address,value;if(!(row>>frame>>std::hex>>pc>>address>>value))continue;
        if((!dq&&address==0xfff0&&pc==0x32cc)||(dq&&address==0xccd1&&pc==0x5e1a)){song=value;starts[song]=frame;notes[song].clear();effects[song].clear();}
        if((!dq&&address==0xdb86&&pc==0x3aa0&&value!=255)||(dq&&address==0xccd1&&pc==0x5e60))effects[song].push_back({frame,value});
        if(address==0xff13)pitch=value;
        if(address==0xff14&&(value&128)&&song!=0)notes[song].push_back({frame,pitch|((value&7)<<8)});
    }
    std::array<gambatte::uint_least32_t,40000> sound{};
    for(unsigned scene:(dq?std::vector<unsigned>{26,41,4}:std::vector<unsigned>{12,15,8})){
        MusicPlayer player(rom,scene);
        Stats isolated;player.core().setWriteFilter(observeNote,&isolated);
        for(unsigned f=0;f<600;f++){
            for(auto effect:effects[scene])if(f==effect.first-starts[scene]+(dq?1:4))player.effect(effect.second);
            isolated.frame=f;player.run(sound);
        }
        auto& original=notes[scene];size_t count=std::min<size_t>(25,original.size()),skipped=0;bool matched=false;
        // DQ3 can retrigger the outgoing channel while initializing a new
        // song. Report, rather than silently include, this transition prefix.
        for(size_t prefix=0;prefix<=(dq?4u:0u)&&prefix<original.size()&&!matched;prefix++){
            size_t candidateCount=std::min<size_t>(25,original.size()-prefix);
            for(size_t start=0;candidateCount>=5&&start+candidateCount<=isolated.notes.size();start++){
                int offset=int(original[prefix].first)-int(isolated.notes[start].first);bool same=true;
                for(size_t i=0;i<candidateCount;i++)if(original[prefix+i].second!=isolated.notes[start+i].second||
                    std::abs(int(original[prefix+i].first)-int(isolated.notes[start+i].first)-offset)>2){same=false;break;}
                if(same){matched=true;count=candidateCount;skipped=prefix;break;}
            }
        }
        std::cout<<"song="<<scene<<" notes="<<count<<" transition_prefix="<<skipped<<" pitch/tempo="<<(matched?"PASS":"FAIL")<<"\n";
        if(!matched){
            for(size_t i=0;i<std::min<size_t>(10,original.size());i++)std::cout<<" original "<<original[i].first<<":"<<original[i].second;
            std::cout<<"\n";
            for(size_t i=0;i<std::min<size_t>(12,isolated.notes.size());i++)std::cout<<" isolated "<<isolated.notes[i].first<<":"<<isolated.notes[i].second;
            std::cout<<"\n";return 1;
        }
    }
    return 0;
}
int wmain(int argc,wchar_t** argv){
    if(argc==3)return verifyScenes(read(argv[1]),argv[2]);
    if(argc!=2)return 1;auto rom=read(argv[1]);auto main=loadCoreBytes(rom,nullptr);
    std::array<gambatte::video_pixel_t,160*144> pixels{};
    auto driver=MusicPlayer::driver(rom);unsigned song=driver.dq?11:driver.color?0x8a:1;std::cout<<"Current song="<<song<<"\n";
    MusicPlayer music(rom,song);Stats a,b;main->setWriteFilter(observeNote,&a);music.core().setWriteFilter(observeNote,&b);
    Resampler resampler;std::array<gambatte::uint_least32_t,40000> sound{};unsigned total=0;int peak=0;
    for(unsigned f=0;f<1200;f++){
        a.frame=b.frame=f;unsigned samples=35112;main->runFor(pixels.data(),160,sound.data(),sound.size(),samples);
        samples=music.run(sound);auto pcm=resampler.convert(sound.data(),samples);total+=pcm.size();for(auto value:pcm)peak=std::max(peak,std::abs(int(value)));
    }
    for(auto* s:{&a,&b}){std::cout<<"note frames:";for(auto n:s->notes)std::cout<<" "<<n.first<<":"<<n.second;std::cout<<"\n";}
    std::cout<<"samples="<<total<<" peak="<<peak<<"\n";
}
