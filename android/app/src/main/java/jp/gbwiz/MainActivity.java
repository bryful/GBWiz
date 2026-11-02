package jp.gbwiz;
import android.app.*;
import android.os.*;
import android.content.*;
import android.database.Cursor;
import android.net.Uri;
import android.provider.*;
import android.graphics.Color;
import android.media.*;
import android.view.*;
import android.widget.*;
import android.hardware.input.InputManager;
import java.io.*;
import java.util.Properties;
import java.util.List;
import java.util.ArrayList;

public final class MainActivity extends Activity implements Choreographer.FrameCallback,InputManager.InputDeviceListener {
    private static final int ROM=1,FOLDER=2,ROM_FOLDER=3;
    private long core,lastFrame,lastSave;
    private final int[] pixels=new int[160*144];
    private final short[] pcm=new short[4096];
    private GameView view;
    private TextView notification;
    private final Handler notificationHandler=new Handler(Looper.getMainLooper());
    private final Runnable clearNotification=()->{if(notification!=null)notification.setText("");};
    private DocumentStore store;
    private AudioTrack audio;
    private boolean resumed,picking,swap,trigger,menuOpen,skipLogo=true;
    private int slot=1,keyButtons,hatButtons,speedHalf=2;
    private int experienceMultiplier=1,goldMultiplier=1;
    private boolean maxBonus,noMpCost,mixedParty,mapOpen,mapTrigger;
    private int[] spellSlots;
    private int selectedCharacter=-1;
    private long lastAxisNavigation;
    private String romName="",stem="";
    private final Properties settings=new Properties();
    private List<DocumentStore.Rom> romEntries=new ArrayList<>();
    private InputManager inputManager;
    private SaveQueue saveQueue;
    private final PcmQueue pcmQueue=new PcmQueue();

