package org.csploit.android.gui.adapters;

import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentManager;
import android.support.v4.app.FragmentStatePagerAdapter;

import org.csploit.android.gui.fragments.PluginList;
import org.csploit.android.net.Target;

/**
 * Adapt an {@link Target} for our page view
 */
public class TargetPagerAdapter extends FragmentStatePagerAdapter {

  private Target target;
  private boolean havePorts;

  public TargetPagerAdapter(FragmentManager fm, Target target) {
    super(fm);
    this.target = target;
    havePorts = target.hasOpenPorts();
  }

  @Override
  public Fragment getItem(int position) {

    switch (position) {
      case 0:
        return PluginList.newInstance(target);
      case 1:
        return null; // TODO: PortList
    }

    return null;
  }

  @Override
  public int getCount() {
    return havePorts ? 2 : 1;
  }
}
