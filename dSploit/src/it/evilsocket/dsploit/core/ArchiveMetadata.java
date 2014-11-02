package it.evilsocket.dsploit.core;

import android.content.Intent;

import com.github.zafarkhaja.semver.Version;

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
          dirToExtract,
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

  public ArchiveMetadata() {
    reset();
  }

  public void reset() {
    synchronized (this) {
      url = name = md5 = sha1 = versionString = path =
              outputDir = dirToExtract = executableOutputDir = null;
      version = null;
      compression = null;
      archiver = null;
      contentIntent = null;
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