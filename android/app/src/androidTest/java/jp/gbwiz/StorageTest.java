package jp.gbwiz;
import android.test.InstrumentationTestCase;
import android.content.ContentResolver;
import android.net.Uri;
import android.provider.DocumentsContract;
import java.io.*;
import java.util.Arrays;

public final class StorageTest extends InstrumentationTestCase {
    public void testFoldersLegacySavesAndRomSelection()throws Exception{
        ContentResolver resolver=getInstrumentation().getTargetContext().getContentResolver();
        Uri tree=DocumentsContract.buildTreeDocumentUri(getInstrumentation().getContext().getPackageName()+".documents","root");
        DocumentStore store=new DocumentStore(resolver,tree);
        byte[] payload={7,8,9};String unique="test"+System.nanoTime();
        store.write(unique+".sav",payload);store.write(unique+".slot1.state",payload);store.write("settings.ini",payload);
        for(String file:new String[]{"Save/"+unique+".sav","State/"+unique+".slot1.state","settings.ini"}){
            Uri uri=DocumentsContract.buildDocumentUriUsingTree(tree,"root/GBWiz/"+file);
            assertTrue(Arrays.equals(payload,DocumentStore.read(resolver,uri,100)));
        }
        Uri root=DocumentsContract.buildDocumentUriUsingTree(tree,"root/GBWiz");
        Uri legacy=DocumentsContract.createDocument(resolver,root,"application/octet-stream",unique+"Legacy.sav");
        try(OutputStream out=resolver.openOutputStream(legacy,"wt")){out.write(payload);}
        assertTrue(Arrays.equals(payload,store.read(unique+"Legacy.sav")));
        store.write(unique+"Legacy.sav",payload);
        assertTrue(Arrays.equals(payload,DocumentStore.read(resolver,legacy,100)));
        Uri previousRoot=DocumentsContract.buildDocumentUriUsingTree(tree,"root");
        Uri flat=DocumentsContract.createDocument(resolver,previousRoot,"application/octet-stream",unique+"OldRoot.sav");
        try(OutputStream out=resolver.openOutputStream(flat,"wt")){out.write(payload);}
        assertTrue(Arrays.equals(payload,store.read(unique+"OldRoot.sav")));
        Uri rom=DocumentsContract.createDocument(resolver,store.romFolder,"application/octet-stream",unique+".gbc");
        assertTrue(DocumentStore.roms(resolver,store.romFolder).stream().anyMatch(r->r.uri.equals(rom)));
        Uri custom=DocumentsContract.createDocument(resolver,root,DocumentsContract.Document.MIME_TYPE_DIR,unique+"Custom");
        Uri other=DocumentsContract.createDocument(resolver,custom,"application/octet-stream","other.gb");
        assertEquals(other,DocumentStore.roms(resolver,custom).get(0).uri);
    }
}
