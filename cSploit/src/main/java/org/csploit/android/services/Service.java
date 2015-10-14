package org.csploit.android.services;

/**
 * Represent a service
 */
public interface Service {
  boolean start();
  boolean stop();
  boolean isRunning();
}
