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
package org.csploit.android.core;

import android.content.Context;

import java.io.BufferedInputStream;
import java.io.BufferedReader;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;

public class ToolsInstaller{
  private final static String TOOLS_FILENAME = "core.zip";
  private final static int BUFFER_SIZE = 4096;

  public final static String[] TOOLS = {
    "ettercap",
    "nmap",
    "arpspoof",
    "tcpdump",
    "hydra"
  };

  private final static String[] INSTALL_COMMANDS = {
    "chmod 777 {PATH}/*/",
    "chmod 777 {PATH}/ettercap/share",
    "chmod 777 {PATH}/ettercap/share/*",
    "chmod 777 {PATH}/ettercap/filters",
    "chmod 777 {PATH}/ettercap/filters/*",
    "chmod 755 {PATH}/ettercap/ettercap",
    "chmod 755 {PATH}/ettercap/etterfilter",
    "chmod 755 {PATH}/ettercap/etterlog",
    "chmod 755 {PATH}/nmap/nmap",
    "chmod 755 {PATH}/arpspoof/arpspoof",
    "chmod 755 {PATH}/tcpdump/tcpdump",
    "chmod 666 {PATH}/hydra/*",
    "chmod 755 {PATH}/hydra/hydra",
    "chmod 755 {PATH}/fusemounts/fusemounts",
    "chmod 755 {PATH}/../dSploitd",
    "mount -o remount,rw /system /system && ( chmod 6755 /system/*/su; mount -o remount,ro /system /system )",
    "chmod 755 {FILES}"
  };

  private Context mAppContext = null;
  private String mDestPath = null;
  private String mVersionFile = null;
  private String mAppVersion = null;

  public ToolsInstaller(Context c){
    mAppContext = c;
    //noinspection ConstantConditions
    mDestPath = mAppContext.getFilesDir().getAbsolutePath();
    mAppVersion = System.getAppVersionName();
    mVersionFile = mDestPath + "/tools/VERSION";
  }

  public boolean needed(){
    Logger.debug("Checking version file " + mVersionFile);

    File file = new File(mVersionFile);
    BufferedReader reader;
    String line;
    boolean needed = true;

    try{
      if(file.exists()){
        reader = new BufferedReader(new FileReader(file));
        line = reader.readLine().trim();
        needed = !line.equals(mAppVersion);

        reader.close();
      }
    }
    catch(IOException e){
      Logger.warning(e.toString());
    }

    return needed;
  }

  public boolean install(){
    Logger.debug("Installing tools ...");

    // Avoid some binary file being busy as a running process.
    System.clean(false);

    System.shutdownCoreDaemon();

    ZipInputStream zipInput;
    ZipEntry zipEntry;
    byte[] buffer = new byte[BUFFER_SIZE];
    int read;
    FileOutputStream fileOutput;
    File file;
    String fileName;

    try{
      zipInput = new ZipInputStream(new BufferedInputStream(mAppContext.getAssets().open(TOOLS_FILENAME)));

      while((zipEntry = zipInput.getNextEntry()) != null){
        fileName = mDestPath + "/" + zipEntry.getName();
        file = new File(mDestPath + "/" + zipEntry.getName());

        if(zipEntry.isDirectory())
          file.mkdirs();

        else{
          fileOutput = new FileOutputStream(fileName);

          while((read = zipInput.read(buffer, 0, BUFFER_SIZE)) > -1){
            fileOutput.write(buffer, 0, read);
          }

          fileOutput.close();
          zipInput.closeEntry();

          file.setExecutable(true, false);
          file.setReadable(true, false);
          file.setWritable(true, true);
        }
      }

      zipInput.close();

      file = new File(mVersionFile);

      fileOutput = new FileOutputStream(file);

      fileOutput.write(mAppVersion.getBytes());

      fileOutput.close();

      file.setExecutable(true, false);
      file.setReadable(true, false);
      file.setWritable(true, false);

      System.reloadTools();

      return true;
    }
    catch(Exception e){
      System.errorLogging(e);

      return false;
    }
  }
}
