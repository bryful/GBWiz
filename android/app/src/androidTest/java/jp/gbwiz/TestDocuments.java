package jp.gbwiz;
import android.content.ContentProvider;
import android.content.ContentValues;
import android.net.Uri;
import android.provider.DocumentsContract;
import android.provider.DocumentsContract.Document;
import android.database.*;
import android.os.*;
import java.io.*;

/** Test APK only: documents backed by its private cache, never user storage. */
public final class TestDocuments extends ContentProvider {
    private File base;
    @Override public boolean onCreate(){base=new File(getContext().getCacheDir(),"test-documents");return base.mkdirs()||base.isDirectory();}
    private File file(String id)throws FileNotFoundException {
        if(!id.startsWith("root")||id.contains(".."))throw new FileNotFoundException();
        File f=new File(base,id);if(id.equals("root"))f.mkdirs();return f;
    }
    private void row(MatrixCursor c,String id)throws FileNotFoundException{
        File f=file(id);if(!f.exists())throw new FileNotFoundException(id);
        MatrixCursor.RowBuilder row=c.newRow();
        for(String column:c.getColumnNames()){
            Object value=null;
            if(column.equals(Document.COLUMN_DOCUMENT_ID))value=id;
            else if(column.equals(Document.COLUMN_DISPLAY_NAME))value=id.equals("root")?"Documents":f.getName();
            else if(column.equals(Document.COLUMN_MIME_TYPE))value=f.isDirectory()?Document.MIME_TYPE_DIR:"application/octet-stream";
            else if(column.equals(Document.COLUMN_FLAGS))value=f.isDirectory()?Document.FLAG_DIR_SUPPORTS_CREATE:Document.FLAG_SUPPORTS_WRITE;
            else if(column.equals(Document.COLUMN_SIZE))value=f.length();row.add(value);
        }
    }
    public Cursor queryDocument(String id,String[] projection)throws FileNotFoundException{
        MatrixCursor c=new MatrixCursor(projection==null?new String[]{Document.COLUMN_DOCUMENT_ID,Document.COLUMN_DISPLAY_NAME,Document.COLUMN_MIME_TYPE,Document.COLUMN_FLAGS}:projection);row(c,id);return c;
    }
    public Cursor queryChildDocuments(String parent,String[] projection,String sort)throws FileNotFoundException{
        MatrixCursor c=new MatrixCursor(projection==null?new String[]{Document.COLUMN_DOCUMENT_ID,Document.COLUMN_DISPLAY_NAME,Document.COLUMN_MIME_TYPE}:projection);
        File[] files=file(parent).listFiles();if(files!=null)for(File f:files)row(c,parent+"/"+f.getName());return c;
    }
    public String createDocument(String parent,String mime,String name)throws FileNotFoundException{
        String id=parent+"/"+name;File f=file(id);
        try{if(Document.MIME_TYPE_DIR.equals(mime))f.mkdirs();else f.createNewFile();}catch(IOException e){throw new FileNotFoundException(e.getMessage());}return id;
    }
    @Override public ParcelFileDescriptor openFile(Uri uri,String mode)throws FileNotFoundException{return ParcelFileDescriptor.open(file(DocumentsContract.getDocumentId(uri)),ParcelFileDescriptor.parseMode(mode));}
    @Override public Cursor query(Uri uri,String[] projection,String selection,String[] args,String sort){
        try{return uri.getLastPathSegment().equals("children")?queryChildDocuments(DocumentsContract.getDocumentId(uri),projection,sort):queryDocument(DocumentsContract.getDocumentId(uri),projection);}
        catch(FileNotFoundException e){return null;}
    }
    @Override public Bundle call(String method,String arg,Bundle extras){
        if(!method.equals("android:createDocument"))throw new UnsupportedOperationException(method);
        Uri parent=extras.getParcelable("uri");
        try{
            String id=createDocument(DocumentsContract.getDocumentId(parent),extras.getString(Document.COLUMN_MIME_TYPE),extras.getString(Document.COLUMN_DISPLAY_NAME));
            Bundle result=new Bundle();result.putParcelable("uri",DocumentsContract.buildDocumentUriUsingTree(parent,id));return result;
        }catch(FileNotFoundException e){throw new IllegalArgumentException(e);}
    }
    @Override public String getType(Uri uri){return "application/octet-stream";}
    @Override public Uri insert(Uri uri,ContentValues values){throw new UnsupportedOperationException();}
    @Override public int update(Uri uri,ContentValues values,String selection,String[] args){throw new UnsupportedOperationException();}
    @Override public int delete(Uri uri,String selection,String[] args){throw new UnsupportedOperationException();}
}
