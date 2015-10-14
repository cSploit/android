/*
 * This file is part of the cSploit.
 *
 * Copyleft of Massimo Dragano aka tux_mind <tux_mind@csploit.org>
 *
 * cSploit is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * cSploit is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with cSploit.  If not, see <http://www.gnu.org/licenses/>.
 */
package org.csploit.android.services;

import android.app.IntentService;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
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
import org.apache.commons.compress.utils.IOUtils;
import org.csploit.android.R;
import org.csploit.android.core.*;
import org.csploit.android.update.CoreUpdate;
import org.csploit.android.update.MsfUpdate;
import org.csploit.android.update.Update;
import org.csploit.android.update.Update.archiveAlgorithm;
import org.csploit.android.core.System;
import org.csploit.android.tools.Raw;

import java.io.BufferedInputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.security.KeyException;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.Arrays;
import java.util.concurrent.CancellationException;

public class UpdateService extends IntentService
{
  // Intent defines
  public static final String START    = "UpdateService.action.START";
  public static final String ERROR    = "UpdateService.action.ERROR";
  public static final String DONE     = "UpdateService.action.DONE";
  public static final String UPDATE   = "UpdateService.data.UPDATE";
  public static final String MESSAGE  = "UpdateService.data.MESSAGE";

  // notification defines
  private static final int NOTIFICATION_ID = 1;
  private static final int DOWNLOAD_COMPLETE_CODE = 1;
  private static final int CANCEL_CODE = 2;
  private static final String NOTIFICATION_CANCELLED = "org.csploit.android.services.UpdateService.CANCELLED";

  private boolean
          mRunning                    = false;
  private Update
          mCurrentTask                = null;
  final private StringBuffer
          mErrorOutput                = new StringBuffer();
  private Raw.RawReceiver
          mErrorReceiver              = null;

  private NotificationManager mNotificationManager = null;
  private NotificationCompat.Builder mBuilder = null;
  private BroadcastReceiver mReceiver = null;

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
   * notify activities that some error occurred
   * @param message an error message
   */
  private void sendError(int message) {
    Intent i = new Intent(ERROR);
    i.putExtra(MESSAGE, message);
    sendBroadcast(i);
  }

