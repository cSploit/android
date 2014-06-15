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

import java.io.BufferedReader;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.IOException;
import java.io.OutputStream;
import java.lang.Process;
import java.util.ArrayList;
import java.util.UUID;
import java.util.concurrent.Semaphore;

public class Shell
{
  private final static int MAX_FIFO = 1024;
  private final static String rubyLib = "%1$s/site_ruby/1.9.1:%1$s/site_ruby/1.9.1/arm-linux-androideabi:%1$s/site_ruby:%1$s/vendor_ruby/1.9.1:%1$s/vendor_ruby/1.9.1/arm-linux-androideabi:%1$s/vendor_ruby:%1$s/1.9.1:%1$s/1.9.1/arm-linux-androideabi";

  private static Semaphore mRootShellLock = new Semaphore(1);
  private static Process mRootShell = null;
  private static DataOutputStream mWriter = null;
  //private static BufferedReader reader = null, error = null;
  private final static ArrayList<Integer> mFIFOS = new ArrayList<Integer>();
  private static boolean rubyEnvironExported = false;
  private static boolean gotR00t = false;

  /**
   * "gobblers" seem to be the recommended way to ensure the streams
   * don't cause issues.
   * this class keeps reading from process output until they close it.
   */
  public static class StreamGobbler extends Thread {
    private BufferedReader mReader = null;
    private OutputReceiver mReceiver = null;
    private OutputStream mOutputStream = null;
    private String mToken = null;
    private String mFIFOPath = null;
    private String mOutputFIFOPath = null;
    private int mFIFONum = -1;
    private int mOutputFIFONum = -1;
    public int exitValue = -1;

    public StreamGobbler(String fifoPath, OutputReceiver receiver, String token, int fifoNum) {
      mFIFOPath = fifoPath;
      mReceiver = receiver;
      mToken = token;
      mFIFONum = fifoNum;
      setDaemon(true);
    }

    public void setOutputFifo(String fifoPath, int fifoNum) {
      mOutputFIFONum = fifoNum;
      mOutputFIFOPath = fifoPath;
    }

    public OutputStream getOutputStream(){
      return mOutputStream;
    }

