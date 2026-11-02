package jp.gbwiz;
import android.test.ActivityInstrumentationTestCase2;
import android.content.Intent;
import android.view.KeyEvent;
import java.io.*;
import java.lang.reflect.Field;
import java.util.Arrays;

public final class CharacterFlowTest extends ActivityInstrumentationTestCase2<MainActivity>{
    public CharacterFlowTest(){super(MainActivity.class);}
    private MainActivity activity;private GameView view;private long handle;
    private void field(String name,Object value)throws Exception{Field field=MainActivity.class.getDeclaredField(name);field.setAccessible(true);field.set(activity,value);}
    private byte[] fixture(String name)throws Exception{
        try(InputStream in=new FileInputStream(new File(getInstrumentation().getTargetContext().getCacheDir(),name));ByteArrayOutputStream out=new ByteArrayOutputStream()){
            byte[] buffer=new byte[16384];int count;while((count=in.read(buffer))!=-1)out.write(buffer,0,count);return out.toByteArray();
        }
    }
    private void prepare(String checkpoint)throws Exception{prepare(checkpoint,"test.gb");}
    private void prepare(String checkpoint,String romFixture)throws Exception{
        setActivityIntent(new Intent().putExtra("test_ui",true));activity=getActivity();
        byte[] rom=fixture(romFixture),state=fixture(checkpoint);final Exception[] failure={null};
        getInstrumentation().runOnMainSync(()->{try{
            // Isolate this UI test from the user's document tree and frame loop.
            field("resumed",false);field("store",null);
            handle=NativeCore.open(rom,null);assertTrue(NativeCore.loadState(handle,state));field("core",handle);
            Field member=MainActivity.class.getDeclaredField("view");member.setAccessible(true);view=(GameView)member.get(activity);
            // Native test sessions default to OFF; isolate the activity's
            // desired settings from any settings loaded from the user's tree.
            field("noMpCost",false);field("mixedParty",false);
            view.noMpCost=view.mixedParty=false;
            view.mpSupported=NativeCore.mpSupported(handle);view.partySupported=NativeCore.partySupported(handle);
        }catch(Exception e){failure[0]=e;}});
        if(failure[0]!=null)throw failure[0];
    }
    private void key(int code){getInstrumentation().runOnMainSync(()->{activity.dispatchKeyEvent(new KeyEvent(KeyEvent.ACTION_DOWN,code));activity.dispatchKeyEvent(new KeyEvent(KeyEvent.ACTION_UP,code));});getInstrumentation().waitForIdleSync();}
    private void picker(){key(KeyEvent.KEYCODE_F1);key(KeyEvent.KEYCODE_DPAD_UP);key(KeyEvent.KEYCODE_X);key(KeyEvent.KEYCODE_DPAD_UP);key(KeyEvent.KEYCODE_DPAD_UP);key(KeyEvent.KEYCODE_DPAD_UP);key(KeyEvent.KEYCODE_X);}
    public void testSelectBeforeExecute()throws Exception{
        prepare("gaiden1-camp.bin");byte[] before=NativeCore.battery(handle);picker();assertNotNull(view.spellCharacters);
        key(KeyEvent.KEYCODE_DPAD_DOWN);key(KeyEvent.KEYCODE_DPAD_DOWN);key(KeyEvent.KEYCODE_DPAD_DOWN);key(KeyEvent.KEYCODE_X);
        assertNull(view.spellCharacters);assertTrue(view.characterMenu);assertTrue(view.characterName.contains("そうりょ1"));
        assertTrue("Selecting a character already applied the action",Arrays.equals(before,NativeCore.battery(handle)));
        key(KeyEvent.KEYCODE_X);byte[] after=NativeCore.battery(handle);int at=0x17+2*128+0x39;
        assertEquals(31,after[at]&31);for(int i=1;i<8;i++)assertEquals(255,after[at+i]&255);
        key(KeyEvent.KEYCODE_DPAD_DOWN);assertEquals(2,view.menuRow);
        int original=NativeCore.alignment(handle,2);assertEquals(original,view.characterAlignment);
        byte[] unchanged=NativeCore.battery(handle);key(KeyEvent.KEYCODE_X);
        assertTrue("A changed alignment",Arrays.equals(unchanged,NativeCore.battery(handle)));
        for(int step=1;step<=3;step++){
            unchanged=NativeCore.battery(handle);key(KeyEvent.KEYCODE_DPAD_RIGHT);
            byte[] changed=NativeCore.battery(handle);int alignment=(original+step)%3;
            assertEquals(alignment,changed[0x17+2*128+10]&255);assertEquals(alignment,view.characterAlignment);
            for(int slot=0;slot<6;slot++)for(int i=0;i<128;i++)if(slot!=2||i!=10)assertEquals(unchanged[0x17+slot*128+i],changed[0x17+slot*128+i]);
        }
        key(KeyEvent.KEYCODE_DPAD_LEFT);assertEquals((original+2)%3,view.characterAlignment);
        key(KeyEvent.KEYCODE_DPAD_RIGHT);assertEquals(original,view.characterAlignment);
        key(KeyEvent.KEYCODE_DPAD_DOWN);assertEquals(0,view.menuRow);
        assertTrue(view.characterMenu);key(KeyEvent.KEYCODE_Z);assertNotNull(view.spellCharacters);assertFalse(view.characterMenu);
        key(KeyEvent.KEYCODE_Z);assertNull(view.spellCharacters);assertTrue(view.supportMenu);
    }
    public void testWizardry1Alignment()throws Exception{wizardryAlignment(1);}
    public void testWizardry2Alignment()throws Exception{wizardryAlignment(2);}
    public void testWizardry3Alignment()throws Exception{wizardryAlignment(3);}
    private void wizardryAlignment(int number)throws Exception{
        prepare("wizardry"+number+"-alignment-camp.bin","wizardry"+number+"-alignment.gbc");byte[] before=NativeCore.battery(handle);picker();
        assertNotNull(view.spellCharacters);key(KeyEvent.KEYCODE_DPAD_DOWN);key(KeyEvent.KEYCODE_X);
        assertTrue(view.characterMenu);assertTrue(Arrays.equals(before,NativeCore.battery(handle)));
        int initial=NativeCore.alignment(handle,0);assertTrue(initial>=0);assertEquals(initial,view.characterAlignment);
        key(KeyEvent.KEYCODE_DPAD_DOWN);assertEquals(2,view.menuRow);
        key(KeyEvent.KEYCODE_X);assertTrue(Arrays.equals(before,NativeCore.battery(handle)));
        for(int step=1;step<=3;step++){
            byte[] prior=NativeCore.battery(handle);key(KeyEvent.KEYCODE_DPAD_RIGHT);byte[] changed=NativeCore.battery(handle);
            int alignment=(initial+step)%3;assertEquals(alignment,view.characterAlignment);
            assertEquals((prior[0x35]&252)|alignment,changed[0x35]&255);
            for(int i=0;i<changed.length;i++)if(i!=0x35&&i!=0xbfe&&i!=0xbff)assertEquals(prior[i],changed[i]);
        }
        key(KeyEvent.KEYCODE_DPAD_LEFT);assertEquals((initial+2)%3,view.characterAlignment);
        key(KeyEvent.KEYCODE_DPAD_RIGHT);assertEquals(initial,view.characterAlignment);
        key(KeyEvent.KEYCODE_Z);assertNotNull(view.spellCharacters);
        assertFalse(NativeCore.setAlignment(handle,20,0));assertFalse(NativeCore.setAlignment(handle,0,3));
    }
    public void testGaiden2Alignment()throws Exception{gaidenAlignment(2);}
    public void testGaiden3Alignment()throws Exception{gaidenAlignment(3);}
    private void gaidenAlignment(int number)throws Exception{
        prepare("gaiden"+number+"-alignment.bin","gaiden"+number+"-alignment.gb");
        byte[] before=NativeCore.battery(handle);picker();assertNotNull(view.spellCharacters);
        key(KeyEvent.KEYCODE_DPAD_DOWN);key(KeyEvent.KEYCODE_X);assertTrue(view.characterMenu);
        assertTrue(Arrays.equals(before,NativeCore.battery(handle)));int initial=NativeCore.alignment(handle,0);assertTrue(initial>=0);
        key(KeyEvent.KEYCODE_DPAD_DOWN);assertEquals(2,view.menuRow);key(KeyEvent.KEYCODE_X);
        assertTrue(Arrays.equals(before,NativeCore.battery(handle)));
        int base=number==3?10:24,mirror=number==3?0xe52:0xe5e,sum=number==3?0xe04:0xe1c;
        for(int step=1;step<=3;step++){
            byte[] prior=NativeCore.battery(handle);key(KeyEvent.KEYCODE_DPAD_RIGHT);byte[] changed=NativeCore.battery(handle);
            int alignment=(initial+step)%3;assertEquals(alignment,view.characterAlignment);assertEquals(alignment,changed[base+10]&255);
            for(int i=0;i<changed.length;i++)if(i!=base+10&&!(i>=base+mirror&&i<base+mirror+128)&&i!=sum&&i!=sum+1&&i!=sum+mirror&&i!=sum+mirror+1)assertEquals(prior[i],changed[i]);
        }
        key(KeyEvent.KEYCODE_DPAD_LEFT);assertEquals((initial+2)%3,view.characterAlignment);
        key(KeyEvent.KEYCODE_DPAD_RIGHT);assertEquals(initial,view.characterAlignment);
        key(KeyEvent.KEYCODE_Z);assertNotNull(view.spellCharacters);
    }
    public void testGlobalMpSetting()throws Exception{
        prepare("gaiden1-town.bin");checkGlobalMpSetting();
    }
    public void testWizardry1GlobalMpSetting()throws Exception{prepare("wizardry1-mp-camp.bin","wizardry1.gbc");checkGlobalMpSetting();}
    public void testWizardry2GlobalMpSetting()throws Exception{prepare("wizardry2-mp-camp.bin","wizardry2.gbc");checkGlobalMpSetting();}
    public void testWizardry3GlobalMpSetting()throws Exception{prepare("wizardry3-mp-camp.bin","wizardry3.gbc");checkGlobalMpSetting();}
    public void testGaiden2GlobalMpSetting()throws Exception{prepare("gaiden2-mp-camp.bin","gaiden2.gb");checkGlobalMpSetting();}
    public void testGaiden3GlobalMpSetting()throws Exception{prepare("gaiden3-mp-camp.bin","gaiden3.gb");checkGlobalMpSetting();}
    private void checkGlobalMpSetting()throws Exception{
        assertTrue(NativeCore.mpSupported(handle));assertFalse(NativeCore.noMpCost(handle));
        key(KeyEvent.KEYCODE_F1);key(KeyEvent.KEYCODE_DPAD_UP);key(KeyEvent.KEYCODE_X);key(KeyEvent.KEYCODE_DPAD_UP);
        key(KeyEvent.KEYCODE_DPAD_UP);assertTrue(view.supportMenu);assertEquals(5,view.menuRow);key(KeyEvent.KEYCODE_X);assertFalse(NativeCore.noMpCost(handle));
        key(KeyEvent.KEYCODE_DPAD_RIGHT);assertTrue(NativeCore.noMpCost(handle));assertTrue(view.noMpCost);
        key(KeyEvent.KEYCODE_DPAD_LEFT);assertFalse(NativeCore.noMpCost(handle));assertFalse(view.noMpCost);
    }
    public void testMixedPartySetting()throws Exception{
        prepare("gaiden1-town.bin");assertTrue(NativeCore.partySupported(handle));assertFalse(NativeCore.mixedParty(handle));
        key(KeyEvent.KEYCODE_F1);key(KeyEvent.KEYCODE_DPAD_UP);key(KeyEvent.KEYCODE_X);key(KeyEvent.KEYCODE_DPAD_UP);
        assertTrue(view.supportMenu);assertEquals(6,view.menuRow);key(KeyEvent.KEYCODE_X);assertFalse(NativeCore.mixedParty(handle));
        byte[] before=NativeCore.state(handle);key(KeyEvent.KEYCODE_DPAD_RIGHT);assertTrue(NativeCore.mixedParty(handle));assertTrue(view.mixedParty);assertTrue(Arrays.equals(before,NativeCore.state(handle)));
        key(KeyEvent.KEYCODE_DPAD_LEFT);assertFalse(NativeCore.mixedParty(handle));assertFalse(view.mixedParty);
    }
    public void testBattleExclusion()throws Exception{
        prepare("gaiden1-before-win.bin");byte[] before=NativeCore.state(handle);picker();
        assertNull("Battle picker opened",view.spellCharacters);assertFalse("Battle character menu opened",view.characterMenu);assertTrue("Rejected battle picker changed state",Arrays.equals(before,NativeCore.state(handle)));
    }
}
