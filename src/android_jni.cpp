#include <jni.h>
#include "game.h"
#include "audio_resampler.h"
#include "game_support.h"
#include "turbo_runner.h"
#include "dungeon_map.h"
#include <array>
void cartridge_set_rumble(unsigned) {}
namespace {
struct Input : gambatte::InputGetter { unsigned buttons=0; unsigned operator()() override { return buttons; } };
struct Session {
    Input input;
    std::unique_ptr<gambatte::GB> core;
    Resampler audio;
    std::array<gambatte::video_pixel_t,160*144> pixels{};
    std::array<gambatte::uint_least32_t,40000> sound{};
    GameProfile profile{GameId::Unknown,"Unknown"};
    unsigned skippedFrames=0;
    std::vector<char> bootRom;
    bool skipLogo=true;
    TurboRunner turbo;
    DungeonMap dungeonMap;
};
Session& get(jlong handle) { return *reinterpret_cast<Session*>(handle); }
std::vector<char> bytes(JNIEnv* env,jbyteArray data) {
    if(!data) return {};
    std::vector<char> result(env->GetArrayLength(data));
    env->GetByteArrayRegion(data,0,result.size(),reinterpret_cast<jbyte*>(result.data()));
    return result;
}
jbyteArray array(JNIEnv* env,const void* data,size_t size) {
    auto result=env->NewByteArray(static_cast<jsize>(size));
    if(result && size) env->SetByteArrayRegion(result,0,size,static_cast<const jbyte*>(data));
    return result;
}
void error(JNIEnv* env,const char* message) { env->ThrowNew(env->FindClass("java/lang/IllegalStateException"),message); }
}
extern "C" JNIEXPORT jlong JNICALL Java_jp_gbwiz_NativeCore_openConfigured(JNIEnv* env,jclass,jbyteArray rom,jbyteArray ram,jboolean skipLogo) {
    try {
        auto session=std::make_unique<Session>();
        session->skipLogo=skipLogo;
        auto romData=bytes(env,rom);session->profile=identifyGame(romData);
        session->core=loadCoreBytes(romData,&session->input);
        if(ram) {
            auto save=bytes(env,ram);
            if(save.size()!=session->core->savedata_size()) throw std::runtime_error("Save size mismatch");
            if(!save.empty()) std::memcpy(session->core->savedata_ptr(),save.data(),save.size());
        }
        if(session->profile.id==GameId::Gaiden1||session->profile.id==GameId::Gaiden2||session->profile.id==GameId::Gaiden3||MusicPlayer::supported(romData))session->bootRom=std::move(romData);
        if(session->skipLogo)session->skippedFrames=skipBootLogos(session->core,session->profile,session->bootRom,&session->input,session->pixels).frames;
        session->turbo.attach(*session->core,session->bootRom,session->profile);
        session->dungeonMap.attach(session->bootRom);
        return reinterpret_cast<jlong>(session.release());
    } catch(const std::exception& e) { error(env,e.what()); return 0; }
}
extern "C" JNIEXPORT jint JNICALL Java_jp_gbwiz_NativeCore_step(JNIEnv* env,jclass,jlong h,jint buttons,jintArray video,jshortArray audio) {
    try {
        auto& s=get(h);
        unsigned samples=s.turbo.run(buttons,s.input.buttons,s.pixels,s.sound);
        std::array<jint,160*144> pixels;
        for(size_t i=0;i<pixels.size();++i) pixels[i]=0xff000000u|s.pixels[i];
        auto pcm=s.audio.convert(s.sound.data(),samples);
        if(env->GetArrayLength(video)<static_cast<jsize>(pixels.size()) || env->GetArrayLength(audio)<static_cast<jsize>(pcm.size()))
            throw std::runtime_error("Output buffer too small");
        env->SetIntArrayRegion(video,0,pixels.size(),pixels.data());
        env->SetShortArrayRegion(audio,0,pcm.size(),pcm.data());
        return pcm.size();
    } catch(const std::exception& e) { error(env,e.what()); return 0; }
}
extern "C" JNIEXPORT jbyteArray JNICALL Java_jp_gbwiz_NativeCore_battery(JNIEnv* env,jclass,jlong h) {
    auto& s=get(h); return array(env,s.core->savedata_ptr(),s.core->savedata_size());
}
extern "C" JNIEXPORT jbyteArray JNICALL Java_jp_gbwiz_NativeCore_state(JNIEnv* env,jclass,jlong h) {
    try{auto data=get(h).turbo.saveState();return array(env,data.data(),data.size());}catch(const std::exception& e){error(env,e.what());return nullptr;}
}
extern "C" JNIEXPORT jboolean JNICALL Java_jp_gbwiz_NativeCore_loadState(JNIEnv* env,jclass,jlong h,jbyteArray state) {
    try{auto& s=get(h);if(!s.turbo.loadState(bytes(env,state)))return false;s.audio.reset();return true;}
    catch(const std::exception& e){error(env,e.what());return false;}
}
extern "C" JNIEXPORT jint JNICALL Java_jp_gbwiz_NativeCore_speed(JNIEnv*,jclass,jlong h){return get(h).turbo.speed();}
extern "C" JNIEXPORT jboolean JNICALL Java_jp_gbwiz_NativeCore_rewardsSupported(JNIEnv*,jclass,jlong h){return get(h).turbo.rewards().supported();}
extern "C" JNIEXPORT jboolean JNICALL Java_jp_gbwiz_NativeCore_goldRewardsSupported(JNIEnv*,jclass,jlong h){return get(h).turbo.rewards().goldSupported();}
extern "C" JNIEXPORT jboolean JNICALL Java_jp_gbwiz_NativeCore_setRewards(JNIEnv*,jclass,jlong h,jint experience,jint gold){return get(h).turbo.rewards().set(experience,gold);}
extern "C" JNIEXPORT jboolean JNICALL Java_jp_gbwiz_NativeCore_maxBonusSupported(JNIEnv*,jclass,jlong h){return get(h).turbo.creation().supported();}
extern "C" JNIEXPORT jboolean JNICALL Java_jp_gbwiz_NativeCore_setMaxBonus(JNIEnv*,jclass,jlong h,jboolean enabled){return get(h).turbo.creation().enable(enabled);}
extern "C" JNIEXPORT jboolean JNICALL Java_jp_gbwiz_NativeCore_mpSupported(JNIEnv*,jclass,jlong h){return get(h).turbo.mp().supported();}
extern "C" JNIEXPORT jboolean JNICALL Java_jp_gbwiz_NativeCore_setNoMpCost(JNIEnv*,jclass,jlong h,jboolean enabled){return get(h).turbo.mp().enable(enabled);}
extern "C" JNIEXPORT jboolean JNICALL Java_jp_gbwiz_NativeCore_noMpCost(JNIEnv*,jclass,jlong h){return get(h).turbo.mp().enabled();}
extern "C" JNIEXPORT jboolean JNICALL Java_jp_gbwiz_NativeCore_partySupported(JNIEnv*,jclass,jlong h){return get(h).turbo.party().supported();}
extern "C" JNIEXPORT jboolean JNICALL Java_jp_gbwiz_NativeCore_setMixedParty(JNIEnv*,jclass,jlong h,jboolean enabled){return get(h).turbo.party().enable(enabled);}
extern "C" JNIEXPORT jboolean JNICALL Java_jp_gbwiz_NativeCore_mixedParty(JNIEnv*,jclass,jlong h){return get(h).turbo.party().enabled();}
extern "C" JNIEXPORT jboolean JNICALL Java_jp_gbwiz_NativeCore_spellsSupported(JNIEnv*,jclass,jlong h){return get(h).turbo.spells().supported();}
extern "C" JNIEXPORT jboolean JNICALL Java_jp_gbwiz_NativeCore_canLearnSpells(JNIEnv*,jclass,jlong h){auto& s=get(h);return s.turbo.spells().available(*s.core);}
extern "C" JNIEXPORT jobjectArray JNICALL Java_jp_gbwiz_NativeCore_spellNames(JNIEnv* env,jclass,jlong h){
    try{auto& s=get(h);auto names=env->NewObjectArray(20,env->FindClass("java/lang/String"),nullptr);
        for(const auto& character:s.turbo.spells().characters(*s.core)){auto name=env->NewStringUTF(character.name.c_str());env->SetObjectArrayElement(names,character.slot,name);env->DeleteLocalRef(name);}return names;
    }catch(const std::exception& e){error(env,e.what());return nullptr;}
}
extern "C" JNIEXPORT jboolean JNICALL Java_jp_gbwiz_NativeCore_learnSpells(JNIEnv*,jclass,jlong h,jint slot){auto& s=get(h);return s.turbo.spells().learn(*s.core,slot);}
extern "C" JNIEXPORT jboolean JNICALL Java_jp_gbwiz_NativeCore_restoreMp(JNIEnv*,jclass,jlong h,jint slot){auto& s=get(h);return s.turbo.spells().restoreMp(*s.core,slot);}
extern "C" JNIEXPORT jboolean JNICALL Java_jp_gbwiz_NativeCore_setAlignment(JNIEnv*,jclass,jlong h,jint slot,jint alignment){auto& s=get(h);return s.turbo.spells().setAlignment(*s.core,slot,alignment);}
extern "C" JNIEXPORT jint JNICALL Java_jp_gbwiz_NativeCore_alignment(JNIEnv*,jclass,jlong h,jint slot){auto& s=get(h);return s.turbo.spells().alignment(*s.core,slot);}
extern "C" JNIEXPORT jint JNICALL Java_jp_gbwiz_NativeCore_skippedFrames(JNIEnv*,jclass,jlong h){return get(h).skippedFrames;}
extern "C" JNIEXPORT void JNICALL Java_jp_gbwiz_NativeCore_setSkipLogo(JNIEnv*,jclass,jlong h,jboolean enabled){get(h).skipLogo=enabled;}
extern "C" JNIEXPORT jboolean JNICALL Java_jp_gbwiz_NativeCore_setSpeed(JNIEnv* env,jclass,jlong h,jint halfSpeed){try{return get(h).turbo.speed(halfSpeed);}catch(const std::exception& e){error(env,e.what());return false;}}
extern "C" JNIEXPORT void JNICALL Java_jp_gbwiz_NativeCore_reset(JNIEnv* env,jclass,jlong h) {
    try{auto& s=get(h);int speed=s.turbo.speed();unsigned exp=s.turbo.rewards().experience(),gold=s.turbo.rewards().gold();bool bonus=s.turbo.creation().enabled(),noMpCost=s.turbo.mp().enabled(),mixedParty=s.turbo.party().enabled();s.turbo.detach();s.input.buttons=0;s.core->reset();s.pixels.fill(0);
        s.skippedFrames=s.skipLogo?skipBootLogos(s.core,s.profile,s.bootRom,&s.input,s.pixels).frames:0;
        s.turbo.attach(*s.core,s.bootRom,s.profile);s.turbo.speed(speed);s.turbo.rewards().set(exp,gold);s.turbo.creation().enable(bonus);s.turbo.mp().enable(noMpCost);s.turbo.party().enable(mixedParty);s.audio.reset();
    }catch(const std::exception& e){error(env,e.what());}
}
extern "C" JNIEXPORT void JNICALL Java_jp_gbwiz_NativeCore_close(JNIEnv*,jclass,jlong h) { delete reinterpret_cast<Session*>(h); }

