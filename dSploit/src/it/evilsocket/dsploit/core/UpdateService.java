/*
 * This file is part of the dSploit.
 *
 * Copyleft of Simone Margaritelli aka evilsocket <evilsocket@gmail.com>
 *
 * dSploit is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * dSploit is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with dSploit.  If not, see <http://www.gnu.org/licenses/>.
 */
package it.evilsocket.dsploit.core;

import android.app.IntentService;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.Uri;
import android.support.v4.app.NotificationCompat;

import org.apache.commons.compress.compressors.bzip2.BZip2CompressorInputStream;
import org.apache.commons.compress.compressors.gzip.GzipCompressorInputStream;
import org.apache.commons.compress.compressors.xz.XZCompressorInputStream;
import org.apache.commons.compress.utils.CountingInputStream;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.security.KeyException;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.concurrent.CancellationException;

import it.evilsocket.dsploit.R;

public class UpdateService extends IntentService
{
  // Resources defines
  private static final String REMOTE_VERSION_URL = "http://update.dsploit.net/version";
  private static final String REMOTE_APK_URL = "http://update.dsploit.net/apk";
  private static final String VERSION_CHAR_MAP = "zyxwvutsrqponmlkjihgfedcba";
  private static final String REMOTE_IMAGE_URL = "http://update.dsploit.net/gentoo_msf.tar.xz";
  private static final String REMOTE_IMAGE_NAME = "gentoo_msf.tar.xz";
  private static final String REMOTE_IMAGE_MD5 = "4e8158c974650970b8add11624a7d68b";
  private static final String REMOTE_IMAGE_SHA1 = "250170ccc11debfd382077b283849d6e80b0ff1a";

  // Intent defines
  public static final String START    = "UpdateService.action.START";
  public static final String ERROR    = "UpdateService.action.ERROR";
  public static final String DONE     = "UpdateService.action.DONE";
  public static final String ACTION   = "UpdateService.data.ACTION";
  public static final String MESSAGE  = "UpdateService.data.MESSAGE";

  // notification defines
  private static final int NOTIFICATION_ID = 1;
  private static final int DOWNLOAD_COMPLETE_CODE = 1;
  private static final int CANCEL_CODE = 2;
  private static final String NOTIFICATION_CANCELLED = "it.evilsocket.dSploit.core.UpdateService.CANCELLED";

  private String  mRemoteUrl        = null,
                  mLocalFile        = null,
                  mDestinationDir   = null,
                  md5sum            = null,
                  sha1sum           = null,
                  mRemoteVersion    = null,
                  mInstalledVersion = null;
  private boolean mRunning          = false,
                  mHoldNotification = false;
  compressionAlgorithm mAlgorithm = compressionAlgorithm.none;

  private NotificationManager mNotificationManager = null;
  private NotificationCompat.Builder mBuilder = null;
  private BroadcastReceiver mReceiver = null;

  public enum action {
    apk_update,
    gentoo_update
  }

  private enum compressionAlgorithm {
    none,
    gzip,
    bzip,
    xz
  }

  public UpdateService(){
    super("UpdateService");
    mInstalledVersion = System.getAppVersionName();
  }

  private static double getVersionCode(String version){
    double code = 0,
           multipliers[] = { 1000, 100, 1 };
    String parts[] = version.split( "[^0-9a-zA-Z]", 3 ),
           item, digit, letter;
    int i, j;
    char c;

    for( i = 0; i < 3; i++ )
    {
      item = parts[i];

      if( item.matches("\\d+[a-zA-Z]")){
        digit = "";
        letter = "";

        for(j = 0; j < item.length(); j++){
          c = item.charAt(j);
          if(c >= '0' && c <= '9')
            digit += c;
          else
            letter += c;
        }

        code += multipliers[i] * ( Integer.parseInt(digit) + 1 ) - ((VERSION_CHAR_MAP.indexOf(letter.toLowerCase()) + 1) / 100.0);
      }
      else if(item.matches("\\d+"))
        code += multipliers[i] * ( Integer.parseInt(item) + 1 );

      else
        code += multipliers[i];
    }

    return code;
  }

