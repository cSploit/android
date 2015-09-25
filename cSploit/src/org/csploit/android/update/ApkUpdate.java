package org.csploit.android.update;

import android.content.Context;
import android.content.Intent;
import android.net.Uri;

import org.csploit.android.R;
import org.csploit.android.core.System;

import java.io.File;

/**
 * an APK update
 */
public class ApkUpdate extends Update {

  public ApkUpdate(Context context, String url, String version) {
    this.url = url;
    this.version = version;
    name = String.format("cSploit-%s.apk", version);
    path = String.format("%s/%s", System.getStoragePath(), name);
    contentIntent = new Intent(Intent.ACTION_VIEW);
    contentIntent.setDataAndType(Uri.fromFile(new File(path)), "application/vnd.android.package-archive");
    contentIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
    prompt = String.format(context.getString(R.string.new_apk_found), version);
  }
}
