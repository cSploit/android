package org.csploit.android.gui.fragments;

import android.os.Bundle;
import android.support.v4.app.ListFragment;
import android.view.View;

import org.csploit.android.helpers.GUIHelper;

/**
 * A class that provide some handy stuff
 */
abstract class BaseListFragment extends ListFragment {
  @Override
  public void onViewCreated(View view, Bundle savedInstanceState) {
    super.onViewCreated(view, savedInstanceState);

    GUIHelper.setupTheme(this, view);
  }
}
