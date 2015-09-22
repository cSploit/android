package org.csploit.android.services;

/**
 * A service that can be controlled from the main app menu.
 */
public interface MenuControllableService {
  int getMenuTitleResource();
  boolean isMenuItemEnabled();
  void onMenuClick();
}