    public void run(){
      try
      {
        String line;
        // wait for the FIFO to be created
        while(!(new File(mFIFOPath).exists()))
          Thread.sleep(200);
        if(mOutputFIFOPath!=null)
          while(!(new File(mOutputFIFOPath)).exists())
            Thread.sleep(200);

        // this will hang until someone will connect to the other side of the pipe.
        mReader = new BufferedReader(new FileReader(mFIFOPath));
        if(mOutputFIFOPath!=null)
          mOutputStream = new FileOutputStream(mOutputFIFOPath);

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
          if(mOutputStream!=null)
            mOutputStream.close();
          synchronized (mFIFOS) {
            mFIFOS.remove(mFIFOS.indexOf(mFIFONum));
            if(mOutputFIFONum>=0)
              mFIFOS.remove(mFIFOS.indexOf(mOutputFIFONum));
          }
        } catch(IOException e){
          System.errorLogging(e);
        }
      }
    }
  }

  private static Process spawnSuShell() throws IOException {
    return new ProcessBuilder().command("su").start();
  }

  public static boolean isRootGranted() {
    if(gotR00t)
      return true;

    try{
      final StringBuilder output = new StringBuilder();
      OutputReceiver receiver = new OutputReceiver() {
        @Override
        public void onStart(String command) {

        }

        @Override
        public void onNewLine(String line) {
          output.append(line + "\n");
        }

        @Override
        public void onEnd(int exitCode) {

        }
      };

      if(exec("id", receiver) == 0)
        gotR00t = output.toString().contains("uid=0");
    } catch(Exception e){
      System.errorLogging(e);
    }
    return gotR00t;
  }

  public interface OutputReceiver {
    public void onStart(String command);

    public void onNewLine(String line);

    public void onEnd(int exitCode);
  }

  public static boolean isBinaryAvailable(String binary) {
    return isBinaryAvailable(binary, false);
  }

  public static boolean isBinaryAvailable(String binary, boolean execute) {
    try{
      if(execute)
        return exec(binary) != 127;
      else
        return exec("which "+binary) == 0;
    } catch(Exception e){
      System.errorLogging(e);
    }
    return false;
  }

  private static void clearFifos() throws IOException {
    File d = new File(System.getFifosPath());
    File[] files;
    if(!d.exists()) {
      if (!d.mkdir())
        throw new IOException("cannot create fifo directory");
    } else if(!d.isDirectory()) {
      if(!d.delete())
        throw new IOException(String.format("cannot delete a file named fifo under %s", d.getAbsolutePath()));
      if(!d.mkdir())
        throw new IOException("cannot create fifo directory");
    }
    if(d.isDirectory() && (files = d.listFiles())!=null) {
      for(File f : files) {
        if(!f.delete())
          throw new IOException(String.format("cannot delete fifo `%s'", f.getAbsolutePath()));
      }
    }
    else {
      throw new IOException("cannot create fifo directory");
    }
  }

  /**
   * execute a command in a root shell
   * @param command    the command to execute
   * @param receiver   the {@link it.evilsocket.dsploit.core.Shell.OutputReceiver} that will receive command output
   * @param withInput  true if should create an output stream connected to the command stdin
   * @return the exit code of the command
   * @throws IOException if cannot create files, streams or processes
   * @throws InterruptedException when interrupt is received
   */
  public static int exec(String command, OutputReceiver receiver, boolean withInput) throws IOException, InterruptedException {
    Thread g = async(command, receiver, withInput);
    if(!(g instanceof StreamGobbler))
      throw new IOException("cannot execute shell commands");
    g.start();
    g.join();
    return  ((StreamGobbler)g).exitValue;
  }

  /**
   * {@code withInput} default to false
   * @see
   */
  public static int exec(String command, OutputReceiver receiver) throws IOException, InterruptedException {
    return exec(command, receiver, false);
  }

  /**
   * {@inheritDoc exec}
   */
  public static int exec(String command) throws IOException, InterruptedException {
    return exec(command, null, false);
  }

  /**
   * start command in a root shell now and read it's output asynchronously
   * @param command    the command to execute
   * @param receiver   the {@link it.evilsocket.dsploit.core.Shell.OutputReceiver} that will receive command output
   * @param withInput  true if should create an output stream connected to the command stdin
   * @return {@link it.evilsocket.dsploit.core.Shell.StreamGobbler} that keep reading from shell output until process dies,
   *          a fakeThread on error.
   */
  @SuppressWarnings("StatementWithEmptyBody")
  public static Thread async(final String command, final OutputReceiver receiver, boolean withInput) {
    int fifonum,inputFifoNum;
    String fifo_path,input_fifo_path,fifo_dir;
    String token = UUID.randomUUID().toString().toUpperCase();
    StreamGobbler gobbler = null;

    try
    {
      // spawn shell for the first time
      mRootShellLock.acquire();
      try {
        if (mRootShell == null) {
          Logger.info("initializing root shell");
          mRootShell = spawnSuShell();
          mWriter = new DataOutputStream(mRootShell.getOutputStream());
          int ret = -1;
          try {
            mWriter.write("exit 0\n".getBytes());
            mWriter.flush();
            ret = mRootShell.waitFor();
          } catch (Exception e) {
            System.errorLogging(e);
          }

          if(ret!=0) {
            mRootShell=null;
            throw new IOException(String.format("cannot execute `su`: exitValue=%d", ret));
          }
          Logger.debug("I have superpowers!");
          mWriter.close();
          mRootShell = spawnSuShell();
          mWriter = new DataOutputStream(mRootShell.getOutputStream());
          // this 2 reader are useful for debugging purposes
          //reader = new BufferedReader(new InputStreamReader(mRootShell.getInputStream()));
          //error = new BufferedReader(new InputStreamReader(mRootShell.getErrorStream()));
          clearFifos();

          Logger.debug("checking fifo...");

          fifo_path = String.format("%s0",System.getFifosPath());
          mWriter.write(String.format("mkfifo -m 666 '%s'\n", fifo_path).getBytes());
          mWriter.flush();
          long start,elapsed;

          start = java.lang.System.currentTimeMillis();
          while((elapsed=(java.lang.System.currentTimeMillis() - start)) < 1000 && !(new File(fifo_path)).exists())
            Thread.sleep(10);

          if(elapsed>1000) {
            mRootShell=null;
            mWriter.close();
            throw new IOException("cannot create fifo");
          }

          Logger.debug(String.format("fifo created after %d ms",elapsed));

          // change umask to give read and execution permission to java,
          // thus to check file existence from java. ( default is 077 )
          mWriter.write("umask 022\n".getBytes());
          mWriter.flush();

          Logger.info("root shell initialized");
          rubyEnvironExported=false;
        }
      } finally {
        mRootShellLock.release();
      }

      if(receiver != null)
        receiver.onStart(command);

      fifo_dir = System.getFifosPath();

      synchronized (mFIFOS)
      {
        // check for next available FIFO number
        for( fifonum = 0; fifonum < MAX_FIFO && mFIFOS.contains(fifonum); fifonum++ );
        if(fifonum==MAX_FIFO)
          throw new IOException("maximum number of fifo reached");

        mFIFOS.add(fifonum);

        if(withInput) {
          for(inputFifoNum=0; inputFifoNum < MAX_FIFO && mFIFOS.contains(inputFifoNum); inputFifoNum++);
          if(inputFifoNum==MAX_FIFO)
            throw new IOException("maximum number of fifo reached");
          mFIFOS.add(inputFifoNum);
        } else
          inputFifoNum=-1;
      }

      fifo_path = String.format("%s%d",fifo_dir,fifonum);

      mWriter.writeBytes(String.format("mkfifo -m 666 '%s'\n", fifo_path));
      if(inputFifoNum>=0) {
        input_fifo_path = String.format("%s%d",fifo_dir, inputFifoNum);
        mWriter.writeBytes(String.format("mkfifo -m 666 '%s'\n", input_fifo_path));
      } else
        input_fifo_path = null;
      mWriter.writeBytes(String.format("(%s%s echo -e \"\\n%s$?\") 2>&1 >%s %s &\n",
              command,
              (command.trim().endsWith(";") ? "" : " ;"),
              token,
              fifo_path,
              (inputFifoNum>=0 ? "<"+input_fifo_path : "")));
      mWriter.flush();

      gobbler = new StreamGobbler(fifo_path, receiver, token, fifonum);
      if(inputFifoNum>=0)
        gobbler.setOutputFifo(input_fifo_path, inputFifoNum);
    } catch ( InterruptedException e) {
      Logger.error("interrupted while acquiring mRootShellLock");
    } catch ( IOException e) {
      System.errorLogging(e);
    }
    if(gobbler==null) {
      return new Thread(new Runnable() {
        @Override
        public void run() {
          if(receiver!=null)
            receiver.onEnd(-1);
        }
      });
    }
    else
      return gobbler;
  }

  /**
   * calls {@link #async(String, it.evilsocket.dsploit.core.Shell.OutputReceiver, boolean)}
   * with {@code withInput} to false
   */
  public static Thread async(String command, OutputReceiver receiver) {
    return async(command, receiver, false);
  }

  /**
   * calls {@link #async(String, it.evilsocket.dsploit.core.Shell.OutputReceiver, boolean)}
   * with null {@link it.evilsocket.dsploit.core.Shell.OutputReceiver}
   * and {@code withInput} to false
   */
  public static Thread async(String command) {
    return async(command, null);
  }

  public static void setupRubyEnviron() throws IOException {
    if(rubyEnvironExported)
      return;
    synchronized (mFIFOS) { // must be sure that no one else write on our main shell
      mWriter.writeBytes(String.format("export RUBYLIB=\"" + rubyLib + "\"\n", System.getRubyPath() + "/lib/ruby"));
      mWriter.writeBytes(String.format("export PATH=\"$PATH:%s:%s\"\n", System.getRubyPath() + "/bin", System.getMsfPath()));
      mWriter.writeBytes(String.format("export HOME=\"%s\"\n", System.getRubyPath() + "/home/ruby"));
      mWriter.flush();
    }
    rubyEnvironExported=true;
  }

  public static void rubySettingsChanged() {
    rubyEnvironExported=false;
  }
}