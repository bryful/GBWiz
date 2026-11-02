package jp.gbwiz;
import android.test.InstrumentationTestCase;
import java.io.IOException;
import java.util.*;
import java.util.concurrent.*;
import java.util.concurrent.atomic.*;

public final class StreamingTest extends InstrumentationTestCase {
    public void testSaveQueueCopiesCoalescesAndSerializes()throws Exception{
        CountDownLatch entered=new CountDownLatch(1),release=new CountDownLatch(1);
        List<String> writes=Collections.synchronizedList(new ArrayList<>());
        Thread caller=Thread.currentThread();AtomicReference<String> error=new AtomicReference<>();
        try(SaveQueue queue=new SaveQueue((path,data)->{
            assertNotSame("Storage ran on caller thread",caller,Thread.currentThread());
            if(entered.getCount()!=0){entered.countDown();assertTrue(release.await(3,TimeUnit.SECONDS));}
            writes.add(path+":"+data[0]);
        },error::set)){
            byte[] first={0};queue.write("a.sav",first);assertTrue(entered.await(3,TimeUnit.SECONDS));first[0]=99;
            // These writes must return while storage is still deliberately blocked.
            queue.write("a.sav",new byte[]{1});queue.write("a.sav",new byte[]{2});queue.write("b.sav",new byte[]{4});
            release.countDown();queue.flush();
            assertEquals(Arrays.asList("a.sav:0","a.sav:2","b.sav:4"),writes);assertNull(error.get());
        }finally{release.countDown();}
    }
    public void testFailedSaveCanRetryLatestSnapshot()throws Exception{
        AtomicBoolean unavailable=new AtomicBoolean(true);AtomicInteger saved=new AtomicInteger(-1);
        AtomicReference<String> error=new AtomicReference<>();
        try(SaveQueue queue=new SaveQueue((path,data)->{if(unavailable.get())throw new IOException("offline");saved.set(data[0]);},error::set)){
            queue.write("a.sav",new byte[]{1});boolean failed=false;
            try{queue.flush();}catch(IOException expected){failed=true;}
            assertTrue("Failed save was reported as successful",failed);assertNotNull(queue.failure());
            unavailable.set(false);queue.write("a.sav",new byte[]{2});queue.flush();
            assertEquals("Retry wrote stale data",2,saved.get());assertNull(queue.failure());
        }
        SaveQueue closed=new SaveQueue((path,data)->{},text->{});closed.close();
        try{closed.write("closed",new byte[]{0});fail("Closed queue accepted a write");}catch(IOException expected){}
    }
    public void testPartialAudioWritesKeepStereoOrder(){
        PcmQueue queue=new PcmQueue();List<Short> heard=new ArrayList<>();
        PcmQueue.Writer writer=(samples,count)->{int used=Math.min(count,2);for(int i=0;i<used;i++)heard.add(samples[i]);return used;};
        short[] first={1,2,3,4};queue.offer(first,4,writer);Arrays.fill(first,(short)99);
        queue.offer(new short[]{5,6},2,writer);queue.offer(new short[0],0,writer);
        assertEquals(Arrays.asList((short)1,(short)2,(short)3,(short)4,(short)5,(short)6),heard);
        queue.offer(new short[]{7,8},2,(samples,count)->0);queue.clear();
        heard.clear();queue.offer(new short[]{9,10},2,writer);
        assertEquals("Pause retained old audio",Arrays.asList((short)9,(short)10),heard);
        try{queue.offer(new short[]{1,2},2,(samples,count)->-1);fail("Audio error ignored");}catch(IllegalStateException expected){}
    }
}
