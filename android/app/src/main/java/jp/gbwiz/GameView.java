package jp.gbwiz;
import android.content.Context;
import android.graphics.*;
import android.view.MotionEvent;
import android.view.View;
import android.text.TextPaint;
import android.text.TextUtils;
import java.util.Map;

final class GameView extends View {
    interface Actions {void pressed(int key);void menuItem(int row,int adjust);}
    private final Bitmap frame=Bitmap.createBitmap(160,144,Bitmap.Config.ARGB_8888);
    private final Paint paint=new Paint(Paint.ANTI_ALIAS_FLAG);
    private final TextPaint menuPaint=new TextPaint(Paint.ANTI_ALIAS_FLAG);
    private ControllerLayout layout;
    private final Actions actions;
    private final String[] labels={"A","B","Select","Start","→","←","↑","↓","ファイル","設定","SAVE 1","LOAD 1","リセット","マップ"};
    private int active;
    private boolean blocked;
    int buttons,menuRow,slot=1,speedHalf=2;
    boolean menu,swap,skipLogo=true,mapMode;
    boolean supportMenu,rewardSupported,goldRewardSupported;
    boolean characterMenu;
    int characterAlignment=-1;
    String characterName="";
    boolean bonusSupported,maxBonus,mpSupported,noMpCost,partySupported,mixedParty;
    int experienceMultiplier=1,goldMultiplier=1;
    String navigationHint="上下:選択  左右:変更  A:決定  B:戻る";
    String[] browserItems;
    String[] spellCharacters;
    int spellRow;
    int browserRow;
    private RectF menuBox;
    private float rowTop,rowHeight;
    GameView(Context context,Actions actions) {super(context);this.actions=actions;setFocusable(true);setContentDescription("GBWiz ゲーム画面とタッチコントローラー");}
    void frame(int[] pixels) {frame.setPixels(pixels,0,160,0,0,160,144);invalidate();}
    void clearInput() {if(active!=0)blocked=true;active=buttons=0;invalidate();}
    private RectF rect(int[] b) {return new RectF(b[0],b[1],b[0]+b[2],b[1]+b[3]);}
    @Override protected void onSizeChanged(int w,int h,int ow,int oh) {clearInput();layout=new ControllerLayout(w,h,getResources().getDisplayMetrics().density);}
    private void text(Canvas c,String text,RectF area,int color,float fraction) {
        paint.setColor(color);paint.setTypeface(Typeface.create("sans-serif",Typeface.BOLD));paint.setTextAlign(Paint.Align.CENTER);
        paint.setTextSize(area.height()*fraction);
        float width=paint.measureText(text);if(width>area.width()*.88f)paint.setTextSize(paint.getTextSize()*area.width()*.88f/width);
        c.drawText(text,area.centerX(),area.centerY()-(paint.ascent()+paint.descent())/2,paint);
    }
    private void menuText(Canvas canvas,String label,RectF area,int color,float size) {
        menuPaint.setColor(color);menuPaint.setTypeface(Typeface.create("sans-serif",Typeface.BOLD));
        menuPaint.setTextAlign(Paint.Align.CENTER);menuPaint.setTextSize(size);
        CharSequence fitted=TextUtils.ellipsize(label,menuPaint,area.width()*.88f,TextUtils.TruncateAt.END);
        canvas.drawText(fitted.toString(),area.centerX(),area.centerY()-(menuPaint.ascent()+menuPaint.descent())/2,menuPaint);
    }
    @Override protected void onDraw(Canvas canvas) {
        canvas.drawColor(0xff101722);if(layout==null)return;
        RectF screen=rect(layout.screen);paint.setColor(Color.BLACK);canvas.drawRect(screen,paint);
        paint.setFilterBitmap(false);canvas.drawBitmap(frame,null,screen,paint);
        if(menu||browserItems!=null||spellCharacters!=null)drawMenu(canvas,screen);
        for(Map.Entry<Integer,int[]> entry:layout.buttons.entrySet()) {
            int key=entry.getKey();RectF box=rect(entry.getValue());
            paint.setColor((active&(1<<key))!=0?0xffb47442:(key<=3?0xffa3333f:0xff344154));
            if(key<=3)canvas.drawOval(box,paint);else canvas.drawRoundRect(box,4,4,paint);
            text(canvas,mapMode&&key==12?"LB / 階層↑":mapMode&&key==10?"RB / 階層↓":labels[key],box,Color.WHITE,key<=3?.3f:key<=7?.6f:.4f);
        }
    }
    private void drawMenu(Canvas c,RectF screen) {
        menuBox=new RectF(screen.left+screen.width()*.025f,screen.top+screen.height()*.035f,screen.right-screen.width()*.025f,screen.bottom-screen.height()*.035f);
        paint.setColor(0xff080c12);c.drawRect(menuBox,paint);
        paint.setStyle(Paint.Style.STROKE);paint.setStrokeWidth(2);paint.setColor(0xffaab8c8);c.drawRect(menuBox,paint);paint.setStyle(Paint.Style.FILL);
        String[] items=browserItems!=null?browserItems:spellCharacters;
        int visibleRows=items!=null?7:characterMenu?3:supportMenu?7:10;
        float header=menuBox.height()*.13f;rowHeight=menuBox.height()*.735f/visibleRows;rowTop=menuBox.top+header;
        menuText(c,browserItems!=null?"ROM選択":spellCharacters!=null?"対象キャラクター選択":characterMenu?characterName:supportMenu?"サポート":"機能メニュー",new RectF(menuBox.left,menuBox.top,menuBox.right,rowTop),0xfff0f2f5,header*.46f);
        String speed=speedHalf%2==0?Integer.toString(speedHalf/2):"1.5";
        String[] rows=items!=null?items:characterMenu?new String[]{"対象を選び直す","全呪文習得","属性   "+(characterAlignment>=0?"‹ "+new String[]{"善","中立","悪"}[characterAlignment]+" ›":"対象外")}:supportMenu?new String[]{"戻る","獲得経験値   "+(rewardSupported?"‹ ×"+experienceMultiplier+" ›":"対象外"),"獲得お金   "+(goldRewardSupported?"‹ ×"+goldMultiplier+" ›":"対象外"),"作成ボーナス最大   "+(bonusSupported?"‹ "+(maxBonus?"ON":"OFF")+" ›":"対象外"),"キャラクターを選択","MP消費なし   "+(mpSupported?"‹ "+(noMpCost?"ON":"OFF")+" ›":"対象外"),"善悪混合   "+(partySupported?"‹ "+(mixedParty?"ON":"OFF")+" ›":"対象外")}:new String[]{"再開","スロット   ‹  "+slot+" / 5  ›","ステート保存","ステート読込","A / B 入替   ‹  "+(swap?"ON":"OFF")+"  ›","ROM選択","リセット","ロゴスキップ   ‹  "+(skipLogo?"ON":"OFF")+"  ›","速度   ‹  "+speed+"倍  ›  音声1倍","サポート"};
        int selected=browserItems!=null?browserRow:spellCharacters!=null?spellRow:menuRow,first=items!=null?(selected/7)*7:0;
        // Use the main menu's font size in every submenu. Row spacing may
        // grow for shorter lists, but font size never depends on row count,
        // label length, ROM capabilities or the selection marker.
        float itemTextSize=menuBox.height()*.735f/10*.53f;
        for(int i=0;i<visibleRows&&first+i<rows.length;i++) {
            RectF r=new RectF(menuBox.left+8,rowTop+i*rowHeight,menuBox.right-8,rowTop+(i+1)*rowHeight);
            if(first+i==selected){paint.setColor(0xff2c3c51);c.drawRect(r,paint);}
            menuText(c,(first+i==selected?"›  ":"")+rows[first+i],r,first+i==selected?0xffffdc83:0xffe0e6ee,itemTextSize);
        }
        menuText(c,spellCharacters!=null?"上下:選択  A:決定  B:戻る":characterMenu?"上下:選択  左右:属性変更  A:実行  B:戻る":navigationHint,new RectF(menuBox.left,rowTop+visibleRows*rowHeight,menuBox.right,menuBox.bottom),0xffa7b4c5,menuBox.height()*.135f*.3f);
    }
    @Override public boolean onTouchEvent(MotionEvent event) {
        if(layout==null)return true;
        int type=event.getActionMasked();
        if(type==MotionEvent.ACTION_UP||type==MotionEvent.ACTION_CANCEL) {blocked=false;active=buttons=0;invalidate();return true;}
        if(blocked)return true;
        if((menu||browserItems!=null||spellCharacters!=null)&&type==MotionEvent.ACTION_DOWN&&menuBox!=null&&menuBox.contains(event.getX(),event.getY())) {
            int row=(int)((event.getY()-rowTop)/rowHeight);
            String[] items=browserItems!=null?browserItems:spellCharacters;
            if(event.getY()>=rowTop&&row>=0&&row<(items!=null?7:characterMenu?3:supportMenu?7:10)) {
                int adjust=items==null&&(characterMenu?row==2:supportMenu?((row>=1&&row<=3)||row==5||row==6):(row==1||row==4||row==7||row==8))?(event.getX()<menuBox.centerX()?-1:1):0;
                int selected=browserItems!=null?browserRow:spellRow,index=items==null?row:(selected/7)*7+row;
                if(items==null||index<items.length)actions.menuItem(index,adjust);blocked=true;
            }
            return true;
        }
        int next=0;
        for(int p=0;p<event.getPointerCount();p++) {
            if(type==MotionEvent.ACTION_POINTER_UP&&p==event.getActionIndex())continue;
            for(Map.Entry<Integer,int[]> entry:layout.buttons.entrySet())if(rect(entry.getValue()).contains(event.getX(p),event.getY(p)))next|=1<<entry.getKey();
        }
        int pressed=next&~active;active=next;buttons=next&255;
        for(int i=0;i<labels.length;i++)if((pressed&(1<<i))!=0)actions.pressed(i);
        invalidate();return true;
    }
}
