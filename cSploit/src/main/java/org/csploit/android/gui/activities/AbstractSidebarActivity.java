package org.csploit.android.gui.activities;

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
import android.widget.FrameLayout;
import android.widget.Toast;

import org.csploit.android.R;
import org.csploit.android.SettingsFragment;
import org.csploit.android.core.*;
import org.csploit.android.core.System;
import org.csploit.android.gui.dialogs.AboutDialog;
import org.csploit.android.gui.dialogs.ConfirmDialog;
import org.csploit.android.gui.dialogs.ErrorDialog;
import org.csploit.android.gui.dialogs.ProgressDialog;
import org.csploit.android.gui.fragments.TargetList;
import org.csploit.android.helpers.FragmentHelper;
import org.csploit.android.helpers.GUIHelper;
import org.csploit.android.helpers.ThreadHelper;
import org.csploit.android.services.Services;

/**
 * An activity that have a sidebar panel ( a navigation drawer )
 */
public abstract class AbstractSidebarActivity extends AppCompatActivity
        implements NavigationView.OnNavigationItemSelectedListener {

  /**
   * seconds to wait for another back to close the app
   */
  private final static int BACK_TO_CLOSE_TIMEOUT = 4;

  private ActionBarDrawerToggle drawerToggle;
  private DrawerLayout drawerLayout;
  private NavigationView nvDrawer;
  private Toolbar toolbar;
  private long backToCloseTimeout = 0;
  private Toast backPressedToast;
  private boolean isShown;

  @Override
  public void setContentView(int layoutResID) {
    drawerLayout = (DrawerLayout) getLayoutInflater().inflate(R.layout.nav, null);
    FrameLayout mainframe = (FrameLayout) drawerLayout.findViewById(R.id.mainframe);

    // Setting the content of layout your provided to the act_content frame
    getLayoutInflater().inflate(layoutResID, mainframe, true);
    super.setContentView(drawerLayout);

    toolbar = (Toolbar) findViewById(R.id.toolbar);
    setSupportActionBar(toolbar);

    GUIHelper.setupTheme(this);

    nvDrawer = (NavigationView) findViewById(R.id.nvView);
    nvDrawer.setNavigationItemSelectedListener(this);

    drawerToggle = new ActionBarDrawerToggle(this,
            drawerLayout, R.string.drawer_was_opened, R.string.drawer_was_closed);
    drawerLayout.setDrawerListener(drawerToggle);
  }

  @Override
  public boolean onNavigationItemSelected(MenuItem item) {
    Fragment fragment = null;
    String iface = null;

    switch(item.getItemId()) {
      case R.id.nav_iface_eth0:
        iface = "eth0";
        break;
      case R.id.nav_iface_rmnet:
        iface = "rmnet_usb0";
        break;
      case R.id.nav_iface_wlan0:
        iface = "wlan0";
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
      changeIface(iface);
    } else if(fragment != null) {
      FragmentHelper.switchToFragment(this, fragment);
      setTitle(item.getTitle());
    }

    drawerLayout.closeDrawers();

    return fragment != null;
  }

  private void submitIssue() {
    String uri = getString(R.string.github_new_issue_url);
    Intent browser = new Intent(Intent.ACTION_VIEW, Uri.parse(uri));
    startActivity(browser);

    // for fat-tire:
    //   String.format(getString(R.string.issue_message), getString(R.string.github_issues_url), getString(R.string.github_new_issue_url));
  }

  private void launchAbout() {
    new AboutDialog(this).show();
  }

  private void launchSettings() {
    FragmentHelper.switchToFragment(this, SettingsFragment.PrefsFrag.class);
  }

  private void changeIface(@NonNull final String ifname) {
    final String current = org.csploit.android.core.System.getNetwork().getInterface().getDisplayName();

    if(ifname.equals(current)) {
      return;
    }

    final ProgressDialog dialog = new ProgressDialog("changing interface", "please wait", this);
    dialog.setMinElapsedTime(200);
    dialog.setMinShownTime(3000);

    dialog.show();

    ThreadHelper.getSharedExecutor().execute(new Runnable() {
      @Override
      public void run() {
        Services.getNetworkRadar().stop();

        System.setIfname(ifname);

        boolean success = System.reloadNetworkMapping();
        String errMsg = null;

        if (!success) {
          errMsg = "failed to change iface from " + current + " to " + ifname;
          Logger.warning(errMsg);

          System.setIfname(current);
          System.reloadNetworkMapping();
        } else if (canSwitchFragment()) {
          FragmentHelper.switchToFragment(AbstractSidebarActivity.this, TargetList.class);
        }

        Services.getNetworkRadar().start();

        dialog.dismiss();

        if(errMsg != null) {
          final String message = errMsg;
          runOnUiThread(new Runnable() {
            @Override
            public void run() {
              new ErrorDialog(getString(R.string.error), message, AbstractSidebarActivity.this).show();
            }
          });
        }
      }
    });
  }

  @Override
  public boolean onOptionsItemSelected(final MenuItem item) {
    if (drawerToggle.onOptionsItemSelected(item)) {
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

  @Override
  protected void onPostCreate(Bundle savedInstanceState) {
    super.onPostCreate(savedInstanceState);
    // Sync the toggle state after onRestoreInstanceState has occurred.
    drawerToggle.syncState();
  }

  @Override
  public void onConfigurationChanged(Configuration newConfig) {
    super.onConfigurationChanged(newConfig);
    // Pass any configuration change to the drawer toggles
    drawerToggle.onConfigurationChanged(newConfig);
  }

  private void onLastBack() {
    if(backPressedToast != null) {
      backPressedToast.cancel();
    }

    if (backToCloseTimeout < java.lang.System.currentTimeMillis()) {
      backPressedToast = Toast.makeText(this, getString(R.string.press_back), Toast.LENGTH_SHORT);
      backPressedToast.show();
      backToCloseTimeout = java.lang.System.currentTimeMillis() + BACK_TO_CLOSE_TIMEOUT;
    } else {
      new ConfirmDialog(getString(R.string.exit),
              getString(R.string.close_confirm), this,
              new ConfirmDialog.ConfirmDialogListener() {
                @Override
                public void onConfirm() {
                  finish();
                }

                @Override
                public void onCancel() {
                }
              }).show();

      backToCloseTimeout = 0;
    }
  }

  @Override
  public void onBackPressed() {
    if(drawerLayout.isDrawerOpen(GravityCompat.START)) {
      drawerLayout.closeDrawer(GravityCompat.START);
    } else if (getSupportFragmentManager().getBackStackEntryCount() == 0) {
      onLastBack();
    } else {
      getSupportFragmentManager().popBackStack();
    }
  }

  protected boolean canSwitchFragment() {
    return isShown;
  }

  @Override
  protected void onPostResume() {
    super.onPostResume();
    isShown = true;
  }

  @Override
  protected void onSaveInstanceState(Bundle outState) {
    isShown = false;
    super.onSaveInstanceState(outState);
  }
}
