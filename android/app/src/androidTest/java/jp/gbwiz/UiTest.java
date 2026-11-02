package jp.gbwiz;
import android.test.ActivityInstrumentationTestCase2;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.graphics.Bitmap;
import android.view.*;
import java.io.*;
import java.util.*;

public final class UiTest extends ActivityInstrumentationTestCase2<MainActivity> {
    public UiTest(){super(MainActivity.class);}
    private GameView view;
    private MainActivity activity;
    private void key(int key){
        getInstrumentation().runOnMainSync(()->{activity.dispatchKeyEvent(new KeyEvent(KeyEvent.ACTION_DOWN,key));activity.dispatchKeyEvent(new KeyEvent(KeyEvent.ACTION_UP,key));});
        getInstrumentation().waitForIdleSync();
    }
    private void tap(float x,float y){
        getInstrumentation().runOnMainSync(()->{
            Bitmap frame=Bitmap.createBitmap(view.getWidth(),view.getHeight(),Bitmap.Config.ARGB_8888);
            view.draw(new android.graphics.Canvas(frame));frame.recycle();
            long t=android.os.SystemClock.uptimeMillis();
            MotionEvent down=MotionEvent.obtain(t,t,MotionEvent.ACTION_DOWN,x,y,0),up=MotionEvent.obtain(t,t+20,MotionEvent.ACTION_UP,x,y,0);
            view.dispatchTouchEvent(down);view.dispatchTouchEvent(up);down.recycle();up.recycle();
        });getInstrumentation().waitForIdleSync();
    }
    private void screenshot(String name)throws IOException {
        try{Thread.sleep(250);}catch(InterruptedException e){Thread.currentThread().interrupt();}
        Bitmap image=getInstrumentation().getUiAutomation().takeScreenshot();
        assertNotNull(image);
        try(OutputStream output=new FileOutputStream(new File(activity.getExternalFilesDir(null),name))){image.compress(Bitmap.CompressFormat.PNG,100,output);}image.recycle();
    }
    public void testMenuAndLayout()throws Exception {
        setActivityIntent(new Intent().putExtra("test_ui",true));activity=getActivity();
        getInstrumentation().waitForIdleSync();
        android.widget.FrameLayout content=activity.findViewById(android.R.id.content);
        view=(GameView)((android.widget.FrameLayout)content.getChildAt(0)).getChildAt(0);
        key(KeyEvent.KEYCODE_F1);assertTrue(view.menu);
        key(KeyEvent.KEYCODE_DPAD_DOWN);assertEquals(1,view.menuRow);
        key(KeyEvent.KEYCODE_DPAD_RIGHT);assertEquals(2,view.slot);
        key(KeyEvent.KEYCODE_X);assertEquals("A must not change the slot",2,view.slot);
        ControllerLayout layout=new ControllerLayout(view.getWidth(),view.getHeight(),activity.getResources().getDisplayMetrics().density);
        int[] screen=layout.screen;
        float boxTop=screen[1]+screen[3]*.035f,boxHeight=screen[3]*.93f;
        tap(screen[0]+screen[2]*.8f,boxTop+boxHeight*(.13f+(.735f/10)*1.5f));
        assertEquals("Touch slot arrow failed",3,view.slot);
        key(KeyEvent.KEYCODE_DPAD_DOWN);key(KeyEvent.KEYCODE_DPAD_DOWN);key(KeyEvent.KEYCODE_DPAD_DOWN);
        assertEquals(4,view.menuRow);key(KeyEvent.KEYCODE_DPAD_RIGHT);assertTrue(view.swap);
        key(KeyEvent.KEYCODE_DPAD_DOWN);key(KeyEvent.KEYCODE_DPAD_DOWN);key(KeyEvent.KEYCODE_DPAD_DOWN);
        assertEquals(7,view.menuRow);key(KeyEvent.KEYCODE_DPAD_RIGHT);assertFalse(view.skipLogo);
        key(KeyEvent.KEYCODE_DPAD_LEFT);assertTrue(view.skipLogo);
        screenshot("ui-portrait.png");
        key(KeyEvent.KEYCODE_Z);assertFalse("B did not close menu",view.menu);
        getInstrumentation().runOnMainSync(()->activity.setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE));
        getInstrumentation().waitForIdleSync();Thread.sleep(800);getInstrumentation().waitForIdleSync();
        key(KeyEvent.KEYCODE_F1);assertTrue(view.menu);screenshot("ui-landscape.png");
    }
    public void testNotificationDoesNotCoverOrBlockControls()throws Exception {
        setActivityIntent(new Intent().putExtra("test_ui",true));activity=getActivity();
        getInstrumentation().waitForIdleSync();
        android.widget.FrameLayout content=activity.findViewById(android.R.id.content);
        android.widget.FrameLayout root=(android.widget.FrameLayout)content.getChildAt(0);
        view=(GameView)root.getChildAt(0);
        android.widget.TextView notification=(android.widget.TextView)root.getChildAt(1);
        java.lang.reflect.Method message=MainActivity.class.getDeclaredMethod("message",String.class);
        message.setAccessible(true);
        getInstrumentation().runOnMainSync(()->{
            try{message.invoke(activity,"最初のエラー");message.invoke(activity,"次のエラー");}
            catch(Exception e){throw new RuntimeException(e);}
        });
        getInstrumentation().waitForIdleSync();
        assertEquals("次のエラー",notification.getText().toString());
        assertTrue("Notification overlaps controls",view.getBottom()<=notification.getTop());
        assertFalse(notification.isClickable());assertFalse(notification.isFocusable());
        key(KeyEvent.KEYCODE_F1);assertTrue(view.menu);
        key(KeyEvent.KEYCODE_DPAD_DOWN);assertEquals(1,view.menuRow);
        key(KeyEvent.KEYCODE_Z);assertFalse(view.menu);
    }
    public void testControllerPositions(){
        assertEquals(1,ControllerMapping.bit(KeyEvent.KEYCODE_BUTTON_A,0x057e,0x2009));
        assertEquals(2,ControllerMapping.bit(KeyEvent.KEYCODE_BUTTON_B,0x057e,0x2009));
        assertEquals(1,ControllerMapping.bit(KeyEvent.KEYCODE_BUTTON_B,0x045e,0x02ea));
        assertEquals(2,ControllerMapping.bit(KeyEvent.KEYCODE_BUTTON_A,0x045e,0x02ea));
    }
    public void testMultiTouchRelease(){
        setActivityIntent(new Intent().putExtra("test_ui",true));activity=getActivity();getInstrumentation().waitForIdleSync();
        android.widget.FrameLayout content=activity.findViewById(android.R.id.content);
        view=(GameView)((android.widget.FrameLayout)content.getChildAt(0)).getChildAt(0);
        getInstrumentation().runOnMainSync(()->{
            ControllerLayout l=new ControllerLayout(view.getWidth(),view.getHeight(),activity.getResources().getDisplayMetrics().density);
            MotionEvent.PointerProperties[] properties={new MotionEvent.PointerProperties(),new MotionEvent.PointerProperties()};
            MotionEvent.PointerCoords[] coords={new MotionEvent.PointerCoords(),new MotionEvent.PointerCoords()};
            for(int i=0;i<2;i++){
                properties[i].id=i;properties[i].toolType=MotionEvent.TOOL_TYPE_FINGER;
                int[] box=l.buttons.get(i==0?4:0);coords[i].x=box[0]+box[2]/2f;coords[i].y=box[1]+box[3]/2f;coords[i].pressure=coords[i].size=1;
            }
            Bitmap frame=Bitmap.createBitmap(view.getWidth(),view.getHeight(),Bitmap.Config.ARGB_8888);
            view.draw(new android.graphics.Canvas(frame));frame.recycle();
            long t=android.os.SystemClock.uptimeMillis();
            int[] types={MotionEvent.ACTION_DOWN,MotionEvent.ACTION_POINTER_DOWN|(1<<MotionEvent.ACTION_POINTER_INDEX_SHIFT),MotionEvent.ACTION_POINTER_UP,MotionEvent.ACTION_CANCEL};
            int[] expected={16,17,1,0};
            for(int i=0;i<types.length;i++){
                MotionEvent event=MotionEvent.obtain(t,t+i*20,types[i],i==0?1:2,properties,coords,0,0,1,1,0,0,InputDevice.SOURCE_TOUCHSCREEN,0);
                view.dispatchTouchEvent(event);event.recycle();assertEquals("Multi-touch press/release failed",expected[i],view.buttons);
            }
            MotionEvent held=MotionEvent.obtain(t,t+100,MotionEvent.ACTION_DOWN,coords[0].x,coords[0].y,0);
            view.dispatchTouchEvent(held);held.recycle();assertEquals(16,view.buttons);
            activity.onInputDeviceRemoved(-1);assertEquals("Disconnect retained input",0,view.buttons);
        });
    }
    public void testLayoutBounds(){
        for(int[] size:new int[][]{{1080,2200},{2200,900},{912,1840},{1840,800},{320,480},{640,320},{800,1200},{1200,800}}){
            float density=size[0]>1500||size[1]>1800?2.5f:1;
            ControllerLayout l=new ControllerLayout(size[0],size[1],density);
            List<int[]> boxes=new ArrayList<>();boxes.add(l.screen);boxes.addAll(l.buttons.values());
            assertEquals(14,l.buttons.size());
            for(int[] b:boxes)assertTrue("Control outside safe area "+Arrays.toString(size)+Arrays.toString(b),b[0]>=0&&b[1]>=0&&b[2]>0&&b[3]>0&&b[0]+b[2]<=size[0]+1&&b[1]+b[3]<=size[1]+1);
            for(int i=0;i<boxes.size();i++)for(int j=i+1;j<boxes.size();j++){
                int[] a=boxes.get(i),b=boxes.get(j);
                int w=Math.min(a[0]+a[2],b[0]+b[2])-Math.max(a[0],b[0]),h=Math.min(a[1]+a[3],b[1]+b[3])-Math.max(a[1],b[1]);
                assertTrue("Overlapping controls "+Arrays.toString(size)+" "+i+"/"+j,w<=1||h<=1);
            }
        }
    }
}
