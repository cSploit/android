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

import org.apache.commons.compress.archivers.ArchiveEntry;
import org.apache.commons.compress.archivers.ArchiveInputStream;
import org.apache.commons.compress.archivers.tar.TarArchiveEntry;
import org.apache.commons.compress.archivers.tar.TarArchiveInputStream;
import org.apache.commons.compress.archivers.zip.ZipArchiveInputStream;
import org.apache.commons.compress.compressors.bzip2.BZip2CompressorInputStream;
import org.apache.commons.compress.compressors.gzip.GzipCompressorInputStream;
import org.apache.commons.compress.compressors.xz.XZCompressorInputStream;
import org.apache.commons.compress.utils.CountingInputStream;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.math.BigInteger;
import java.net.HttpURLConnection;
import java.net.URL;
import java.security.KeyException;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.CancellationException;
import java.util.regex.Pattern;

import it.evilsocket.dsploit.R;
import it.evilsocket.dsploit.net.GemParser;
import it.evilsocket.dsploit.net.GitHubParser;
import it.evilsocket.dsploit.core.ArchiveMetadata.archiveAlgorithm;
import it.evilsocket.dsploit.core.ArchiveMetadata.compressionAlgorithm;

public class UpdateService extends IntentService
{
  // Resources defines
  private static final String REMOTE_VERSION_URL = "http://update.dsploit.net/version";
  private static final String REMOTE_APK_URL = "http://update.dsploit.net/apk";
  private static final String VERSION_CHAR_MAP = "zyxwvutsrqponmlkjihgfedcba";
  private static final String REMOTE_RUBY_VERSION_URL = "https://gist.githubusercontent.com/tux-mind/e594b1cf923183cfcdfe/raw/ruby.json";
  private static final String REMOTE_GEMS_VERSION_URL = "https://gist.githubusercontent.com/tux-mind/9c85eced88fd88367fa9/raw/gems.json";
  private static final String REMOTE_MSF_URL = "https://github.com/rapid7/metasploit-framework/archive/%s.zip";
  private static final String LOCAL_MSF_NAME = "%s.zip";
  private static final String REMOTE_GEM_SERVER = "http://gems.dsploit.net/";
  private static final Pattern VERSION_CHECK = Pattern.compile("^[0-9]+\\.[0-9]+\\.[0-9]+[a-z]?$");

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

  // remote data
  private static final ArchiveMetadata  mApkInfo          = new ArchiveMetadata();
  private static final ArchiveMetadata  mMsfInfo          = new ArchiveMetadata();
  private static final ArchiveMetadata  mRubyInfo         = new ArchiveMetadata();
  private static final GitHubParser     mMsfRepoParser    = new GitHubParser("rapid7", "metasploit-framework");
  private static final GemParser        mGemUploadParser  = new GemParser(REMOTE_GEMS_VERSION_URL);

  private boolean
          mRunning                    = false;
  private ArchiveMetadata
          mCurrentTask                = null;
  final private StringBuffer
          mErrorOutput                = new StringBuffer();
  private Shell.OutputReceiver
          mErrorReceiver              = null;

  private NotificationManager mNotificationManager = null;
  private NotificationCompat.Builder mBuilder = null;
  private BroadcastReceiver mReceiver = null;

  public enum action {
    apk_update,
    ruby_update,
    gems_update,
    msf_update
  }

  public UpdateService(){
    super("UpdateService");
    // prepare error receiver
    mErrorReceiver = new Shell.OutputReceiver() {
      @Override
      public void onStart(String command) {
        mErrorOutput.delete(0, mErrorOutput.length());
        mErrorOutput.append("running: ");
        mErrorOutput.append(command);
        mErrorOutput.append("\n");
      }

      @Override
      public void onNewLine(String line) {
        mErrorOutput.append(line);
        mErrorOutput.append("\n");
      }

      @Override
      public void onEnd(int exitCode) {
        mErrorOutput.append("exitValue: ");
        mErrorOutput.append(exitCode);
      }
    };
  }

  /**
   * <p>
   * parse a string containing the version of the apk
   * into a double for easily compare them.
   * </p>
   * <p>
   * the algorithm works as follows:
   * </p>
   * <p>
   * {@code version = "1.2.3d"}
   * <br/>
   * {@code output = (((1+1) * 1000) + ((2+1) * 100) + ((3+1) * 1)) - ((charOffset+1) / 100.0)}
   * <br/>
   * {@code output = 2304.77}
   * </p>
   * <p>
   * where {@code charOffset} is the distance of the letter from the 'z'
   * in the ASCII table.
   * </p>
   * @param version the apk version
   * @return the input version represented as double
   */
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

