package jp.gbwiz;
final class NativeCore {
    static { System.loadLibrary("gbwiz"); }
    static long open(byte[] rom,byte[] ram){return openConfigured(rom,ram,true);}
    static native long openConfigured(byte[] rom,byte[] ram,boolean skipLogo);
    static native void setSkipLogo(long core,boolean enabled);
    static native boolean setSpeed(long core,int halfSpeed);
    static native int speed(long core);
    static native boolean rewardsSupported(long core);
    static native boolean goldRewardsSupported(long core);
    static native boolean setRewards(long core,int experience,int gold);
    static native boolean maxBonusSupported(long core);
    static native boolean setMaxBonus(long core,boolean enabled);
    static native boolean learnsMaximumMp(long core);
    static native boolean spellsSupported(long core);
    static native boolean canLearnSpells(long core);
    static native String[] spellNames(long core);
    static native boolean restoreMp(long core,int slot);
    static native boolean learnSpells(long core,int slot);
    static native boolean setAlignment(long core,int slot,int alignment);
    static native int alignment(long core,int slot);
    static native boolean mpSupported(long core);
    static native boolean setNoMpCost(long core,boolean enabled);
    static native boolean noMpCost(long core);
    static native boolean partySupported(long core);
    static native boolean setMixedParty(long core,boolean enabled);
    static native boolean mixedParty(long core);
    static native int step(long core, int buttons, int[] pixels, short[] audio);
    static native byte[] battery(long core);
    static native byte[] state(long core);
    static native boolean loadState(long core, byte[] state);
    static native void reset(long core);
    static native int skippedFrames(long core);
    static native boolean openMap(long core);
    static native boolean mapIsOpen(long core);
    static native boolean mapKey(long core,int bits);
    static native void closeMap(long core);
    static native void mapFrame(long core,int[] pixels);
    static native boolean moveMapAfterBackup(long core);
    static native void close(long core);
}
