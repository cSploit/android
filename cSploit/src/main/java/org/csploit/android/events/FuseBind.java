package org.csploit.android.events;

/**
 * a new fuse mountpoint has been found
 */
public class FuseBind implements Event {
  public final String source, mountpoint;

  public FuseBind(String source, String mountpoint) {
    this.source = source;
    this.mountpoint = mountpoint;
  }

  @Override
  public String toString() {
    return String.format("FuseBind: { source='%s', mountpoint='%s' }", this.source, this.mountpoint);
  }
}