  public static boolean isUpdateAvailable(){
    boolean exitForError = true;
    String localVersion = System.getAppVersionName();

    // cannot retrieve installed apk version
    if(localVersion==null)
      return false;

    try {
      synchronized (mApkInfo) {
        // Read remote version
        if (mApkInfo.version == null) {
          URL url = new URL(REMOTE_VERSION_URL);
          HttpURLConnection connection = (HttpURLConnection) url.openConnection();
          BufferedReader reader = new BufferedReader(new InputStreamReader(connection.getInputStream()));
          String line,
                  buffer = "";

          while ((line = reader.readLine()) != null) {
            buffer += line + "\n";
          }

          reader.close();
          mApkInfo.url = REMOTE_APK_URL;
          mApkInfo.versionString = buffer.split("\n")[0].trim();
          if (!VERSION_CHECK.matcher(mApkInfo.versionString).matches())
            throw new org.apache.http.ParseException(
                    String.format("remote version parse failed: '%s'", mApkInfo.versionString));
          mApkInfo.version = getVersionCode(mApkInfo.versionString);
          mApkInfo.name = String.format("dSploit-%s.apk", mApkInfo.versionString);
          mApkInfo.path = String.format("%s/%s", System.getStoragePath(), mApkInfo.name);
          mApkInfo.contentIntent = new Intent(Intent.ACTION_VIEW);
          mApkInfo.contentIntent.setDataAndType(Uri.fromFile(new File(mApkInfo.path)), "application/vnd.android.package-archive");
          mApkInfo.contentIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        }

        // Compare versions
        double installedVersionCode = getVersionCode(localVersion);

        Logger.debug(String.format("mApkInstalledVersion = %s ( %s ) ", localVersion, installedVersionCode));
        Logger.debug(String.format("mRemoteVersion       = %s ( %s ) ", mApkInfo.versionString, mApkInfo.version));

        exitForError = false;

        if (mApkInfo.version > installedVersionCode)
          return true;
      }
    } catch (org.apache.http.ParseException e) {
      Logger.error(e.getMessage());
    } catch(Exception e){
      System.errorLogging(e);
    } finally {
      if(exitForError)
        mApkInfo.reset();
    }

    return false;
  }

  public static String getRemoteVersion(){
    return mApkInfo.versionString;
  }

  /**
   * is ruby update available?
   * @return true if ruby can be updated, false otherwise
   */
  public static boolean isRubyUpdateAvailable() {
    HttpURLConnection connection = null;
    BufferedReader reader = null;
    String line;
    boolean exitForError = true;
    Double localVersion = System.getLocalRubyVersion();

    try {
      synchronized (mRubyInfo) {
        if (mRubyInfo.version == null) {

          HttpURLConnection.setFollowRedirects(true);
          URL url = new URL(REMOTE_RUBY_VERSION_URL);
          connection = (HttpURLConnection) url.openConnection();
          connection.connect();

          if (connection.getResponseCode() != 200)
            return false;

          reader = new BufferedReader(new InputStreamReader(connection.getInputStream()));
          StringBuilder sb = new StringBuilder();

          while ((line = reader.readLine()) != null) {
            sb.append(line);
          }

          JSONObject info = new JSONObject(sb.toString());
          mRubyInfo.url = info.getString("url");
          mRubyInfo.version = info.getDouble("version");
          mRubyInfo.versionString = String.format("%d", mRubyInfo.version.intValue());
          mRubyInfo.path = String.format("%s/%s", System.getStoragePath(), info.getString("name"));
          mRubyInfo.archiver = archiveAlgorithm.valueOf(info.getString("archiver"));
          mRubyInfo.compression = compressionAlgorithm.valueOf(info.getString("compression"));
          mRubyInfo.md5 = info.getString("md5");
          mRubyInfo.sha1 = info.getString("sha1");
          mRubyInfo.outputDir = System.getRubyPath();
        }
        exitForError = false;

        if (Shell.canExecuteInDir(mRubyInfo.outputDir)) {
          mRubyInfo.executableOutputDir = mRubyInfo.outputDir;
        } else {
          String realPath = Shell.getRealPath(mRubyInfo.outputDir);
          if(Shell.canRootExecuteInDir(realPath))
            mRubyInfo.executableOutputDir = realPath;
          else {
            Logger.error(String.format("cannot create executable files in '%s' or '%s'",
                    mRubyInfo.outputDir, realPath));
            return false;
          }
        }

        if (localVersion == null || localVersion < mRubyInfo.version)
          return true;
      }
    } catch(Exception e){
      System.errorLogging(e);
    } finally {
      try {
        if (reader != null)
          reader.close();
      } catch (Exception e) {
        //ignored
      }
      if(connection!=null)
        connection.disconnect();
      if(exitForError)
        mRubyInfo.reset();
    }
    return false;
  }

  public static boolean isGemUpdateAvailable() {

    try {
      synchronized (mGemUploadParser) {
        GemParser.RemoteGemInfo[] gemInfoArray = mGemUploadParser.parse();
        ArrayList<GemParser.RemoteGemInfo> gemsToUpdate = new ArrayList<GemParser.RemoteGemInfo>();

        if (gemInfoArray.length == 0)
          return false;

        String format = String.format("%s/lib/ruby/gems/1.9.1/specifications/%%s-%%s-arm-linux.gemspec", System.getRubyPath());

        for (GemParser.RemoteGemInfo gemInfo : gemInfoArray) {
          File f = new File(String.format(format, gemInfo.name, gemInfo.version));
          if (!f.exists() || f.lastModified() < gemInfo.uploaded.getTime()) {
            Logger.debug(String.format("'%s' %s", f.getAbsolutePath(), (f.exists() ? "is old" : "does not exists")));
            gemsToUpdate.add(gemInfo);
          }
        }

        if(gemsToUpdate.size() == 0)
          return false;

        mGemUploadParser.setOldGems(gemsToUpdate.toArray(new GemParser.RemoteGemInfo[gemsToUpdate.size()]));
        return true;
      }
    } catch (IOException e) {
      Logger.warning(e.getClass() + ": " + e.getMessage());
    } catch (JSONException e) {
      System.errorLogging(e);
    }
    return false;
  }