extern "C" JNIEXPORT jboolean JNICALL Java_jp_gbwiz_NativeCore_openMap(JNIEnv*,jclass,jlong h){auto& s=get(h);return s.dungeonMap.show(*s.core);}
extern "C" JNIEXPORT jboolean JNICALL Java_jp_gbwiz_NativeCore_mapKey(JNIEnv*,jclass,jlong h,jint bits){return get(h).dungeonMap.key(bits);}
extern "C" JNIEXPORT jboolean JNICALL Java_jp_gbwiz_NativeCore_mapIsOpen(JNIEnv*,jclass,jlong h){return get(h).dungeonMap.open;}
extern "C" JNIEXPORT void JNICALL Java_jp_gbwiz_NativeCore_closeMap(JNIEnv*,jclass,jlong h){get(h).dungeonMap.open=false;}
extern "C" JNIEXPORT void JNICALL Java_jp_gbwiz_NativeCore_mapFrame(JNIEnv* env,jclass,jlong h,jintArray video){
    auto& s=get(h);auto frame=s.pixels;if(s.dungeonMap.open)drawDungeonMap(s.dungeonMap,*s.core,frame);
    std::array<jint,160*144> output{};for(size_t i=0;i<output.size();i++)output[i]=0xff000000u|frame[i];
    if(env->GetArrayLength(video)>=static_cast<jsize>(output.size()))env->SetIntArrayRegion(video,0,output.size(),output.data());
}
// Java calls this only after the slot1 write has completed and been read back.
extern "C" JNIEXPORT jboolean JNICALL Java_jp_gbwiz_NativeCore_moveMapAfterBackup(JNIEnv* env,jclass,jlong h){
    try{auto& s=get(h);bool ok=s.dungeonMap.move(*s.core,[]{return true;});if(ok){s.turbo.resetInput();s.audio.reset();}return ok;}
    catch(const std::exception& e){error(env,e.what());return false;}
}

extern "C" JNIEXPORT jboolean JNICALL Java_jp_gbwiz_NativeCore_learnsMaximumMp(JNIEnv*,jclass,jlong h){return get(h).turbo.spells().learnsMaximumMp();}
