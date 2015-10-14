package org.csploit.android.net.reference;

/**
 * OSVDB vulnerability reference
 */
public class OSVDB implements Vulnerability {
  private static final String URLBASE = "http://osvdb.org/show/osvdb/";

  private final int id;
  private String summary;
  private String description;
  private short severity;

  public OSVDB(int id, String description) {
    this.id = id;
    this.description = description;
  }

  public OSVDB(int id) {
    this(id, null);
  }

  @Override
  public String getName() {
    return "OSVDB-" + id;
  }

  @Override
  public String getSummary() {
    return description;
  }

  @Override
  public String getDescription() {
    return null;
  }

  @Override
  public String getUrl() {
    return URLBASE + id;
  }

  @Override
  public short getSeverity() {
    return 0;
  }

  @Override
  public String toString() {
    return "OSVDB-" + id + " reference";
  }

  @Override
  public boolean equals(Object o) {
    return o.getClass() == OSVDB.class && id == ((OSVDB) o).id;
  }

  public static boolean owns(String url) {
    return url.startsWith(URLBASE);
  }
}
