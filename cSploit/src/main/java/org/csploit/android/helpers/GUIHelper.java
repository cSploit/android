package org.csploit.android.helpers;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.app.Fragment;
import android.support.v4.content.ContextCompat;
import android.support.v7.app.ActionBar;
import android.support.v7.app.AppCompatActivity;
import android.view.View;

import org.csploit.android.R;
import org.csploit.android.core.System;

/**
 * Help us with the GUI
 */
public final class GUIHelper {
  public static boolean isDarkThemeEnabled() {
    return System.getSettings().getBoolean("PREF_DARK_THEME", false);
  }

  public static void setupTheme(@NonNull Context context) {
    setupTheme(context, null);
  }

  public static void setupTheme(@NonNull Fragment fragment, @Nullable View view) {
    setupTheme(fragment.getActivity(), view);
  }

  public static void setupTheme(@NonNull Context context, @Nullable View view) {
    int backgroundId;
    int styleId;

    if (isDarkThemeEnabled()) {
      styleId = R.style.DarkTheme;
      backgroundId = R.color.background_window_dark;
    } else {
      styleId = R.style.AppTheme;
      backgroundId = R.color.background_window;
    }

    context.setTheme(styleId);

    if(view != null) {
      view.setBackgroundColor(ContextCompat.getColor(context, backgroundId));
    }

    if(context instanceof AppCompatActivity) {
      ActionBar ab = ((AppCompatActivity)context).getSupportActionBar();

      if (ab != null) {
        ab.setDisplayHomeAsUpEnabled(true);
        ab.setHomeButtonEnabled(true);
      }
    }
  }

  /**
   * add a logo to an image
   */
  public static Bitmap addLogo(Bitmap mainImage, Bitmap logoImage) {
    Bitmap finalImage;
    int width, height;

    width = mainImage.getWidth();
    height = mainImage.getHeight();

    finalImage = Bitmap.createBitmap(width, height,
            mainImage.getConfig());

    Canvas canvas = new Canvas(finalImage);

    canvas.drawBitmap(mainImage, 0, 0, null);
    canvas.drawBitmap(logoImage,
            canvas.getWidth() - logoImage.getWidth(),
            canvas.getHeight() - logoImage.getHeight(), null);

    return finalImage;
  }
}