  public boolean isUpdateAvailable(){

    try{
      if(mInstalledVersion != null){
        // Read remote version
        if(mRemoteVersion == null){
          URL url = new URL(REMOTE_VERSION_URL);
          HttpURLConnection connection = (HttpURLConnection) url.openConnection();
          BufferedReader reader = new BufferedReader(new InputStreamReader(connection.getInputStream()));
          String line,
            buffer = "";

          while((line = reader.readLine()) != null){
            buffer += line + "\n";
          }

          reader.close();

          mRemoteVersion = buffer.trim();
        }

        // Compare versions
        double installedVersionCode = getVersionCode(mInstalledVersion),
          remoteVersionCode = getVersionCode(mRemoteVersion);

        Logger.debug( "mInstalledVersion = " + mInstalledVersion + " ( " + getVersionCode(mInstalledVersion)+ " ) " );
        Logger.debug( "mRemoteVersion    = " + mRemoteVersion + " ( " + getVersionCode(mRemoteVersion)+ " ) " );

        if(remoteVersionCode > installedVersionCode)
          return true;
      }
    } catch(Exception e){
      System.errorLogging(e);
    }

    return false;
  }

  public String getRemoteVersion(){
    return mRemoteVersion;
  }

  /**
   * is gentoo image available?
   * @return true if the image can be downloaded, false otherwise
   */
  public boolean isGentooAvailable() {
    try {
      URL url = new URL(REMOTE_IMAGE_URL);
      HttpURLConnection connection = (HttpURLConnection) url.openConnection();
      connection.connect();

      int retCode = connection.getResponseCode();

      connection.disconnect();

      return retCode == 200;
    } catch(Exception e){
      System.errorLogging(e);
    }
    return false;
  }

  /**
   * notify activities that some error occurred
   * @param message error message
   */
  private void sendError(int message) {
    Intent i = new Intent(ERROR);
    i.putExtra(MESSAGE,message);
    sendBroadcast(i);
  }

  /**
   * notify activities that we finished our job
   * @param a the performed action
   */
  private void sendDone(action a) {
    Intent i = new Intent(DONE);
    i.putExtra(ACTION,a);
    sendBroadcast(i);
  }

  private String digest2string(byte[] digest) {
    StringBuilder sb = new StringBuilder();
    for (byte b : digest) {
      sb.append(String.format("%02x", b));
    }
    return sb.toString();
  }

  /**
   * open a compressed InputStream
   * @param in the InputStream to decompress
   * @return the InputStream to read from
   * @throws IOException
   */
  private InputStream openCompressedStream(InputStream in) throws IOException {
    switch(mAlgorithm) {
      default:
      case none:
        return in;
      case gzip:
        return new GzipCompressorInputStream(in);
      case bzip:
        return new BZip2CompressorInputStream(in);
      case xz:
        return new XZCompressorInputStream(in);
    }
  }

  /**
   * wipe the mDestinationDir ( rm -rf )
   */
  private void wipe() {
    try {
      Shell.exec("rm -rf '"+mDestinationDir+"'");
    } catch (Exception e) {
      System.errorLogging(e);
    }
  }

