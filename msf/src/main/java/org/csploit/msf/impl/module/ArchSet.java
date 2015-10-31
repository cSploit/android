package org.csploit.msf.impl.module;

import java.util.Collection;
import java.util.HashSet;
import org.csploit.msf.api.Arch;

/**
 * A container for architectures supported by a module
 */
public class ArchSet extends HashSet<Arch> {

  public ArchSet() {
    super();
  }

  public ArchSet(String s) {
    this(s.split(","));
  }

  public ArchSet(String[] a) {
    super(a.length);

    for(String arch : a) {
      add(Arch.valueOf(arch.trim().toUpperCase()));
    }
  }

  private ArchSet(Collection<? extends Arch> other) {
    super(other);
  }

  public boolean isSupported(Arch arch) {
    return arch == Arch.ANY || contains(arch);
  }

  public ArchSet intersect(Collection<Arch> other) {
    ArchSet res = new ArchSet(this);
    res.retainAll(other);
    return res;
  }

  @Override
  public String toString() {
    if(isEmpty())
      return "";
    StringBuilder sb = new StringBuilder();

    for (Arch a : this) {
      if(sb.length() > 0) {
        sb.append(", ");
      }
      sb.append(a.toString());
    }

    return sb.toString();
  }
}