  /**
   * is a MetaSploitFramework update available?
   * @return true if the framework can be updated, false otherwise
   */
  public static boolean isMsfUpdateAvailable() {
    boolean exitForError = true;
    String branch = System.getSettings().getString("MSF_BRANCH", "release");
    Double localVersion = System.getLocalMsfVersion();
    HashMap<Integer, String> msfModeMap = new HashMap<Integer, String>() {
      {
        put(0755, "msfrpcd msfconsole msfcli $(find . -name '*.rb')");
      }
    };

    try {
      String name = String.format(LOCAL_MSF_NAME, branch);
      String path = String.format("%s/%s", System.getStoragePath(), name);
      File local = new File(path);

      synchronized (mMsfInfo) {

        if (local.exists() && local.isFile() && local.canRead()) {
          mMsfInfo.url = null;
          mMsfInfo.version = (localVersion != null ? localVersion + 1 : 0);
        } else if (mMsfInfo.url == null) {
          synchronized (mMsfRepoParser) {
            if (!branch.equals(mMsfRepoParser.getBranch()))
              mMsfRepoParser.setBranch(branch);
            mMsfInfo.versionString = mMsfRepoParser.getLastCommitSha();
          }
          mMsfInfo.url = String.format(REMOTE_MSF_URL, branch);
          // see System.getLocalMsfVersion for more info about this line of code
          mMsfInfo.version = (new BigInteger(mMsfInfo.versionString.substring(0, 7), 16)).doubleValue();
        }

        mMsfInfo.name = name;
        mMsfInfo.path = path;
        mMsfInfo.outputDir = System.getMsfPath();
        mMsfInfo.archiver = archiveAlgorithm.zip;
        mMsfInfo.dirToExtract = "metasploit-framework-" + branch + "/";
        mMsfInfo.modeMap = msfModeMap;

        exitForError = false;

        if (Shell.canExecuteInDir(mMsfInfo.outputDir)) {
          mMsfInfo.executableOutputDir = mMsfInfo.outputDir;
        } else {
          String realPath = Shell.getRealPath(mMsfInfo.outputDir);
          if (Shell.canRootExecuteInDir(realPath)) {
            mMsfInfo.executableOutputDir = realPath;
          } else {
            Logger.error(String.format("cannot create executable files in '%s' or '%s'",
                    mMsfInfo.outputDir, realPath));
            return false;
          }
        }

        if (!mMsfInfo.version.equals(localVersion))
          return true;
      }
    } catch (Exception e) {
      System.errorLogging(e);
    } finally {
      if(exitForError)
        mMsfInfo.reset();
    }
    return false;
  }