  /**
   * connect to the notification manager and create a notification builder.
   * it also setup the cancellation Intent for get notified when our notification got cancelled.
   */
  private void setupNotification() {
    // get notification manager
    mNotificationManager = (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);
    // get notification builder
    mBuilder = new NotificationCompat.Builder(this);
    // create a broadcast receiver to get actions
    // performed on the notification by the user
    mReceiver = new BroadcastReceiver() {
      @Override
      public void onReceive(Context context, Intent intent) {
        String action = intent.getAction();
        if(action==null)
          return;
        // user cancelled our notification
        if(action.equals(NOTIFICATION_CANCELLED)) {
          mRunning=false;
          stopSelf();
        }
      }
    };
    // register our receiver
    registerReceiver(mReceiver,new IntentFilter(NOTIFICATION_CANCELLED));
    // set common notification actions
    mBuilder.setDeleteIntent(PendingIntent.getBroadcast(this, CANCEL_CODE, new Intent(NOTIFICATION_CANCELLED), 0));
    // will leave our notification on exit?
    mHoldNotification=false;
  }

  /**
   * delete our notification if mHoldNotification is false
   */
  private void deleteNotification() {
    if(mHoldNotification)
      return;
    Logger.debug("deleting notifications");
    if(mNotificationManager!=null) {
      mNotificationManager.cancel(NOTIFICATION_ID);
      mNotificationManager = null;
    }
    if(mReceiver!=null) {
      unregisterReceiver(mReceiver);
      mReceiver = null;
    }
  }

