package org.csploit.android.helpers;

import android.support.annotation.Nullable;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentActivity;

import org.csploit.android.R;
import org.csploit.android.core.*;
import org.csploit.android.core.System;

/**
 * A class that help us dealing with fragments
 */
public final class FragmentHelper {
  public static Fragment getCachedFragment(FragmentActivity activity, Class<? extends Fragment> fClass) {
    return activity.getSupportFragmentManager().findFragmentByTag(fClass.getSimpleName());
  }


  public static void openFragment(FragmentActivity activity, Fragment fragment) {
    activity.getSupportFragmentManager().beginTransaction()
            .replace(R.id.mainframe, fragment, fragment.getClass().getSimpleName())
            .addToBackStack(null)
            .setCustomAnimations(R.anim.fadein, R.anim.fadeout, R.anim.fadein, R.anim.fadeout)
            .commit();
  }

  public static void switchToFragment(FragmentActivity activity, Fragment fragment) {
    activity.getSupportFragmentManager().beginTransaction()
            .replace(R.id.mainframe, fragment, fragment.getClass().getSimpleName())
            .setCustomAnimations(R.anim.fadein, R.anim.fadeout, R.anim.fadein, R.anim.fadeout)
            .commit();
  }

  public static void switchToFragment(Fragment source, Fragment dest) {
    switchToFragment(source.getActivity(), dest);
  }

  private static @Nullable Fragment loadFragment(FragmentActivity activity, Class<? extends Fragment> fClass) {
    Fragment res = getCachedFragment(activity, fClass);

    if(res != null)
      return res;

    try {
      res = fClass.newInstance();
    } catch (InstantiationException e) {
      Logger.warning(e.getMessage());
    } catch (IllegalAccessException e) {
      System.errorLogging(e);
    }

    return res;
  }

  public static void switchToFragment(FragmentActivity  activity, Class<? extends Fragment> dest) {
    Fragment dst = loadFragment(activity, dest);

    if(dst != null) {
      switchToFragment(activity, dst);
    }
  }

  public static void switchToFragment(Fragment source, Class<? extends Fragment> dest) {
    FragmentActivity activity = source.getActivity();
    switchToFragment(activity, dest);
  }

  public static void openFragment(FragmentActivity  activity, Class<? extends Fragment> dest) {
    Fragment dst = loadFragment(activity, dest);

    if(dst != null) {
      openFragment(activity, dst);
    }
  }
}