  public static String[] getMsfBranches() {
    synchronized (mMsfRepoParser) {
      try {
        return mMsfRepoParser.getBranches();
      } catch (JSONException e) {
        System.errorLogging(e);
      } catch (IOException e) {
        Logger.warning("getMsfBranches: " + e.getMessage());
      }
    }
    return new String[] {"master"};
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

  /**
   * convert a byte array to it's hexadecimal string representation
   * @param digest the byte array to convert
   * @return the hexadecimal string that represent the given array
   */
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
   * @throws IOException if an I/O error occurs
   */
  private InputStream openCompressedStream(InputStream in) throws IOException {
    if(mCurrentTask.compression==null)
      return in;
    switch(mCurrentTask.compression) {
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
   * open an Archive InputStream
   * @param in the InputStream to the archive
   * @return the ArchiveInputStream to read from
   * @throws IOException if an I/O error occurs
   * @throws java.lang.IllegalStateException if no archive method has been choose
   */
  private ArchiveInputStream openArchiveStream(InputStream in) throws IOException, IllegalStateException {
    switch (mCurrentTask.archiver) {
      case tar:
        return new TarArchiveInputStream(new BufferedInputStream(openCompressedStream(in)));
      case zip:
        return new ZipArchiveInputStream(new BufferedInputStream(openCompressedStream(in)));
      default:
        throw new IllegalStateException("trying to open an archive, but no archive algorithm selected.");
    }
  }

  /**
   * delete a directory recursively
   * @param f the file/directory to delete
   * @throws IOException if cannot delete something
   */
  private void deleteRecursively(File f) throws IOException {
    if (f.isDirectory()) {
      for (File c : f.listFiles())
        deleteRecursively(c);
    }
    if (!f.delete())
      throw new IOException("Failed to delete file: " + f);
  }

  /**
   * wipe the destination dir ( rm -rf )
   */
  private void wipe() {
    File outputFile;
    if(mCurrentTask==null||mCurrentTask.outputDir==null||mCurrentTask.outputDir.isEmpty()||
            !(outputFile = new File(mCurrentTask.outputDir)).exists())
      return;

    try {
      Shell.exec("rm -rf '"+mCurrentTask.outputDir+"'");
      return;
    } catch (Exception e) {
      System.errorLogging(e);
    }

    try {
      deleteRecursively(outputFile);
    } catch (IOException e) {
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
    mBuilder.setContentIntent(PendingIntent.getActivity(this, 0, new Intent(), 0));
  }

  /**
   * if mContentIntent is null delete our notification,
   * else assign it to the notification onClick
   */
  private void finishNotification() {
    if(mCurrentTask.contentIntent==null){
      Logger.debug("deleting notifications");
      if(mNotificationManager!=null)
        mNotificationManager.cancel(NOTIFICATION_ID);
    } else {
      Logger.debug("assign '"+mCurrentTask.contentIntent.toString()+"' to notification");
     if(mBuilder!=null&&mNotificationManager!=null) {
       mBuilder.setContentIntent(PendingIntent.getActivity(this, DOWNLOAD_COMPLETE_CODE, mCurrentTask.contentIntent, 0));
       mNotificationManager.notify(NOTIFICATION_ID, mBuilder.build());
     }
    }
    if(mReceiver!=null)
      unregisterReceiver(mReceiver);
    mReceiver             = null;
    mBuilder              = null;
    mNotificationManager  = null;
  }

  /**
   * wait that a shell terminate or user cancel the notification.
   * @param shell the Thread returned by {@link it.evilsocket.dsploit.core.Shell#async(String, it.evilsocket.dsploit.core.Shell.OutputReceiver, boolean)}
   * @param cancellationMessage the message of the CancellationException
   * @throws java.io.IOException when cannot execute shell
   * @throws java.util.concurrent.CancellationException when user cancelled the notification
   */
  private int execShell(Thread shell, String cancellationMessage) throws IOException, CancellationException, InterruptedException {
    if(!(shell instanceof Shell.StreamGobbler))
      throw new IOException("cannot execute shell commands");
    shell.start();
    while(mRunning && shell.getState()!= Thread.State.TERMINATED)
      Thread.sleep(10);
    if(!mRunning) {
      shell.interrupt();
      throw new CancellationException(cancellationMessage);
    } else
      shell.join();

    int ret = ((Shell.StreamGobbler)shell).exitValue;

    if(ret!=0 && mErrorOutput.length() > 0)
      for(String line : mErrorOutput.toString().split("\n"))
        if(line.length()>0)
          Logger.error(line);

    return ret;
  }

  /**
   * check if an archive is valid by reading it.
   * @throws RuntimeException if trying to run this with no archive
   */
  private void verifyArchiveIntegrity() throws RuntimeException, KeyException {
    File f;
    long total;
    short old_percentage,percentage;
    CountingInputStream counter;
    ArchiveInputStream is;
    byte[] buffer;
    boolean dirToExtractFound;

    Logger.info("verifying archive integrity");

    if(mCurrentTask==null||mCurrentTask.path==null)
      throw new RuntimeException("no archive to test");

    mBuilder.setContentTitle(getString(R.string.checking))
            .setSmallIcon(android.R.drawable.ic_popup_sync)
            .setContentText("")
            .setProgress(100, 0, false);
    mNotificationManager.notify(NOTIFICATION_ID,mBuilder.build());

    f = new File(mCurrentTask.path);
    try {
      counter = new CountingInputStream(new FileInputStream(f));
    } catch (FileNotFoundException e) {
      throw new RuntimeException(String.format("archive '%s' does not exists", mCurrentTask.path));
    }

    dirToExtractFound = mCurrentTask.dirToExtract == null;

    try {
      is = openArchiveStream(counter);
      ArchiveEntry entry;
      buffer = new byte[2048];
      total = f.length();
      old_percentage = -1;
      // consume the archive
      while (mRunning && (entry = is.getNextEntry()) != null)
        if(!dirToExtractFound && entry.getName().startsWith(mCurrentTask.dirToExtract))
          dirToExtractFound=true;
        while (mRunning && is.read(buffer) > 0) {
          percentage = (short) (((double) counter.getBytesRead() / total) * 100);
          if (percentage != old_percentage) {
            mBuilder.setProgress(100, percentage, false)
                    .setContentInfo(percentage + "%");
            mNotificationManager.notify(NOTIFICATION_ID, mBuilder.build());
            old_percentage = percentage;
          }
        }
    } catch (IOException e) {
      throw new KeyException("corrupted archive: "+e.getMessage());
    }

    if(!mRunning)
      throw new CancellationException("archive integrity check cancelled");

    if(!dirToExtractFound)
      throw new KeyException(String.format("archive '%s' does not contains required '%s' directory", mCurrentTask.path, mCurrentTask.dirToExtract));
  }

  /**
   * check if mLocalFile exists.
   *
   * @return true if file exists and match md5sum and sha1sum.
   * @throws java.util.concurrent.CancellationException when check is cancelled by user
   * @throws SecurityException bad file permissions
   * @throws IOException when IOException occurs
   * @throws java.security.NoSuchAlgorithmException when digests cannot be created
   * @throws java.security.KeyException when file checksum fails
   */
  private boolean haveLocalFile() throws CancellationException, SecurityException, IOException, NoSuchAlgorithmException, KeyException {

    File file = null;
    InputStream reader = null;
    boolean exitForError = true;

    if(mCurrentTask.path==null)
      return false;

    try {
      MessageDigest md5, sha1;
      byte[] buffer;
      int read;
      short percentage,previous_percentage;
      long read_counter,total;

      file = new File(mCurrentTask.path);
      buffer = new byte[4096];
      total= file.length();
      read_counter=0;
      previous_percentage=-1;

      if(!file.exists() || !file.isFile())
        return false;

      if(!file.canWrite() || !file.canRead()) {
        read = -1;
        try {
          read = Shell.exec(String.format("chmod 777 '%s'", mCurrentTask.path));
        } catch ( Exception e) {
          System.errorLogging(e);
        }
        if(read!=0)
          throw new SecurityException(String.format("bad file permissions for '%s', chmod returned: %d", mCurrentTask.path, read));
      }

      if(mCurrentTask.md5!=null || mCurrentTask.sha1!=null) {
        mBuilder.setContentTitle(getString(R.string.checking))
                .setSmallIcon(android.R.drawable.ic_popup_sync)
                .setContentText("")
                .setProgress(100, 0, false);
        mNotificationManager.notify(NOTIFICATION_ID,mBuilder.build());

        md5 = (mCurrentTask.md5!=null ? MessageDigest.getInstance("MD5") : null);
        sha1 = (mCurrentTask.sha1!=null ? MessageDigest.getInstance("SHA-1") : null);

        reader = new FileInputStream(file);
        while (mRunning && (read = reader.read(buffer)) != -1) {
          if(md5!=null)
            md5.update(buffer, 0, read);
          if(sha1!=null)
            sha1.update(buffer, 0, read);

          read_counter += read;

          percentage = (short) (((double) read_counter / total) * 100);
          if (percentage != previous_percentage) {
            mBuilder.setProgress(100, percentage, false)
                    .setContentInfo(percentage + "%");
            mNotificationManager.notify(NOTIFICATION_ID, mBuilder.build());
            previous_percentage = percentage;
          }
        }
        reader.close();
        reader = null;
        if (!mRunning) {
          exitForError = false;
          throw new CancellationException("local file check cancelled");
        }
        if (md5!=null && !mCurrentTask.md5.equals(digest2string(md5.digest())))
          throw new KeyException("wrong MD5");
        if (sha1!=null && !mCurrentTask.sha1.equals(digest2string(sha1.digest())))
          throw new KeyException("wrong SHA-1");
        Logger.info(String.format("checksum ok: '%s'", mCurrentTask.path));
      } else if (mCurrentTask.archiver != null) {
        verifyArchiveIntegrity();
      }
      Logger.info(String.format("file already exists: '%s'", mCurrentTask.path));
      mBuilder.setSmallIcon(android.R.drawable.stat_sys_download_done)
              .setContentTitle(getString(R.string.update_available))
              .setContentText(getString(R.string.click_here_to_upgrade))
              .setProgress(0, 0, false) // remove progress bar
              .setAutoCancel(true);
      exitForError=false;
      return true;
    } finally {
      if(exitForError&&file!=null&&file.exists()&&!file.delete())
        Logger.error(String.format("cannot delete local file '%s'", mCurrentTask.path));
      try {
        if(reader!=null)
          reader.close();
      } catch (IOException e) {
        System.errorLogging(e);
      }
    }
  }

  /**
   * download mCurrentTask.url to mCurrentTask.path
   *
   * @throws KeyException when MD5 or SHA1 sum fails
   * @throws IOException when IOError occurs
   * @throws NoSuchAlgorithmException when required digest algorithm is not available
   * @throws CancellationException when user cancelled the download via notification
   */
  private void downloadFile() throws SecurityException, KeyException, IOException, NoSuchAlgorithmException, CancellationException {
    if(mCurrentTask.url==null||mCurrentTask.path==null)
      return;

    File file = null;
    FileOutputStream writer = null;
    InputStream reader = null;
    HttpURLConnection connection = null;
    boolean exitForError = true;

    try
    {
      MessageDigest md5, sha1;
      URL url;
      byte[] buffer;
      int read;
      short percentage,previous_percentage;
      long downloaded,total;

      mBuilder.setContentTitle(getString(R.string.downloading_update))
              .setContentText(getString(R.string.connecting))
              .setSmallIcon(android.R.drawable.stat_sys_download)
              .setProgress(100, 0, true);
      mNotificationManager.notify(NOTIFICATION_ID,mBuilder.build());

      md5 = (mCurrentTask.md5!=null  ? MessageDigest.getInstance("MD5") : null);
      sha1= (mCurrentTask.sha1!=null ? MessageDigest.getInstance("SHA-1") : null);
      buffer = new byte[4096];
      file = new File(mCurrentTask.path);

      if(file.exists()&&file.isFile())
        //noinspection ResultOfMethodCallIgnored
        file.delete();

      HttpURLConnection.setFollowRedirects(true);
      url = new URL(mCurrentTask.url);
      connection = (HttpURLConnection) url.openConnection();

      connection.connect();

      writer = new FileOutputStream(file);
      reader = connection.getInputStream();

      total = connection.getContentLength();
      read = connection.getResponseCode();

      if(read!=200)
        throw new IOException(String.format("cannot download '%s': responseCode: %d",
                mCurrentTask.url, read));

      downloaded=0;
      previous_percentage=-1;

      mBuilder.setContentText(file.getName());
      mNotificationManager.notify(NOTIFICATION_ID, mBuilder.build());

      Logger.info(String.format("downloading '%s' to '%s'", mCurrentTask.url, mCurrentTask.path));

      while( mRunning && (read = reader.read(buffer)) != -1 ) {
        writer.write(buffer, 0, read);
        if(md5!=null)
          md5.update(buffer, 0, read);
        if(sha1!=null)
          sha1.update(buffer, 0, read);

        if(total>=0) {
          downloaded += read;

          percentage = (short) (((double) downloaded / total) * 100);

          if (percentage != previous_percentage) {
            mBuilder.setProgress(100, percentage, false)
                    .setContentInfo(percentage + "%");
            mNotificationManager.notify(NOTIFICATION_ID, mBuilder.build());
            previous_percentage = percentage;
          }
        }
      }

      if(!mRunning)
        throw new CancellationException("download cancelled");

      Logger.info("download finished successfully");

      if( md5 != null || sha1 != null ) {
        if (md5 != null && !mCurrentTask.md5.equals(digest2string(md5.digest()))) {
          throw new KeyException("wrong MD5");
        } else if (sha1 != null && !mCurrentTask.sha1.equals(digest2string(sha1.digest()))) {
          throw new KeyException("wrong SHA-1");
        }
      } else if(mCurrentTask.archiver != null) {
        verifyArchiveIntegrity();
      }

      exitForError=false;

    } finally {
      if(exitForError&&file!=null&&file.exists()&&!file.delete())
          Logger.error(String.format("cannot delete file '%s'", mCurrentTask.path));
      try {
        if(writer!=null)
          writer.close();
        if(reader!=null)
          reader.close();
        if(connection!=null)
          connection.disconnect();
      } catch (IOException e) {
        System.errorLogging(e);
      }
    }
  }

  /**
   * extract an archive into a directory
   *
   * @throws IOException if some I/O error occurs
   * @throws java.util.concurrent.CancellationException if task is cancelled by user
   * @throws java.lang.InterruptedException when the the running thread get cancelled.
   */
  private void extract() throws CancellationException, RuntimeException, IOException, InterruptedException {
    ArchiveInputStream is = null;
    ArchiveEntry entry;
    CountingInputStream counter;
    File f,inFile;
    File[] list;
    String name;
    FileOutputStream fos = null;
    byte data[] = new byte[2048];
    int mode;
    int count;
    long total;
    short percentage,old_percentage;

    if(mCurrentTask.path==null||mCurrentTask.outputDir==null)
      return;

    mBuilder.setContentTitle(getString(R.string.extracting))
            .setContentText("")
            .setContentInfo("")
            .setSmallIcon(android.R.drawable.ic_popup_sync)
            .setProgress(100, 0, false);
    mNotificationManager.notify(NOTIFICATION_ID,mBuilder.build());

    Logger.info(String.format("extracting '%s' to '%s'",mCurrentTask.path, mCurrentTask.outputDir));

    try {
      inFile = new File(mCurrentTask.path);
      total = inFile.length();
      counter = new CountingInputStream(new FileInputStream(inFile));
      is = openArchiveStream(counter);
      old_percentage=-1;

      f = new File(mCurrentTask.outputDir);
      if(f.exists() && f.isDirectory() && (list = f.listFiles()) != null &&  list.length > 2)
        wipe();

      if(is instanceof TarArchiveInputStream && mCurrentTask.modeMap==null)
        mCurrentTask.modeMap = new HashMap<Integer, String>();

      while (mRunning && (entry = is.getNextEntry()) != null) {
        name = entry.getName().replaceFirst("^\\./?", "");

        if(mCurrentTask.dirToExtract!=null) {
          if (!name.startsWith(mCurrentTask.dirToExtract))
            continue;
          else
            name = name.substring(mCurrentTask.dirToExtract.length());
        }

        f = new File(mCurrentTask.outputDir, name);

        if (entry.isDirectory()) {
          if (!f.exists()) {
            if (!f.mkdirs()) {
              throw new IOException(String.format("Couldn't create directory '%s'.", f.getAbsolutePath()));
            }
          }
        } else {
          BufferedOutputStream bof = new BufferedOutputStream(new FileOutputStream(f));

          while(mRunning && (count = is.read(data)) != -1) {
            bof.write(data, 0, count);
            percentage=(short)(((double)counter.getBytesRead()/total)*100);
            if(percentage!=old_percentage) {
              mBuilder.setProgress(100,percentage,false)
                      .setContentInfo(percentage+"%");
              mNotificationManager.notify(NOTIFICATION_ID,mBuilder.build());
              old_percentage=percentage;
            }
          }
          bof.flush();
          bof.close();
        }
        // Zip does not store file permissions.
        if(entry instanceof TarArchiveEntry) {
          mode=((TarArchiveEntry)entry).getMode();

          if(!mCurrentTask.modeMap.containsKey(mode))
            mCurrentTask.modeMap.put(mode, entry.getName() + " ");
          else
            mCurrentTask.modeMap.put(mode,
                    mCurrentTask.modeMap.get(mode).concat(entry.getName() + " "));
        }
      }

      if(!mRunning)
        throw new CancellationException("extraction cancelled.");

      Logger.info("extraction completed");

      f = new File(mCurrentTask.outputDir, ".nomedia");
      if(f.createNewFile())
        Logger.info(".nomedia created");

      if(mCurrentTask.versionString!=null&&!mCurrentTask.versionString.isEmpty()) {
        f = new File(mCurrentTask.outputDir, "VERSION");
        fos = new FileOutputStream(f);
        fos.write(mCurrentTask.versionString.getBytes());
      } else
        Logger.warning("version string not found");

      mBuilder.setContentInfo("")
              .setProgress(100,100,true);
      mNotificationManager.notify(NOTIFICATION_ID,mBuilder.build());
    } finally {
      if(is != null)
        is.close();
      if(fos!=null)
        fos.close();
    }
  }

  /**
   * correct file modes on extracted files
   * @throws CancellationException if task get cancelled by user
   */
  private void correctModes() throws CancellationException, IOException, RuntimeException, InterruptedException {
    /*
     * NOTE:  this horrible solution to chmod is only
     *        a temporary way to changing mode to files.
     *        we have to run chmod as root, our android App
     *        does not usually have write permissions
     *        outside of it's data dir.
     *        we will make a better way to achieve that shortly
     *        by executing native code as root.
     */
    if(mCurrentTask.modeMap==null||mCurrentTask.modeMap.size()==0)
      return;

    if(mCurrentTask.executableOutputDir==null)
      throw new IOException("output directory does not allow executable contents.");

    mBuilder.setContentTitle(getString(R.string.setting_file_modes))
            .setContentText("")
            .setContentInfo("")
            .setSmallIcon(android.R.drawable.ic_popup_sync)
            .setProgress(100, 0, true);
    mNotificationManager.notify(NOTIFICATION_ID, mBuilder.build());

    StringBuilder sb = new StringBuilder();
    sb.append("cd '");
    sb.append(mCurrentTask.executableOutputDir);
    sb.append("' ");
    for (Map.Entry<Integer, String> e : mCurrentTask.modeMap.entrySet()) {
      sb.append(" && ");
      sb.append(String.format("chmod %o %s", e.getKey(), e.getValue()));
    }

    if (execShell(Shell.async(sb.toString(),mErrorReceiver), "chmod cancelled") != 0) {
      Logger.debug("chmod command: "+sb.toString());
      throw new IOException("cannot chmod extracted files.");
    }
  }

  /**
   * patch shebang on extracted files.
   * simply replace standard '/usr/bin/env' with busybox one
   * @throws IOException if cannot execute shell commands
   * @throws InterruptedException if current thread get interrupted
   * @throws RuntimeException if something goes wrong
   * @throws java.util.concurrent.CancellationException if user cancelled this task
   */
  private void patchShebang() throws IOException, InterruptedException, RuntimeException, CancellationException {

    if(mCurrentTask.outputDir==null)
      return;

    if(mCurrentTask.executableOutputDir==null)
      throw new IOException("output directory does not allow executable contents.");

    final StringBuilder envPath = new StringBuilder();

    mBuilder.setContentTitle(getString(R.string.patching_shebang))
            .setContentText("")
            .setContentInfo("")
            .setSmallIcon(android.R.drawable.ic_popup_sync)
            .setProgress(100, 0, true);
    mNotificationManager.notify(NOTIFICATION_ID, mBuilder.build());

    if (Shell.exec("which env", new Shell.OutputReceiver() {
      @Override
      public void onStart(String command) {
      }

      @Override
      public void onNewLine(String line) {
        if(line.length()>0) {
          envPath.delete(0, envPath.length());
          envPath.append(line);
        }
      }

      @Override
      public void onEnd(int exitCode) {
      }
    }) != 0)
      throw new RuntimeException("cannot find 'env' executable");

    Logger.debug("envPath: " + envPath);

    Thread shell = Shell.async(
            String.format("sed -i '1s,^#!/usr/bin/env,#!%s,' $(find '%s' -type f -perm +111 )",
                    envPath.toString(), mCurrentTask.executableOutputDir), mErrorReceiver);

    if (execShell(shell, "cancelled while changing shebangs") != 0)
      throw new RuntimeException("cannot change shebang");
  }

  /**
   * install gems required by the MSF
   */
  private void installGems() throws CancellationException, RuntimeException, IOException, InterruptedException {
    String msfPath = System.getMsfPath();

    mBuilder.setContentTitle(getString(R.string.installing_gems))
            .setContentText(getString(R.string.installing_bundle))
            .setContentInfo("")
            .setSmallIcon(android.R.drawable.stat_sys_download)
            .setProgress(100, 0, true);
    mNotificationManager.notify(NOTIFICATION_ID, mBuilder.build());
    Shell.setupRubyEnviron();
    Thread shell;

    shell = Shell.async("gem install bundle", mErrorReceiver);

    // install bundle gem, required for install msf
    if(execShell(shell,"cancelled while install bundle")!=0)
      throw new RuntimeException("cannot install bundle");

    mBuilder.setContentText(getString(R.string.installing_msf_gems));
    mNotificationManager.notify(NOTIFICATION_ID, mBuilder.build());

    // append our REMOTE_GEM_SERVER to msf Gemfile sources.
    // we use an our gem server to provide cross compiled gems,
    // because android does not comes with a compiler.

    shell = Shell.async( String.format(
            "sed -i \"/source 'https:\\/\\/rubygems.org'/a\\\nsource '%s'\" '%s/Gemfile'",
            REMOTE_GEM_SERVER, msfPath), mErrorReceiver);

    if(execShell(shell, "cancelled while adding our gem server")!=0)
      throw new RuntimeException("cannot add our gem server");

    // i was able to cross compile pcaprub 0.10.0,
    // newer version use pcap version that are not used by android.

    shell = Shell.async(String.format(
            "sed -i \"s/'pcaprub'\\$/'pcaprub', '0.10.0'/\" '%s/Gemfile'",
            msfPath
    ), mErrorReceiver);

    if(execShell(shell, "cancelled while patching pcaprub version")!=0)
      throw new RuntimeException("cannot specify pcaprub version");

    shell = Shell.async(
            String.format("cd '%s' && bundle install --without development test", msfPath),
            mErrorReceiver);

    // install gem required by msf using bundle
    if(execShell(shell, "cancelled on bundle install")!=0)
      throw new RuntimeException("cannot install msf gems");
  }

  /**
   * update rubygems thus to correct an SSL certificate mismatch error.
   */
  private void updateRubyGems() throws CancellationException, IOException, InterruptedException {
    mBuilder.setContentTitle(getString(R.string.installing_gems))
            .setContentText(getString(R.string.updating_rubygem))
            .setContentInfo("")
            .setSmallIcon(android.R.drawable.stat_sys_download)
            .setProgress(100, 0, true);
    mNotificationManager.notify(NOTIFICATION_ID, mBuilder.build());
    Shell.setupRubyEnviron();
    Thread shell;

    shell = Shell.async("gem update --system --source 'http://rubygems.org/'", mErrorReceiver);

    if (execShell(shell, "cancelled on gem system update") != 0)
      throw new IOException("cannot update RubyGems");

    // rubygems update rewrite the shebang
    patchShebang();
  }

  private void updateGems() throws IOException, InterruptedException, CancellationException, RuntimeException, KeyException, NoSuchAlgorithmException {
    GemParser.RemoteGemInfo[] gemsToUpdate = mGemUploadParser.getGemsToUpdate();

    if(gemsToUpdate==null||gemsToUpdate.length==0)
      return;

    Shell.setupRubyEnviron();
    String localFormat = String.format("%s/%%s",System.getStoragePath());
    String remoteFormat = String.format("%s/gems/%%s", REMOTE_GEM_SERVER);
    mCurrentTask.archiver = archiveAlgorithm.tar;

    Thread shell;

    for(GemParser.RemoteGemInfo gemInfo : gemsToUpdate) {

      String gemFilename = String.format("%s-%s-arm-linux.gem", gemInfo.name, gemInfo.version);

      mCurrentTask.url = String.format(remoteFormat, gemFilename);
      mCurrentTask.path = String.format(localFormat, gemFilename);
      if(!haveLocalFile())
        downloadFile();

      mBuilder.setContentTitle(getString(R.string.installing_gems))
              .setContentText(gemInfo.name)
              .setContentInfo("")
              .setSmallIcon(android.R.drawable.ic_popup_sync)
              .setProgress(100, 0, true);
      mNotificationManager.notify(NOTIFICATION_ID, mBuilder.build());

      shell = Shell.async(String.format(
              "gem uninstall --force -x -v '%s' '%s' && gem install -l '%s'",
              gemInfo.version, gemInfo.name, mCurrentTask.path), mErrorReceiver);

      String cancelMessage = String.format("cancelled while updating '%s-%s'",
              gemInfo.name, gemInfo.version);

      if(execShell(shell,cancelMessage)!=0)
        throw new RuntimeException(String.format("cannot update '%s-%s'", gemInfo.name, gemInfo.version));

      if(!(new File(mCurrentTask.path).delete()))
        Logger.warning(String.format("cannot delete downloaded gem '%s'", mCurrentTask.path));
      mCurrentTask.path = null;
    }
  }

  private void clearGemsCache() {

    if(!System.getSettings().getBoolean("MSF_ENABLED", true))
      return;

    try {
      Shell.exec(String.format("rm -rf '%s/lib/ruby/gems/1.9.1/cache/'", System.getRubyPath()));
    } catch (Exception e) {
      System.errorLogging(e);
    }
  }

  private void deleteTemporaryFiles() {
    if(mCurrentTask.path==null||mCurrentTask.path.isEmpty())
      return;
    if(!(new File(mCurrentTask.path)).delete())
      Logger.error(String.format("cannot delete temporary file '%s'", mCurrentTask.path));
  }

  @Override
  protected void onHandleIntent(Intent intent) {
    action what_to_do = (action)intent.getSerializableExtra(ACTION);
    boolean exitForError=true;

    if(what_to_do==null) {
      Logger.error("received null action");
      return;
    }

    mRunning=true;

    switch (what_to_do) {
      case apk_update:
        mCurrentTask = mApkInfo;
        break;
      case ruby_update:
        mCurrentTask = mRubyInfo;
        break;
      case msf_update:
        mCurrentTask = mMsfInfo;
        break;
      case gems_update:
        mCurrentTask = new ArchiveMetadata();
        break;
    }

    try {
      setupNotification();

      synchronized (mCurrentTask) {
        if (!haveLocalFile())
          downloadFile();
        extract();
        correctModes();
        patchShebang();

        if (what_to_do == action.ruby_update)
          updateRubyGems();
        else if (what_to_do == action.msf_update)
          installGems();
        else if (what_to_do == action.gems_update)
          updateGems();

        if (what_to_do != action.apk_update)
          deleteTemporaryFiles();
      }
      exitForError=false;
      if(what_to_do==action.msf_update)
        System.updateLocalMsfVersion();
      if(what_to_do==action.ruby_update)
        System.updateLocalRubyVersion();
      sendDone(what_to_do);
    } catch ( SecurityException e) {
      sendError(R.string.bad_permissions);
      Logger.warning(e.getClass().getName() + ": " + e.getMessage());
    } catch (KeyException e) {
      sendError(R.string.checksum_failed);
      Logger.warning(e.getClass().getName() + ": " + e.getMessage());
    } catch (NoSuchAlgorithmException e) {
      sendError(R.string.error_occured);
      System.errorLogging(e);
    } catch (CancellationException e) {
      Logger.warning(e.getClass().getName() + ": " + e.getMessage());
    } catch (IOException e) {
      sendError(R.string.error_occured);
      System.errorLogging(e);
    } catch ( RuntimeException e) {
      sendError(R.string.error_occured);
      if(e.getClass() == NullPointerException.class)
        System.errorLogging(e);
      else
        Logger.error(e.getClass().getName() + ": " + e.getMessage());
    } catch (InterruptedException e) {
      sendError(R.string.error_occured);
      System.errorLogging(e);
    } finally {
      if(exitForError) {
        clearGemsCache();
        wipe();
      }
      stopSelf();
      mRunning = false;
    }
  }

  @Override
  public void onDestroy() {
    finishNotification();
    super.onDestroy();
  }
}