  private void setNotificationActions() {
    if(mLocalFile.endsWith("apk")){
      Intent intent = new Intent(Intent.ACTION_VIEW);
      intent.setDataAndType(Uri.fromFile(new File(mLocalFile)),"application/vnd.android.package-archive");
      intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);

      mBuilder.setSmallIcon(android.R.drawable.stat_sys_download_done)
              .setContentTitle(getString(R.string.update_available))
              .setContentText(getString(R.string.click_here_to_upgrade))
              .setProgress(0, 0, false) // remove progress bar
              .setAutoCancel(true)
              .setContentIntent(PendingIntent.getActivity(this, DOWNLOAD_COMPLETE_CODE, intent, 0));
      mNotificationManager.notify(NOTIFICATION_ID, mBuilder.build());
      mHoldNotification=true;
    }
  }

  /**
   * download mRemoteUrl to mLocalFile.
   *
   * @throws KeyException when MD5 or SHA1 sum fails
   * @throws IOException when IOError occurs
   * @throws NoSuchAlgorithmException when required digest algorithm is not available
   * @throws CancellationException when user cancelled the download via notification
   */

  private void downloadFile() throws KeyException, IOException, NoSuchAlgorithmException, CancellationException {
    if(mRemoteUrl==null||mLocalFile==null)
      return;

    FileOutputStream writer = null;
    InputStream reader = null;

    try
    {
      MessageDigest md5, sha1;
      URL url;
      HttpURLConnection connection;
      byte[] buffer;
      int read;
      String digest;
      File file;
      short percentage,previous_percentage;
      long downloaded,total;

      mBuilder.setContentTitle(getString(R.string.downloading_update))
              .setSmallIcon(android.R.drawable.stat_sys_download)
              .setProgress(100, 0, false);


      md5 = (md5sum!=null  ? MessageDigest.getInstance("MD5") : null);
      sha1= (sha1sum!=null ? MessageDigest.getInstance("SHA-1") : null);

      // checks if file already exists
      // only if we have md5sum and sha1sum
      //noinspection ResultOfMethodCallIgnored
      file = new File(mLocalFile);
      buffer = new byte[4096];
      total= file.length();
      downloaded=0;
      previous_percentage=-1;
      mBuilder.setContentText(getString(R.string.checking));

      if(file.exists()) {
        if(md5!=null && sha1!=null && file.isFile()) {
          reader = new FileInputStream(file);
          while(mRunning && (read = reader.read(buffer))!=-1) {
            md5.update(buffer,0,read);
            sha1.update(buffer,0,read);

            downloaded+=read;

            percentage=(short)(((double)downloaded/total)*100);
            if(percentage!=previous_percentage) {
              mBuilder.setProgress(100,percentage,false)
                      .setContentInfo(percentage+"%");
              mNotificationManager.notify(NOTIFICATION_ID,mBuilder.build());
              previous_percentage=percentage;
            }
          }
          reader.close();
          reader=null;
          if(!mRunning)
            throw new CancellationException("download cancelled while checking local file");
          if(md5sum.equals(digest2string(md5.digest())) && sha1sum.equals(digest2string(sha1.digest()))) {
            Logger.warning("file already exists: "+mLocalFile);
            setNotificationActions();
            return;
          }
        }
        //noinspection ResultOfMethodCallIgnored
        file.delete();
      }

      mBuilder.setContentText(getString(R.string.connecting))
              .setProgress(100,0,true);
      mNotificationManager.notify(NOTIFICATION_ID,mBuilder.build());

      HttpURLConnection.setFollowRedirects(true);
      url = new URL(mRemoteUrl);
      connection = (HttpURLConnection) url.openConnection();

      connection.connect();

      writer = new FileOutputStream(file);
      reader = connection.getInputStream();

      total = connection.getContentLength();

      downloaded=0;
      previous_percentage=-1;

      mBuilder.setContentText("");

      Logger.debug(String.format("downloading '%s' to '%s'",mRemoteUrl,mLocalFile));

      while( mRunning && (read = reader.read(buffer)) != -1 ) {
        writer.write(buffer, 0, read);
        if(md5!=null)
          md5.update(buffer, 0, read);
        if(sha1!=null)
          sha1.update(buffer, 0, read);

        downloaded+=read;

        percentage = (short)(((double)downloaded/ total) * 100);

        if(percentage!=previous_percentage) {
          mBuilder.setProgress(100,percentage,false)
                  .setContentInfo(percentage + "%");
          mNotificationManager.notify(NOTIFICATION_ID, mBuilder.build());
          previous_percentage=percentage;
        }
      }

      writer.close();
      reader.close();

      writer=null;
      reader=null;

      if(!mRunning) {
        file.delete();
        throw new CancellationException("download cancelled");
      } else
        Logger.debug("download finished successfully");

      if(md5sum!=null && md5 != null && !md5sum.equals((digest = digest2string(md5.digest())))) {
        Logger.warning(String.format("expected md5: %s, got %s",md5sum,digest));
        file.delete();
        throw new KeyException("wrong MD5");
      } else if(sha1sum!=null && sha1 != null && !sha1sum.equals((digest = digest2string(sha1.digest())))){
        Logger.warning(String.format("expected sha1: %s, got %s",sha1sum,digest));
        file.delete();
        throw new KeyException("wrong SHA-1");
      }

      setNotificationActions();

    } finally {
      try {
        if(writer!=null)
          writer.close();
        if(reader!=null)
          reader.close();
      } catch (IOException e) {
        System.errorLogging(e);
      }
    }
  }


  /**
   * extract mLocalFile tarball to mDestinationDir
   * we cannot untar from Java:
   *   -  cannot make symlinks
   *   -  cannot change file owner
   *   -  cannot fully change file mode
   * decompress it using mAlgorithm
   */

  private void tarballExtract() throws CancellationException, RuntimeException, IOException, InterruptedException {
    if(mLocalFile==null||mDestinationDir==null)
      return;

    OutputStream writer = null;
    InputStream reader = null;
    boolean exitForError = true;

    try {
      Thread result;
      Shell.StreamGobbler gobbler;
      File inFile;
      CountingInputStream counter;
      long total;
      short percentage,old_percentage;
      int read;
      byte[] buffer = new byte[4096];
      long time = java.lang.System.currentTimeMillis();

      mBuilder.setContentTitle(getString(R.string.extracting))
              .setContentText("")
              .setContentInfo("")
              .setSmallIcon(android.R.drawable.ic_popup_sync)
              .setProgress(100, 0, true);
      mNotificationManager.notify(NOTIFICATION_ID,mBuilder.build());

      result = Shell.async(String.format("mkdir -p '%s' && cd '%s' && tar -x",mDestinationDir,mDestinationDir), null, true );
      if(!(result instanceof Shell.StreamGobbler))
        throw new IOException("cannot execute shell commands");
      gobbler = (Shell.StreamGobbler) result;
      gobbler.start();
      // wait that stdin get connected to OutputStream
      while(mRunning && (writer = gobbler.getOutputStream()) == null)
        Thread.sleep(50);

      if(writer == null) // check writer instead of mRunning to shut up NullPointer inspection
        throw new CancellationException("tar extraction cancelled while connecting to stdin");

      inFile = new File(mLocalFile);
      counter = new CountingInputStream(new FileInputStream(inFile));
      reader = openCompressedStream(counter);

      old_percentage=-1;
      total=inFile.length();

      while(mRunning && (read = reader.read(buffer))!=-1) {
        writer.write(buffer,0,read);
        percentage=(short)(((double)counter.getBytesRead()/total)*100);
        if(percentage!=old_percentage) {
          mBuilder.setProgress(100,percentage,false)
                  .setContentInfo(percentage+"%");
          mNotificationManager.notify(NOTIFICATION_ID,mBuilder.build());
          old_percentage=percentage;
        }
      }

      writer.close();
      writer = null;
      reader.close();
      reader = null;

      if(!mRunning)
        throw new CancellationException("tar extraction cancelled while decompressing");

      inFile.delete();

      mBuilder.setContentInfo("")
              .setProgress(100,100,true);
      mNotificationManager.notify(NOTIFICATION_ID,mBuilder.build());
      try {
        gobbler.join();
      } catch (InterruptedException e) {
        throw new CancellationException("tar extraction cancelled while extracting");
      }

      if(gobbler.exitValue!=0) {
        throw new RuntimeException("tar failed with code: "+gobbler.exitValue);
      }

      Logger.debug("extraction took "+((java.lang.System.currentTimeMillis() - time)/1000)+" s");

      exitForError=false;
    } finally {
      if(reader!=null)
        reader.close();
      if(writer!=null)
        writer.close();
      if(exitForError)
        wipe();
    }
  }

  @Override
  protected void onHandleIntent(Intent intent) {
    action what_to_do = (action)intent.getSerializableExtra(ACTION);

    if(what_to_do==null) {
      Logger.error("received null action");
      return;
    }

    mRunning=true;

    switch (what_to_do) {
      case apk_update:
        mRemoteUrl = REMOTE_APK_URL;
        mLocalFile =System.getStoragePath() + "/dSploit-" + mRemoteVersion + ".apk";
        break;
      case gentoo_update:
        mRemoteUrl =REMOTE_IMAGE_URL;
        md5sum=REMOTE_IMAGE_MD5;
        sha1sum=REMOTE_IMAGE_SHA1;
        mLocalFile=System.getStoragePath() + "/" + REMOTE_IMAGE_NAME;
        mDestinationDir=System.getGentooPath();
        mAlgorithm=compressionAlgorithm.xz;
        break;
    }

    try {
      setupNotification();
      downloadFile();
      tarballExtract();
      sendDone(what_to_do);
    } catch (KeyException e) {
      sendError(R.string.checksum_failed);
      Logger.warning(e.getMessage());
    } catch (NoSuchAlgorithmException e) {
      sendError(R.string.error_occured);
      System.errorLogging(e);
    } catch (CancellationException e) {
      Logger.warning(e.getMessage());
    } catch (IOException e) {
      sendError(R.string.error_occured);
      System.errorLogging(e);
    } catch ( RuntimeException e) {
      sendError(R.string.error_occured);
      Logger.error(e.getMessage());
    } catch (InterruptedException e) {
      sendError(R.string.error_occured);
      System.errorLogging(e);
    } finally {
      stopSelf();
      mRunning = false;
    }
  }

  @Override
  public void onDestroy() {
    deleteNotification();
    super.onDestroy();
  }
}
