package org.csploit.android.helpers;

import android.os.Looper;

import java.util.concurrent.Executor;
import java.util.concurrent.Executors;

/**
 * Help cSploit dealing with threads
 */
public final class ThreadHelper {

  private final static Executor EXECUTOR = Executors.newCachedThreadPool();

  /**
   * share an Executor among all cSploit components
   * @return an Executor for running background stuff
   */
  public static Executor getSharedExecutor() {
    return EXECUTOR;
  }

  public static boolean isOnMainThread() {
    return Looper.myLooper() == Looper.getMainLooper();
  }
}
