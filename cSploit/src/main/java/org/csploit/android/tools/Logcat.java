package org.csploit.android.tools;

/**
 * Manage android logcat
 *
 * this isn't a cSploit tool
 */
public class Logcat extends Raw {
  /**
   * receive logcat output
   */
  public abstract static class FullReceiver extends RawReceiver {

    private final StringBuilder sb = new StringBuilder();

    @Override
    public void onNewLine(String line) {
      sb.append(line);
      sb.append("\n");
    }

    @Override
    final public void onEnd(int exitValue) {
      onLogcat(sb.toString());
    }

    public abstract void onLogcat(String logcat);
  }

  /**
   * receive libc fatal messages from logcat
   */
  public abstract static class LibcReceiver extends FullReceiver {
    private String libcFingerprint = null;

    @Override
    public void onNewLine(String line) {
      if (libcFingerprint == null) {
        if(!line.contains("*** *** ***")) {
          return;
        }
        libcFingerprint = line.substring(0, line.indexOf(':') + 1);
      } else if (!line.startsWith(libcFingerprint)) {
        return;
      }

      super.onNewLine(line);
    }
  }

  public Logcat() {
    super();
    mCmdPrefix = "logcat -d";
  }
}
