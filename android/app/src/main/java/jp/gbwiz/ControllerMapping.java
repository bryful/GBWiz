package jp.gbwiz;
import android.view.KeyEvent;
final class ControllerMapping {
    // Wiiz's Android Switch Pro exception: Android reports Nintendo labels,
    // whereas Xbox reports south=A/east=B. Bind the same physical NES positions.
    static int bit(int key,int vendor,int product) { return bit(key,vendor,product,false); }
    static int bit(int key,int vendor,int product,boolean swapXY) {
        boolean pro=vendor==0x057e&&product==0x2009;
        if(key==KeyEvent.KEYCODE_BUTTON_A)return pro?1:2;
        if(key==KeyEvent.KEYCODE_BUTTON_B)return pro?2:1;
        if(key==KeyEvent.KEYCODE_BUTTON_X)return swapXY?4:8;
        if(key==KeyEvent.KEYCODE_BUTTON_Y)return swapXY?8:4;
        switch(key) {
            case KeyEvent.KEYCODE_X:return 1;case KeyEvent.KEYCODE_Z:return 2;
            case KeyEvent.KEYCODE_BUTTON_SELECT:return 4;
            case KeyEvent.KEYCODE_BUTTON_START:case KeyEvent.KEYCODE_ENTER:return 8;
            case KeyEvent.KEYCODE_DPAD_RIGHT:return 16;case KeyEvent.KEYCODE_DPAD_LEFT:return 32;
            case KeyEvent.KEYCODE_DPAD_UP:return 64;case KeyEvent.KEYCODE_DPAD_DOWN:return 128;
        }return 0;
    }
}
