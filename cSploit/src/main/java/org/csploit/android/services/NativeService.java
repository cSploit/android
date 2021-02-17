package org.csploit.android.services;

import android.content.Context;
import android.content.Intent;

import org.csploit.android.core.Child;

import java.io.Serializable;

/**
 * Implementation of common stuff for native Services
 */
public abstract class NativeService implements Service {
  protected Child nativeProcess;
  protected Context context;

  protected abstract int getStopSignal();

  @Override
  public boolean stop() {
    if(nativeProcess == null || !nativeProcess.running)
      return false;

    nativeProcess.kill(getStopSignal());
    try {
      nativeProcess.join();
    } catch (InterruptedException e) {
      return false;
    }
    return true;
  }

  @Override
  public boolean isRunning() {
    return nativeProcess != null && nativeProcess.running;
  }

  /**
   * send an intent to context
   * @param action to associate to the intent
   * @param extraKey key of te extra value ( no extra if this parameter is null )
   * @param extraValue value of the extra
   */
  protected void sendIntent(String action, String extraKey, Serializable extraValue) {
    assert context != null;

    Intent intent = new Intent(action);
    if(extraKey != null) {
      intent.putExtra(extraKey, extraValue);
    }

    context.sendBroadcast(intent);
  }

  /**
   * send an intent to context
   * @param action to associate to the intent
   */
  protected void sendIntent(String action) {
    sendIntent(action, null, null);
  }
}
