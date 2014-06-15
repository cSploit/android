package it.evilsocket.dsploit.core;

import android.content.Intent;

import java.util.HashMap;

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
          dirToExtract,
          md5,
          sha1;
  public Double
          version;
  public compressionAlgorithm
          compression;
  public archiveAlgorithm
          archiver;
  public Intent
          contentIntent;
  public HashMap<Integer, String>
          modeMap;

  public ArchiveMetadata() {
    reset();
  }

  synchronized public void reset() {
    url = name = md5 = sha1 = versionString = path = outputDir = dirToExtract = null;
    version = null;
    compression = null;
    archiver = null;
    contentIntent = null;
    modeMap = null;
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