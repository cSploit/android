package org.csploit.msf.impl.module;

import java.util.HashMap;
import java.util.Map;
import java.util.regex.Pattern;

/**
 * A reference to a website.
 */
public class SiteReference extends Reference implements org.csploit.msf.api.module.SiteReference {
  protected String ctx_id;
  protected String ctx_val;

  private final static Pattern SIMPLE_URL_PATTERN = Pattern.compile("(http://|https://|ftp://)");

  private final static Map<String, String> formats = new HashMap<String, String>() {{
    put("OSVDB", "http://www.osvdb.org/%s");
    put("CVE", "http://cvedetails.com/cve/%s/");
    put("CWE", "https://cwe.mitre.org/data/definitions/%s.html");
    put("BID", "http://www.securityfocus.com/bid/%s");
    put("MSB", "http://technet.microsoft.com/en-us/security/bulletin/%s");
    put("EDB", "https://www.exploit-db.com/exploits/%s");
    put("US-CERT-VU", "http://www.kb.cert.org/vuls/id/%s");
    put("ZDI", "http://www.zerodayinitiative.com/advisories/ZDI-%s");
    put("WPVDB", "https://wpvulndb.com/vulnerabilities/%s");
    put("PACKETSTORM", "https://packetstormsecurity.com/files/%s");
    put("URL", "%s");
  }};

  public SiteReference(String ctx_id, String ctx_val) {
    super("");
    this.ctx_id = ctx_id;
    this.ctx_val = ctx_val;

    if(formats.containsKey(ctx_id)) {
      str = String.format(formats.get(ctx_id), ctx_val);
    } else {
      str = ctx_id;
      if(ctx_val != null) {
        str += " (" + ctx_val + ")";
      }
    }
  }

  public static SiteReference fromString(String str) {
    if(!SIMPLE_URL_PATTERN.matcher(str).matches()) {
      return null;
    }
    return new SiteReference("URL", str);
  }

  @Override
  public String getUrl() {
    return toString();
  }
}
