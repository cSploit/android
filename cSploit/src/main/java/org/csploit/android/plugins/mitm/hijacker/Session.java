package org.csploit.android.plugins.mitm.hijacker;

import android.graphics.Bitmap;

import org.csploit.android.core.System;
import org.csploit.android.net.http.RequestParser;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.net.HttpCookie;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.zip.GZIPInputStream;
import java.util.zip.GZIPOutputStream;

public class Session
{
  public Bitmap mPicture = null;
  public String mUserName = null;
  public boolean mInited = false;
  public boolean mHTTPS = false;
  public String mAddress = "";
  public String mDomain = "";
  public String mUserAgent = "";
  public HashMap<String, HttpCookie> mCookies = null;

  public Session(){
    mCookies = new HashMap<String, HttpCookie>();
  }

  public String getFileName(){
    String name = mDomain + "-" + (mUserName != null ? mUserName : mAddress);
    return name.replaceAll("[ .\\\\/:*?\"<>|\\\\/:*?\"<>|]", "-");
  }

  public String save(String sessionName) throws IOException {
    StringBuilder builder = new StringBuilder();
    String filename = System.getStoragePath() + '/' + sessionName + ".dhs",
           buffer = null;

    builder.append( System.SESSION_MAGIC + "\n");

    builder.append(mUserName == null ? "null" : mUserName).append("\n");
    builder.append(mHTTPS).append("\n");
    builder.append(mAddress).append("\n");
    builder.append(mDomain).append("\n");
    builder.append(mUserAgent).append("\n");
    builder.append(mCookies.size()).append("\n");

    for(HttpCookie cookie : mCookies.values()){
      builder
        .append(cookie.getName()).append( "=" ).append(cookie.getValue())
        .append( "; domain=").append(cookie.getDomain())
        .append( "; path=/" )
        .append( mHTTPS ? ";secure" : "" )
        .append("\n");
    }

    buffer = builder.toString();

    FileOutputStream ostream = new FileOutputStream(filename);
    GZIPOutputStream gzip = new GZIPOutputStream(ostream);

    gzip.write(buffer.getBytes());
    gzip.close();

    return filename;
  }

  private static String decodeLine( BufferedReader reader ) throws IOException {
    String line = reader.readLine();

    return line.equals("null") ? null : line;
  }

  private static boolean decodeBoolean( BufferedReader reader ) throws IOException {
    String line = decodeLine(reader);
    try
    {
      return Boolean.parseBoolean(line);
    }
    catch( Exception e )
    {

    }

    return false;
  }

  private static int decodeInteger( BufferedReader reader ) throws IOException {
    String line = decodeLine(reader);
    try
    {
      return Integer.parseInt(line);
    }
    catch( Exception e )
    {

    }

    return 0;
  }

  public static Session load(String filename) throws Exception{
    Session session;
    File file = new File( System.getStoragePath() + '/' + filename);

    if(file.exists() && file.length() > 0){
      BufferedReader reader = new BufferedReader(new InputStreamReader(new GZIPInputStream(new FileInputStream(file))));
      String line;

      // begin decoding procedure
      try
      {
        line = reader.readLine();
        if(line == null || !line.equals(System.SESSION_MAGIC))
          throw new Exception("Not a cSploit hijacker session file.");

        session = new Session();

        session.mUserName  = decodeLine( reader );
        session.mHTTPS     = decodeBoolean( reader );
        session.mAddress   = decodeLine( reader );
        session.mDomain    = decodeLine( reader );
        session.mUserAgent = decodeLine( reader );

        for( int i = 0, ncookies = decodeInteger( reader ); i < ncookies; i++ ) {
          ArrayList<HttpCookie> cookies = RequestParser.parseRawCookie(reader.readLine());
          for (HttpCookie cookie : cookies) {
            session.mCookies.put(cookie.getName(), cookie);
          }
        }

        reader.close();
      }
      catch(Exception e){
        reader.close();
        throw e;
      }
    }
    else
      throw new Exception(filename + " does not exists or is empty.");

    return session;
  }
}
