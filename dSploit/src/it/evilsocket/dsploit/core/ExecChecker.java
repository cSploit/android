package it.evilsocket.dsploit.core;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.UUID;

import it.evilsocket.dsploit.tools.Fusemounts;

/**
 * this class check if there is a way to execute binary files in some place
 */
public class ExecChecker {

  private static final ArrayList<fuseBind> mFuseBinds = new ArrayList<fuseBind>();

  private static class fuseBind {
    public String source,mountpoint;

    @Override
    public boolean equals(Object o) {
      if(o.getClass() != fuseBind.class)
        return false;

      fuseBind b = (fuseBind)o;

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
  public static boolean root(String dir) {

    if(dir==null)
      return false;

    try {
      String tmpname;
      do {
        tmpname = dir + "/" + UUID.randomUUID().toString();
      } while (Shell.exec(String.format("test -f '%s'", tmpname))==0);

      return Shell.exec(
              String.format("touch '%1$s' && chmod %2$o '%1$s' && test -x '%1$s' && rm '%1$s'",
                      tmpname, 0755)) == 0;
    } catch (InterruptedException e) {
      Logger.error(e.getMessage());
    } catch (IOException e) {
      Logger.error(e.getMessage());
    }
    return false;
  }

  private static void updateFuseBinds() {
    final StringBuilder sb = new StringBuilder();

    System.getFusemounts().getSources(new Fusemounts.fuseReceiver() {
      @Override
      public void onNewMountpoint(String source, String mountpoint) {
        sb.append(source);
        sb.append("\t");
        sb.append(mountpoint);
        sb.append("\n");
      }
    }).run();

    if (sb.length() == 0) {
      Logger.warning("no fuse mounts found.");
      return;
    }

    for (String line : sb.toString().split("\n")) {
      fuseBind b = new fuseBind();
      String[] parts = line.split("\t");
      b.source = parts[0];
      b.mountpoint = parts[1];
      mFuseBinds.add(b);
    }
  }

  /**
   * search for the real path of file.
   * it retrieve FUSE bind mounts and search if {@code file} is inside one of them.
   * @param file the file/directory to check
   * @return the unrestricted path to file, or null if not found
   */
  public static String getRealPath(final String file) {

    if(file==null)
      return null;

    synchronized (mFuseBinds) {
      if (mFuseBinds.size() == 0)
        updateFuseBinds();
      if(mFuseBinds.size()>0) {
        for(fuseBind b : mFuseBinds) {
          if(file.startsWith(b.mountpoint)) {
            return b.source + file.substring(b.mountpoint.length());
          }
        }
      }
      return null;
    }
  }

  /**
   * check if the dSploit user can create executable files inside a directory.
   * @param dir directory to check
   * @return true if can execute files into {@code dir}, false otherwise
   */
  public static boolean user(String dir) {
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
   * @param restore restore the mountpoint flags after checking
   */
  public static boolean remount(String dir, boolean restore) {
    BufferedReader mounts = null;
    String line,mountpoint,tmp;
    boolean ret;
    ArrayList<String> mountpoints;

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
      if(Shell.exec(String.format("mount -oremount,exec '%s'", mountpoint))!=0) {
        Logger.warning(String.format("cannot remount '%s' with exec flag", mountpoint));
        return false;
      }

      // check it now
      ret = user(dir);

      // restore
      if((!ret || restore) && Shell.exec(String.format("mount -oremount,noexec '%s'", mountpoint))!=0) {
        Logger.warning(String.format("cannot remount '%s' with noexec flag", mountpoint));
      }

      return ret;

    } catch (Exception e) {
      System.errorLogging(e);
    } finally {
      if(mounts!=null) {
        try { mounts.close(); }
        catch (IOException e) {
          //ignored
        }
      }
    }

    return false;
  }
}