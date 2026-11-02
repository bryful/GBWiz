package jp.gbwiz;
import java.util.LinkedHashMap;
import java.util.Map;

/** Wiiz/sample vertical and horizontal layout adapted for the GB 160:144 screen. */
final class ControllerLayout {
    final Map<Integer,int[]> buttons=new LinkedHashMap<>();
    int[] screen;
    private void put(int key,float x,float y,float w,float h) {
        buttons.put(key,new int[]{Math.round(x),Math.round(y),Math.max(1,Math.round(w)),Math.max(1,Math.round(h))});
    }
    ControllerLayout(int width,int height,float density) {
        float w=width,h=height,g=8*density,small=Math.min(40*density,h*.075f);
        if(w<=h) {
            float bottom=Math.min(44*density,h*.09f),minimum=Math.min(w*.16f,44*density);
            float sh=Math.max(1,Math.min(w*.9f,h-2*small-bottom-4*g-3*minimum));
            float sw=sh/0.9f;
            screen=new int[]{Math.round((w-sw)/2),Math.round(small+g),Math.round(sw),Math.round(sh)};
            put(13,w*.375f,0,w*.25f,small);
            put(8,g,0,w*.25f,small); put(9,w-w*.25f-g,0,w*.25f,small);
            float secondary=small+g+sh+g;
            float utility=(w-4*g)/3;
            put(12,g,secondary,utility,small);
            put(10,2*g+utility,secondary,utility,small);
            put(11,3*g+2*utility,secondary,utility,small);
            float controlsTop=secondary+small+g,available=h-bottom-2*g-controlsTop;
            float key=Math.min(Math.min(w*.175f,94*density),available/3.15f);
            float dx=w*.025f,dy=controlsTop+(available-3*key)/2;
            put(6,dx+key,dy,key,key);put(5,dx,dy+key,key,key);
            put(4,dx+2*key,dy+key,key,key);put(7,dx+key,dy+2*key,key,key);
            float diameter=Math.min(Math.min(w*.195f,104*density),available/2.5f);
            float right=w-g-diameter,left=right-diameter-w*.025f,cluster=controlsTop+(available-2.35f*diameter)/2;
            put(2,left,cluster+diameter*.25f,diameter,diameter);put(3,right,cluster,diameter,diameter);
            put(1,left,cluster+diameter*1.35f,diameter,diameter);put(0,right,cluster+diameter*1.1f,diameter,diameter);
        } else {
            float sw=Math.min(w*.5f,(h-2*g)/.9f),sh=sw*.9f,side=(w-sw)/2;
            screen=new int[]{Math.round(side),Math.round((h-sh)/2),Math.round(sw),Math.round(sh)};
            float bw=(side-3*g)/2;
            put(8,g,g,bw,small);put(9,2*g+bw,g,bw,small);
            put(13,g,small+3*g,bw,small);
            put(10,2*g+bw,small+3*g,bw,small);
            float key=Math.min(Math.min(Math.min(side*.32f,94*density),h*.198f),(side-g)/3);
            float dx=(side-3*key)/2,dy=Math.min(Math.max(2*small+4*g,h*.35f),h-g-3*key);
            put(6,dx+key,dy,key,key);put(5,dx,dy+key,key,key);put(4,dx+2*key,dy+key,key,key);put(7,dx+key,dy+2*key,key,key);
            float diameter=Math.min(side*.41f,h*.21f),rs=side+sw,left=rs+side*.05f,right=rs+side*.52f,cluster=h*.34f;
            put(2,left,cluster+h*.05f,diameter,diameter);put(3,right,cluster,diameter,diameter);
            put(1,left,cluster+h*.29f,diameter,diameter);put(0,right,cluster+h*.24f,diameter,diameter);
            put(11,rs+g,g,side-2*g,small);
            put(12,rs+g,small+3*g,side-2*g,small);
        }
    }
}
