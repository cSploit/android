package org.csploit.android.core;

import android.content.Intent;

import com.github.zafarkhaja.semver.Version;
import com.sksamuel.diffpatch.DiffMatchPatch;

import java.util.HashMap;
import java.util.LinkedList;

/**
 * just a simple struct to handle archives metadata
 */
public class ArchiveMetadata {
  public String
          url,
          name,
          path,
          versionString,
          outputDir,
          executableOutputDir,
          md5,
          sha1;
  public Version
          version;
  public compressionAlgorithm
          compression;
  public archiveAlgorithm
          archiver;
  public Intent
          contentIntent;
  public boolean
          skipRoot,
          fixShebang,
          errorOccurred;

  public ArchiveMetadata() {
    reset();
  }

  public void reset() {
    synchronized (this) {
      url = name = md5 = sha1 = versionString = path =
              outputDir = executableOutputDir = null;
      version = null;
      compression = null;
      archiver = null;
      contentIntent = null;
      fixShebang = errorOccurred = skipRoot = false;
    }
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