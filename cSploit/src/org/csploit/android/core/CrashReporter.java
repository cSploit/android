package org.csploit.android.core;

import org.acra.ACRA;
import org.acra.ErrorReporter;
import org.csploit.android.events.Event;
import org.csploit.android.tools.Raw;

/**
 * a class that manage crash reports
 */
public class CrashReporter {

  /**
   * receive logcat output and take only libc fatal messages
   */
  private static class LogcatNativeReceiver extends Raw.RawReceiver {
    private final StringBuilder sb = new StringBuilder();
    private String libcFingerprint = null;
    private final String message;

    public LogcatNativeReceiver(String message) {
      this.message = message;
    }

    @Override
    public void onNewLine(String line) {
      if (libcFingerprint == null) {
        if(!line.contains("*** *** ***")) {
          return;
        }
        libcFingerprint = line.substring(0, line.indexOf(':' + 1));
      } else if (!line.startsWith(libcFingerprint)) {
        return;
      }

      sb.append(line);
      sb.append("\n"); // replace with something else ?
    }

    @Override
    public void onEnd(int exitValue) {
      ErrorReporter reporter = ACRA.getErrorReporter();
      reporter.putCustomData("nativeBacktrace", sb.toString());
      reporter.putCustomData("coreVersion", System.getCoreVersion());
      reporter.putCustomData("rubyVersion", System.getLocalRubyVersion());
      reporter.putCustomData("msfVersion", System.getLocalMsfVersion());
      reporter.handleException(new RuntimeException(message), false);
    }
  }

  public static void beginChildCrashReport(final int childID, final Event event) {
    ACRA.getErrorReporter().putCustomData("childID", Integer.toString(childID));
    ACRA.getErrorReporter().putCustomData("event", event.toString());
    beginNativeCrashReport("a child has died unexpectedly");
  }

  public static void beginNativeCrashReport(String message) {
    try {
      System.getTools().raw.async("logcat -d", new LogcatNativeReceiver(message));
    } catch (ChildManager.ChildNotStartedException e) {
      Logger.error(e.getMessage());
    }
  }
}
