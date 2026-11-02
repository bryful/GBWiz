package jp.gbwiz;
import android.test.InstrumentationTestCase;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.view.MotionEvent;
import java.io.*;

public final class RewardViewTest extends InstrumentationTestCase {
    public void testLauncherIconRendering()throws Exception{
        android.content.Context context=getInstrumentation().getTargetContext();
        android.graphics.drawable.Drawable icon=context.getPackageManager().getApplicationIcon(context.getPackageName());
        assertTrue("Adaptive launcher icon missing",icon instanceof android.graphics.drawable.AdaptiveIconDrawable);
        Bitmap image=Bitmap.createBitmap(256,256,Bitmap.Config.ARGB_8888);icon.setBounds(0,0,256,256);icon.draw(new Canvas(image));
        assertTrue("Launcher icon center is transparent",(image.getPixel(128,128)>>>24)!=0);
        try(OutputStream out=new FileOutputStream(new File(context.getCacheDir(),"launcher-icon.png"))){image.compress(Bitmap.CompressFormat.PNG,100,out);}image.recycle();
    }
    public void testCharacterActionTouches(){
        getInstrumentation().runOnMainSync(()->{
          for(int[] size:new int[][]{{1080,2200},{2200,900}}){
            int[] selected={-1,0};
            GameView view=new GameView(getInstrumentation().getTargetContext(),new GameView.Actions(){public void pressed(int key){}public void menuItem(int row,int adjust){selected[0]=row;selected[1]=adjust;}});
            view.menu=true;view.characterMenu=true;view.characterName="3: そうりょ1";view.layout(0,0,size[0],size[1]);
            Bitmap image=Bitmap.createBitmap(size[0],size[1],Bitmap.Config.ARGB_8888);view.draw(new Canvas(image));image.recycle();
            ControllerLayout layout=new ControllerLayout(size[0],size[1],view.getResources().getDisplayMetrics().density);int[] screen=layout.screen;
            for(int row=2;row<5;row++)for(int direction:new int[]{1,-1}){
                float y=screen[1]+screen[3]*.035f+screen[3]*.93f*(.13f+.735f/5*(row+.5f));
                float x=screen[0]+screen[2]*(direction>0?.8f:.2f);long time=android.os.SystemClock.uptimeMillis();
                MotionEvent down=MotionEvent.obtain(time,time,MotionEvent.ACTION_DOWN,x,y,0),up=MotionEvent.obtain(time,time+20,MotionEvent.ACTION_UP,x,y,0);
                view.dispatchTouchEvent(down);view.dispatchTouchEvent(up);down.recycle();up.recycle();assertEquals(row,selected[0]);assertEquals(0,selected[1]);
            }
          }
        });
    }
    public void testGlobalMpRowTouch(){
        getInstrumentation().runOnMainSync(()->{
          for(int[] size:new int[][]{{1080,2200},{2200,900}}){
            int[] selected={-1,0};GameView view=new GameView(getInstrumentation().getTargetContext(),new GameView.Actions(){public void pressed(int key){}public void menuItem(int row,int adjust){selected[0]=row;selected[1]=adjust;}});
            view.menu=true;view.supportMenu=true;view.mpSupported=true;view.layout(0,0,size[0],size[1]);
            Bitmap image=Bitmap.createBitmap(size[0],size[1],Bitmap.Config.ARGB_8888);view.draw(new Canvas(image));image.recycle();
            ControllerLayout layout=new ControllerLayout(size[0],size[1],view.getResources().getDisplayMetrics().density);int[] screen=layout.screen;
            for(int row:new int[]{5,6})for(int direction:new int[]{1,-1}){
                float y=screen[1]+screen[3]*.035f+screen[3]*.93f*(.13f+.735f/7*(row+.5f));
                float x=screen[0]+screen[2]*(direction>0?.8f:.2f);long time=android.os.SystemClock.uptimeMillis();
                MotionEvent down=MotionEvent.obtain(time,time,MotionEvent.ACTION_DOWN,x,y,0),up=MotionEvent.obtain(time,time+20,MotionEvent.ACTION_UP,x,y,0);
                view.dispatchTouchEvent(down);view.dispatchTouchEvent(up);down.recycle();up.recycle();assertEquals(row,selected[0]);assertEquals(direction,selected[1]);
            }
          }
        });
    }
    public void testBonusRowTouch(){
        getInstrumentation().runOnMainSync(()->{
            int[] selected={-1,0};
            GameView view=new GameView(getInstrumentation().getTargetContext(),new GameView.Actions(){public void pressed(int key){}public void menuItem(int row,int adjust){selected[0]=row;selected[1]=adjust;}});
            view.menu=true;view.supportMenu=true;view.bonusSupported=true;view.layout(0,0,1080,2200);
            Bitmap image=Bitmap.createBitmap(1080,2200,Bitmap.Config.ARGB_8888);view.draw(new Canvas(image));image.recycle();
            ControllerLayout layout=new ControllerLayout(1080,2200,view.getResources().getDisplayMetrics().density);int[] screen=layout.screen;
            float top=screen[1]+screen[3]*.035f,height=screen[3]*.93f,y=top+height*(.13f+.735f/7*3.5f);
            for(int direction:new int[]{1,-1}){
                float x=screen[0]+screen[2]*(direction>0?.8f:.2f);long time=android.os.SystemClock.uptimeMillis();
                MotionEvent down=MotionEvent.obtain(time,time,MotionEvent.ACTION_DOWN,x,y,0),up=MotionEvent.obtain(time,time+20,MotionEvent.ACTION_UP,x,y,0);
                view.dispatchTouchEvent(down);view.dispatchTouchEvent(up);down.recycle();up.recycle();
                assertEquals(3,selected[0]);assertEquals(direction,selected[1]);
            }
        });
    }
    public void testSpellPickerTouch(){
        getInstrumentation().runOnMainSync(()->{
            int[] selected={-1,-1};
            GameView view=new GameView(getInstrumentation().getTargetContext(),new GameView.Actions(){public void pressed(int key){}public void menuItem(int row,int adjust){selected[0]=row;selected[1]=adjust;}});
            view.menu=true;view.supportMenu=true;view.spellCharacters=new String[21];view.spellCharacters[0]="戻る";for(int i=1;i<21;i++)view.spellCharacters[i]=i+": テスト";view.spellRow=14;view.layout(0,0,1080,2200);
            Bitmap image=Bitmap.createBitmap(1080,2200,Bitmap.Config.ARGB_8888);view.draw(new Canvas(image));image.recycle();
            ControllerLayout layout=new ControllerLayout(1080,2200,view.getResources().getDisplayMetrics().density);int[] screen=layout.screen;
            float y=screen[1]+screen[3]*.035f+screen[3]*.93f*(.13f+.735f/7*6.5f),x=screen[0]+screen[2]*.8f;long time=android.os.SystemClock.uptimeMillis();
            MotionEvent down=MotionEvent.obtain(time,time,MotionEvent.ACTION_DOWN,x,y,0),up=MotionEvent.obtain(time,time+20,MotionEvent.ACTION_UP,x,y,0);
            view.dispatchTouchEvent(down);view.dispatchTouchEvent(up);down.recycle();up.recycle();assertEquals(20,selected[0]);assertEquals(0,selected[1]);
        });
    }
    public void testSupportMenuRendering()throws Exception{
        final Exception[] failure={null};
        getInstrumentation().runOnMainSync(()->{
            try{
                for(int[] size:new int[][]{{1080,2200},{2200,900}}){
                    GameView view=new GameView(getInstrumentation().getTargetContext(),new GameView.Actions(){public void pressed(int key){}public void menuItem(int row,int adjust){}});
                    view.menu=true;view.supportMenu=true;view.rewardSupported=true;view.goldRewardSupported=true;view.experienceMultiplier=8;view.goldMultiplier=16;view.menuRow=6;view.bonusSupported=true;view.maxBonus=true;view.mpSupported=true;view.noMpCost=true;view.partySupported=true;view.mixedParty=true;
                    view.layout(0,0,size[0],size[1]);
                    Bitmap image=Bitmap.createBitmap(size[0],size[1],Bitmap.Config.ARGB_8888);view.draw(new Canvas(image));
                    try(OutputStream out=new FileOutputStream(new File(getInstrumentation().getTargetContext().getCacheDir(),"reward-menu-"+size[0]+".png"))){image.compress(Bitmap.CompressFormat.PNG,100,out);}image.recycle();
                    view.spellCharacters=new String[]{"戻る","1: せんし1","2: せんし2","3: そうりょ1","4: とうぞく1","5: まほうつかい1","6: ノストラダムス"};view.spellRow=3;
                    image=Bitmap.createBitmap(size[0],size[1],Bitmap.Config.ARGB_8888);view.draw(new Canvas(image));
                    try(OutputStream out=new FileOutputStream(new File(getInstrumentation().getTargetContext().getCacheDir(),"spell-picker-"+size[0]+".png"))){image.compress(Bitmap.CompressFormat.PNG,100,out);}image.recycle();
                    view.spellCharacters=null;view.characterMenu=true;view.characterName="3: そうりょ1";view.menuRow=2;
                    image=Bitmap.createBitmap(size[0],size[1],Bitmap.Config.ARGB_8888);view.draw(new Canvas(image));
                    try(OutputStream out=new FileOutputStream(new File(getInstrumentation().getTargetContext().getCacheDir(),"character-actions-"+size[0]+".png"))){image.compress(Bitmap.CompressFormat.PNG,100,out);}image.recycle();
                }
            }catch(Exception e){failure[0]=e;}
        });
        if(failure[0]!=null)throw failure[0];
    }
}
