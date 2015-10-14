package org.csploit.android.update;

import android.content.Intent;

import java.io.Serializable;

/**
 * just a simple struct to handle archives metadata
 */
public abstract class Update implements Serializable {
  public String
          url,
          name,
          path,
          version,
          outputDir,
          executableOutputDir,
          md5,
          sha1;
  public compressionAlgorithm
          compression;
  public archiveAlgorithm
          archiver;
  public boolean
          skipRoot,
          fixShebang,
          errorOccurred,
          wipeOnFail;
  public String
          prompt;

  public Update() {
    reset();
  }

  public void reset() {
    synchronized (this) {
      url = name = md5 = sha1 = version = path =
              outputDir = executableOutputDir = null;
      version = null;
      compression = null;
      archiver = null;
      wipeOnFail = fixShebang = errorOccurred = skipRoot = false;
    }
  }

  public boolean haveIntent() {
    return false;
  }

  public Intent buildIntent() {
    return null;
  }

  public enum compressionAlgorithm {
    none,
    gzip,
    bzip,
    xz
  }

  public enum archiveAlgorithm {
    none,
    tar,
    zip
  }
}