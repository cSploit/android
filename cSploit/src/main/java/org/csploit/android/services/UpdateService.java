package org.csploit.android.services;

import android.content.Context;

import org.csploit.android.core.System;
import org.csploit.android.helpers.ThreadHelper;

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
  public void startAsync() {
    if(isRunning())
      return;

    ThreadHelper.getSharedExecutor().execute(new Runnable() {
      @Override
      public void run() {
        start();
      }
    });
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
  public void stopAsync() {
    if(checker == null)
      return;
    ThreadHelper.getSharedExecutor().execute(new Runnable() {
      @Override
      public void run() {
        stop();
      }
    });
  }

  @Override
  public boolean isRunning() {
    return checker != null && checker.isAlive();
  }
}
