package org.csploit.android.services;

/**
 * Represent a service
 *
 * TODO: use Future for async ?
 */
public interface Service {
  boolean start();
  void startAsync();
  boolean stop();
  void stopAsync();
  boolean isRunning();
}
