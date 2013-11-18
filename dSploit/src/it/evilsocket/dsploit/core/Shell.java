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

import java.io.BufferedReader;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.UUID;

public class Shell
{
  private final static int MAX_FIFO = 1024;
  private static Process mRootShell = null;
  private static DataOutputStream mWriter = null;
  //private static BufferedReader reader = null, error = null;
  private final static ArrayList<Integer> mFIFOS = new ArrayList<Integer>();
  private static Thread fakeThread;

  /*
   * "gobblers" seem to be the recommended way to ensure the streams
   * don't cause issues
   */
  public static class StreamGobbler extends Thread {
    private BufferedReader mReader = null;
    private OutputReceiver mReceiver = null;
    private String mToken = null;
    private String mFIFOPath = null;
    private int mFIFONum = -1;
    public int exitValue = -1;

    public StreamGobbler(String fifoPath, OutputReceiver receiver, String token, int fifoNum) throws IOException {
      mFIFOPath = fifoPath;
      mReceiver = receiver;
      mToken = token;
      mFIFONum = fifoNum;
      setDaemon(true);
    }

    public void run(){
      try
      {
        String line = null;
        // wait for the FIFO to be created
        while(!(new File(mFIFOPath).exists()))
          Thread.sleep(200);

        // this will hang until someone will connect to the other side of the pipe.
        mReader = new BufferedReader(new FileReader(mFIFOPath));

        while((line=mReader.readLine())!=null) {
          if(line.startsWith(mToken)) {
            exitValue=Integer.parseInt(line.substring(mToken.length()));
            break;
          } else if(mReceiver!=null)
            mReceiver.onNewLine(line);
        }
        if(mReceiver!=null)
          mReceiver.onEnd(exitValue);
      }
      catch(IOException e){
        System.errorLogging(e);
      }
      catch(InterruptedException e) {
        System.errorLogging(e);
      }
      finally{
        try{
          if(mReader!=null)
            mReader.close();
          synchronized (mFIFOS) {
            mFIFOS.remove(mFIFOS.indexOf(mFIFONum));
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
    try{
      return exec("id | grep -q 'uid=0'") == 0;
    } catch(Exception e){
      System.errorLogging(e);
    }
    return false;
  }

  public interface OutputReceiver {
    public void onStart(String command);

    public void onNewLine(String line);

    public void onEnd(int exitCode);
  }

  public static boolean isBinaryAvailable(String binary) {
    try{
      return exec("which "+binary) == 0;
    } catch(Exception e){
      System.errorLogging(e);
    }
    return false;
  }

  private static void clearFifos() throws IOException {
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
    }
    else {
      throw new IOException("cannot create fifos directory");
    }
  }

  public static int exec(String command, OutputReceiver receiver) throws IOException, InterruptedException {
    Thread g = async(command, receiver);
    if(g==fakeThread)
      return -1;
    g.start();
    g.join();
    return  ((StreamGobbler)g).exitValue;
  }

  public static int exec(String command) throws IOException, InterruptedException {
    return exec(command, null);
  }

  public static Thread async(final String command, final OutputReceiver receiver) {
    int fifonum;
    String fifo_path;
    String token = UUID.randomUUID().toString().toUpperCase();
    StreamGobbler gobbler = null;

    try
    {
      // spawn shell for the first time
      if(mRootShell==null) {
        mRootShell = spawnShell("su");
        mWriter = new DataOutputStream(mRootShell.getOutputStream());
        // this 2 reader are useful for debugging purposes
        //reader = new BufferedReader(new InputStreamReader(mRootShell.getInputStream()));
        //error = new BufferedReader(new InputStreamReader(mRootShell.getErrorStream()));
        clearFifos();
      }

      if(receiver != null)
        receiver.onStart(command);

      synchronized (mFIFOS)
      {
        // check for next available FIFO number
        for( fifonum = 0; fifonum < MAX_FIFO && mFIFOS.contains(fifonum); fifonum++ );
        if(fifonum==1024)
          throw new IOException("maximum number of fifo reached");

        mFIFOS.add(fifonum);
      }

      fifo_path = String.format("%s%d",System.getFifosPath(),fifonum);

      mWriter.writeBytes(String.format("mkfifo -m 666 '%s'\n", fifo_path));
      mWriter.flush();
      mWriter.writeBytes(String.format("(%s ; echo -e \"\\n%s$?\") 2>&1 >%s &\n", command, token, fifo_path));
      mWriter.flush();

      gobbler = new StreamGobbler(fifo_path, receiver, token, fifonum);
    }
    catch ( IOException e) {
      System.errorLogging(e);
    }
    if(gobbler==null) {
      fakeThread = new Thread(new Runnable() {
        @Override
        public void run() {
          if(receiver!=null)
            receiver.onEnd(-1);
          return;
        }
      });
      return fakeThread;
    }
    else
      return gobbler;
  }

  public static Thread async(String command) {
    return async(command, null);
  }
}