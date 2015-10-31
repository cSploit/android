package org.csploit.msf.impl.module;

import org.csploit.msf.api.module.Platform;

import java.util.Collection;
import java.util.HashSet;

/**
 * A list of platforms
 */
public class PlatformList extends HashSet<Platform> {

  public PlatformList() {
    super();
  }

  public PlatformList(String s) {
    this(s.split(","));
  }

  public PlatformList(String[] plats) {
    super(plats.length);
    for(String s : plats) {
      Platform platform = Platform.fromString(s);
      if(platform != null) {
        add(platform);
      }
    }
  }

  public PlatformList intersect(Collection<Platform> other) {
    PlatformList res = new PlatformList();

    for(Platform pl1 : this) {
      for(Platform pl2 : other) {
        if(pl1.subsetOf(pl2)) {
          res.add(pl1);
        } else if(pl2.subsetOf(pl1)) {
          res.add(pl2);
        }
      }
    }

    return res;
  }
}
