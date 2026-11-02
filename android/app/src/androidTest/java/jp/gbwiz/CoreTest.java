package jp.gbwiz;
import android.test.InstrumentationTestCase;
import java.io.*;
import java.util.Arrays;

// Fixture ROMs are pushed into the test device's cache by the developer,
// never packaged in the application or test APK.
public final class CoreTest extends InstrumentationTestCase {
    private byte[] fixture(String name) throws IOException {
        File path=new File(getInstrumentation().getTargetContext().getCacheDir(),name);
        try(InputStream input=new FileInputStream(path); ByteArrayOutputStream output=new ByteArrayOutputStream()) {
            byte[] buffer=new byte[16384]; int count;
            while((count=input.read(buffer))!=-1) output.write(buffer,0,count);
            return output.toByteArray();
        }
    }
    public void testGbAndGbcCore() throws Exception {
        int[] pixels=new int[160*144]; short[] audio=new short[4096];
        for(String rom:new String[]{"test.gb","test.gbc"}) {
            byte[] bytes=fixture(rom); long handle=NativeCore.open(bytes,null);
            try {
                assertTrue("Boot skip profile mismatch",NativeCore.skippedFrames(handle)>0);
                int samples=0;
                for(int frame=0;frame<180;frame++) samples+=NativeCore.step(handle,0,pixels,audio);
                assertTrue("Audio samples missing",samples>150000);
                boolean varied=false;
                for(int pixel:pixels) {assertEquals(0xff000000,pixel&0xff000000); if(pixel!=pixels[0]) varied=true;}
                assertTrue("Video frame is flat",varied);
                byte[] state=NativeCore.state(handle),ram=NativeCore.battery(handle);
                for(int i=0;i<8;i++)assertTrue("Repeated snapshot is not deterministic",Arrays.equals(state,NativeCore.state(handle)));
                assertTrue(NativeCore.loadState(handle,state));
                assertFalse(NativeCore.loadState(handle,new byte[]{1,2,3}));
                assertTrue("Failed state damaged core",NativeCore.loadState(handle,state));
                NativeCore.reset(handle);
                assertTrue("Reset boot skip profile mismatch",NativeCore.skippedFrames(handle)>0);
                assertTrue("Reset lost battery",Arrays.equals(ram,NativeCore.battery(handle)));
                long reopened=NativeCore.open(bytes,ram);
                try {assertTrue(Arrays.equals(ram,NativeCore.battery(reopened)));} finally {NativeCore.close(reopened);}
            } finally {NativeCore.close(handle);}
        }
    }
    public void testTurboAndLogoSwitch()throws Exception{
        byte[] rom=fixture("test.gb");long handle=NativeCore.openConfigured(rom,null,false);
        try{
            assertEquals(0,NativeCore.skippedFrames(handle));NativeCore.setSkipLogo(handle,true);NativeCore.reset(handle);
            assertTrue(NativeCore.skippedFrames(handle)>0);
            int[] pixels=new int[160*144];short[] audio=new short[4096];
            assertTrue(NativeCore.setSpeed(handle,4));int samples=0;
            for(int f=0;f<120;f++)samples+=NativeCore.step(handle,0,pixels,audio);
            assertTrue("2x audio clock was accelerated",samples>130000&&samples<133000);
            byte[] state=NativeCore.state(handle);assertTrue(NativeCore.loadState(handle,state));
            assertTrue(NativeCore.step(handle,0,pixels,audio)>0);
            NativeCore.setSkipLogo(handle,false);NativeCore.reset(handle);assertEquals(0,NativeCore.skippedFrames(handle));
        }finally{NativeCore.close(handle);}
        byte[] unknown=fixture("test.gbc");unknown[0x134]='X';
        handle=NativeCore.openConfigured(unknown,null,false);
        try{assertFalse("Unsupported game accepted turbo",NativeCore.setSpeed(handle,4));}finally{NativeCore.close(handle);}
    }
    public void testTurboShortMenuTaps()throws Exception{
        byte[] rom=fixture("test.gb");long normal=NativeCore.open(rom,null),fast=NativeCore.open(rom,null);
        try{
            assertTrue(NativeCore.setSpeed(fast,4));
            int[] normalPixels=new int[160*144],fastPixels=new int[160*144];short[] audio=new short[4096];
            for(int press=0;press<3;press++)for(int frame=0;frame<128;frame++){
                int keys=frame<8?1:0;
                NativeCore.step(normal,keys,normalPixels,audio);NativeCore.step(fast,keys,fastPixels,audio);
            }
            assertTrue("2x short taps missed tavern navigation",Arrays.equals(normalPixels,fastPixels));
        }finally{NativeCore.close(normal);NativeCore.close(fast);}
    }
    public void testGaiden2Turbo()throws Exception{
        checkTurbo("gaiden2.gb");
    }
    public void testGaiden3Turbo()throws Exception{
        checkTurbo("gaiden3.gb");
    }
    public void testWizardry1Turbo()throws Exception{
        checkTurbo("test.gbc");
    }
    public void testWizardry2Turbo()throws Exception{
        checkTurbo("wizardry2.gbc");
    }
    public void testWizardry3Turbo()throws Exception{
        checkTurbo("wizardry3.gbc");
    }
    public void testDq3Turbo()throws Exception{
        checkTurbo("dq3.gbc");
    }
    private long rewardNumber(byte[] data,int at){
        long value=0;for(int i=0;i<4;i++)value|=(long)(data[at+i]&255)<<(i*8);return value;
    }
    public void testGaiden2LogoSkip()throws Exception{gaidenLogoSkip(2);}
    public void testGaiden3LogoSkip()throws Exception{gaidenLogoSkip(3);}
    private void gaidenLogoSkip(int number)throws Exception{
        byte[] rom=fixture("gaiden"+number+"-alignment.gb"),checkpoint=fixture("gaiden"+number+"-alignment.bin");
        long seed=NativeCore.openConfigured(rom,null,false);byte[] saved;
        try{assertTrue(NativeCore.loadState(seed,checkpoint));saved=NativeCore.battery(seed);}finally{NativeCore.close(seed);}
        int[] video=new int[160*144];short[] audio=new short[4096];
        for(boolean validSave:new boolean[]{false,true}){
            byte[] battery=validSave?saved:null;
            long normal=NativeCore.openConfigured(rom,battery,false),fast=NativeCore.openConfigured(rom,battery,true);
            try{
                int skipped=NativeCore.skippedFrames(fast);assertTrue(skipped>0&&skipped<=1200);
                for(int frame=0;frame<skipped;frame++)NativeCore.step(normal,0,video,audio);
                assertTrue("Skip changed ordinary boot state",Arrays.equals(NativeCore.state(normal),NativeCore.state(fast)));
                byte[] expected=NativeCore.battery(normal);
                if(validSave){byte[] checked=saved.clone();if(number==3)Arrays.fill(checked,0x6078,0x607e,(byte)255);assertTrue("Valid save changed",Arrays.equals(checked,expected));}
                NativeCore.reset(fast);assertTrue(NativeCore.skippedFrames(fast)>0);
                assertTrue(NativeCore.loadState(fast,checkpoint));assertTrue(Arrays.equals(saved,NativeCore.battery(fast)));
                NativeCore.setSkipLogo(fast,false);NativeCore.reset(fast);assertEquals(0,NativeCore.skippedFrames(fast));
                NativeCore.setSkipLogo(fast,true);NativeCore.reset(fast);assertTrue(NativeCore.skippedFrames(fast)>0);
                byte[] rebooted=saved.clone();if(number==3)Arrays.fill(rebooted,0x6078,0x607e,(byte)255);
                assertTrue(Arrays.equals(rebooted,NativeCore.battery(fast)));
            }finally{NativeCore.close(normal);NativeCore.close(fast);}
        }
        byte[] unknown=rom.clone();unknown[0x100]=0x18;unknown[0x101]=(byte)0xfe;
        long normal=NativeCore.openConfigured(unknown,saved,false),fast=NativeCore.openConfigured(unknown,saved,true);
        try{assertEquals(0,NativeCore.skippedFrames(fast));assertTrue(Arrays.equals(NativeCore.state(normal),NativeCore.state(fast)));}
        finally{NativeCore.close(normal);NativeCore.close(fast);}
    }
    public void testWizardry123LogoSkip()throws Exception{
        int[] video=new int[160*144];short[] audio=new short[4096];
        for(int game:new int[]{1,2,3}){
            byte[] rom=fixture("wizardry"+game+".gbc");long normal=NativeCore.openConfigured(rom,null,false),fast=NativeCore.openConfigured(rom,null,true);
            try{
                int skipped=NativeCore.skippedFrames(fast);assertTrue(skipped>0&&skipped<=1200);
                for(int frame=0;frame<skipped;frame++)NativeCore.step(normal,0,video,audio);
                assertTrue("Skip differs from ordinary boot",Arrays.equals(NativeCore.state(normal),NativeCore.state(fast)));
                NativeCore.reset(fast);assertTrue(NativeCore.skippedFrames(fast)>0);
                NativeCore.setSkipLogo(fast,false);NativeCore.reset(fast);assertEquals(0,NativeCore.skippedFrames(fast));
            }finally{NativeCore.close(normal);NativeCore.close(fast);}
            byte[] unknown=rom.clone();unknown[0x100]=0x18;unknown[0x101]=(byte)0xfe;
            long handle=NativeCore.openConfigured(unknown,null,true);try{assertEquals("Unknown title must fall back",0,NativeCore.skippedFrames(handle));}finally{NativeCore.close(handle);}
        }
    }
    private byte[] stateRam(long handle){
        byte[] state=NativeCore.state(handle);int start=state.length>=32&&state[0]=='G'?32:0;
        int pos=start+5+((state[start+2]&255)<<16)+((state[start+3]&255)<<8)+(state[start+4]&255);
        while(pos<state.length){
            int label=pos;while(state[pos]!=0)pos++;
            String name=new String(state,label,pos-label,java.nio.charset.StandardCharsets.US_ASCII);pos++;
            int length=((state[pos]&255)<<16)|((state[pos+1]&255)<<8)|(state[pos+2]&255);pos+=3;
            if(name.equals("wram"))return Arrays.copyOfRange(state,pos,pos+length);pos+=length;
        }
        throw new AssertionError("State has no WRAM");
    }
    private int fieldOffset(byte[] state,String wanted){
        int pos=5+((state[2]&255)<<16)+((state[3]&255)<<8)+(state[4]&255);
        while(pos<state.length){
            int label=pos;while(state[pos]!=0)pos++;
            String name=new String(state,label,pos-label,java.nio.charset.StandardCharsets.US_ASCII);pos++;
            int length=((state[pos]&255)<<16)|((state[pos+1]&255)<<8)|(state[pos+2]&255);pos+=3;
            if(name.equals(wanted))return pos;pos+=length;
        }
        throw new AssertionError("State field missing: "+wanted);
    }
    public void testGaiden1GlobalMp()throws Exception{
        byte[] rom=fixture("test.gb"),scenario=fixture("gaiden1-town.bin").clone();int wram=fieldOffset(scenario,"wram"),pc=fieldOffset(scenario,"pc");
        scenario[pc]=(byte)0xc0;scenario[pc+1]=0;
        ByteArrayOutputStream code=new ByteArrayOutputStream();code.write(new byte[]{(byte)0xf3,0x31,(byte)0xf0,(byte)0xcf,0x3e,3,(byte)0xea,0,0x20,(byte)0xea,(byte)0xae,(byte)0xd0});
        for(int slot=0;slot<6;slot++)for(int level=0;level<14;level++){
            scenario[wram+0x1186+slot*128+0x41+level]=5;
            code.write(new byte[]{0x3e,(byte)level,(byte)0xea,(byte)0xfa,(byte)0xd0,0x3e,(byte)slot,(byte)0xcd,0x7a,0x6e});
        }
        // The normal save transaction finalizes record checksums and its mirror
        // after the character save routine. Execute those original routines too.
        code.write(new byte[]{(byte)0xcd,0x56,0x0c,0x3e,(byte)0xa5,(byte)0xea,(byte)0xf2,(byte)0xc3,0x18,(byte)0xfe});assertTrue(code.size()<0x3f0);
        System.arraycopy(code.toByteArray(),0,scenario,wram,code.size());scenario[wram+0x3f2]=0;
        int[] video=new int[160*144];short[] audio=new short[4096];
        for(int speed:new int[]{2,3,4,6,8}){
            long handle=NativeCore.open(rom,null);
            try{
                assertTrue(NativeCore.mpSupported(handle));assertFalse(NativeCore.noMpCost(handle));
                for(boolean enabled:new boolean[]{false,true,false}){
                    assertTrue(NativeCore.setNoMpCost(handle,enabled));assertTrue(NativeCore.loadState(handle,scenario));assertEquals(enabled,NativeCore.noMpCost(handle));assertTrue(NativeCore.setSpeed(handle,speed));
                    byte[] before=stateRam(handle);
                    for(int frame=0;frame<60&&(stateRam(handle)[0x3f2]&255)!=0xa5;frame++)NativeCore.step(handle,0,video,audio);
                    byte[] after=stateRam(handle),battery=NativeCore.battery(handle);assertEquals(0xa5,after[0x3f2]&255);
                    for(int slot=0;slot<6;slot++)for(int i=0;i<128;i++){
                        int at=0x1186+slot*128+i;
                        if(i>=0x41&&i<0x4f){assertEquals(enabled?5:4,after[at]&255);assertEquals(enabled?5:4,battery[0x17+slot*128+i]&255);}
                        else assertEquals("Unrelated character field changed",before[at],after[at]);
                    }
                    long fresh=NativeCore.open(rom,battery);
                    try{for(int f=0;f<128;f++)NativeCore.step(fresh,f<8?1:0,video,audio);byte[] restored=stateRam(fresh);
                        for(int slot=0;slot<6;slot++)for(int i=0;i<14;i++)assertEquals(enabled?5:4,restored[0x1186+slot*128+0x41+i]&255);
                    }finally{NativeCore.close(fresh);}
                    NativeCore.reset(handle);assertEquals("Reset lost global setting",enabled,NativeCore.noMpCost(handle));
                }
            }finally{NativeCore.close(handle);}
        }
        byte[] unknown=rom.clone();unknown[0xee7a]^=1;long handle=NativeCore.open(unknown,null);
        try{assertFalse(NativeCore.mpSupported(handle));assertFalse(NativeCore.setNoMpCost(handle,true));assertTrue(NativeCore.setNoMpCost(handle,false));}finally{NativeCore.close(handle);}
    }
    public void testGaiden1MixedParty()throws Exception{
        byte[] rom=fixture("test.gb"),town=fixture("gaiden1-town.bin");int[] video=new int[160*144];short[] audio=new short[4096];
        for(int speed:new int[]{2,3,4,6,8}){
            long handle=NativeCore.open(rom,null);
            try{
                assertTrue(NativeCore.partySupported(handle));assertFalse(NativeCore.mixedParty(handle));assertTrue(NativeCore.loadState(handle,town));
                assertTrue(NativeCore.setAlignment(handle,0,0));assertTrue(NativeCore.setAlignment(handle,1,2));byte[] roster=stateRam(handle);
                assertTrue(NativeCore.setSpeed(handle,speed));for(int f=0;f<60;f++)NativeCore.step(handle,0,video,audio);
                creationPress(handle,1,120);creationPress(handle,1,120);creationPress(handle,1,120);
                assertEquals(1,stateRam(handle)[0x94f]&255);creationPress(handle,1,120);assertEquals("OFF accepted evil member",1,stateRam(handle)[0x94f]&255);
                byte[] before=NativeCore.state(handle);assertTrue(NativeCore.setMixedParty(handle,true));assertTrue("Toggle changed game state",Arrays.equals(before,NativeCore.state(handle)));
                creationPress(handle,1,120);creationPress(handle,1,120);byte[] formed=stateRam(handle);
                assertEquals(2,formed[0x94f]&255);assertEquals(0,formed[0x950]&255);assertEquals(0x81,formed[0x951]&255);
                byte[] mixed=NativeCore.state(handle);assertTrue(NativeCore.setMixedParty(handle,false));assertEquals(2,stateRam(handle)[0x94f]&255);
                creationPress(handle,1,120);assertEquals("OFF accepted incompatible addition",2,stateRam(handle)[0x94f]&255);
                assertTrue(NativeCore.loadState(handle,mixed));assertFalse("State load replaced OFF",NativeCore.mixedParty(handle));
                assertTrue(NativeCore.setMixedParty(handle,true));assertTrue(NativeCore.loadState(handle,mixed));assertTrue(NativeCore.mixedParty(handle));
                creationPress(handle,2,120);creationPress(handle,2,120);byte[] after=stateRam(handle);
                assertTrue("Party setting changed character stats",Arrays.equals(Arrays.copyOfRange(roster,0x1186,0x1b86),Arrays.copyOfRange(after,0x1186,0x1b86)));
                long fresh=NativeCore.open(rom,NativeCore.battery(handle));
                try{for(int f=0;f<128;f++)NativeCore.step(fresh,f<8?1:0,video,audio);byte[] restored=stateRam(fresh);
                    // Original town saves restore characters with an empty party.
                    assertEquals(0,restored[0x94f]&255);
                    assertEquals(0,restored[0x1186+10]&255);assertEquals(2,restored[0x1186+128+10]&255);
                }finally{NativeCore.close(fresh);}
                NativeCore.reset(handle);assertTrue("Reset lost mixed-party setting",NativeCore.mixedParty(handle));
                assertTrue(NativeCore.setMixedParty(handle,false));NativeCore.reset(handle);assertFalse(NativeCore.mixedParty(handle));
            }finally{NativeCore.close(handle);}
        }
        byte[] unknown=rom.clone();unknown[0x12e7]^=1;long handle=NativeCore.open(unknown,null);
        try{assertFalse(NativeCore.partySupported(handle));assertFalse(NativeCore.setMixedParty(handle,true));assertTrue(NativeCore.setMixedParty(handle,false));}finally{NativeCore.close(handle);}
    }
    public void testGaiden23MixedPartyChecks()throws Exception{
        int[] video=new int[160*144];short[] audio=new short[4096];
        for(int game:new int[]{2,3}){
            byte[] rom=fixture("gaiden"+game+".gb");int check=game==2?0x1144:0x0ff0,last=game==2?0xd2e:0x9b4;
            long handle=NativeCore.openConfigured(rom,null,false);
            try{
                assertTrue(NativeCore.partySupported(handle));assertFalse(NativeCore.mixedParty(handle));byte[] base=NativeCore.state(handle);
                for(int slot=0;slot<6;slot++)for(int existing=0;existing<4;existing++)for(int candidate=0;candidate<3;candidate++){
                    byte[] scenario=base.clone();int wram=fieldOffset(scenario,"wram"),pc=fieldOffset(scenario,"pc");scenario[pc]=(byte)0xc0;scenario[pc+1]=0;
                    Arrays.fill(scenario,wram+last-5,wram+last+1,(byte)0xff);if(existing<3)scenario[wram+last-5+slot]=(byte)((existing<<6)|slot);
                    byte[] code={(byte)0xf3,0x31,(byte)0xf0,(byte)0xcf,0x3e,(byte)candidate,(byte)0xcd,(byte)check,(byte)(check>>8),0x3e,0,(byte)0xce,0,(byte)0xea,(byte)0x80,(byte)0xc3,0x3e,(byte)0xa5,(byte)0xea,(byte)0x81,(byte)0xc3,0x18,(byte)0xfe};
                    System.arraycopy(code,0,scenario,wram,code.length);scenario[wram+0x381]=0;
                    for(boolean enabled:new boolean[]{false,true,false}){
                        assertTrue(NativeCore.loadState(handle,scenario));byte[] before=NativeCore.state(handle);assertTrue(NativeCore.setMixedParty(handle,enabled));assertTrue(Arrays.equals(before,NativeCore.state(handle)));
                        NativeCore.step(handle,0,video,audio);byte[] after=stateRam(handle);assertEquals(0xa5,after[0x381]&255);
                        boolean conflict=existing<3&&existing!=1&&candidate!=1&&existing!=candidate;
                        assertEquals(!enabled&&conflict?1:0,after[0x380]&255);
                    }
                }
                assertTrue(NativeCore.setMixedParty(handle,true));NativeCore.reset(handle);assertTrue(NativeCore.mixedParty(handle));
            }finally{NativeCore.close(handle);}
            byte[] unknown=rom.clone();unknown[check]^=1;handle=NativeCore.openConfigured(unknown,null,false);
            try{assertFalse(NativeCore.partySupported(handle));assertFalse(NativeCore.setMixedParty(handle,true));}finally{NativeCore.close(handle);}
        }
    }
    public void testGaiden2GlobalMp()throws Exception{
        byte[] rom=fixture("gaiden2.gb");long seed=NativeCore.openConfigured(rom,null,false);byte[] scenario;
        try{scenario=NativeCore.state(seed);}finally{NativeCore.close(seed);}
        int wram=fieldOffset(scenario,"wram"),pc=fieldOffset(scenario,"pc");scenario[pc]=(byte)0xc0;scenario[pc+1]=0;
        ByteArrayOutputStream code=new ByteArrayOutputStream();code.write(new byte[]{(byte)0xf3,0x31,(byte)0xf0,(byte)0xcf,0x3e,3,(byte)0xea,0,0x20,(byte)0xea,(byte)0xbd,(byte)0xcf});
        for(int slot=0;slot<6;slot++){
            for(int i=0;i<128;i++)scenario[wram+0x1095+slot*128+i]=(byte)((slot*17+i)%251+1);
            for(int level=0;level<14;level++){
                scenario[wram+0x1095+slot*128+0x41+level]=5;
                code.write(new byte[]{0x3e,(byte)level,(byte)0xea,9,(byte)0xd0,0x3e,(byte)slot,(byte)0xcd,(byte)0x9d,0x70});
            }
        }
        code.write(new byte[]{0x3e,(byte)0xa5,(byte)0xea,(byte)0xf2,(byte)0xc3,0x18,(byte)0xfe});assertTrue(code.size()<0x3f0);
        System.arraycopy(code.toByteArray(),0,scenario,wram,code.size());scenario[wram+0x3f2]=0;
        int[] video=new int[160*144];short[] audio=new short[4096];
        for(int speed:new int[]{2,3,4,6,8}){
            long handle=NativeCore.openConfigured(rom,null,false);
            try{
                assertTrue(NativeCore.mpSupported(handle));assertFalse(NativeCore.noMpCost(handle));
                for(boolean enabled:new boolean[]{false,true,false}){
                    assertTrue(NativeCore.setNoMpCost(handle,enabled));assertTrue(NativeCore.loadState(handle,scenario));assertEquals(enabled,NativeCore.noMpCost(handle));assertTrue(NativeCore.setSpeed(handle,speed));
                    byte[] before=stateRam(handle);
                    for(int f=0;f<60&&(stateRam(handle)[0x3f2]&255)!=0xa5;f++)NativeCore.step(handle,0,video,audio);
                    byte[] after=stateRam(handle),battery=NativeCore.battery(handle);assertEquals(0xa5,after[0x3f2]&255);
                    for(int slot=0;slot<6;slot++)for(int i=0;i<128;i++){
                        int at=0x1095+slot*128+i;
                        if(i>=0x41&&i<0x4f){assertEquals(enabled?5:4,after[at]&255);assertEquals(enabled?5:4,battery[0x18+slot*128+i]&255);}
                        else assertEquals("Unrelated character field changed",before[at],after[at]);
                    }
                    NativeCore.reset(handle);assertEquals(enabled,NativeCore.noMpCost(handle));
                }
            }finally{NativeCore.close(handle);}
        }
        byte[] unknown=rom.clone();unknown[0xf09d]^=1;long handle=NativeCore.openConfigured(unknown,null,false);
        try{assertFalse(NativeCore.mpSupported(handle));assertFalse(NativeCore.setNoMpCost(handle,true));}finally{NativeCore.close(handle);}
    }
    public void testGaiden1Spells()throws Exception{
        byte[] rom=fixture("test.gb"),town=fixture("gaiden1-town.bin");
        for(int selected=0;selected<6;selected++){
            long handle=NativeCore.open(rom,null);
            try{
                assertTrue(NativeCore.spellsSupported(handle));assertTrue(NativeCore.loadState(handle,town));assertTrue(NativeCore.canLearnSpells(handle));
                String[] names=NativeCore.spellNames(handle);assertEquals("せんし1",names[0]);assertEquals("そうりょ1",names[2]);assertNull(names[6]);
                byte[] before=stateRam(handle),battery=NativeCore.battery(handle);
                assertFalse(NativeCore.learnSpells(handle,20));assertFalse(NativeCore.learnSpells(handle,6));assertTrue(Arrays.equals(battery,NativeCore.battery(handle)));
                assertTrue(NativeCore.learnSpells(handle,selected));byte[] after=stateRam(handle);int at=0x1186+selected*128+0x39;
                for(int i=0;i<after.length;i++)if(i>=at&&i<at+8)assertEquals(i==at?31:255,after[i]&255);else if(i>=at+8&&i<at+22)assertEquals(9,after[i]&255);else assertEquals("Unrelated RAM changed",before[i],after[i]);
                byte[] learned=NativeCore.battery(handle);assertTrue(NativeCore.learnSpells(handle,selected));assertTrue(Arrays.equals(learned,NativeCore.battery(handle)));
                long fresh=NativeCore.open(rom,learned);
                try{
                    int[] video=new int[160*144];short[] audio=new short[4096];
                    for(int f=0;f<128;f++)NativeCore.step(fresh,f<8?1:0,video,audio);
                    assertTrue(NativeCore.canLearnSpells(fresh));byte[] restored=stateRam(fresh);
                    assertTrue("Normal save reload lost learned spells",Arrays.equals(Arrays.copyOfRange(after,at,at+8),Arrays.copyOfRange(restored,at,at+8)));
                    assertTrue("Reload changed MP",Arrays.equals(Arrays.copyOfRange(after,at+8,at+22),Arrays.copyOfRange(restored,at+8,at+22)));
                }finally{NativeCore.close(fresh);}
                assertTrue(NativeCore.loadState(handle,town));assertTrue(Arrays.equals(before,stateRam(handle)));
                assertTrue(NativeCore.loadState(handle,fixture("gaiden1-before-win.bin")));assertFalse(NativeCore.canLearnSpells(handle));assertFalse(NativeCore.learnSpells(handle,selected));
            }finally{NativeCore.close(handle);}
        }
        byte[] unknown=rom.clone();unknown[0x2644]^=1;long handle=NativeCore.open(unknown,null);
        try{assertFalse(NativeCore.spellsSupported(handle));assertFalse(NativeCore.learnSpells(handle,0));}finally{NativeCore.close(handle);}
    }
    public void testGaiden1Alignment()throws Exception{
        byte[] rom=fixture("test.gb"),town=fixture("gaiden1-town.bin");
        for(int selected=0;selected<6;selected++)for(int alignment=0;alignment<3;alignment++){
            long handle=NativeCore.open(rom,null);
            try{
                assertTrue(NativeCore.loadState(handle,town));byte[] before=stateRam(handle),battery=NativeCore.battery(handle);
                assertFalse(NativeCore.setAlignment(handle,selected,-1));assertFalse(NativeCore.setAlignment(handle,selected,3));
                assertFalse(NativeCore.setAlignment(handle,-1,0));assertFalse(NativeCore.setAlignment(handle,6,0));assertFalse(NativeCore.setAlignment(handle,20,0));
                assertTrue(Arrays.equals(before,stateRam(handle)));assertTrue(Arrays.equals(battery,NativeCore.battery(handle)));
                assertTrue(NativeCore.setAlignment(handle,selected,alignment));byte[] after=stateRam(handle),saved=NativeCore.battery(handle);int at=0x1186+selected*128+10;
                assertEquals(alignment,after[at]&255);assertEquals(alignment,saved[0x17+selected*128+10]&255);
                for(int i=0;i<after.length;i++)if(i!=at)assertEquals("Unrelated RAM changed",before[i],after[i]);
                long fresh=NativeCore.open(rom,saved);
                try{
                    int[] video=new int[160*144];short[] audio=new short[4096];for(int f=0;f<128;f++)NativeCore.step(fresh,f<8?1:0,video,audio);
                    assertEquals(alignment,stateRam(fresh)[at]&255);
                }finally{NativeCore.close(fresh);}
                assertTrue(NativeCore.loadState(handle,town));assertTrue(Arrays.equals(before,stateRam(handle)));
                assertTrue(NativeCore.loadState(handle,fixture("gaiden1-before-win.bin")));byte[] battle=NativeCore.state(handle);
                assertTrue("Repeated snapshot is not deterministic",Arrays.equals(battle,NativeCore.state(handle)));
                assertFalse("Battle edit accepted",NativeCore.setAlignment(handle,selected,alignment));byte[] rejected=NativeCore.state(handle);
                assertTrue("Rejected battle edit changed state",Arrays.equals(battle,rejected));
            }finally{NativeCore.close(handle);}
        }
        byte[] unknown=rom.clone();unknown[0x16dd7]^=1;long handle=NativeCore.open(unknown,null);
        try{assertTrue(NativeCore.loadState(handle,town));assertFalse(NativeCore.setAlignment(handle,0,0));}finally{NativeCore.close(handle);}
    }
    public void testGaiden1CharacterContexts()throws Exception{
        byte[] rom=fixture("test.gb");
        for(String context:new String[]{"gaiden1-town.bin","gaiden1-facility.bin","gaiden1-create.bin","gaiden1-dungeon.bin","gaiden1-camp.bin"})for(int selected=0;selected<6;selected++){
            long handle=NativeCore.open(rom,null);
            try{
                assertTrue(NativeCore.loadState(handle,fixture(context)));int[] video=new int[160*144];short[] audio=new short[4096];
                for(int f=0;f<60&&!NativeCore.canLearnSpells(handle);f++)NativeCore.step(handle,0,video,audio);
                assertTrue(context+" unavailable",NativeCore.canLearnSpells(handle));
                byte[] before=stateRam(handle),battery=NativeCore.battery(handle);assertTrue(context+" edit failed: "+selected,NativeCore.learnSpells(handle,selected));
                byte[] after=stateRam(handle),saved=NativeCore.battery(handle);int at=0x1186+selected*128;
                for(int i=0;i<20;i++){
                    int record=0x1186+128*i,primary=0x17+128*i;
                    for(int j=0;j<128;j++)if(i!=selected||j<0x39||j>=0x4f){assertEquals(before[record+j],after[record+j]);assertEquals(battery[primary+j],saved[primary+j]);}
                }
                assertEquals(31,after[at+0x39]&31);for(int i=1;i<8;i++)assertEquals(255,after[at+0x39+i]&255);
                for(int i=0;i<14;i++){assertEquals(9,after[at+0x41+i]&255);assertEquals(9,saved[0x17+selected*128+0x41+i]&255);}
                for(int f=0;f<180;f++)NativeCore.step(handle,0,video,audio);
                after=stateRam(handle);for(int i=0;i<14;i++)assertEquals(9,after[at+0x41+i]&255);assertEquals(31,after[at+0x39]&31);for(int i=1;i<8;i++)assertEquals(255,after[at+0x39+i]&255);
            }finally{NativeCore.close(handle);}
        }
    }
    private void creationPress(long handle,int keys,int wait){
        int[] video=new int[160*144];short[] audio=new short[4096];
        for(int f=0;f<4+wait;f++)NativeCore.step(handle,f<4?keys:0,video,audio);
    }
    private void checkMaxBonus(String romName,String stateName,int raceAt,int bonus,int stats,int roster,int signature)throws Exception{
        byte[] rom=fixture(romName),checkpoint=fixture(stateName);boolean third=signature==0x211ca;
        for(int speed:new int[]{2,3,4,6,8})for(int sex=0;sex<(third?2:1);sex++)for(int race=0;race<(third?10:5);race++){
            long handle=NativeCore.open(rom,null);
            try{
                assertTrue(NativeCore.maxBonusSupported(handle));assertTrue(NativeCore.setMaxBonus(handle,true));
                NativeCore.reset(handle);assertTrue(NativeCore.loadState(handle,checkpoint));assertTrue(NativeCore.setSpeed(handle,speed));
                byte[] before=NativeCore.battery(handle);
                if(third){if(sex==1)creationPress(handle,128,60);creationPress(handle,1,120);}
                for(int i=0;i<race;i++)creationPress(handle,128,60);
                creationPress(handle,1,120);creationPress(handle,1,120);
                byte[] ram=stateRam(handle);assertEquals(race,ram[raceAt]&255);assertEquals(30,ram[bonus]&255);
                // Gaiden III also uses SRAM bank 3 as menu scratch. Its
                // registered characters, backup and checksums are in bank 0.
                byte[] generated=NativeCore.battery(handle);
                assertTrue("Generation changed saved characters",third?Arrays.equals(Arrays.copyOf(before,0x2000),Arrays.copyOf(generated,0x2000)):Arrays.equals(before,generated));
                if(speed!=2||race!=0)continue;
                int initial=ram[stats]&255;creationPress(handle,16,60);assertEquals(29,stateRam(handle)[bonus]&255);assertEquals(initial+1,stateRam(handle)[stats]&255);
                byte[] spent=NativeCore.state(handle);creationPress(handle,32,60);assertEquals(30,stateRam(handle)[bonus]&255);
                assertTrue(NativeCore.loadState(handle,spent));assertEquals(29,stateRam(handle)[bonus]&255);creationPress(handle,32,60);
                byte[] base=stateRam(handle);
                for(int stat=0;stat<6&&(stateRam(handle)[bonus]&255)>0;stat++){for(int i=0;i<10&&(stateRam(handle)[bonus]&255)>0;i++)creationPress(handle,16,30);if(stat<5&&(stateRam(handle)[bonus]&255)>0)creationPress(handle,128,30);}
                ram=stateRam(handle);assertEquals(0,ram[bonus]&255);int total=0;
                for(int i=0;i<6;i++){assertTrue((ram[stats+i]&255)<=(third?(base[stats+i]&255)+10-((sex==0&&i==0)||(sex==1&&i==3)?1:0):18));total+=(ram[stats+i]&255)-(base[stats+i]&255);}assertEquals(30,total);
                int empty=0;while(empty<20&&ram[roster+128*empty]!=0)empty++;assertTrue(empty<20);
                byte[] attributes=Arrays.copyOfRange(ram,stats,stats+6);
                creationPress(handle,1,120);creationPress(handle,1,120);creationPress(handle,1,120);
                ram=stateRam(handle);assertTrue(ram[roster+128*empty]!=0);
                assertTrue("Saved attributes differ",Arrays.equals(attributes,Arrays.copyOfRange(ram,roster+128*empty+14,roster+128*empty+20)));
            }finally{NativeCore.close(handle);}
        }
        byte[] unknown=rom.clone();unknown[signature]^=1;long handle=NativeCore.open(unknown,null);
        try{assertFalse(NativeCore.maxBonusSupported(handle));assertFalse(NativeCore.setMaxBonus(handle,true));}finally{NativeCore.close(handle);}
    }
    public void testGaiden1MaxBonus()throws Exception{checkMaxBonus("test.gb","gaiden1-create.bin",0x110c,0x10fa,0x1112,0x1186,0x16e1b);}
    public void testGaiden2MaxBonus()throws Exception{checkMaxBonus("gaiden2.gb","gaiden2-create.bin",0x101b,0x1009,0x1021,0x1095,0x20d57);}
    public void testGaiden3MaxBonus()throws Exception{checkMaxBonus("gaiden3.gb","gaiden3-create.bin",0xb27,0xb14,0xb2d,0xba1,0x211ca);}
    public void testGaiden1Rewards()throws Exception{
        byte[] rom=fixture("test.gb"),checkpoint=fixture("gaiden1-before-win.bin");
        for(int factor:new int[]{1,8,16,32,64,128,256})for(boolean experience:new boolean[]{true,false}){
            long handle=NativeCore.open(rom,null);
            try{
                assertTrue(NativeCore.rewardsSupported(handle));
                assertTrue(NativeCore.loadState(handle,checkpoint));
                byte[] before=NativeCore.battery(handle);
                assertTrue(NativeCore.setRewards(handle,experience?factor:1,experience?1:factor));
                assertFalse(NativeCore.setRewards(handle,2,1));
                // Reset must preserve user settings; reload only the game state.
                NativeCore.reset(handle);assertTrue(NativeCore.loadState(handle,checkpoint));
                int[] pixels=new int[160*144];short[] audio=new short[4096];
                for(int f=0;f<1500;f++)NativeCore.step(handle,f>=60&&(f-60)%128<8?1:0,pixels,audio);
                byte[] after=NativeCore.battery(handle);int rewarded=0;
                for(int slot=0;slot<20;slot++){
                    int gold=0x17+slot*128+0x19,exp=0x17+slot*128+0x2d;
                    long expGain=rewardNumber(after,exp)-rewardNumber(before,exp),goldGain=rewardNumber(after,gold)-rewardNumber(before,gold);
                    if(expGain==0&&goldGain==0)continue;
                    assertEquals("EXP multiplier",290L*(experience?factor:1),expGain);
                    if(!experience)assertEquals("Gold multiplier",18L*factor,goldGain);
                    rewarded++;
                }
                assertEquals("Ineligible party members received rewards",3,rewarded);
            }finally{NativeCore.close(handle);}
        }
        byte[] unknown=rom.clone();unknown[0x134]='X';long handle=NativeCore.open(unknown,null);
        try{assertFalse(NativeCore.rewardsSupported(handle));assertFalse(NativeCore.setRewards(handle,8,8));}finally{NativeCore.close(handle);}
    }
    public void testDq3HeldScroll()throws Exception{
        byte[] rom=fixture("dq3.gbc"),checkpoint=fixture("dq3-walk.bin");
        for(int speed:new int[]{3,4,6,8}){
            long fast=NativeCore.open(rom,null),reference=NativeCore.open(rom,null);
            try{
                assertTrue(NativeCore.loadState(fast,checkpoint));
                assertTrue(NativeCore.loadState(reference,checkpoint));
                assertTrue(NativeCore.setSpeed(fast,speed));
                int[] actual=new int[160*144],expected=new int[160*144];short[] audio=new short[4096];
                int remainder=0;
                for(int tick=0;tick<90;tick++){
                    int keys=tick<32?32:0;
                    NativeCore.step(fast,keys,actual,audio);remainder+=speed;
                    for(int frame=0;frame<remainder/2;frame++)NativeCore.step(reference,keys,expected,audio);
                    remainder%=2;
                    assertTrue("Held scroll/release differs: speed="+speed+" tick="+tick,Arrays.equals(actual,expected));
                }
            }finally{NativeCore.close(fast);NativeCore.close(reference);}
        }
    }
    public void testDq3FieldBattle()throws Exception{
        byte[] rom=fixture("dq3.gbc"),checkpoint=fixture("dq3-field.bin");
        String script=new String(fixture("dq3-field-battle.txt"),java.nio.charset.StandardCharsets.UTF_8);
        for(int speed:new int[]{3,4,6,8}){
            long handle=NativeCore.open(rom,null);
            try{
                assertTrue("DQ3 checkpoint rejected",NativeCore.loadState(handle,checkpoint));
                assertTrue(NativeCore.setSpeed(handle,speed));
                int[] pixels=new int[160*144];short[] audio=new short[4096];
                int step=0,totalFrames=0;long samples=0;boolean audible=false;
                for(String line:script.split("\\r?\\n")){
                    if(line.trim().isEmpty())continue;
                    String[] fields=line.trim().split("\\s+");
                    int keys=fields[0].equals("LEFT")?32:fields[0].equals("A")?1:0;
                    int frames=Integer.parseInt(fields[1]);
                    totalFrames+=frames;
                    for(int f=0;f<frames;f++){
                        int count=NativeCore.step(handle,keys,pixels,audio);samples+=count;
                        for(int i=0;i<count;i++)if(audio[i]!=0)audible=true;
                    }
                    byte[] expected=fixture("dq3-expected/"+speed+"/"+(++step)+".bmp");
                    assertEquals(54+160*144*4,expected.length);
                    for(int y=0;y<144;y++)for(int x=0;x<160;x++){
                        int offset=54+((143-y)*160+x)*4;
                        int rgb=(expected[offset]&255)|((expected[offset+1]&255)<<8)|((expected[offset+2]&255)<<16);
                        if((pixels[y*160+x]&0xffffff)!=rgb)fail("DQ3 battle screen differs: speed="+speed+" step="+step+" x="+x+" y="+y);
                    }
                }
                assertEquals(58,step);
                assertTrue("DQ3 battle audio missing",audible);
                assertTrue("DQ3 battle audio clock changed",samples>totalFrames*1096L&&samples<totalFrames*1099L);
            }finally{NativeCore.close(handle);}
        }
    }
    private void checkTurbo(String name)throws Exception{
        long handle=NativeCore.open(fixture(name),null);
        try{
            int[] pixels=new int[160*144];short[] audio=new short[4096];
            for(int speed:new int[]{3,4,6,8}){
                assertTrue(name+" speed rejected",NativeCore.setSpeed(handle,speed));
                int samples=0;
                for(int frame=0;frame<120;frame++)samples+=NativeCore.step(handle,0,pixels,audio);
                assertTrue(name+" audio clock changed",samples>130000&&samples<133000);
            }
            NativeCore.reset(handle);
            assertTrue(name+" reset lost turbo support",NativeCore.setSpeed(handle,4));
            assertTrue(NativeCore.step(handle,0,pixels,audio)>0);
        }finally{NativeCore.close(handle);}
    }
    public void testTurboMusicStateAcrossSessions()throws Exception{
        byte[] rom=fixture("test.gb");long handle=NativeCore.open(rom,null);
        try{
            assertTrue(NativeCore.setSpeed(handle,3));int[] pixels=new int[160*144];short[] audio=new short[4096];
            for(int f=0;f<121;f++)NativeCore.step(handle,0,pixels,audio);
            byte[] state=NativeCore.state(handle);
            assertEquals("GBWZST02",new String(state,0,8,java.nio.charset.StandardCharsets.US_ASCII));
            int[][] expected=new int[30][];
            for(int f=0;f<30;f++){NativeCore.step(handle,0,pixels,audio);expected[f]=pixels.clone();}
            assertTrue(NativeCore.setSpeed(handle,8));for(int f=0;f<30;f++)NativeCore.step(handle,0,pixels,audio);
            assertTrue(NativeCore.loadState(handle,state));assertEquals(3,NativeCore.speed(handle));
            assertFalse(NativeCore.loadState(handle,Arrays.copyOf(state,state.length-1)));
            for(int f=0;f<30;f++){NativeCore.step(handle,0,pixels,audio);assertTrue("Fractional-speed position changed",Arrays.equals(expected[f],pixels));}
            long fresh=NativeCore.openConfigured(rom,null,false);
            try{
                assertTrue(NativeCore.loadState(fresh,state));assertEquals(3,NativeCore.speed(fresh));
                for(int f=0;f<30;f++){NativeCore.step(fresh,0,pixels,audio);assertTrue(Arrays.equals(expected[f],pixels));}
                int length=0;for(int i=0;i<4;i++)length|=(state[12+i]&255)<<(8*i);
                byte[] legacy=Arrays.copyOfRange(state,32,32+length);
                assertTrue(NativeCore.setSpeed(fresh,4));assertTrue(NativeCore.loadState(fresh,legacy));assertEquals(4,NativeCore.speed(fresh));
            }finally{NativeCore.close(fresh);}
        }finally{NativeCore.close(handle);}
    }
    private void checkGaidenExperienceRewards(String romName,String stateName,int primary,int base)throws Exception{
        byte[] rom=fixture(romName),state=fixture(stateName);
        int[] pixels=new int[160*144];short[] audio=new short[4096];
        for(int factor:new int[]{1,8,16,32,64,128,256}){
            long handle=NativeCore.openConfigured(rom,null,false);
            try{
                assertTrue(NativeCore.rewardsSupported(handle));assertTrue(NativeCore.goldRewardsSupported(handle));
                assertTrue(NativeCore.setRewards(handle,factor,1));assertFalse(NativeCore.setRewards(handle,factor,2));
                for(int pass=0;pass<2;pass++){
                    if(pass==1)NativeCore.reset(handle);
                    assertTrue(NativeCore.loadState(handle,state));
                    for(int frame=0;frame<280;frame++)NativeCore.step(handle,frame%128<8?1:0,pixels,audio);
                    byte[] save=NativeCore.battery(handle);
                    for(int slot=0;slot<6;slot++){
                        int at=primary+slot*128+0x2d;
                        long experience=(save[at]&255L)|((save[at+1]&255L)<<8)|((save[at+2]&255L)<<16)|((save[at+3]&255L)<<24);
                        assertEquals((long)base*factor,experience);
                    }
                }
            }finally{NativeCore.close(handle);}
        }
    }
    public void testGaiden2ExperienceRewards()throws Exception{checkGaidenExperienceRewards("gaiden2.gb","gaiden2-before-experience.bin",0x18,125);}
    public void testGaiden3ExperienceRewards()throws Exception{checkGaidenExperienceRewards("gaiden3.gb","gaiden3-before-experience.bin",0x0a,80);}
    private void checkGaidenGoldRewards(String romName,String stateName,int primary,int base)throws Exception{
        byte[] rom=fixture(romName),state=fixture(stateName);
        int[] pixels=new int[160*144];short[] audio=new short[4096];
        for(int factor:new int[]{1,8,16,32,64,128,256}){
            long handle=NativeCore.openConfigured(rom,null,false);
            try{
                assertTrue(NativeCore.goldRewardsSupported(handle));assertTrue(NativeCore.setRewards(handle,1,factor));
                for(int pass=0;pass<2;pass++){
                    if(pass==1)NativeCore.reset(handle);
                    assertTrue(NativeCore.loadState(handle,state));
                    for(int frame=0;frame<280;frame++)NativeCore.step(handle,frame%128<8?1:0,pixels,audio);
                    byte[] save=NativeCore.battery(handle);
                    for(int slot=0;slot<6;slot++){
                        int at=primary+slot*128+0x19;
                        long gold=(save[at]&255L)|((save[at+1]&255L)<<8)|((save[at+2]&255L)<<16)|((save[at+3]&255L)<<24);
                        assertEquals(100L+(long)base*factor,gold);
                    }
                }
            }finally{NativeCore.close(handle);}
        }
    }
    public void testGaiden2GoldRewards()throws Exception{checkGaidenGoldRewards("gaiden2.gb","gaiden2-before-experience.bin",0x18,18);}
    public void testGaiden3GoldRewards()throws Exception{checkGaidenGoldRewards("gaiden3.gb","gaiden3-before-experience.bin",0x0a,11);}
    private long rewardBcd(byte[] data,int at){
        long value=0;for(int i=0;i<6;i++){int b=data[at+i]&255;assertTrue((b>>4)<10&&(b&15)<10);value=value*100+(b>>4)*10+(b&15);}return value;
    }
    public void testWizardry1ExperienceRewards()throws Exception{
        byte[] rom=fixture("test.gbc"),state=fixture("wizardry1-before-experience.bin");
        int[] pixels=new int[160*144];short[] audio=new short[4096];
        long[] initial={1724,1724,2192,1810,1551,11896};
        for(int factor:new int[]{1,8,16,32,64,128,256}){
            long handle=NativeCore.openConfigured(rom,null,false);
            try{
                assertTrue(NativeCore.rewardsSupported(handle));assertTrue(NativeCore.goldRewardsSupported(handle));
                assertTrue(NativeCore.setRewards(handle,factor,1));assertFalse(NativeCore.setRewards(handle,factor,2));
                for(int pass=0;pass<2;pass++){
                    if(pass==1)NativeCore.reset(handle);
                    assertTrue(NativeCore.loadState(handle,state));
                    for(int f=0;f<1200;f++)NativeCore.step(handle,f%64<8?1:0,pixels,audio);
                    byte[] save=NativeCore.battery(handle);
                    for(int slot=0;slot<6;slot++)assertEquals(initial[slot]+156L*factor,rewardBcd(save,0x2b+slot*110+0x17));
                }
            }finally{NativeCore.close(handle);}
        }
    }
    public void testWizardry1GoldRewards()throws Exception{
        byte[] rom=fixture("test.gbc"),state=fixture("wizardry1-before-experience.bin");
        int[] pixels=new int[160*144];short[] audio=new short[4096];
        long[] initial={300,300,250,320,10000320,1350};
        for(int factor:new int[]{1,8,16,32,64,128,256}){
            long handle=NativeCore.openConfigured(rom,null,false);
            try{
                assertTrue(NativeCore.goldRewardsSupported(handle));assertTrue(NativeCore.setRewards(handle,1,factor));
                assertFalse(NativeCore.setRewards(handle,1,2));
                for(int pass=0;pass<2;pass++){
                    if(pass==1)NativeCore.reset(handle);
                    assertTrue(NativeCore.loadState(handle,state));
                    for(int f=0;f<1200;f++)NativeCore.step(handle,f%64<8?1:0,pixels,audio);
                    byte[] save=NativeCore.battery(handle);
                    for(int slot=0;slot<6;slot++)assertEquals(initial[slot]+15L*factor,rewardBcd(save,0x2b+slot*110+0x11));
                }
            }finally{NativeCore.close(handle);}
        }
    }
    private void checkWizardryBcdRewards(boolean third,boolean gold)throws Exception{
        String game=third?"wizardry3":"wizardry2";
        byte[] rom=fixture(game+".gbc"),state=fixture(game+(gold?"-before-gold.bin":"-before-experience.bin"));
        long[] initial=third?(gold?new long[]{300,300,250,320,320,350}:new long[]{1724,1724,2192,1810,1551,1896}):(gold?new long[]{300,300,250,250,320,320,350}:new long[]{1800,1800,1850,1850,1600,1900,1900});
        int[] pixels=new int[160*144];short[] audio=new short[4096];
        for(int factor:new int[]{1,8,16,32,64,128,256}){
            long handle=NativeCore.openConfigured(rom,null,false);
            try{
                assertTrue(NativeCore.rewardsSupported(handle));assertTrue(NativeCore.goldRewardsSupported(handle));
                assertTrue(NativeCore.setRewards(handle,gold?1:factor,gold?factor:1));
                for(int pass=0;pass<2;pass++){
                    if(pass==1)NativeCore.reset(handle);
                    assertTrue(NativeCore.loadState(handle,state));
                    for(int f=0;f<1200;f++)NativeCore.step(handle,f%64<8?1:0,pixels,audio);
                    byte[] save=NativeCore.battery(handle);
                    for(int slot=0;slot<initial.length;slot++)assertEquals(initial[slot]+(!third&&slot==3?0:(gold?(third?16L:21L):(third?117L:66L))*factor),rewardBcd(save,0x2b+slot*110+(gold?0x11:0x17)));
                }
            }finally{NativeCore.close(handle);}
        }
    }
    public void testWizardry2ExperienceRewards()throws Exception{checkWizardryBcdRewards(false,false);}
    public void testWizardry2GoldRewards()throws Exception{checkWizardryBcdRewards(false,true);}
    public void testWizardry3ExperienceRewards()throws Exception{checkWizardryBcdRewards(true,false);}
    public void testWizardry3GoldRewards()throws Exception{checkWizardryBcdRewards(true,true);}
    private int dqRewardNumber(byte[] bytes,int at){return (bytes[at]&255)|((bytes[at+1]&255)<<8)|((bytes[at+2]&255)<<16);}
    public void testDq3Rewards()throws Exception{
        byte[] rom=fixture("dq3.gbc"),checkpoint=fixture("dq3-before-reward.bin").clone();
        checkpoint[fieldOffset(checkpoint,"wram")+0x561]=99;
        int[] pixels=new int[160*144];short[] audio=new short[4096];
        for(int factor:new int[]{1,8,16,32,64,128,256})for(int speed:new int[]{2,3,4,6,8})for(int mode=0;mode<3;mode++){
            int expFactor=mode==2?1:factor,goldFactor=mode==1?1:factor;
            long handle=NativeCore.openConfigured(rom,null,false);
            try{
                assertTrue(NativeCore.rewardsSupported(handle));assertTrue(NativeCore.goldRewardsSupported(handle));
                assertTrue(NativeCore.setRewards(handle,expFactor,goldFactor));assertFalse(NativeCore.setRewards(handle,2,1));
                NativeCore.reset(handle);assertTrue(NativeCore.loadState(handle,checkpoint));assertTrue(NativeCore.setSpeed(handle,speed));
                byte[] before=stateRam(handle);int initialExp=dqRewardNumber(before,0x562),initialGold=dqRewardNumber(before,0x4dc);boolean awarded=false;
                for(int frame=0;frame<1000;frame++){
                    NativeCore.step(handle,frame%32<4?1:0,pixels,audio);
                    if(frame%4!=3)continue;
                    byte[] after=stateRam(handle);
                    if(dqRewardNumber(after,0x4dc)==initialGold+8*goldFactor){
                        assertEquals("DQ3 battle EXP",initialExp+16*expFactor,dqRewardNumber(after,0x562));awarded=true;break;
                    }
                }
                assertTrue("DQ3 original victory path did not award gold",awarded);
            }finally{NativeCore.close(handle);}
        }
        byte[] unknown=rom.clone();unknown[0x5b68f]^=1;long handle=NativeCore.openConfigured(unknown,null,false);
        try{assertFalse(NativeCore.rewardsSupported(handle));assertFalse(NativeCore.setRewards(handle,8,8));}finally{NativeCore.close(handle);}
    }
    public void testWizardryColorMaxBonus()throws Exception{
        String[] romNames={"test.gbc","wizardry2.gbc","wizardry3.gbc"};
        for(int game=0;game<3;game++){
            byte[] rom=fixture(romNames[game]),checkpoint=fixture("wizardry"+(game+1)+"-create.bin");
            for(int speed:new int[]{2,3,4,6,8})for(int race=0;race<5;race++){
                long handle=NativeCore.openConfigured(rom,null,false);
                try{
                    assertTrue(NativeCore.maxBonusSupported(handle));assertTrue(NativeCore.setMaxBonus(handle,true));
                    NativeCore.reset(handle);assertTrue(NativeCore.loadState(handle,checkpoint));assertTrue(NativeCore.setSpeed(handle,speed));
                    byte[] before=stateRam(handle);
                    creationPress(handle,0,60);for(int n=0;n<race;n++)creationPress(handle,128,90);
                    creationPress(handle,1,90);creationPress(handle,1,120);
                    byte[] ram=stateRam(handle);assertEquals(race,(ram[0x9ac]&255)>>5);assertEquals(60,ram[0x0c]&255);
                    assertTrue("Creation changed existing characters",Arrays.equals(Arrays.copyOfRange(before,0x10a,0x9a2),Arrays.copyOfRange(ram,0x10a,0x9a2)));
                    if(speed!=2||race!=0)continue;
                    int[] base=new int[6];for(int n=0;n<6;n++)base[n]=ram[0x9ad+n]&255;
                    creationPress(handle,16,30);assertEquals(59,stateRam(handle)[0x0c]&255);
                    byte[] spent=NativeCore.state(handle);creationPress(handle,32,30);assertEquals(60,stateRam(handle)[0x0c]&255);
                    assertTrue(NativeCore.loadState(handle,spent));assertEquals(59,stateRam(handle)[0x0c]&255);creationPress(handle,32,30);
                    for(int stat=0;stat<6;stat++){
                        for(int point=0;point<11;point++)creationPress(handle,16,30);
                        assertEquals("Native race base+10 cap",base[stat]+10,stateRam(handle)[0x9ad+stat]&255);
                        if(stat<5)creationPress(handle,128,30);
                    }
                    ram=stateRam(handle);assertEquals(0,ram[0x0c]&255);int empty=0;while(empty<20&&ram[0x10a+empty*110]!=0)empty++;assertTrue(empty<20);
                    for(int attempt=0;attempt<8&&stateRam(handle)[0x10a+empty*110]==0;attempt++)creationPress(handle,1,120);
                    ram=stateRam(handle);assertTrue(ram[0x10a+empty*110]!=0);
                    for(int stat=0;stat<6;stat++)assertEquals(base[stat]+10,ram[0x10a+empty*110+0x0b+stat]&255);
                    for(int attempt=0;attempt<4;attempt++){
                        byte[] save=NativeCore.battery(handle);
                        if(Arrays.equals(Arrays.copyOfRange(ram,0x10a+empty*110,0x10a+(empty+1)*110),Arrays.copyOfRange(save,0x2b+empty*110,0x2b+(empty+1)*110)))break;
                        creationPress(handle,2,120);
                    }
                    assertTrue("Native save mismatch",Arrays.equals(Arrays.copyOfRange(ram,0x10a+empty*110,0x10a+(empty+1)*110),Arrays.copyOfRange(NativeCore.battery(handle),0x2b+empty*110,0x2b+(empty+1)*110)));
                }finally{NativeCore.close(handle);}
            }
            int signature=game==0?0x19b0b:game==1?0x19b9c:0x19b02;byte[] unknown=rom.clone();unknown[signature]^=1;
            long handle=NativeCore.openConfigured(unknown,null,false);try{assertFalse(NativeCore.maxBonusSupported(handle));assertFalse(NativeCore.setMaxBonus(handle,true));}finally{NativeCore.close(handle);}
        }
    }
    public void testWizardry123GlobalMp()throws Exception{
        int[][] sites={{0x105ef,0x1283e,0x1a2df,0x1cf33},{0x105f7,0x128ff,0x1a37a,0x1cf42},{0x105ef,0x12833,0x1a2ec,0x1cf33}};
        int[] lookups={0x25d4,0x25cc,0x259a};
        int[] video=new int[160*144];short[] audio=new short[4096];
        for(int game=0;game<3;game++){
            byte[] original=fixture("wizardry"+(game+1)+".gbc");
            for(int siteIndex=0;siteIndex<4;siteIndex++){
                int site=sites[game][siteIndex],lookup=lookups[game];byte[] rom=original.clone();
                // Disposable test ROM returns after the original decrement.
                rom[site+3]=(byte)0xc9;
                long handle=NativeCore.openConfigured(rom,null,false);
                try{
                    assertTrue("MP profile game "+game,NativeCore.mpSupported(handle));assertFalse(NativeCore.noMpCost(handle));
                    byte[] base=NativeCore.state(handle);int wram=fieldOffset(base,"wram"),pc=fieldOffset(base,"pc");
                    for(int slot=0;slot<6;slot++)for(int speed:new int[]{2,3,4,6,8}){
                        byte[] scenario=base.clone();scenario[pc]=(byte)0xcb;scenario[pc+1]=0;
                        for(int i=0;i<6;i++)scenario[wram+0x103+i]=(byte)i;
                        Arrays.fill(scenario,wram+0x10a,wram+0x10a+20*110,(byte)0x59);
                        scenario[wram+0x80]=0;
                        ByteArrayOutputStream code=new ByteArrayOutputStream();
                        code.write(new byte[]{(byte)0xf3,0x31,(byte)0xf0,(byte)0xcf,0x3e,(byte)(site/0x4000),(byte)0xea,0,0x21,(byte)0xea,0x49,(byte)0xc0,0x3e,(byte)slot,(byte)0xea,(byte)0xb0,(byte)0xff});
                        for(int level=0;level<14;level++){
                            if(siteIndex==2&&level!=8)continue;
                            int address=0xc130+slot*110+level;
                            if(siteIndex==2)code.write(new byte[]{0x21,(byte)address,(byte)(address>>8),0x7e});
                            else code.write(new byte[]{0x3e,(byte)level,(byte)0xea,(byte)0xba,(byte)0xc0,(byte)0xcd,(byte)lookup,(byte)(lookup>>8)});
                            int entry=(site&0x3fff)+0x4000;code.write(new byte[]{(byte)0xcd,(byte)entry,(byte)(entry>>8)});
                        }
                        code.write(new byte[]{0x3e,(byte)0xa5,(byte)0xea,(byte)0x80,(byte)0xc0,0x18,(byte)0xfe});
                        assertTrue(code.size()<0x400);System.arraycopy(code.toByteArray(),0,scenario,wram+0xb00,code.size());
                        for(boolean enabled:new boolean[]{false,true,false}){
                            assertTrue(NativeCore.setNoMpCost(handle,enabled));assertTrue(NativeCore.loadState(handle,scenario));assertEquals(enabled,NativeCore.noMpCost(handle));assertTrue(NativeCore.setSpeed(handle,speed));
                            for(int f=0;f<60&&(stateRam(handle)[0x80]&255)!=0xa5;f++)NativeCore.step(handle,0,video,audio);
                            byte[] after=stateRam(handle);assertEquals("Execution game "+game+" site "+siteIndex,0xa5,after[0x80]&255);
                            for(int i=0;i<20*110;i++){
                                int field=i%110;boolean target=i/110==slot&&field>=0x26&&field<0x34&&(siteIndex!=2||field==0x2e);
                                assertEquals("MP/other field game "+game+" site "+siteIndex+" offset "+i,target&&!enabled?0x49:0x59,after[0x10a+i]&255);
                            }
                        }
                    }
                    NativeCore.setNoMpCost(handle,true);NativeCore.reset(handle);assertTrue(NativeCore.noMpCost(handle));
                }finally{NativeCore.close(handle);}
            }
        }
    }
    public void testGaiden23AllMpPaths()throws Exception{
        int[] video=new int[160*144];short[] audio=new short[4096];
        for(int game=2;game<=3;game++){
            boolean third=game==3;int roster=third?0xba1:0x1095,primary=third?0x0a:0x18;
            int bankAddress=third?0xcac7:0xcfbd,lookup=third?0xdf7:0xf20,scratch=third?0xcb14:0xd009;
            int load=third?0x1021:0x1175,save=third?0x105a:0x11a3,menu=third?0x506f:0x4f8e,battle=third?0x4e7a:0x4d40;
            byte[] original=fixture("gaiden"+game+".gb");
            for(int site=0;site<(third?4:3);site++){
                byte[] rom=original.clone();rom[0x8000+menu-0x4000+4]=(byte)0xc9;rom[(third?0x14e7a:0x14d40)+5]=(byte)0xc9;
                long handle=NativeCore.openConfigured(rom,null,false);
                try{
                    assertTrue(NativeCore.mpSupported(handle));byte[] base=NativeCore.state(handle);int wram=fieldOffset(base,"wram"),pc=fieldOffset(base,"pc");
                    for(int slot=0;slot<6;slot++)for(int speed:new int[]{2,3,4,6,8}){
                        byte[] scenario=base.clone();scenario[pc]=(byte)0xc0;scenario[pc+1]=0;
                        for(int i=0;i<20*128;i++)scenario[wram+roster+i]=(byte)((i*17+3)%251+1);
                        for(int member=0;member<6;member++)for(int level=0;level<(third?21:14);level++)scenario[wram+roster+member*128+(level<14?0x41+level:0x6e+level-14)]=5;
                        scenario[wram+0x380]=0;
                        int bank=site==0?(third?6:3):site==1?2:site==2?5:14;
                        ByteArrayOutputStream code=new ByteArrayOutputStream();code.write(new byte[]{(byte)0xf3,0x31,(byte)0xf0,(byte)0xdf,0x3e,(byte)bank,(byte)0xea,0,0x20,(byte)0xea,(byte)bankAddress,(byte)(bankAddress>>8)});
                        for(int level=0;level<(third?21:14);level++){
                            if(site==0||site==3){
                                int field=third?(site==0?0x6656:0x5ebc):0x709d;
                                code.write(new byte[]{0x3e,(byte)level,(byte)0xea,(byte)scratch,(byte)(scratch>>8),0x3e,(byte)slot,(byte)0xcd,(byte)field,(byte)(field>>8)});
                            }else{
                                code.write(new byte[]{0x3e,(byte)slot,(byte)0xcd,(byte)load,(byte)(load>>8)});
                                int number=level;
                                if(site==2){number=1+(third?(level/7)*0x50:(level>=7?128:0));for(int i=(level/7)*7;i<level;i++)number+=original[(third?0x24f4:0x26ae)+i]&255;}
                                int get=site==1?lookup:(third?0x4ea7:0x4d6d);
                                code.write(new byte[]{0x3e,(byte)number,(byte)0xcd,(byte)get,(byte)(get>>8)});
                                // Battle POP AF consumes a dummy pushed by the RAM stub.
                                if(site==2)code.write(new byte[]{0x3e,(byte)slot,(byte)0xcd,0x60,(byte)0xc3});
                                else code.write(new byte[]{(byte)0xcd,(byte)menu,(byte)(menu>>8)});
                                code.write(new byte[]{0x3e,(byte)slot,(byte)0xcd,(byte)save,(byte)(save>>8)});
                            }
                        }
                        code.write(new byte[]{0x3e,(byte)0xa5,(byte)0xea,(byte)0x80,(byte)0xc3,0x18,(byte)0xfe});assertTrue(code.size()<0x350);
                        System.arraycopy(code.toByteArray(),0,scenario,wram,code.size());
                        byte[] stub={(byte)0xf5,(byte)0xc3,(byte)battle,(byte)(battle>>8)};System.arraycopy(stub,0,scenario,wram+0x360,stub.length);
                        for(boolean enabled:new boolean[]{false,true,false}){
                            assertTrue(NativeCore.setNoMpCost(handle,enabled));assertTrue(NativeCore.loadState(handle,scenario));assertEquals(enabled,NativeCore.noMpCost(handle));NativeCore.setSpeed(handle,speed);
                            for(int f=0;f<60&&(stateRam(handle)[0x380]&255)!=0xa5;f++)NativeCore.step(handle,0,video,audio);
                            byte[] after=stateRam(handle),battery=NativeCore.battery(handle);assertEquals("Execution gaiden "+game+" site "+site,0xa5,after[0x380]&255);
                            for(int i=0;i<20*128;i++){
                                int field=i%128;boolean target=i/128==slot&&((field>=0x41&&field<0x4f)||(third&&field>=0x6e&&field<0x75));
                                int expected=target?(enabled?5:4):(scenario[wram+roster+i]&255);
                                assertEquals("Character gaiden "+game+" site "+site+" offset "+i,expected,after[roster+i]&255);
                                if(target)assertEquals(expected,battery[primary+i]&255);
                            }
                        }
                    }
                    NativeCore.setNoMpCost(handle,true);NativeCore.reset(handle);assertTrue(NativeCore.noMpCost(handle));
                }finally{NativeCore.close(handle);}
            }
        }
    }
}
