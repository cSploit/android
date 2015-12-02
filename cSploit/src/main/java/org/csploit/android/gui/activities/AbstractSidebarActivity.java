package org.csploit.android.gui.activities;

import android.content.Intent;
import android.content.res.Configuration;
import android.net.Uri;
import android.os.Bundle;
import android.support.annotation.DrawableRes;
import android.support.annotation.NonNull;
import android.support.design.widget.NavigationView;
import android.support.v4.app.Fragment;
import android.support.v4.view.GravityCompat;
import android.support.v4.widget.DrawerLayout;
import android.support.v7.app.ActionBarDrawerToggle;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
import android.view.Menu;
import android.view.MenuItem;
import android.view.SubMenu;
import android.widget.FrameLayout;
import android.widget.Toast;

import org.csploit.android.R;
import org.csploit.android.SettingsFragment;
import org.csploit.android.core.Logger;
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
import org.csploit.android.services.receivers.ConnectivityReceiver;

import java.util.List;

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
  private ConnectivityReceiver connectivityReceiver;

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);

    connectivityReceiver = new ConnectivityReceiver(this);
    connectivityReceiver.register(this);
  }

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
    String iface;

    if(item.getGroupId() == R.id.nav_network_group) {
      iface = item.getTitle().toString();
      Logger.info("selected interface " + iface);
      changeIface(iface);
    } else {
      switch (item.getItemId()) {
        case R.id.nav_network_single:
          connectivityReceiver.tryLoadDefaultInterface();
          FragmentHelper.switchToFragment(this, TargetList.class);
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
    }

    if(fragment != null) {
      FragmentHelper.switchToFragment(this, fragment);
      setTitle(item.getTitle());
    }

    drawerLayout.closeDrawers();

    return fragment != null;
  }

  @Override
  protected void onDestroy() {
    connectivityReceiver.unregister();
    super.onDestroy();
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
    final String current = System.getIfname();

    if(ifname.equals(current)) {
      if (canSwitchFragment()) {
        FragmentHelper.switchToFragment(AbstractSidebarActivity.this, TargetList.class);
      }
      return;
    }

    final ProgressDialog dialog = new ProgressDialog("changing interface", "please wait", this);
    dialog.setMinElapsedTime(300);
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

        if (errMsg != null) {
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
      backToCloseTimeout = java.lang.System.currentTimeMillis() + (BACK_TO_CLOSE_TIMEOUT * 1000);
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
    connectivityReceiver.updateSidebar();
  }

  @Override
  protected void onSaveInstanceState(Bundle outState) {
    isShown = false;
    super.onSaveInstanceState(outState);
  }

  public void updateNetworks(List<? extends NavNetworkItem> items) {
    Menu mainMenu = nvDrawer.getMenu();
    MenuItem single = mainMenu.findItem(R.id.nav_network_single);
    MenuItem listWrapper = mainMenu.findItem(R.id.nav_network_list_wrapper);
    SubMenu networkMenu = listWrapper.getSubMenu();

    networkMenu.clear();

    boolean isSingle = items.isEmpty() || items.size() == 1;

    if(isSingle) {
      if(items.isEmpty()) {
        single.setTitle("Network unavailable");
        single.setIcon(null);
      } else {
        single.setTitle("Network");
        single.setIcon(items.get(0).getDrawableId());
      }
    } else {
      int i = 0;
      for (NavNetworkItem item : items) {
        MenuItem item1 = networkMenu.add(R.id.nav_network_group, Menu.NONE, i++, item.getName());
        item1.setIcon(item.getDrawableId());
      }
    }

    single.setVisible(isSingle);
    listWrapper.setVisible(!isSingle);

    StringBuilder sb = new StringBuilder();

    for(NavNetworkItem item : items) {
      if(sb.length() > 0)
        sb.append(", ");

      sb.append(item.getName());
    }

    Logger.debug("updateNetworks([" + sb.toString() + "]): isSingle=" + isSingle);
  }

  public interface NavNetworkItem {
    String getName();
    @DrawableRes int getDrawableId();
  }
}
