package org.csploit.android.services;

import android.content.Context;

import org.csploit.android.core.System;

/**
 * A service for manage updates
 */
public class UpdateService implements Service {

  private final Context context;
  private Thread checker;

  public UpdateService(Context context) {
    this.context = context;
  }

  @Override
  public boolean start() {
    if(isRunning())
      return false;
    stop();
    checker = new Thread(new UpdateChecker(context), "UpdateChecker");
    checker.start();
    return true;
  }

  @Override
  public boolean stop() {
    if(checker != null) {
      checker.interrupt();
      try {
        checker.join();
        return true;
      } catch (InterruptedException e) {
        System.errorLogging(e);
      }
    }
    return true;
  }

  @Override
  public boolean isRunning() {
    return checker != null && checker.isAlive();
  }
}
