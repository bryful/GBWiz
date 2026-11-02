package jp.gbwiz;

import android.content.ContentResolver;
import android.database.Cursor;
import android.net.Uri;
import android.provider.DocumentsContract;
import java.io.*;
import java.util.*;

final class DocumentStore {
    private final ContentResolver resolver;
    private final Uri tree;
    private final Uri root,saveFolder,stateFolder,legacyRoot;
    final Uri romFolder;
    DocumentStore(ContentResolver resolver, Uri tree) throws IOException {
        this.resolver=resolver;this.tree=tree;
        Uri selected=DocumentsContract.buildDocumentUriUsingTree(tree,DocumentsContract.getTreeDocumentId(tree));
        legacyRoot=selected;
        String name=null;
        try(Cursor c=resolver.query(selected,new String[]{DocumentsContract.Document.COLUMN_DISPLAY_NAME},null,null,null)) {
            if(c!=null&&c.moveToFirst())name=c.getString(0);
        }
        root="GBWiz".equalsIgnoreCase(name)?selected:folder(selected,"GBWiz");
        saveFolder=folder(root,"Save");stateFolder=folder(root,"State");romFolder=folder(root,"rom");
    }
    private Uri folder(Uri parent,String name) throws IOException {
        Uri found=find(parent,name);
        if(found==null)found=DocumentsContract.createDocument(resolver,parent,DocumentsContract.Document.MIME_TYPE_DIR,name);
        if(found==null)throw new IOException("フォルダを作成できません: "+name);
        try(Cursor c=resolver.query(found,new String[]{DocumentsContract.Document.COLUMN_MIME_TYPE},null,null,null)) {
            if(c==null||!c.moveToFirst()||!DocumentsContract.Document.MIME_TYPE_DIR.equals(c.getString(0)))
                throw new IOException("同名のファイルがあるためフォルダを作成できません: "+name);
        }
        return found;
    }
    private Uri target(String name) {
        if(name.endsWith(".sav")||name.endsWith(".sav.bak"))return saveFolder;
        if(name.endsWith(".state")||name.endsWith(".state.bak"))return stateFolder;
        return root;
    }
    private Uri find(Uri parentUri,String name) throws IOException {
        String parent=DocumentsContract.getDocumentId(parentUri);
        Uri children=DocumentsContract.buildChildDocumentsUriUsingTree(tree,parent);
        try(Cursor cursor=resolver.query(children,new String[]{DocumentsContract.Document.COLUMN_DOCUMENT_ID,
                DocumentsContract.Document.COLUMN_DISPLAY_NAME},null,null,null)) {
            if(cursor==null) throw new IOException("保存フォルダを開けません");
            while(cursor.moveToNext()) if(name.equals(cursor.getString(1)))
                return DocumentsContract.buildDocumentUriUsingTree(tree,cursor.getString(0));
        }
        return null;
    }
    static byte[] read(ContentResolver resolver,Uri uri,int limit) throws IOException {
        try(InputStream input=resolver.openInputStream(uri); ByteArrayOutputStream output=new ByteArrayOutputStream()) {
            if(input==null) throw new IOException("ファイルを開けません");
            byte[] buffer=new byte[16384]; int count;
            while((count=input.read(buffer))!=-1) {
                if(output.size()+count>limit) throw new IOException("ファイルが大きすぎます");
                output.write(buffer,0,count);
            }
            return output.toByteArray();
        }
    }
    byte[] read(String name) throws IOException {
        Uri uri=find(target(name),name);
        // Read older flat saves without deleting or silently renaming them.
        if(uri==null&&!target(name).equals(root))uri=find(root,name);
        if(uri==null&&!legacyRoot.equals(root))uri=find(legacyRoot,name);
        return uri==null?null:read(resolver,uri,16*1024*1024);
    }
    private void put(String name,byte[] data) throws IOException {
        Uri uri=find(target(name),name);
        if(uri==null) uri=DocumentsContract.createDocument(resolver,
            target(name),
            "application/octet-stream",name);
        if(uri==null) throw new IOException("保存ファイルを作成できません");
        try(OutputStream output=resolver.openOutputStream(uri,"wt")) {
            if(output==null) throw new IOException("保存ファイルを開けません");
            output.write(data); output.flush();
        }
    }
    static final class Rom {
        final String name;final Uri uri;
        Rom(String name,Uri uri){this.name=name;this.uri=uri;}
    }
    static List<Rom> roms(ContentResolver resolver,Uri folder)throws IOException {
        String id;
        try{id=DocumentsContract.getDocumentId(folder);}catch(IllegalArgumentException e){id=DocumentsContract.getTreeDocumentId(folder);}
        Uri children=DocumentsContract.buildChildDocumentsUriUsingTree(folder,id);
        List<Rom> result=new ArrayList<>();
        try(Cursor c=resolver.query(children,new String[]{DocumentsContract.Document.COLUMN_DOCUMENT_ID,DocumentsContract.Document.COLUMN_DISPLAY_NAME,DocumentsContract.Document.COLUMN_MIME_TYPE},null,null,null)) {
            if(c==null)throw new IOException("ROMフォルダを開けません");
            while(c.moveToNext()) {
                String name=c.getString(1),lower=name.toLowerCase(Locale.ROOT);
                if(!DocumentsContract.Document.MIME_TYPE_DIR.equals(c.getString(2))&&(lower.endsWith(".gb")||lower.endsWith(".gbc")))
                    result.add(new Rom(name,DocumentsContract.buildDocumentUriUsingTree(folder,c.getString(0))));
            }
        }
        result.sort((a,b)->a.name.compareToIgnoreCase(b.name));return result;
    }
    void write(String name,byte[] data) throws IOException {
        byte[] previous=read(name);
        if(previous!=null) put(name+".bak",previous);
        try { put(name,data); }
        catch(IOException failure) {
            if(previous!=null) try { put(name,previous); } catch(IOException ignored) {}
            throw failure;
        }
    }
}
