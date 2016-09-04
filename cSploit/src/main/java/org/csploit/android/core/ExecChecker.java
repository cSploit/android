package org.csploit.android.core;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.UUID;

import org.csploit.android.tools.Fusemounts;

/**
 * this class check if there is a way to execute binary files in some place
 */
public class ExecChecker {

  private static final ArrayList<FuseBind> mFuseBinds = new ArrayList<FuseBind>();

  private final static ExecChecker mRubyChecker = new ExecChecker();
  private final static ExecChecker mMsfChecker = new ExecChecker();

  private String modifiedMountpoit = null;
  private String resolvedDir = null;


  public static ExecChecker ruby() {
    return mRubyChecker;
  }

  public static ExecChecker msf() {
    return mMsfChecker;
  }

  private static class FuseBind {
    public String source,mountpoint;

    @Override
    public boolean equals(Object o) {
      if(o == null || o.getClass() != FuseBind.class)
        return false;

      FuseBind b = (FuseBind)o;

      if(b.source != null) {
        if(!b.source.equals(this.source))
          return false;
      } else if(this.source != null)
        return false;

      if(b.mountpoint != null) {
        if(!b.mountpoint.equals(this.mountpoint))
          return false;
      } else if(this.mountpoint != null)
        return false;

      return true;
    }
  }



  /**
   * test if root can execute stuff inside a directory
   * @param dir the directory to check
   * @return true if root can create executable files inside {@code dir}
   */
  private boolean root(String dir) {

    if(dir==null)
      return false;

    try {
      String tmpname;
      do {
        tmpname = dir + "/" + UUID.randomUUID().toString();
      } while (System.getTools().raw.run(String.format("test -f '%s'", tmpname))==0);

      return System.getTools().shell.run(
              String.format("touch '%1$s' && chmod %2$o '%1$s' && test -x '%1$s' && rm '%1$s'",
                      tmpname, 0755)) == 0;
    } catch (Exception e) {
      Logger.error(e.getMessage());
    }
    return false;
  }

  private static void updateFuseBinds() {
    try {
      System.getTools().fusemounts.getSources(new Fusemounts.fuseReceiver() {
        @Override
        public void onNewMountpoint(String source, String mountpoint) {
          boolean newMountpointFound;
          FuseBind fb = new FuseBind();
          fb.mountpoint = mountpoint;
          fb.source = source;

          synchronized (mFuseBinds) {
            newMountpointFound = !mFuseBinds.contains(fb);
            if(newMountpointFound)
              mFuseBinds.add(fb);
          }

          if(newMountpointFound)
            Logger.info("found a fuse mount: source='" + source + "' mountpoint='" + mountpoint + "'");
        }
      }).join();
    } catch (Exception e) {
      Logger.error(e.getMessage());
    }
  }

  /**
   * search for the real path of file.
   * it retrieve FUSE bind mounts and search if {@code file} is inside one of them.
   * @param file the file/directory to check
   * @return the unrestricted path to file, or null if not found
   */
  private String getRealPath(final String file) {

    if(file==null)
      return null;

    if(mFuseBinds.size()==0)
      updateFuseBinds();

    synchronized (mFuseBinds) {
      for(FuseBind b : mFuseBinds) {
        if(file.startsWith(b.mountpoint)) {
          return b.source + file.substring(b.mountpoint.length());
        }
      }
      return null;
    }
  }

  /**
   * check if the cSploit user can create executable files inside a directory.
   * @param dir directory to check
   * @return true if can execute files into {@code dir}, false otherwise
   */
  private boolean user(String dir) {
    String tmpname;
    File tmpfile = null;

    if(dir==null)
      return false;

    tmpfile = new File(dir);

    try {
      if(!tmpfile.exists())
        tmpfile.mkdirs();

      do {
        tmpname = UUID.randomUUID().toString();
      } while((tmpfile = new File(dir, tmpname)).exists());

      tmpfile.createNewFile();

      return (tmpfile.canExecute() || tmpfile.setExecutable(true, false));

    } catch (IOException e) {
      Logger.warning(String.format("cannot create files over '%s'",dir));
    } finally {
      if(tmpfile!=null && tmpfile.exists())
        tmpfile.delete();
    }
    return false;
  }

  /**
   * check if we can execute binaries in {@code dir}
   * after remounting it without the {@code noexec} mount
   * flag.
   *
   * @param dir the directory to check
   */
  private boolean remount(String dir) {
    BufferedReader mounts = null;
    String line,mountpoint,tmp;

    try {
      // find the mountpoint
      mounts = new BufferedReader(new FileReader("/proc/mounts"));

      mountpoint = null;

      while((line = mounts.readLine()) != null) {
        tmp = line.split(" ")[1];
        if( line.contains("noexec") &&
            dir.startsWith(tmp) &&
            (mountpoint == null || tmp.length() > mountpoint.length())) {
          mountpoint = tmp;
        }
      }

      if(mountpoint == null)
        return false;

      // remount
      if(System.getTools().raw.run(String.format("mount -oremount,exec '%s'", mountpoint))!=0) {
        Logger.warning(String.format("cannot remount '%s' with exec flag", mountpoint));
        return false;
      }

      // check it now
      if(user(dir)) {
        synchronized (this) {
          if(modifiedMountpoit != null && !modifiedMountpoit.equals(mountpoint))
            restoreMountpoint(modifiedMountpoit);
          modifiedMountpoit = mountpoint;
          resolvedDir = dir;
        }
        return true;
      }
    } catch (Exception e) {
      System.errorLogging(e);
    } finally {
      if(mounts!=null) {
        try { mounts.close(); }
        catch (IOException ignored) { }
      }
    }

    return false;
  }

  private void restoreMountpoint(String mountpoint) {
    try {
      if(System.getTools().raw.run(String.format("mount -oremount,noexec '%s'", mountpoint))!=0) {
        Logger.warning(String.format("cannot remount '%s' with noexec flag", mountpoint));
      }
    } catch (Exception e) {
      Logger.error(e.getMessage());
    }
  }

  public boolean canExecuteInDir(String dir) {

    if(!user(dir) && !remount(dir)) {
      dir = getRealPath(dir);
      if(!root(dir)) {
        dir = null;
      }
    }

    if(dir != null) {
      if(System.getKnownIssues().isIssueFound(1)) {
        String corePath = System.getCorePath();

        dir = dir.replace(corePath, "/cSploit");

        corePath = getRealPath(corePath);

        if(corePath != null)
          dir = dir.replace(corePath, "/cSploit");
      }

      synchronized (this) {
        resolvedDir = dir;
      }
      return true;
    }
    return false;
  }

  public String getRoot() {
    return resolvedDir;
  }

  public void clear() {
    synchronized (this) {
      if(modifiedMountpoit!=null) {
        restoreMountpoint(modifiedMountpoit);
        modifiedMountpoit = null;
      }
    }
  }

  @Override
  protected void finalize() throws Throwable {
    clear();
    super.finalize();
  }
}