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

import android.app.ProgressDialog;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.util.Log;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.net.HttpURLConnection;
import java.net.URL;
import java.util.Arrays;

import it.evilsocket.dsploit.MainActivity;

public class UpdateManager{
  private static final String TAG = "UPDATEMANAGER";
  private static final String REMOTE_VERSION_URL = "http://update.dsploit.net/version";
  private static final String REMOTE_DOWNLOAD_URL = "http://update.dsploit.net/apk";
  private static final String VERSION_CHAR_MAP = "zyxwvutsrqponmlkjihgfedcba";

  private Context mContext = null;
  private String mInstalledVersion = null;
  private String mRemoteVersion = null;

  public UpdateManager(Context context){
    mContext = context;
    mInstalledVersion = System.getAppVersionName();
  }

  private static double getVersionCode(String version){
    String[] padded = new String[3],
      parts = version.split("[^0-9a-zA-Z]");
    String item, digit, letter;
    double code = 0, coeff;
    int i, j;
    char c;

    Arrays.fill(padded, 0, 3, "0");

    for(i = 0; i < Math.min(3, parts.length); i++){
      padded[i] = parts[i];
    }

    for(i = padded.length - 1; i >= 0; i--){
      item = padded[i];
      coeff = Math.pow(10, padded.length - i);

      if(item.matches("\\d+[a-zA-Z]")){
        digit = "";
        letter = "";

        for(j = 0; j < item.length(); j++){
          c = item.charAt(j);
          if(c >= '0' && c <= '9')
            digit += c;
          else
            letter += c;
        }

        code += ((Integer.parseInt(digit) + 1) * coeff) - ((VERSION_CHAR_MAP.indexOf(letter.toLowerCase()) + 1) / 100.0);
      }
      else if(item.matches("\\d+"))
        code += (Integer.parseInt(item) + 1) * coeff;

      else
        code += coeff;
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

        if(remoteVersionCode > installedVersionCode)
          return true;
      }
    } catch(Exception e){
      System.errorLogging(TAG, e);
    }

    return false;
  }

  public String getRemoteVersion(){
    return mRemoteVersion;
  }

  public String getRemoteVersionFileName(){
    return "dSploit-" + mRemoteVersion + ".apk";
  }

  public String getRemoteVersionUrl(){
    return REMOTE_DOWNLOAD_URL;
  }

  private String formatSize(int size){
    if(size < 1024)
      return size + " B";

    else if(size < (1024 * 1024))
      return (size / 1024) + " KB";

    else if(size < (1024 * 1024 * 1024))
      return (size / (1024 * 1024)) + " MB";

    else
      return (size / (1024 * 1024 * 1024)) + " GB";
  }

  private String formatSpeed(int speed){
    if(speed < 1024)
      return speed + " B/s";

    else if(speed < (1024 * 1024))
      return (speed / 1024) + " KB/s";

    else if(speed < (1024 * 1024 * 1024))
      return (speed / (1024 * 1024)) + " MB/s";

    else
      return (speed / (1024 * 1024 * 1024)) + " GB/s";
  }

  public boolean downloadUpdate(MainActivity activity, final ProgressDialog progress){
    try{
      HttpURLConnection.setFollowRedirects(true);

      URL url = new URL(getRemoteVersionUrl());
      HttpURLConnection connection = (HttpURLConnection) url.openConnection();
      File file = new File(System.getStoragePath());
      String fileName = getRemoteVersionFileName();
      byte[] buffer = new byte[1024];
      int read = 0;

      connection.connect();

      //noinspection ResultOfMethodCallIgnored
      file.mkdirs();
      file = new File(file, fileName);
      if(file.exists())
        //noinspection ResultOfMethodCallIgnored
        file.delete();

      FileOutputStream writer = new FileOutputStream(file);
      InputStream reader = connection.getInputStream();

      int total = connection.getContentLength(),
        downloaded = 0,
        sampled = 0;
      long time = java.lang.System.currentTimeMillis();
      double speed = 0.0,
             deltat;

      while(progress.isShowing() && (read = reader.read(buffer)) != -1){
        writer.write(buffer, 0, read);

        downloaded += read;

        deltat = (java.lang.System.currentTimeMillis() - time) / 1000.0;

        if(deltat > 1.0){
          speed = (downloaded - sampled) / deltat;
          time = java.lang.System.currentTimeMillis();
          sampled = downloaded;
        }

        // update the progress ui
        final int fdown = downloaded,
          ftot = total;
        final double fspeed = speed;

        activity.runOnUiThread(new Runnable(){
          @Override
          public void run(){
          progress.setMessage("[ " + formatSpeed((int) fspeed) + " ] " + formatSize(fdown) + " / " + formatSize(ftot) + " ...");
          progress.setProgress((100 * fdown) / ftot);
          }
        });

      }

      writer.close();
      reader.close();

      if(progress.isShowing()){
        Intent intent = new Intent(Intent.ACTION_VIEW);
        intent.setDataAndType(Uri.fromFile(file), "application/vnd.android.package-archive");
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);

        mContext.startActivity(intent);
      }
      else
        Log.d(TAG, "Download cancelled.");

      return true;
    }
    catch(Exception e){
      System.errorLogging(TAG, e);
    }

    if(progress.isShowing())
      progress.dismiss();

    return false;
  }
}
