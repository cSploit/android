package org.csploit.android.update;

import android.content.Context;

import org.csploit.android.R;
import org.csploit.android.core.*;
import org.csploit.android.core.System;

/**
 * A MSF update
 */
public class MsfUpdate extends Update {
  public MsfUpdate(Context context, String url, String version) {
    this.url = url;
    this.version =  version;

    name = "msf.tar.xz";
    path = String.format("%s/%s", System.getStoragePath(), name);
    outputDir = System.getMsfPath();
    executableOutputDir = ExecChecker.msf().getRoot();
    archiver = Update.archiveAlgorithm.tar;
    compression = Update.compressionAlgorithm.xz;
    fixShebang = wipeOnFail = true;
    prompt = context.getString(R.string.new_msf_found);
  }
}
