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
import java.net.UnknownHostException;
import java.security.KeyException;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.ArrayList;
import java.util.concurrent.CancellationException;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import it.evilsocket.dsploit.R;
import it.evilsocket.dsploit.net.GemParser;
import it.evilsocket.dsploit.net.GitHubParser;
import it.evilsocket.dsploit.core.ArchiveMetadata.archiveAlgorithm;
import it.evilsocket.dsploit.core.ArchiveMetadata.compressionAlgorithm;
import it.evilsocket.dsploit.tools.Raw;

public class UpdateService extends IntentService
{
  // Resources defines
  private static final String REMOTE_VERSION_URL = "http://update.dsploit.net/version";
  private static final String REMOTE_APK_URL = "http://update.dsploit.net/apk";
  private static final String REMOTE_RUBY_VERSION_URL = "https://gist.githubusercontent.com/tux-mind/e594b1cf923183cfcdfe/raw/ruby.json";
  private static final String REMOTE_GEMS_VERSION_URL = "https://gist.githubusercontent.com/tux-mind/9c85eced88fd88367fa9/raw/gems.json";
  private static final String REMOTE_MSF_URL = "https://github.com/rapid7/metasploit-framework/archive/%s.zip";
  private static final String LOCAL_MSF_NAME = "%s.zip";
  private static final String REMOTE_GEM_SERVER = "http://gems.dsploit.net/";
  private static final Pattern VERSION_CHECK = Pattern.compile("^[0-9]+\\.[0-9]+\\.[0-9]+[a-z]?$");
  private static final Pattern GEM_FROM_LIST = Pattern.compile("^([^ ]+) \\(([^ ]+) ");

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
  private Raw.RawReceiver
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
    mErrorReceiver = new Raw.RawReceiver() {
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
      public void onStderr(String line) {
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
   * {@code output = ((1 * 10000) + (2 * 100) + (3 * 1))}
   * <br/>
   * {@code output = 2304}
   * </p>
   * @param version the apk version
   * @return the input version represented as double
   */
  private static long getVersionCode(String version){
    long code = 0,
           multipliers[] = { 10000, 100, 1 };
    String parts[] = version.split( "[^0-9a-zA-Z]", 3 ),
           item;

    for(int i = 0; i < 3; i++ )
    {
      item = parts[i];

      if(item.matches("\\d+"))
        code += multipliers[i] * Integer.parseInt(item);

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
    Long localVersion = System.getLocalRubyVersion();

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
          mRubyInfo.version = info.getLong("version");
          mRubyInfo.versionString = String.format("%d", mRubyInfo.version.intValue());
          mRubyInfo.path = String.format("%s/%s", System.getStoragePath(), info.getString("name"));
          mRubyInfo.archiver = archiveAlgorithm.valueOf(info.getString("archiver"));
          mRubyInfo.compression = compressionAlgorithm.valueOf(info.getString("compression"));
          mRubyInfo.md5 = info.getString("md5");
          mRubyInfo.sha1 = info.getString("sha1");
        }

        mRubyInfo.outputDir = System.getRubyPath();
        mRubyInfo.executableOutputDir = ExecChecker.ruby().getRoot();

        if (!mSettingReceiver.getFilter().contains("RUBY_DIR")) {
          mSettingReceiver.addFilter("RUBY_DIR");
          System.registerSettingListener(mSettingReceiver);
        }

        exitForError = false;

        if (localVersion == null || localVersion < mRubyInfo.version)
          return true;
      }
    } catch (UnknownHostException e) {
      Logger.error(e.getMessage());
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
    Long localVersion = System.getLocalMsfVersion();

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
          mMsfInfo.version = (new BigInteger(mMsfInfo.versionString.substring(0, 7), 16)).longValue();
        }

        mMsfInfo.name = name;
        mMsfInfo.path = path;
        mMsfInfo.outputDir = System.getMsfPath();
        mMsfInfo.executableOutputDir = ExecChecker.msf().getRoot();
        mMsfInfo.archiver = archiveAlgorithm.zip;
        mMsfInfo.dirToExtract = "metasploit-framework-" + branch + "/";

        if (!mSettingReceiver.getFilter().contains("MSF_DIR")) {
          mSettingReceiver.addFilter("MSF_DIR");
          System.registerSettingListener(mSettingReceiver);
        }

        exitForError = false;

        if (!mMsfInfo.version.equals(localVersion))
          return true;
      }
    } catch (UnknownHostException e) {
      Logger.error(e.getMessage());
    } catch (Exception e) {
      System.errorLogging(e);
    } finally {
      if(exitForError)
        mMsfInfo.reset();
    }
    return false;
  }

  private static SettingReceiver mSettingReceiver = new SettingReceiver() {

    @Override
    public void onSettingChanged(String key) {
      if(key.equals("RUBY_DIR")) {
        synchronized (mRubyInfo) {
          mRubyInfo.outputDir = System.getRubyPath();
          mRubyInfo.executableOutputDir = ExecChecker.ruby().getRoot();
        }
      } else if(key.equals("MSF_DIR")) {
        synchronized (mMsfInfo) {
          mMsfInfo.outputDir = System.getMsfPath();
          mMsfInfo.executableOutputDir = ExecChecker.msf().getRoot();
        }
      }
    }
  };

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
      System.getTools().raw.run("rm -rf '" + mCurrentTask.outputDir + "'");
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
   * @param child the Child returned by {@link it.evilsocket.dsploit.tools.Tool#async(String)}
   * @param cancellationMessage the message of the CancellationException
   * @throws java.io.IOException when cannot execute shell
   * @throws java.util.concurrent.CancellationException when user cancelled the notification
   */
  private int execShell(Child child, String cancellationMessage) throws IOException, CancellationException, InterruptedException {
    while(mRunning && child.running)
      Thread.sleep(10);
    if(!mRunning) {
      child.kill();
      throw new CancellationException(cancellationMessage);
    }

    if((child.exitValue!=0 || child.signal >= 0) && mErrorOutput.length() > 0)
      for(String line : mErrorOutput.toString().split("\n"))
        if(line.length()>0)
          Logger.error(line);

    if(child.signal>=0) {
      return 128 + child.signal;
    }
    return child.exitValue;
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
          read = System.getTools().raw.run(String.format("chmod 777 '%s'", mCurrentTask.path));
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
  private void extract() throws CancellationException, RuntimeException, IOException, InterruptedException, ChildManager.ChildNotStartedException {
    ArchiveInputStream is = null;
    ArchiveEntry entry;
    CountingInputStream counter;
    BufferedOutputStream bof = null;
    File f,inFile;
    File[] list;
    String name;
    String envPath;
    final StringBuffer sb = new StringBuffer();
    byte data[] = new byte[2048];
    int mode;
    int count;
    int absCount;
    long total;
    boolean isTar, r,w,x, isElf, isScript;
    short percentage,old_percentage;
    Child which;

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
      which = System.getTools().raw.async("which env", new Raw.RawReceiver() {
        @Override
        public void onNewLine(String line) {
          sb.delete(0, sb.length());
          sb.append(line);
        }
      });

      inFile = new File(mCurrentTask.path);
      total = inFile.length();
      counter = new CountingInputStream(new FileInputStream(inFile));
      is = openArchiveStream(counter);
      isTar = mCurrentTask.archiver.equals(archiveAlgorithm.tar);
      old_percentage = -1;

      f = new File(mCurrentTask.outputDir);
      if (f.exists() && f.isDirectory() && (list = f.listFiles()) != null && list.length > 2)
        wipe();

      if(execShell(which, "cancelled while retrieving env path") != 0)
        throw new RuntimeException("cannot find 'env' executable");
      envPath = sb.toString();

      while (mRunning && (entry = is.getNextEntry()) != null) {
        name = entry.getName().replaceFirst("^\\./?", "");

        if (mCurrentTask.dirToExtract != null) {
          if (!name.startsWith(mCurrentTask.dirToExtract))
            continue;
          else
            name = name.substring(mCurrentTask.dirToExtract.length());
        }

        f = new File(mCurrentTask.outputDir, name);

        isElf = isScript = false;

        if (entry.isDirectory()) {
          if (!f.exists()) {
            if (!f.mkdirs()) {
              throw new IOException(String.format("Couldn't create directory '%s'.", f.getAbsolutePath()));
            }
          }
        } else {
          bof = new BufferedOutputStream(new FileOutputStream(f));

          // check il file is an ELF or a script
          if(!isTar && entry.getSize() > 4) {
            // read the first 4 bytes of the file
            for (absCount = 0, count = -1;
                 mRunning && absCount < 4 && (count = is.read(data, absCount, 4 - absCount)) != -1;
                 absCount += count);

            if (count == -1) {
              // don't go further, we reached EOF
            } else if (data[0] == 0x7F && data[1] == 0x45 && data[2] == 0x4C && data[3] == 0x46) {
              isElf = true;
            } else if (data[0] == '#' && data[1] == '!') {
              isScript = true;
              // read until a '\n' is found ( assume that the first line is longer than 4 bytes )
              while (mRunning && is.read(data, absCount, 1) != -1) {
                if (data[absCount++] == '\n')
                  break;
              }
              byte[] firstLine = new String(data, 0, absCount).replace("/usr/bin/env", envPath).getBytes();
              absCount = firstLine.length;
              java.lang.System.arraycopy(firstLine, 0, data, 0, absCount);
            }

            bof.write(data, 0, absCount);
          }

          while (mRunning && (count = is.read(data)) != -1) {
            bof.write(data, 0, count);
            percentage = (short) (((double) counter.getBytesRead() / total) * 100);
            if (percentage != old_percentage) {
              mBuilder.setProgress(100, percentage, false)
                      .setContentInfo(percentage + "%");
              mNotificationManager.notify(NOTIFICATION_ID, mBuilder.build());
              old_percentage = percentage;
            }
          }
          bof.flush();
          bof.close();
          bof=null;
        }
        // Zip does not store file permissions.
        if (isTar) {
          mode = ((TarArchiveEntry) entry).getMode();

          r = (mode & 0400) > 0;
          w = (mode & 0200) > 0;
          x = (mode & 0100) > 0;
        } else if( isElf || isScript) {
          r = w = x = true;
        } else {
          continue;
        }

        if(f.setExecutable(x, true)) {
          Logger.warning(String.format("cannot set executable permission of '%s'",name));
        }

        if(f.setWritable(w, true)) {
          Logger.warning(String.format("cannot set writable permission of '%s'",name));
        }

        if(f.setReadable(r, true)) {
          Logger.warning(String.format("cannot set readable permission of '%s'",name));
        }
      }

      if (!mRunning)
        throw new CancellationException("extraction cancelled.");

      Logger.info("extraction completed");

      f = new File(mCurrentTask.outputDir, ".nomedia");
      if (f.createNewFile())
        Logger.info(".nomedia created");

      mBuilder.setContentInfo("")
              .setProgress(100, 100, true);
      mNotificationManager.notify(NOTIFICATION_ID, mBuilder.build());
    } finally {
      if(is != null)
        is.close();
      if(bof != null)
        bof.close();
    }
  }

  /**
   * install gems required by the MSF
   */
  private void installGems() throws CancellationException, RuntimeException, IOException, InterruptedException, ChildManager.ChildNotStartedException {
    String msfPath = System.getMsfPath();
    final ArrayList<String> ourGems = new ArrayList<String>();
    StringBuilder sb = new StringBuilder();

    mBuilder.setContentTitle(getString(R.string.installing_gems))
            .setContentText(getString(R.string.installing_bundle))
            .setContentInfo("")
            .setSmallIcon(android.R.drawable.stat_sys_download)
            .setProgress(100, 0, true);
    mNotificationManager.notify(NOTIFICATION_ID, mBuilder.build());
    Child shell;

    //TODO: use rubyShell.async() ( do not spawn a shell for every command, or maybe not [ spawn child with custom environment ? ] )
    //TODO: run install bundle while patching msf files.
    shell = System.getTools().rubyShell.async("which bundle || gem install bundle", mErrorReceiver);

    // install bundle gem, required for install msf
    if(execShell(shell,"cancelled while install bundle")!=0)
      throw new RuntimeException("cannot install bundle");

    mBuilder.setContentText(getString(R.string.installing_msf_gems));
    mNotificationManager.notify(NOTIFICATION_ID, mBuilder.build());

    // get gems stored on our gem server.

    shell = System.getTools().rubyShell.async(
        String.format("gem list -r --clear-sources --source '%s'", REMOTE_GEM_SERVER),
        new Raw.RawReceiver() {
          @Override
          public void onNewLine(String line) {
            Matcher matcher = GEM_FROM_LIST.matcher(line);
            if(matcher.find()) {
              ourGems.add(matcher.group(1) + " " + matcher.group(2));
            }
          }
        });

    if(execShell(shell, "cancelled while retrieving compiled gem list")!=0)
      throw new RuntimeException("cannot fetch remote gem info");

    // substitute gems version and gem sources with our one

    sb.append("sed -i ");

    // append our REMOTE_GEM_SERVER to msf Gemfile sources.
    // we use an our gem server to provide cross compiled gems,
    // because android does not comes with a compiler.
    sb.append(String.format("-e \"/source 'https:\\/\\/rubygems.org'/a\\\nsource '%s'\" ",
                    REMOTE_GEM_SERVER));

    for(String compiledGem : ourGems) {
      String[] parts = compiledGem.split(" ");

      // patch Gemfile
      sb.append(String.format("-e \"s/gem  *'%1$s'.*/gem '%1$s', '%2$s', :source => '%3$s'/g\" ",
              parts[0], parts[1], REMOTE_GEM_SERVER));
      // patch gemspec
      sb.append(String.format("-e \"s/spec.add_runtime_dependency  *'%1$s'.*/spec.add_runtime_dependency '%1$s', '%2$s'/g\" ",
              parts[0], parts[1]));
    }

    // android does not have git, but we downloaded the archive from the git repo.
    // so it's content it's exactly the same seen by git.
    sb.append("-e 's/`git ls-files`/`find . -type f`/' ");

    // send files to work on
    sb.append(String.format("'%1$s/Gemfile' '%1$s/metasploit-framework.gemspec'", msfPath));

    shell = System.getTools().raw.async(sb.toString(), mErrorReceiver);

    if(execShell(shell, "cancelled while patching bundle files")!=0)
      throw new RuntimeException("cannot patch bundle files");

    // remove cache version file
    new File(msfPath, "Gemfile.lock").delete();

    shell = System.getTools().rubyShell.async(
            String.format("cd '%s' && bundle install --verbose --without development test", msfPath),
            mErrorReceiver);

    // install gem required by msf using bundle
    if(execShell(shell, "cancelled on bundle install")!=0)
      throw new RuntimeException("cannot install msf gems");
  }

  private void updateGems() throws IOException, InterruptedException, CancellationException, RuntimeException, KeyException, NoSuchAlgorithmException, ChildManager.ChildNotStartedException {
    GemParser.RemoteGemInfo[] gemsToUpdate = mGemUploadParser.getGemsToUpdate();

    if(gemsToUpdate==null||gemsToUpdate.length==0)
      return;

    String localFormat = String.format("%s/%%s",System.getStoragePath());
    String remoteFormat = String.format("%s/gems/%%s", REMOTE_GEM_SERVER);
    mCurrentTask.archiver = archiveAlgorithm.tar;

    Child shell;

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

      shell = System.getTools().rubyShell.async(String.format(
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
      System.getTools().raw.run(String.format("rm -rf '%s/lib/ruby/gems/1.9.1/cache/'", System.getRubyPath()));
    } catch (Exception e) {
      System.errorLogging(e);
    }
  }

  private void deleteTemporaryFiles() {
    if(mCurrentTask.outputDir==null||mCurrentTask.path==null||mCurrentTask.path.isEmpty())
      return;
    if(!(new File(mCurrentTask.path)).delete())
      Logger.error(String.format("cannot delete temporary file '%s'", mCurrentTask.path));
  }

  private void createVersionFile() throws IOException {
    File f;
    FileOutputStream fos = null;

    if(mCurrentTask.outputDir==null)
      return;

    if(mCurrentTask.versionString == null || mCurrentTask.versionString.isEmpty()) {
      Logger.warning("version string not found");
      return;
    }

    try {
        f = new File(mCurrentTask.outputDir, "VERSION");
        fos = new FileOutputStream(f);
        fos.write(mCurrentTask.versionString.getBytes());
    } catch (Exception e) {
      System.errorLogging(e);
      throw new IOException("cannot create VERSION file");
    } finally {
      if(fos!=null) {
        try { fos.close(); }
        catch (Exception e) {
          // ignored
        }
      }
    }
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

        if (what_to_do == action.msf_update)
          installGems();
        else if (what_to_do == action.gems_update)
          updateGems();

        deleteTemporaryFiles();
        createVersionFile();
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
    } catch (ChildManager.ChildNotStartedException e) {
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
