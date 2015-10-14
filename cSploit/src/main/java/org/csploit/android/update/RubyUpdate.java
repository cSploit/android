package org.csploit.android.update;

import android.content.Context;

import org.csploit.android.R;
import org.csploit.android.core.*;
import org.csploit.android.core.System;

/**
 * A ruby update
 */
public class RubyUpdate extends Update {
  public RubyUpdate(Context context, String url, String version) {
    this.url = url;
    this.version = version;
    name = "ruby.tar.xz";
    path = String.format("%s/%s", System.getStoragePath(), name);
    archiver = Update.archiveAlgorithm.tar;
    compression = Update.compressionAlgorithm.xz;

    outputDir = System.getRubyPath();
    executableOutputDir = ExecChecker.ruby().getRoot();
    fixShebang = wipeOnFail = true;
    prompt = context.getString(R.string.new_ruby_found);
  }
}
