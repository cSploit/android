package org.csploit.android.net.datasource;

import org.csploit.android.core.Logger;
import org.csploit.android.net.RemoteReader;
import org.csploit.android.net.Target;
import org.csploit.android.net.reference.CVE;

import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * get CVE info from cvedetails.com
 */
class CVEDetails {

  public static class Receiver implements RemoteReader.Receiver {
    private static final Pattern TITLE = Pattern.compile("<title>([^<]+)");
    private static final Pattern DESCRIPTION = Pattern.compile("<meta *name=\"description\" *content=\"[^:]+: ([^\"]+)");
    private static final Pattern CVSSCORE = Pattern.compile("<meta *name=\"keywords\" *content=\"[^\"]+CVSS *([0-9]+\\.[0-9])");
    private static final String VERSION_PATTERN = "/version/[0-9]+/(%1$s-[^ ]+|[^ ]+-%1$s)-([^ ]+)\\.html";

    private final Target.Exploit exploit;
    private final CVE reference;
    private final Search.Receiver<Target.Exploit> receiver;

    public Receiver(Target.Exploit exploit, CVE reference, Search.Receiver<Target.Exploit> receiver) {
      this.exploit = exploit;
      this.reference = reference;
      this.receiver = receiver;
    }

    private void parseReference(String html) {
      Matcher matcher = TITLE.matcher(html);

      if(matcher.find()) {
        reference.setSummary(new String(matcher.group(1).toCharArray()));
      }

      matcher = DESCRIPTION.matcher(html);

      if(matcher.find()) {
        reference.setDescription(new String(matcher.group(1).toCharArray()));
      }

      matcher = CVSSCORE.matcher(html);

      if(matcher.find()) {
        reference.setSeverity(Short.parseShort(matcher.group(1).replace(".", "")));
      }
    }

    private void checkCompatibility(String html) {
      Matcher matcher;
      Target target;

      if(exploit.isEnabled())
        return;

      target = exploit.getParent();

      if(target == null || !target.hasOpenPorts()) {
        return;
      }

      for( Target.Port p : target.getOpenPorts()) {
        String version = p.getVersion();

        if(version == null)
          continue;

        matcher = Pattern.compile(
                String.format(VERSION_PATTERN, p.getService()),
                Pattern.CASE_INSENSITIVE).matcher(html);

        while(matcher.find()) {
          String foundVersion = matcher.group(2);

          if(foundVersion.length() <= version.length()) {
            if(version.startsWith(foundVersion)) {
              exploit.setEnabled(true);
              receiver.onFoundItemChanged(exploit);
              return;
            }
          } else if (foundVersion.startsWith(version)) {
            exploit.setEnabled(true);
            receiver.onFoundItemChanged(exploit);
            return;
          }
        }
      }
    }

    @Override
    public void onContentFetched(byte[] content) {
      String html = new String(content);

      parseReference(html);
      checkCompatibility(html);
    }

    @Override
    public void onError(byte[] description) {
      Logger.error(String.format("%s: %s", reference.getUrl(), new String(description)));
    }
  }
}
