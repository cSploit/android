package org.csploit.android.services;

import android.app.Activity;
import android.view.MenuItem;

/**
 * A service that can be controlled from the main app menu.
 */
public interface MenuControllableService {
  void onMenuClick(Activity activity, MenuItem item);
  void buildMenuItem(MenuItem item);
}
