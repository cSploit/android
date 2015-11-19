
/*
 * This file is part of the dSploit.
 *
 * Copyleft of Simone Margaritelli aka evilsocket <evilsocket@gmail.com>
 *
 * dSploit is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * dSploit is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with dSploit.  If not, see <http://www.gnu.org/licenses/>.
 */
package org.csploit.android;

import android.content.Intent;
import android.content.SharedPreferences;
import android.content.res.Configuration;
import android.net.Uri;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.design.widget.NavigationView;
import android.support.v4.app.Fragment;
import android.support.v4.widget.DrawerLayout;
import android.support.v7.app.ActionBarDrawerToggle;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
import android.view.Menu;
import android.view.MenuItem;

import org.csploit.android.core.*;
import org.csploit.android.core.System;
import org.csploit.android.gui.dialogs.AboutDialog;

public class MainActivity extends AppCompatActivity
        implements NavigationView.OnNavigationItemSelectedListener {

  private MainFragment mMainFragment;
  private ActionBarDrawerToggle mDrawerToggle;
  private DrawerLayout mDrawerLayout;
  private NavigationView nvDrawer;
  private Toolbar toolbar;
  private Menu mMenu = null;

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    SharedPreferences themePrefs = getSharedPreferences("THEME", 0);
    if (themePrefs.getBoolean("isDark", false))
      setTheme(R.style.DarkTheme);
    else
      setTheme(R.style.AppTheme);

    setContentView(R.layout.main);

    toolbar = (Toolbar) findViewById(R.id.toolbar);
    setSupportActionBar(toolbar);

    if (savedInstanceState != null) {
      return;
    }

    mDrawerLayout = (DrawerLayout) findViewById(R.id.drawer_layout);
    nvDrawer = (NavigationView) findViewById(R.id.nvView);

    nvDrawer.setNavigationItemSelectedListener(this);

    mDrawerToggle = new ActionBarDrawerToggle(this,
            mDrawerLayout, R.string.drawer_was_opened, R.string.drawer_was_closed);
    mDrawerLayout.setDrawerListener(mDrawerToggle);

    getSupportActionBar().setDisplayHomeAsUpEnabled(true);
    getSupportActionBar().setHomeButtonEnabled(true);


    mMainFragment = new MainFragment();
    getSupportFragmentManager().beginTransaction()
            .add(R.id.mainframe, mMainFragment, "MainFragment")
            .setCustomAnimations(R.anim.fadein, R.anim.fadeout, R.anim.fadein, R.anim.fadeout)
            .commit();
  }

  @Override
  public boolean onNavigationItemSelected(MenuItem item) {
    Fragment fragment = null;
    String iface = null;

    switch(item.getItemId()) {
      case R.id.nav_iface_eth0:
        iface = "eth0";
        fragment = getSupportFragmentManager().findFragmentByTag("MainFragment");
        break;
      case R.id.nav_iface_rmnet:
        iface = "rmnet_usb0";
        fragment = getSupportFragmentManager().findFragmentByTag("MainFragment");
        break;
      case R.id.nav_iface_wlan0:
        iface = "wlan0";
        fragment = getSupportFragmentManager().findFragmentByTag("MainFragment");
        break;
      case R.id.nav_settings:
        fragment = new SettingsFragment.PrefsFrag();
        break;
      case R.id.nav_about:
        launchAbout();
        break;
      case R.id.nav_report_issue:
        submitIssue();
        break;
    }

    if(iface != null) {
      if(!changeIface(iface)) {
        mDrawerLayout.closeDrawers();
        return false;
      }
    }

    if(fragment != null) {
      changeFragment(fragment);
      setTitle(item.getTitle());
    }

    mDrawerLayout.closeDrawers();

    return fragment != null;
  }

  private boolean changeIface(@NonNull String ifname) {
    String current = System.getNetwork().getInterface().getDisplayName();

    if(ifname.equals(current)) {
      return true;
    }

    System.setIfname(ifname);
    if(System.reloadNetworkMapping()) {
      return true;
    }

    Logger.warning("failed to change iface from " + current + " to " + ifname);

    System.setIfname(current);
    System.reloadNetworkMapping();
    return false;
  }

  private void changeFragment(@NonNull Fragment fragment) {
    Logger.debug("loading fragment " + fragment);
    getSupportFragmentManager().beginTransaction()
            .replace(R.id.mainframe, fragment)
            .addToBackStack(null)
            .setCustomAnimations(R.anim.fadein, R.anim.fadeout, R.anim.fadein, R.anim.fadeout)
            .commit();
  }

  private void launchSettings() {
    changeFragment(new SettingsFragment.PrefsFrag());
  }


  @Override
  public boolean onOptionsItemSelected(final MenuItem item) {
    if (mDrawerToggle.onOptionsItemSelected(item)) {
      return true;
    }

    switch (item.getItemId()) {

      case R.id.nav_settings:
        launchSettings();
        return true;

      case R.id.nav_about:
        launchAbout();
        return true;

      case R.id.nav_report_issue:
        submitIssue();
        return true;
    }

    return super.onOptionsItemSelected(item);
  }

  public void submitIssue() {
    String uri = getString(R.string.github_new_issue_url);
    Intent browser = new Intent(Intent.ACTION_VIEW, Uri.parse(uri));
    startActivity(browser);

    // for fat-tire:
    //   String.format(getString(R.string.issue_message), getString(R.string.github_issues_url), getString(R.string.github_new_issue_url));

  }

  public void launchAbout() {
    new AboutDialog(this).show();
  }

  @Override
  protected void onPostCreate(Bundle savedInstanceState) {
    super.onPostCreate(savedInstanceState);
    // Sync the toggle state after onRestoreInstanceState has occurred.
    mDrawerToggle.syncState();
  }

  @Override
  public void onConfigurationChanged(Configuration newConfig) {
    super.onConfigurationChanged(newConfig);
    // Pass any configuration change to the drawer toggles
    mDrawerToggle.onConfigurationChanged(newConfig);
  }

  @Override
  public void onBackPressed() {
    // Only ask for the double-backpressed when MainFragment is all that's left.
    if (getSupportFragmentManager().getBackStackEntryCount() == 0) {
      mMainFragment.onBackPressed();
    } else {
      super.onBackPressed();
    }
  }
}