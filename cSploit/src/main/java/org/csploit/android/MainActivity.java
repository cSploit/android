
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
import android.os.Bundle;
import android.support.design.widget.NavigationView;
import android.widget.Toast;

import org.csploit.android.core.Plugin;
import org.csploit.android.core.System;
import org.csploit.android.gui.activities.AbstractSidebarActivity;
import org.csploit.android.gui.fragments.Heartless;
import org.csploit.android.gui.fragments.Init;
import org.csploit.android.gui.fragments.TargetDetail;
import org.csploit.android.gui.fragments.TargetList;
import org.csploit.android.gui.fragments.plugins.mitm.Hijacker;
import org.csploit.android.helpers.FragmentHelper;
import org.csploit.android.net.Target;
import org.csploit.android.plugins.mitm.hijacker.HijackerWebView;
import org.csploit.android.plugins.mitm.hijacker.Session;
import org.csploit.android.services.receivers.ConnectivityReceiver;
import org.csploit.android.services.receivers.UpdateReceiver;

public class MainActivity extends AbstractSidebarActivity
        implements NavigationView.OnNavigationItemSelectedListener,
        TargetList.OnFragmentInteraction,
        TargetDetail.OnFragmentInteractionListener,
        Init.OnFragmentInteractionListener,
        UpdateReceiver.CoreUpdateListener,
        Hijacker.OnFragmentInteractionListener {

  private UpdateReceiver updateReceiver;

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);

    setContentView(R.layout.main);

    if (savedInstanceState != null) {
      return;
    }

    updateReceiver = new UpdateReceiver(this);
    updateReceiver.register(this);
  }

  @Override
  protected void onPostResume() {
    super.onPostResume();
    init();
  }

  @Override
  protected void onDestroy() {
    updateReceiver.unregister();

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
    if(canSwitchFragment()) {
      init();
    }
  }

  @Override
  public void onInitDone() {
    if(!canSwitchFragment()) {
      return;
    }

    FragmentHelper.switchToFragment(this, TargetList.class);
  }

  @Override
  public void onPluginChosen(Plugin plugin) {
    if (System.checkNetworking(this)) {
      System.setCurrentPlugin(plugin);

      if (plugin.hasLayoutToShow()) {
        Toast.makeText(this, getString(R.string.selected) + getString(plugin.getName()), Toast.LENGTH_SHORT).show();
        FragmentHelper.openFragment(this, plugin);
      } else
        plugin.onActionClick(this);
    }
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
  public void onHijackerSessionSelected(Session session) {
    System.setCustomData(session);
    //TODO: fragment it
    startActivity(new Intent(this, HijackerWebView.class));
  }
}