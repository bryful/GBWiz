package jp.gbwiz;

/** Keep the unwritten tail when AudioTrack accepts only part of a stereo buffer. */
final class PcmQueue {
    interface Writer {int write(short[] samples,int length);}
    private final short[] pending=new short[8192];
    private int size;
    void clear(){size=0;}
    void offer(short[] samples,int count,Writer writer){
        if(count<0||count>samples.length||count>pending.length)throw new IllegalArgumentException("Invalid PCM size");
        // Bound latency if the output device stalls for more than 125 ms.
        if(size+count>pending.length)size=0;
        System.arraycopy(samples,0,pending,size,count);size+=count;
        int written=writer.write(pending,size);
        if(written<0||written>size)throw new IllegalStateException("音声出力に失敗しました: "+written);
        System.arraycopy(pending,written,pending,0,size-written);size-=written;
    }
}
