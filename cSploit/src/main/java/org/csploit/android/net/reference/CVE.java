package org.csploit.android.net.reference;

/**
 * CVE vulnerability reference
 */
public class CVE implements Vulnerability {
    private static final String URLBASE = "http://www.cvedetails.com/cve/";

    private final String id;
    private String summary;
    private String description;
    private short severity;

    public CVE(String id, String summary, String description) {
        this.id = id;
        this.summary = summary;
        this.description = description;
    }

    public CVE(String id, String summary) {
        this(id, summary, null);
    }

    public CVE(String id) {
        this(id, null, null);
    }

    public static boolean owns(String url) {
        return url.startsWith(URLBASE);
    }

    public String getId() {
        return id;
    }

    @Override
    public String getName() {
        return "CVE-" + id;
    }

    @Override
    public String getSummary() {
        return summary;
    }

    public void setSummary(String summary) {
        this.summary = summary;
    }

    @Override
    public String getDescription() {
        return description;
    }

    public void setDescription(String description) {
        this.description = description;
    }

    @Override
    public String getUrl() {
        return URLBASE + id;
    }

    @Override
    public short getSeverity() {
        return severity;
    }

    public void setSeverity(short severity) {
        this.severity = severity;
    }

    @Override
    public String toString() {
        return "CVE-" + id + " reference";
    }

    @Override
    public boolean equals(Object o) {
        if (o == null) {
            return false;
        }

        return o.getClass() == this.getClass() && id.equals(((CVE) o).id);
    }
}
