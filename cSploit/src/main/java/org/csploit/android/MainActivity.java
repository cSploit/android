
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
import android.content.res.Configuration;
import android.net.Uri;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.design.widget.NavigationView;
import android.support.v4.app.Fragment;
import android.support.v4.view.GravityCompat;
import android.support.v4.widget.DrawerLayout;
import android.support.v7.app.ActionBarDrawerToggle;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
import android.view.MenuItem;
import android.widget.Toast;

import org.csploit.android.core.*;
import org.csploit.android.core.System;
import org.csploit.android.gui.dialogs.AboutDialog;
import org.csploit.android.gui.fragments.Heartless;
import org.csploit.android.gui.fragments.Init;
import org.csploit.android.gui.fragments.TargetDetail;
import org.csploit.android.gui.fragments.TargetList;
import org.csploit.android.helpers.FragmentHelper;
import org.csploit.android.helpers.GUIHelper;
import org.csploit.android.net.Target;
import org.csploit.android.services.Services;
import org.csploit.android.services.receivers.UpdateReceiver;

public class MainActivity extends AppCompatActivity
        implements NavigationView.OnNavigationItemSelectedListener,
        TargetList.OnFragmentInteraction,
        TargetDetail.OnFragmentInteractionListener,
        Init.OnFragmentInteractionListener,
        UpdateReceiver.CoreUpdateListener {

  private ActionBarDrawerToggle mDrawerToggle;
  private DrawerLayout mDrawerLayout;
  private NavigationView nvDrawer;
  private Toolbar toolbar;
  private UpdateReceiver updateReceiver;
  private boolean isShown;

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);

    setContentView(R.layout.main);

    toolbar = (Toolbar) findViewById(R.id.toolbar);
    setSupportActionBar(toolbar);

    GUIHelper.setupTheme(this);

    if (savedInstanceState != null) {
      return;
    }

    mDrawerLayout = (DrawerLayout) findViewById(R.id.drawer_layout);
    nvDrawer = (NavigationView) findViewById(R.id.nvView);

    nvDrawer.setNavigationItemSelectedListener(this);

    mDrawerToggle = new ActionBarDrawerToggle(this,
            mDrawerLayout, R.string.drawer_was_opened, R.string.drawer_was_closed);
    mDrawerLayout.setDrawerListener(mDrawerToggle);

    updateReceiver = new UpdateReceiver(this);
    updateReceiver.register(this);
  }

  @Override
  protected void onPostResume() {
    super.onPostResume();
    isShown = true;
    init();
  }

  @Override
  protected void onSaveInstanceState(Bundle outState) {
    isShown = false;
    super.onSaveInstanceState(outState);
  }

  @Override
  protected void onDestroy() {
    updateReceiver.unregister();
    updateReceiver = null;

    super.onDestroy();
  }

  private void init() {
    if(!System.isCoreInstalled()) {
      FragmentHelper.switchToFragment(this, Heartless.class);
    } else if(!System.isInitialized() || !System.isCoreInitialized()) {
      FragmentHelper.switchToFragment(this, Init.class);
    } else {
      onInitDone();
    }
  }

  @Override
  public void onCoreUpdated() {
    if(isShown) {
      init();
    }
  }

  @Override
  public void onInitDone() {
    if(!isShown) {
      return;
    }

    // loadInterfaces(); => load interfaces into drawer
    // services should be started by Init
    FragmentHelper.switchToFragment(this, TargetList.class);
  }

  @Override
  public boolean onNavigationItemSelected(MenuItem item) {
    Fragment fragment = null;
    String iface = null;

    switch(item.getItemId()) {
      case R.id.nav_iface_eth0:
        iface = "eth0";
        fragment = getSupportFragmentManager().findFragmentByTag("TargetListFragment");
        break;
      case R.id.nav_iface_rmnet:
        iface = "rmnet_usb0";
        fragment = getSupportFragmentManager().findFragmentByTag("TargetListFragment");
        break;
      case R.id.nav_iface_wlan0:
        iface = "wlan0";
        fragment = getSupportFragmentManager().findFragmentByTag("TargetListFragment");
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
      FragmentHelper.switchToFragment(this, fragment);
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

    Services.getNetworkRadar().stop();

    System.setIfname(ifname);

    boolean success = System.reloadNetworkMapping();

    if(!success) {
      Logger.warning("failed to change iface from " + current + " to " + ifname);

      System.setIfname(current);
      System.reloadNetworkMapping();
    }

    Services.getNetworkRadar().start();

    return success;
  }

  private void launchSettings() {
    FragmentHelper.switchToFragment(this, SettingsFragment.PrefsFrag.class);
  }

  @Override
  public void onPluginChosen(Plugin plugin) {
    if (System.checkNetworking(this)) {
      System.setCurrentPlugin(plugin);

      if (plugin.hasLayoutToShow()) {
        Toast.makeText(this, getString(R.string.selected) + getString(plugin.getName()), Toast.LENGTH_SHORT).show();
        FragmentHelper.switchToFragment(this, plugin);
      } else
        plugin.onActionClick(this);
    }
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
  public void onTargetSelected(Target target) {
    //TODO: master/detail view

    Toast.makeText(this, getString(R.string.selected_) + target.getDisplayAddress(), Toast.LENGTH_SHORT).show();

    TargetDetail fragment = (TargetDetail) FragmentHelper.getCachedFragment(this, TargetDetail.class);

    if(fragment == null) {
      fragment = TargetDetail.newInstance(target);
    } else {
      TargetDetail.setUp(fragment, target);
    }

    FragmentHelper.openFragment(this, fragment);
  }

  @Override
  public void onBackPressed() {
    if(mDrawerLayout.isDrawerOpen(GravityCompat.START)) {
      mDrawerLayout.closeDrawer(GravityCompat.START);
    } else if (getSupportFragmentManager().getBackStackEntryCount() == 0) {
      super.onBackPressed();
    } else {
      getSupportFragmentManager().popBackStack();
    }
  }
}