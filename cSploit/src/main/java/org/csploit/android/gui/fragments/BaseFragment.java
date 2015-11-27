package org.csploit.android.gui.fragments;

import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.view.View;

import org.csploit.android.helpers.GUIHelper;

/**
 * A Fragment that give us some handy stuff
 */
abstract class BaseFragment extends Fragment {

  @Override
  public void onViewCreated(View view, Bundle savedInstanceState) {
    super.onViewCreated(view, savedInstanceState);

    GUIHelper.setupTheme(this, view);
  }
}