    @Override public void onCreate(Bundle state) {
        super.onCreate(state); getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        FrameLayout layout=new FrameLayout(this);layout.setBackgroundColor(0xff101722);
        view=new GameView(this,new GameView.Actions(){
            public void pressed(int key){
                view.navigationHint="上下:選択  左右:変更  A:決定  B:戻る";
                if(key<8){if(mapOpen||menuOpen||view.browserItems!=null)navigate(1<<key);}
                else if(key==8)pickRom();else if(key==9)menu();
                else if(mapOpen&&(key==12||key==10))navigate(key==12?256:512);
                else if(key==10)state(1,false);else if(key==11)state(1,true);else if(key==12)resetGame();else if(key==13)openMap();
            }
            public void menuItem(int row,int adjust){if(view.browserItems!=null){view.browserRow=row;activateRom();}else if(view.spellCharacters!=null){view.spellRow=row;selectCharacter();}else{view.menuRow=row;activateMenu(adjust);}}
        });
        int noticeHeight=Math.round(44*getResources().getDisplayMetrics().density);
        FrameLayout.LayoutParams gameParams=new FrameLayout.LayoutParams(-1,-1);
        gameParams.bottomMargin=noticeHeight;
        layout.addView(view,gameParams);
        notification=new TextView(this);
        notification.setTextColor(0xffffe696);notification.setTextSize(13);
        notification.setGravity(Gravity.CENTER_VERTICAL);
        notification.setPadding(12,0,12,0);notification.setMaxLines(2);
        notification.setEllipsize(android.text.TextUtils.TruncateAt.END);
        notification.setClickable(false);notification.setFocusable(false);
        notification.setAccessibilityLiveRegion(View.ACCESSIBILITY_LIVE_REGION_POLITE);
        layout.addView(notification,new FrameLayout.LayoutParams(-1,noticeHeight,Gravity.BOTTOM));
        setContentView(layout);
        // Follow Wiiz: reserve bars, cutouts and mandatory gestures in both orientations.
        layout.setOnApplyWindowInsetsListener((v,insets)->{
            int left,top,right,bottom;
            if(Build.VERSION.SDK_INT>=30){
                android.graphics.Insets bars=insets.getInsetsIgnoringVisibility(WindowInsets.Type.systemBars()|WindowInsets.Type.displayCutout());
                android.graphics.Insets gestures=insets.getInsets(WindowInsets.Type.mandatorySystemGestures());
                left=Math.max(bars.left,gestures.left);top=Math.max(bars.top,gestures.top);
                right=Math.max(bars.right,gestures.right);bottom=Math.max(bars.bottom,gestures.bottom);
            }else{left=insets.getStableInsetLeft();top=insets.getStableInsetTop();right=insets.getStableInsetRight();bottom=insets.getStableInsetBottom();}
            v.setPadding(left,top,right,bottom+Math.round(8*getResources().getDisplayMetrics().density));return insets;
        });layout.requestApplyInsets();
        inputManager=(InputManager)getSystemService(INPUT_SERVICE);
        inputManager.registerInputDeviceListener(this,new Handler(Looper.getMainLooper()));
        saveQueue=new SaveQueue((path,data)->{
            DocumentStore destination=store;
            if(destination==null)throw new IOException("保存フォルダがありません");
            destination.write(path,data);
        },text->runOnUiThread(()->message(text)));
        String saved=getPreferences(0).getString("save_tree",null);
        if(saved!=null) try { store=new DocumentStore(getContentResolver(),Uri.parse(saved)); loadSettings(); }
        catch(Exception e) { store=null; message("保存フォルダを選び直してください"); }
        try {
            int size=AudioTrack.getMinBufferSize(32768,AudioFormat.CHANNEL_OUT_STEREO,AudioFormat.ENCODING_PCM_16BIT);
            audio=new AudioTrack.Builder().setAudioAttributes(new AudioAttributes.Builder().setUsage(AudioAttributes.USAGE_GAME).build())
                .setAudioFormat(new AudioFormat.Builder().setSampleRate(32768).setChannelMask(AudioFormat.CHANNEL_OUT_STEREO).setEncoding(AudioFormat.ENCODING_PCM_16BIT).build())
                .setBufferSizeInBytes(Math.max(size,8192)).setTransferMode(AudioTrack.MODE_STREAM).build();
        } catch(Exception e) { audio=null; message("音声デバイスが使えません。無音で続行します"); }
        if(!BuildConfig.DEBUG||!getIntent().getBooleanExtra("test_ui",false))view.post(this::pickRom);
    }
    private void message(String text) {
        // A single non-modal line outside the game and controls; never queue notifications.
        notificationHandler.removeCallbacks(clearNotification);
        notification.setText(text);
        notificationHandler.postDelayed(clearNotification,5000);
    }
    private boolean paused() { return !resumed||picking||mapOpen||menuOpen||view.browserItems!=null; }
    private void clearInput() { keyButtons=hatButtons=0; trigger=false; view.clearInput(); }
    private void muteAudio(){
        if(audio!=null){try{audio.release();}catch(RuntimeException ignored){}audio=null;}
        pcmQueue.clear();message("音声デバイスが使えません。無音で続行します");
    }
    private void silence() {
        clearInput();pcmQueue.clear();
        if(audio!=null)try{audio.pause();audio.flush();}catch(RuntimeException e){muteAudio();}
        lastFrame=0;
    }
    private void playAudio(int count){
        if(audio==null)return;
        try{
            if(audio.getPlayState()!=AudioTrack.PLAYSTATE_PLAYING)audio.play();
            pcmQueue.offer(pcm,count,(data,length)->audio.write(data,0,length,AudioTrack.WRITE_NON_BLOCKING));
        }catch(RuntimeException e){muteAudio();}
    }
    private Uri initial(String folder) { return DocumentsContract.buildDocumentUri("com.android.externalstorage.documents","primary:Documents/"+folder); }
    private void pickRom() {
        if(picking) return;
        closeMap();
        view.spellCharacters=null;
        view.characterMenu=false;selectedCharacter=-1;
        menuOpen=false;view.menu=false;view.invalidate();
        silence();
        if(store==null) {
            message("Documentsを選ぶとGBWiz/Save・State・romを作成します。既存のGBWizも選べます");
            Intent intent=new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE);
            intent.putExtra(DocumentsContract.EXTRA_INITIAL_URI,initial(""));
            intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION|Intent.FLAG_GRANT_WRITE_URI_PERMISSION|Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION);
            picking=true;
            startActivityForResult(intent,FOLDER);
        } else {
            try {
                String configured=settings.getProperty("rom_tree");
                Uri folder=configured==null?store.romFolder:Uri.parse(configured);
                romEntries=DocumentStore.roms(getContentResolver(),folder);
                String[] items=new String[romEntries.size()+1];items[0]="ROMフォルダを変更…";
                for(int i=0;i<romEntries.size();i++)items[i+1]=romEntries.get(i).name;
                view.browserItems=items;view.browserRow=items.length>1?1:0;view.invalidate();
                if(romEntries.isEmpty())message("ROMがありません。「ROMフォルダを変更」で場所を選べます");
            }catch(Exception e){message("ROMフォルダを選び直してください");chooseRomFolder();}
        }
    }
    private void chooseRomFolder(){
        silence();picking=true;
        Intent intent=new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE);
        intent.putExtra(DocumentsContract.EXTRA_INITIAL_URI,store==null?initial("GBWiz/rom"):store.romFolder);
        intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION|Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION);
        startActivityForResult(intent,ROM_FOLDER);
    }
    private void activateRom(){
        int selected=view.browserRow;
        if(selected==0){chooseRomFolder();return;}
        if(selected>romEntries.size())return;
        try{openRom(romEntries.get(selected-1).uri);view.browserItems=null;view.invalidate();silence();}
        catch(Exception e){message("切り替え失敗（元のゲームを保持）: "+e.getMessage());}
    }
    private void openRom(Uri uri)throws IOException {
        String name=null;
        try(Cursor cursor=getContentResolver().query(uri,new String[]{OpenableColumns.DISPLAY_NAME},null,null,null)) {
            if(cursor!=null&&cursor.moveToFirst())name=cursor.getString(0);
        }
        if(name==null||name.contains("/")||name.contains("\\"))throw new IOException("ROM名を取得できません");
        int dot=name.lastIndexOf('.');String nextStem=dot>0?name.substring(0,dot):name;
        byte[] rom=DocumentStore.read(getContentResolver(),uri,16*1024*1024);flush();
        long next=NativeCore.openConfigured(rom,store.read(nextStem+".sav"),skipLogo);
        NativeCore.setRewards(next,NativeCore.rewardsSupported(next)?experienceMultiplier:1,NativeCore.goldRewardsSupported(next)?goldMultiplier:1);
        view.rewardSupported=NativeCore.rewardsSupported(next);view.goldRewardSupported=NativeCore.goldRewardsSupported(next);
        NativeCore.setMaxBonus(next,maxBonus);view.bonusSupported=NativeCore.maxBonusSupported(next);
        NativeCore.setNoMpCost(next,noMpCost);view.mpSupported=NativeCore.mpSupported(next);
        NativeCore.setMixedParty(next,mixedParty);view.partySupported=NativeCore.partySupported(next);
        if(!NativeCore.setSpeed(next,speedHalf)){speedHalf=2;message("このROMの倍速・等速音声は未対応です。1倍速で再生します");}
        if(core!=0)NativeCore.close(core);
        core=next;romName=name;stem=nextStem;lastFrame=0;lastSave=SystemClock.elapsedRealtime();
    }
    @Override protected void onActivityResult(int request,int result,Intent intent) {
        super.onActivityResult(request,result,intent); picking=false; clearInput();
        if(result!=RESULT_OK||intent==null||intent.getData()==null) return;
        try {
            Uri uri=intent.getData();
            if(request==FOLDER) {
                boolean readGranted=(intent.getFlags()&Intent.FLAG_GRANT_READ_URI_PERMISSION)!=0;
                boolean writeGranted=(intent.getFlags()&Intent.FLAG_GRANT_WRITE_URI_PERMISSION)!=0;
                if(readGranted&&writeGranted)getContentResolver().takePersistableUriPermission(uri,Intent.FLAG_GRANT_READ_URI_PERMISSION|Intent.FLAG_GRANT_WRITE_URI_PERMISSION);
                else if(readGranted)getContentResolver().takePersistableUriPermission(uri,Intent.FLAG_GRANT_READ_URI_PERMISSION);
                else if(writeGranted)getContentResolver().takePersistableUriPermission(uri,Intent.FLAG_GRANT_WRITE_URI_PERMISSION);
                DocumentStore candidate=new DocumentStore(getContentResolver(),uri);
                candidate.read("settings.ini"); store=candidate;
                getPreferences(0).edit().putString("save_tree",uri.toString()).apply(); loadSettings(); pickRom();
            } else if(request==ROM_FOLDER){
                if((intent.getFlags()&Intent.FLAG_GRANT_READ_URI_PERMISSION)!=0)getContentResolver().takePersistableUriPermission(uri,Intent.FLAG_GRANT_READ_URI_PERMISSION);
                DocumentStore.roms(getContentResolver(),uri);
                settings.setProperty("rom_tree",uri.toString());saveSettings();pickRom();
            } else if(request==ROM) {
                openRom(uri);
            }
        } catch(Exception e) { message("切り替え失敗（元のゲームを保持）: "+e.getMessage()); }
    }
    private void loadSettings() throws IOException {
        byte[] data=store.read("settings.ini"); settings.clear();
        if(data!=null) settings.load(new ByteArrayInputStream(data));
        swap=Boolean.parseBoolean(settings.getProperty("swap_ab","false"));
        skipLogo=Boolean.parseBoolean(settings.getProperty("skip_logo","true"));view.skipLogo=skipLogo;
        try{int value=Integer.parseInt(settings.getProperty("speed_half","2"));speedHalf=(value==2||value==3||value==4||value==6||value==8)?value:2;}catch(NumberFormatException e){speedHalf=2;}
        view.speedHalf=speedHalf;
        experienceMultiplier=rewardSetting("experience_multiplier");goldMultiplier=rewardSetting("gold_multiplier");
        view.experienceMultiplier=experienceMultiplier;view.goldMultiplier=goldMultiplier;
        maxBonus=Boolean.parseBoolean(settings.getProperty("max_bonus","false"));view.maxBonus=maxBonus;
        noMpCost=Boolean.parseBoolean(settings.getProperty("no_mp_cost","false"));view.noMpCost=noMpCost;
        mixedParty=Boolean.parseBoolean(settings.getProperty("mixed_party","false"));view.mixedParty=mixedParty;
        try { slot=Math.max(1,Math.min(5,Integer.parseInt(settings.getProperty("slot","1")))); } catch(NumberFormatException e) { slot=1; }
    }
    private void saveSettings() {
        try {
            if(store==null) return;
            settings.setProperty("swap_ab",Boolean.toString(swap)); settings.setProperty("slot",Integer.toString(slot));
            settings.setProperty("skip_logo",Boolean.toString(skipLogo));
            settings.setProperty("speed_half",Integer.toString(speedHalf));
            settings.setProperty("experience_multiplier",Integer.toString(experienceMultiplier));
            settings.setProperty("gold_multiplier",Integer.toString(goldMultiplier));
            settings.setProperty("max_bonus",Boolean.toString(maxBonus));
            settings.setProperty("no_mp_cost",Boolean.toString(noMpCost));
            settings.setProperty("mixed_party",Boolean.toString(mixedParty));
            ByteArrayOutputStream output=new ByteArrayOutputStream(); settings.store(output,"GBWiz"); saveQueue.write("settings.ini",output.toByteArray());
        } catch(Exception e) { message("設定を保存できません: "+e.getMessage()); }
    }
    private void flush() throws IOException {
        saveBattery();saveQueue.flush();
    }
    private void saveBattery() throws IOException {
        if(core!=0&&store!=null){byte[] data=NativeCore.battery(core);if(data.length>0)saveQueue.write(stem+".sav",data);}
    }
    private void menu() {
        if(picking)return;
        closeMap();
        view.browserItems=null;
        view.spellCharacters=null;
        view.characterMenu=false;selectedCharacter=-1;
        menuOpen=!menuOpen;view.menu=menuOpen;view.slot=slot;view.swap=swap;view.skipLogo=skipLogo;view.speedHalf=speedHalf;
        view.supportMenu=false;
        if(menuOpen)view.menuRow=0;silence();view.invalidate();
    }
    private void resetGame(){
        if(core==0)return;
        closeMap();
        try{flush();NativeCore.reset(core);silence();menuOpen=false;view.menu=false;view.browserItems=null;view.spellCharacters=null;view.characterMenu=false;selectedCharacter=-1;view.invalidate();}
        catch(Exception e){message("保存失敗のためリセットを中止しました");}
    }
    private void activateMenu(int adjust){
        int row=view.menuRow;
        if(view.characterMenu){
            if(row==2&&adjust!=0&&view.characterAlignment>=0)applyAlignment((view.characterAlignment+(adjust>0?1:2))%3);
            if(adjust==0){if(row==0)pickSpellCharacter();else if(row==1)applySpells();}
            view.invalidate();return;
        }
        if(view.supportMenu){
            if(row==0&&adjust==0){view.supportMenu=false;view.menuRow=9;}
            else if((row==1||row==2)&&adjust!=0){
                if(core==0||!(row==1?NativeCore.rewardsSupported(core):NativeCore.goldRewardsSupported(core))){message(row==1?"獲得経験値の倍率は対応版の外伝1〜3・GBC版I〜III・DQ3で使用できます":"獲得お金の倍率は対応版の外伝1〜3・GBC版I〜III・DQ3で使用できます");return;}
                int exp=row==1?nextReward(experienceMultiplier,adjust):experienceMultiplier;
                int gold=row==2?nextReward(goldMultiplier,adjust):goldMultiplier;
                if(NativeCore.setRewards(core,exp,NativeCore.goldRewardsSupported(core)?gold:1)){experienceMultiplier=exp;goldMultiplier=gold;view.experienceMultiplier=exp;view.goldMultiplier=gold;saveSettings();}
            }
            else if(row==3&&adjust!=0){
                if(core!=0&&NativeCore.setMaxBonus(core,!maxBonus)){maxBonus=!maxBonus;view.maxBonus=maxBonus;saveSettings();}
                else message("作成ボーナス最大は対応版の外伝1〜3・GBC版Wizardry1〜3で使用できます");
            }
            else if(row==4&&adjust==0)pickSpellCharacter();
            else if(row==6&&adjust!=0){
                if(core!=0&&NativeCore.setMixedParty(core,!mixedParty)){mixedParty=!mixedParty;view.mixedParty=mixedParty;saveSettings();}
                else message("善悪混合は対応版の外伝1〜3・GBC版Wizardry1〜3で使用できます");
            }
            else if(row==5&&adjust!=0){
                if(core!=0&&NativeCore.setNoMpCost(core,!noMpCost)){noMpCost=!noMpCost;view.noMpCost=noMpCost;saveSettings();}
                else message("MP消費なしは対応版のGB外伝1〜3で使用できます");
            }
            view.invalidate();return;
        }
        if(row==1){if(adjust!=0){slot=(slot-1+(adjust>0?1:4))%5+1;saveSettings();}}
        else if(row==4){swap=!swap;saveSettings();}
        else if(row==7){skipLogo=!skipLogo;if(core!=0)NativeCore.setSkipLogo(core,skipLogo);saveSettings();}
        else if(row==8&&adjust!=0){
            int[] speeds={2,3,4,6,8};int index=0;while(index<speeds.length&&speeds[index]!=speedHalf)index++;
            int next=speeds[(index+(adjust>0?1:4))%speeds.length];
            if(core!=0&&NativeCore.setSpeed(core,next)){speedHalf=next;silence();saveSettings();}
            else message("倍速＋等速音声はGB版外伝1で対応しています");
        }
        else if(adjust==0){
            if(row==0)menu();else if(row==2)state(slot,false);else if(row==3)state(slot,true);
            else if(row==5)pickRom();else if(row==6)resetGame();
            else if(row==9){view.supportMenu=true;view.menuRow=0;}
        }
        view.slot=slot;view.swap=swap;view.skipLogo=skipLogo;view.speedHalf=speedHalf;view.invalidate();
    }
    private void closeMap(){
        if(core!=0&&mapOpen){NativeCore.closeMap(core);mapOpen=false;view.mapMode=false;NativeCore.mapFrame(core,pixels);view.frame(pixels);silence();}
    }
    private void openMap(){
        if(core==0||picking)return;
        if(mapOpen){closeMap();return;}
        if(!NativeCore.openMap(core)){message("マップは対応するウィザードリィの迷宮・キャンプ中に使用できます（戦闘・更新中は不可）");return;}
        menuOpen=false;view.menu=false;view.spellCharacters=null;view.characterMenu=false;view.browserItems=null;
        mapOpen=true;view.mapMode=true;silence();NativeCore.mapFrame(core,pixels);view.frame(pixels);

    }
    private void navigate(int bit){
        if(mapOpen){
            try{
                if(NativeCore.mapKey(core,bit)){
                    if(store==null)throw new IOException("保存先がありません");
                    byte[] backup=NativeCore.state(core);String path=stem+".slot1.state";
                    saveQueue.write(path,backup);saveQueue.flush();
                    if(!java.util.Arrays.equals(backup,store.read(path)))throw new IOException("スロット1保存の確認に失敗しました");
                    if(!NativeCore.moveMapAfterBackup(core))message("現在の状態では移動できません");
                }
            }catch(Exception e){message("移動を中止しました: "+e.getMessage());}
            mapOpen=NativeCore.mapIsOpen(core);view.mapMode=mapOpen;NativeCore.mapFrame(core,pixels);view.frame(pixels);silence();return;
        }
        if(view.spellCharacters!=null){
            int count=view.spellCharacters.length;
            if((bit&64)!=0)view.spellRow=(view.spellRow+count-1)%count;
            else if((bit&128)!=0)view.spellRow=(view.spellRow+1)%count;
            else if((bit&1)!=0)selectCharacter();
            else if((bit&2)!=0){view.spellCharacters=null;view.menuRow=4;}
            view.invalidate();return;
        }
        if(view.browserItems!=null){
            int count=view.browserItems.length;
            if((bit&64)!=0)view.browserRow=(view.browserRow+count-1)%count;
            else if((bit&128)!=0)view.browserRow=(view.browserRow+1)%count;
            else if((bit&1)!=0)activateRom();
            else if((bit&2)!=0){view.browserItems=null;silence();}
            view.invalidate();return;
        }
        if(!menuOpen)return;
        if(view.characterMenu&&(bit&2)!=0){pickSpellCharacter();view.invalidate();return;}
        int rows=view.characterMenu?3:view.supportMenu?7:10;
        if((bit&64)!=0)view.menuRow=(view.menuRow+rows-1)%rows;
        else if((bit&128)!=0)view.menuRow=(view.menuRow+1)%rows;
        else if((bit&16)!=0)activateMenu(1);else if((bit&32)!=0)activateMenu(-1);
        else if((bit&1)!=0)activateMenu(0);else if((bit&2)!=0){if(view.supportMenu){view.supportMenu=false;view.menuRow=9;}else menu();}
        view.invalidate();
    }
    private void pickSpellCharacter(){
        if(core==0||!NativeCore.spellsSupported(core)){message("キャラクターサポートは対応版の外伝1〜3・GBC版I〜IIIで使用できます");return;}
        if(!NativeCore.canLearnSpells(core)){message(NativeCore.learnsMaximumMp(core)?"戦闘中またはデータ更新中は使用できません":"戦闘中やデータ更新中は使用できません。少し再開してから選び直してください");return;}
        String[] names=NativeCore.spellNames(core);List<String> labels=new ArrayList<>();List<Integer> slots=new ArrayList<>();
        labels.add("戻る");slots.add(-1);
        for(int i=0;i<names.length;i++)if(names[i]!=null){labels.add((i+1)+": "+java.text.Normalizer.normalize(names[i],java.text.Normalizer.Form.NFC));slots.add(i);}
        if(labels.size()==1){message("登録キャラクターがいません");return;}
        spellSlots=new int[slots.size()];for(int i=0;i<slots.size();i++)spellSlots[i]=slots.get(i);
        view.characterMenu=false;selectedCharacter=-1;view.spellCharacters=labels.toArray(new String[0]);view.spellRow=0;
    }
    private void selectCharacter(){
        if(view.spellRow==0){view.spellCharacters=null;view.menuRow=4;return;}
        int selected=view.spellRow;if(spellSlots==null||selected<0||selected>=spellSlots.length)return;
        selectedCharacter=spellSlots[selected];view.characterAlignment=NativeCore.alignment(core,selectedCharacter);view.characterName=view.spellCharacters[selected];
        view.spellCharacters=null;view.characterMenu=true;view.menuRow=1;
    }
    private void applySpells(){
        if(core==0||selectedCharacter<0)return;
        try{flush();}catch(Exception e){message("通常セーブの保存に失敗したため適用を中止しました");return;}
        if(!NativeCore.learnSpells(core,selectedCharacter)){message("戦闘中やデータ更新中は適用できません。キャラクターの操作を終えてから選び直してください");return;}
        try{flush();}
        catch(Exception e){message("適用済みですが通常セーブを保存できません。再度保存してください");}
    }
    private void applyAlignment(int alignment){
        if(core==0||selectedCharacter<0)return;
        try{flush();}catch(Exception e){message("通常セーブの保存に失敗したため適用を中止しました");return;}
        if(!NativeCore.setAlignment(core,selectedCharacter,alignment)){message("未対応版・戦闘中・データ更新中は属性を変更できません。操作を終えてから選び直してください");return;}
        view.characterAlignment=NativeCore.alignment(core,selectedCharacter);
        try{flush();}
        catch(Exception e){message("変更済みですが通常セーブを保存できません。再度保存してください");}
    }
    private int rewardSetting(String key){
        try{int value=Integer.parseInt(settings.getProperty(key,"1"));for(int allowed:new int[]{1,8,16,32,64,128,256})if(value==allowed)return value;}catch(NumberFormatException ignored){}
        return 1;
    }
    private static int nextReward(int current,int direction){
        int[] values={1,8,16,32,64,128,256};int index=0;
        while(index<values.length-1&&values[index]!=current)index++;
        return values[(index+(direction>0?1:values.length-1))%values.length];
    }
    private void state(int number,boolean load) {
        if(core==0||store==null) return;
        if(mapOpen)return;
        if(load&&(view.spellCharacters!=null||view.characterMenu)){view.spellCharacters=null;view.characterMenu=false;selectedCharacter=-1;view.menuRow=4;}
        try {
            String path=stem+".slot"+number+".state";
            if(load) { saveQueue.flush();byte[] data=store.read(path); if(data==null) {message("保存がありません");return;} if(!NativeCore.loadState(core,data)) throw new IOException("不正なステートです");
                speedHalf=NativeCore.speed(core);view.speedHalf=speedHalf;saveSettings();silence(); }
            else { saveQueue.write(path,NativeCore.state(core)); flush(); }

        } catch(Exception e) { message("保存・読込失敗: "+e.getMessage()); }
    }
    @Override protected void onResume() { super.onResume(); resumed=true; lastFrame=0; Choreographer.getInstance().postFrameCallback(this); }
    @Override protected void onPause() {
        resumed=false; Choreographer.getInstance().removeFrameCallback(this); silence();
        try { flush(); } catch(Exception e) {message("通常セーブの保存に失敗しました");} super.onPause();
    }
    @Override protected void onDestroy() {notificationHandler.removeCallbacks(clearNotification);if(saveQueue!=null)saveQueue.close();if(inputManager!=null)inputManager.unregisterInputDeviceListener(this);if(core!=0) {NativeCore.close(core);core=0;} if(audio!=null) audio.release(); super.onDestroy(); }
    @Override public void onInputDeviceAdded(int id){}
    @Override public void onInputDeviceChanged(int id){clearInput();}
    @Override public void onInputDeviceRemoved(int id){clearInput();}
    @Override public void doFrame(long now) {
        if(resumed) Choreographer.getInstance().postFrameCallback(this);
        if(paused()||core==0) {lastFrame=0;return;}
        if(lastFrame==0) lastFrame=now-16742706;
        if(now-lastFrame>67000000) lastFrame=now-16742706;
        try {
            while(now-lastFrame>=16742706) {
                int buttons=view.buttons|keyButtons|hatButtons;
                if(swap) buttons=(buttons&~3)|((buttons&1)<<1)|((buttons&2)>>1);
                int count=NativeCore.step(core,buttons,pixels,pcm);
                playAudio(count);
                lastFrame+=16742706;
            }
            view.frame(pixels);
            long time=SystemClock.elapsedRealtime(); if(time-lastSave>10000) {lastSave=time;saveBattery();}
        } catch(Exception e) {message("動作を停止しました: "+e.getMessage()); menu();}
    }
    @Override public void onBackPressed() {if(mapOpen||view.browserItems!=null||view.supportMenu)navigate(2);else menu();}
    @Override public void onWindowFocusChanged(boolean focused){super.onWindowFocusChanged(focused);if(!focused&&view!=null)clearInput();}
    @Override public boolean dispatchKeyEvent(KeyEvent event) {
        if(picking)return super.dispatchKeyEvent(event);
        int key=event.getKeyCode(); boolean down=event.getAction()==KeyEvent.ACTION_DOWN;
        if(down&&event.getRepeatCount()==0) {
            if(mapOpen&&(key==KeyEvent.KEYCODE_PAGE_UP||key==KeyEvent.KEYCODE_BUTTON_L1)){navigate(256);return true;}
            if(mapOpen&&(key==KeyEvent.KEYCODE_PAGE_DOWN||key==KeyEvent.KEYCODE_BUTTON_R1)){navigate(512);return true;}
            if(key==KeyEvent.KEYCODE_BUTTON_R2||key==KeyEvent.KEYCODE_M){openMap();return true;}
            if(key==KeyEvent.KEYCODE_BUTTON_THUMBL||key==KeyEvent.KEYCODE_F1) {menu();return true;}
            if(key==KeyEvent.KEYCODE_BUTTON_THUMBR||key==KeyEvent.KEYCODE_F8) {if(!mapOpen&&!menuOpen)state(1,true);return true;}
            if(key==KeyEvent.KEYCODE_BUTTON_L2||key==KeyEvent.KEYCODE_F5) {if(!mapOpen&&!menuOpen)state(1,false);return true;}
        }
        InputDevice device=event.getDevice();int vendor=device==null?0:device.getVendorId(),product=device==null?0:device.getProductId();
        int value=ControllerMapping.bit(key,vendor,product);
        if(value!=0){
            if(device!=null&&(event.getSource()&InputDevice.SOURCE_GAMEPAD)!=0)
                view.navigationHint=vendor==0x057e&&product==0x2009?"上下:選択  左右:変更  A:決定  B:戻る":"上下:選択  左右:変更  B:決定  A:戻る";
            if(mapOpen||menuOpen||view.browserItems!=null){if(down&&(event.getRepeatCount()==0||(value&240)!=0))navigate(value);keyButtons=0;}
            else if(down)keyButtons|=value;else keyButtons&=~value;
            return true;
        }
        return super.dispatchKeyEvent(event);
    }
    @Override public boolean onGenericMotionEvent(MotionEvent event) {
        if(!picking&&resumed&&(event.getSource()&InputDevice.SOURCE_JOYSTICK)!=0) {
            float x=event.getAxisValue(MotionEvent.AXIS_HAT_X),y=event.getAxisValue(MotionEvent.AXIS_HAT_Y);
            // Wiiz threshold hysteresis: activate at 50%, release at 35%.
            if(Math.abs(x)<.1f)x=event.getAxisValue(MotionEvent.AXIS_X);
            if(Math.abs(y)<.1f)y=event.getAxisValue(MotionEvent.AXIS_Y);
            int previous=hatButtons;
            hatButtons=(x>((previous&16)!=0?.35f:.5f)?16:x< -((previous&32)!=0?.35f:.5f)?32:0)|
                (y< -((previous&64)!=0?.35f:.5f)?64:y>((previous&128)!=0?.35f:.5f)?128:0);
            if((mapOpen||menuOpen||view.browserItems!=null)&&hatButtons!=0&&(hatButtons!=previous||SystemClock.uptimeMillis()-lastAxisNavigation>180)){
                navigate(hatButtons);lastAxisNavigation=SystemClock.uptimeMillis();
            }
            boolean pressed=event.getAxisValue(MotionEvent.AXIS_LTRIGGER)>0.5||event.getAxisValue(MotionEvent.AXIS_BRAKE)>0.5;
            boolean mapPressed=event.getAxisValue(MotionEvent.AXIS_RTRIGGER)>.5||event.getAxisValue(MotionEvent.AXIS_GAS)>.5;
            if(mapPressed&&!mapTrigger)openMap();mapTrigger=mapPressed;
            if(pressed&&!trigger&&!mapOpen&&!menuOpen)state(1,false);trigger=pressed;return true;
        }
        return super.onGenericMotionEvent(event);
    }
}
