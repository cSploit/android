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

import android.content.Context;
import android.content.Entity;

import java.io.BufferedReader;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.Map;
import java.util.UUID;

public class Shell
{
  private static Process shell = null;
  private static DataOutputStream writer = null;
  //private static BufferedReader reader = null, error = null;
  private final static ArrayList<Integer> usedfifo = new ArrayList<Integer>();

  /*
   * "gobblers" seem to be the recommended way to ensure the streams
   * don't cause issues
   */
  private static class StreamGobbler extends Thread {
    private BufferedReader mReader = null;
    private OutputReceiver mReceiver = null;
    private String mToken = null;
    private String fifo_path = null;
    private int fifonum = -1;
    public int exit_code = -1;

    public StreamGobbler(String _fifo_path, OutputReceiver receiver, String token, int _fifonum) throws IOException {
      fifo_path = _fifo_path;
      mReceiver = receiver;
      mToken = token;
      fifonum = _fifonum;
      setDaemon(true);
    }

    public void run(){
      try{
        while(!(new File(fifo_path).exists())) // let mkfifo finish
          Thread.sleep(200);
        // this will hang until someone will connect to the other side of the pipe.
        mReader = new BufferedReader(new InputStreamReader(new FileInputStream(fifo_path)));
        while(true){
          String line = "";
          if(mReader.ready()){
            if((line = mReader.readLine()) == null)
              continue;
          } else{
            try{
              Thread.sleep(200);
            } catch(InterruptedException e){
            }

            continue;
          }

          if(!line.isEmpty())
          {
            if(line.startsWith(mToken)) {
              exit_code = Integer.parseInt(line.substring(mToken.length()));
              if(mReceiver!=null)
                mReceiver.onEnd(exit_code);
              break;
            }
            else if(mReceiver!=null) {
              mReceiver.onNewLine(line);
            }
          }
        }
      } catch(IOException e){
        System.errorLogging(e);
      } catch(InterruptedException e) {
        Logger.error(e.getMessage());
      } finally{
        try{
          if(mReader!=null)
            mReader.close();
          synchronized (usedfifo) {
            usedfifo.remove(usedfifo.indexOf(fifonum));
          }
        } catch(IOException e){
          //swallow error
        }
      }
    }
  }

  private static Process spawnShell(String command) throws IOException {
    return new ProcessBuilder().command(command).start();
  }

  public static boolean isRootGranted() {
    Process process = null;
    DataOutputStream writer = null;
    BufferedReader reader = null;
    String line = null;
    boolean granted = false;

    try{
      process = spawnShell("su");
      writer = new DataOutputStream(process.getOutputStream());
      reader = new BufferedReader(new InputStreamReader(process.getInputStream()));

      writer.writeBytes("id\n");
      writer.flush();
      writer.writeBytes("exit\n");
      writer.flush();

      while((line = reader.readLine()) != null && !granted){
        if(line.toLowerCase().contains("uid=0"))
          granted = true;
      }

      process.waitFor();

    } catch(Exception e){
      System.errorLogging(e);
    } finally{
      try{
        if(writer != null)
          writer.close();

        if(reader != null)
          reader.close();
      } catch(IOException e){
        // ignore errors
      }
    }

    return granted;
  }

  public interface OutputReceiver {
    public void onStart(String command);

    public void onNewLine(String line);

    public void onEnd(int exitCode);
  }

  public static boolean isBinaryAvailable(String binary) {
    try{
      Process process = spawnShell("sh");
      DataOutputStream writer = new DataOutputStream(process.getOutputStream());
      BufferedReader reader = new BufferedReader(new InputStreamReader(process.getInputStream()));
      String line = null;

      writer.writeBytes("which " + binary + "\n");
      writer.flush();
      writer.writeBytes("exit\n");
      writer.flush();

      while((line = reader.readLine()) != null){
        if(line.isEmpty() == false && line.startsWith("/"))
          return true;
      }
    } catch(Exception e){
      System.errorLogging(e);
    }

    return false;
  }

  public static boolean isLibraryPathOverridable(Context context) {
    boolean linkerError = false;

    try{
      String libPath = System.getLibraryPath(),
        fileName = context.getFilesDir().getAbsolutePath() + "/tools/nmap/nmap";
      File file = new File(fileName);
      String dirName = file.getParent(),
        command = "cd " + dirName + " && ./nmap --version";
      Process process = spawnShell("su");
      DataOutputStream writer = null;
      BufferedReader reader = null,
        stderr = null;
      String line = null,
        error = null;

      writer = new DataOutputStream(process.getOutputStream());
      reader = new BufferedReader(new InputStreamReader(process.getInputStream()));
      stderr = new BufferedReader(new InputStreamReader(process.getErrorStream()));

      writer.writeBytes("export LD_LIBRARY_PATH=" + libPath + ":$LD_LIBRARY_PATH\n");
      writer.flush();

      writer.writeBytes(command + "\n");
      writer.flush();
      writer.writeBytes("exit\n");
      writer.flush();

      while(((line = reader.readLine()) != null || (error = stderr.readLine()) != null) && !linkerError){
        line = line == null ? error : line;

        if(line.contains("CANNOT LINK EXECUTABLE"))
          linkerError = true;
      }
      // flush stderr
      while((error = stderr.readLine()) != null && !linkerError){
        if(line.contains("CANNOT LINK EXECUTABLE"))
          linkerError = true;
      }

      process.waitFor();
    } catch(Exception e){
      System.errorLogging(e);

      return !linkerError;
    }

    return !linkerError;
  }