  /**
   * notify activities that we finished our job
   */
  private void sendDone() {
    Intent i = new Intent(DONE);
    i.putExtra(UPDATE, mCurrentTask);
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
    File[] files = f.listFiles();

    if(files != null) {
      for( File c : files) {
        deleteRecursively(c);
      }
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
    boolean errorOccurred;
    Intent contentIntent;

    errorOccurred = mCurrentTask.errorOccurred;
    contentIntent = mCurrentTask.haveIntent() ? mCurrentTask.buildIntent() : null;

    if(errorOccurred || contentIntent==null){
      Logger.debug("deleting notifications");
      if(mNotificationManager!=null)
        mNotificationManager.cancel(NOTIFICATION_ID);
    } else {
      Logger.debug("assign '"+contentIntent.toString()+"' to notification");
     if(mBuilder!=null&&mNotificationManager!=null) {
       mBuilder.setContentIntent(PendingIntent.getActivity(this, DOWNLOAD_COMPLETE_CODE, contentIntent, 0));
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
   * @param child the Child returned by {@link org.csploit.android.tools.Tool#async(String)}
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
    String rootDirectory;

    Logger.info("verifying archive integrity");

    if(mCurrentTask==null||mCurrentTask.path==null)
      throw new RuntimeException("no archive to test");

    mBuilder.setContentTitle(getString(R.string.checking))
            .setSmallIcon(android.R.drawable.ic_popup_sync)
            .setContentText("")
            .setContentInfo("")
            .setProgress(100, 0, true);
    mNotificationManager.notify(NOTIFICATION_ID,mBuilder.build());

    f = new File(mCurrentTask.path);
    try {
      counter = new CountingInputStream(new FileInputStream(f));
    } catch (FileNotFoundException e) {
      throw new RuntimeException(String.format("archive '%s' does not exists", mCurrentTask.path));
    }

    try {
      is = openArchiveStream(counter);
      ArchiveEntry entry;
      buffer = new byte[2048];
      total = f.length();
      old_percentage = -1;
      rootDirectory = null;

      // consume the archive
      while (mRunning && (entry = is.getNextEntry()) != null) {
        if(!mCurrentTask.skipRoot) continue;

        String name = entry.getName();

        if(rootDirectory == null) {
          if(name.contains("/")) {
            rootDirectory = name.substring(0, name.indexOf('/'));
          } else if(entry.isDirectory()) {
            rootDirectory = name;
          } else {
            throw new IOException(String.format("archive '%s' contains files under it's root", mCurrentTask.path));
          }
        } else {
          if(!name.startsWith(rootDirectory)) {
            throw new IOException("multiple directories found in the archive root");
          }
        }
      }

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
    } finally {
      try {
        counter.close();
      } catch (IOException ignore) { }
    }

    if(!mRunning)
      throw new CancellationException("archive integrity check cancelled");

    if(mCurrentTask.skipRoot && rootDirectory == null)
      throw new KeyException(String.format("archive '%s' is empty", mCurrentTask.path));
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
  private void extract() throws RuntimeException, IOException, InterruptedException, ChildManager.ChildNotStartedException {
    ArchiveInputStream is = null;
    ArchiveEntry entry;
    CountingInputStream counter;
    OutputStream outputStream = null;
    File f,inFile;
    File[] list;
    String name;
    String envPath;
    final StringBuffer sb = new StringBuffer();
    int mode;
    int count;
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

    Logger.info(String.format("extracting '%s' to '%s'", mCurrentTask.path, mCurrentTask.outputDir));

    envPath = null;
    which = null;

    try {
      if (mCurrentTask.fixShebang) {
        which = System.getTools().raw.async("which env", new Raw.RawReceiver() {
          @Override
          public void onNewLine(String line) {
            sb.delete(0, sb.length());
            sb.append(line);
          }
        });
      }

      inFile = new File(mCurrentTask.path);
      total = inFile.length();
      counter = new CountingInputStream(new FileInputStream(inFile));
      is = openArchiveStream(counter);
      isTar = mCurrentTask.archiver.equals(archiveAlgorithm.tar);
      old_percentage = -1;

      f = new File(mCurrentTask.outputDir);
      if (f.exists() && f.isDirectory() && (list = f.listFiles()) != null && list.length > 2)
        wipe();

      if (mCurrentTask.fixShebang) {
        if(execShell(which, "cancelled while retrieving env path") != 0) {
          throw new RuntimeException("cannot find 'env' executable");
        }
        envPath = sb.toString();
      }

      while (mRunning && (entry = is.getNextEntry()) != null) {
        name = entry.getName().replaceFirst("^\\./?", "");

        if (mCurrentTask.skipRoot) {
          if(name.contains("/"))
            name = name.substring(name.indexOf('/') + 1);
          else if(entry.isDirectory())
            continue;
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
          byte[] buffer = null;
          byte[] writeMe = null;

          outputStream = new FileOutputStream(f);

          // check il file is an ELF or a script
          if((!isTar || mCurrentTask.fixShebang) && entry.getSize() > 4) {
            writeMe = buffer = new byte[4];

            IOUtils.readFully(is, buffer);

            if (buffer[0] == 0x7F && buffer[1] == 0x45 && buffer[2] == 0x4C && buffer[3] == 0x46) {
              isElf = true;
            } else if (buffer[0] == '#' && buffer[1] == '!') {
              isScript = true;

              ByteArrayOutputStream firstLine = new ByteArrayOutputStream();
              int newline = -1;

              // assume that '\n' is more far then 4 chars.
              firstLine.write(buffer);
              buffer = new byte[1024];
              count = 0;

              while (mRunning && (count = is.read(buffer)) >= 0 &&
                      (newline = Arrays.binarySearch(buffer, 0, count, (byte) 0x0A)) < 0) {
                firstLine.write(buffer, 0, count);
              }

              if (!mRunning) {
                throw new CancellationException("cancelled while searching for newline.");
              } else if(count < 0) {
                newline = count = 0;
              } else if(newline < 0) {
                newline = count;
              }

              firstLine.write(buffer, 0, newline);
              firstLine.close();

              byte[] newFirstLine = new String(firstLine.toByteArray()).replace("/usr/bin/env", envPath).getBytes();

              writeMe = new byte[newFirstLine.length + (count - newline)];

              java.lang.System.arraycopy(newFirstLine, 0, writeMe, 0, newFirstLine.length);
              java.lang.System.arraycopy(buffer, newline, writeMe, newFirstLine.length, count - newline);
            }
          }

          if(writeMe != null) {
            outputStream.write(writeMe);
          }

          IOUtils.copy(is, outputStream);

          outputStream.close();
          outputStream = null;

          percentage = (short) (((double) counter.getBytesRead() / total) * 100);
          if (percentage != old_percentage) {
            mBuilder.setProgress(100, percentage, false)
                    .setContentInfo(percentage + "%");
            mNotificationManager.notify(NOTIFICATION_ID, mBuilder.build());
            old_percentage = percentage;
          }
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

        if(!f.setExecutable(x, true)) {
          Logger.warning(String.format("cannot set executable permission of '%s'",name));
        }

        if(!f.setWritable(w, true)) {
          Logger.warning(String.format("cannot set writable permission of '%s'",name));
        }

        if(!f.setReadable(r, true)) {
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
      if(outputStream != null)
        outputStream.close();
    }
  }

  /**
   * install gems required by the MSF
   */
  private void installGems() throws RuntimeException, IOException, InterruptedException, ChildManager.ChildNotStartedException, ChildManager.ChildDiedException {
    String msfPath = System.getMsfPath();

    mBuilder.setContentTitle(getString(R.string.installing_gems))
            .setContentText(getString(R.string.installing_bundle))
            .setContentInfo("")
            .setSmallIcon(android.R.drawable.stat_sys_download)
            .setProgress(100, 0, true);
    mNotificationManager.notify(NOTIFICATION_ID, mBuilder.build());
    Child bundleInstallTask;

    bundleInstallTask = null;

    try {
      // install bundle gem, required for install msf
      if (System.getTools().ruby.run("which bundle") != 0) {
        bundleInstallTask = System.getTools().ruby.async("gem install bundle", mErrorReceiver);
      }

      mBuilder.setContentText(getString(R.string.installing_msf_gems));
      mNotificationManager.notify(NOTIFICATION_ID, mBuilder.build());

      // remove cache version file
      new File(msfPath, "Gemfile.lock").delete();

      if (bundleInstallTask != null && execShell(bundleInstallTask, "cancelled while install bundle") != 0)
        throw new RuntimeException("cannot install bundle");

      bundleInstallTask = System.getTools().ruby.async(
              String.format("bundle install --verbose --local --gemfile '%s/Gemfile' --without development test", msfPath),
              mErrorReceiver);

      // install gem required by msf using bundle
      if (execShell(bundleInstallTask, "cancelled on bundle install") != 0)
        throw new RuntimeException("cannot install msf gems");
    } finally {
      if(bundleInstallTask != null && bundleInstallTask.running)
        bundleInstallTask.kill(2);
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

    if(mCurrentTask.version == null || mCurrentTask.version.isEmpty()) {
      Logger.warning("version string not found");
      return;
    }

    try {
        f = new File(mCurrentTask.outputDir, "VERSION");
        fos = new FileOutputStream(f);
        fos.write(mCurrentTask.version.getBytes());
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
    mCurrentTask = (Update) intent.getSerializableExtra(UPDATE);
    boolean exitForError=true;

    if(mCurrentTask==null) {
      Logger.error("received null update");
      return;
    }

    mRunning = true;

    try {
      setupNotification();

      mCurrentTask.errorOccurred = true;
      if (!haveLocalFile())
        downloadFile();

      if (mCurrentTask instanceof CoreUpdate)
        System.shutdownCoreDaemon();

      extract();

      if (mCurrentTask instanceof MsfUpdate)
        installGems();

      deleteTemporaryFiles();
      createVersionFile();

      mCurrentTask.errorOccurred = exitForError = false;

      sendDone();
    } catch ( SecurityException e) {
      sendError(R.string.bad_permissions);
      Logger.warning(e.getClass().getName() + ": " + e.getMessage());
    } catch (KeyException e) {
      sendError(R.string.checksum_failed);
      Logger.warning(e.getClass().getName() + ": " + e.getMessage());
    } catch (CancellationException e) {
      sendError(R.string.update_cancelled);
      Logger.warning(e.getClass().getName() + ": " + e.getMessage());
    } catch (NoSuchAlgorithmException | RuntimeException | ChildManager.ChildDiedException | ChildManager.ChildNotStartedException | InterruptedException | IOException e) {
      sendError(R.string.error_occured);
      System.errorLogging(e);
    } finally {
      if(exitForError) {
        if(mCurrentTask instanceof MsfUpdate)
          clearGemsCache();
        if(!(mCurrentTask instanceof CoreUpdate))
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
