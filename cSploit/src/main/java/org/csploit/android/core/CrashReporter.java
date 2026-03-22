package org.csploit.android.core;
import org.csploit.android.helpers.LoggingHelper;

import org.acra.ACRA;
import org.acra.ErrorReporter;
import org.csploit.android.tools.Logcat;

/**
 * report crashes to cSploit crash store
 */
public class CrashReporter {

  private static void notifyNativeError(final ErrorReporter reporter, Throwable error) {
    try {
      if (System.getTools() == null) {
        reporter.putCustomData("coreVersion", System.getCoreVersion());
        reporter.putCustomData("rubyVersion", System.getLocalRubyVersion());
        reporter.putCustomData("msfVersion", System.getLocalMsfVersion());
        reporter.handleException(error);
        return;
      }
      Child process = System.getTools().logcat.async(new Logcat.LibcReceiver() {
        @Override
        public void onLogcat(String logcat) {
          reporter.putCustomData("jniBacktrace", logcat);
        }
      });
      reporter.putCustomData("coreVersion", System.getCoreVersion());
      reporter.putCustomData("rubyVersion", System.getLocalRubyVersion());
      reporter.putCustomData("msfVersion", System.getLocalMsfVersion());
      ChildManager.wait(process);
    } catch (ChildManager.ChildDiedException | InterruptedException | ChildManager.ChildNotStartedException e) {
      reporter.putCustomData("coreVersion", System.getCoreVersion());
      reporter.putCustomData("rubyVersion", System.getLocalRubyVersion());
      reporter.putCustomData("msfVersion", System.getLocalMsfVersion());
      LoggingHelper.error(e.getMessage());
    }
    reporter.handleException(error);
  }

  public static void notifyChildCrashed(int childID, int signal) {
    ErrorReporter reporter = ACRA.getErrorReporter();
    if (reporter == null) return;

    reporter.putCustomData("childID", Integer.toString(childID));
    notifyNativeError(reporter, new ChildManager.ChildDiedException(signal));
  }

  public static void notifyNativeLibraryCrash() {
    ErrorReporter reporter = ACRA.getErrorReporter();
    if (reporter == null) return;
    notifyNativeError(reporter, new RuntimeException("JNI library crashed"));
  }
}