  private static void check_binaries() throws IOException {
    File bin_dir = new File(System.getBinaryPath());
    File tools_dir = new File(System.getToolsPath());
    String dirname,bindir,filename;

    bindir = System.getBinaryPath();
    if(!bin_dir.exists())
      bin_dir.mkdir();
    else if(!bin_dir.isDirectory()) {
      bin_dir.delete();
      bin_dir.mkdir();
    }
    if(!bin_dir.isDirectory())
      throw new IOException("cannot make binaries directory");
    for(File d1 : tools_dir.listFiles()) {
      if(!d1.isDirectory())
        continue;
      dirname = d1.getName();
      for(File f : d1.listFiles()) {
        if(!f.canExecute() || f.isDirectory())
          continue;
        filename = f.getName();
        if(!new File(bindir+filename).exists())
          try {
            Process p = new ProcessBuilder().command("ln", "-s", "../"+dirname+"/"+filename, bindir+filename).start();
            int exit_code;
            synchronized (p) {
              exit_code = p.waitFor();
            }
            if(exit_code!=0)
              throw new IOException("cannot create symlinks");
          } catch (InterruptedException e) {
            Logger.warning("interrupted while executing: ln -s \"../"+dirname+"/"+filename+"\" \""+bindir+"/"+filename+"\"");
            throw new IOException("interrupted while creating symlinks");
          }
      }
    }
  }

  private static void clear_fifos() throws IOException {
    File d = new File(System.getFifosPath());
    if(!d.exists())
      d.mkdir();
    else if(!d.isDirectory()) {
      d.delete();
      d.mkdir();
    }
    if(d.isDirectory()) {
      for(File f : d.listFiles()) {
        f.delete();
      }
    } else {
      throw new IOException("cannot create fifos directory");
    }
  }

  public static int exec(String command, OutputReceiver receiver) throws IOException, InterruptedException {
    StreamGobbler g = (StreamGobbler)async(command, receiver);
    g.start();
    g.join();
    return  g.exit_code;
  }

  public static int exec(String command) throws IOException, InterruptedException {
    return exec(command, null);
  }

  /**
   * @deprecated
   */
  public static Thread async(String cmd, OutputReceiver receiver, boolean b) {
    return async(cmd,receiver);
  }

  public static Thread async(final String command, final OutputReceiver receiver) {
    int fifonum;
    String fifo_path;
    String token = UUID.randomUUID().toString().toUpperCase();
    StreamGobbler gobbler = null;
    try
    {
      if(shell==null)
      {
        shell = spawnShell("su");
        writer = new DataOutputStream(shell.getOutputStream());
        writer.writeBytes("export LD_LIBRARY_PATH=\""+System.getLibraryPath()+":$LD_LIBRARY_PATH\"\n");
        writer.writeBytes("export PATH=\""+System.getBinaryPath()+":$PATH\"\n");
        writer.flush();
        // this 2 reader are useful for debugging purposes
        //reader = new BufferedReader(new InputStreamReader(shell.getInputStream()));
        //error = new BufferedReader(new InputStreamReader(shell.getErrorStream()));
        clear_fifos();
        check_binaries();
      }
      if(receiver != null) receiver.onStart(command);
      synchronized (usedfifo) {
        for(fifonum=0;fifonum<1024&& usedfifo.contains(fifonum);fifonum++);
        if(fifonum==1024)
          throw new IOException("maximum number of fifo reached");
        usedfifo.add(fifonum);
      }
      fifo_path = String.format("%s%d",System.getFifosPath(),fifonum);
      writer.writeBytes(String.format("mkfifo -m 666 \"%s\"\n",fifo_path));
      writer.flush();
      gobbler = new StreamGobbler(fifo_path, receiver, token, fifonum);
      writer.writeBytes(String.format("(out=$(%s) ; echo -e \"$out\\n%s$?\") 2>&1 >%s &\n", command, token, fifo_path));
      writer.flush();
    } catch ( IOException e) {
      System.errorLogging(e);
    }

    return gobbler;
  }

  public static Thread async(String command) {
    return async(command, null);
  }
}
