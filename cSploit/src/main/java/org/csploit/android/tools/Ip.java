package org.csploit.android.tools;

import org.csploit.android.core.Child;
import org.csploit.android.core.ChildManager;

import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class Ip extends Raw {
  private static final Pattern defaultRoutePattern = Pattern.compile("^default\\s+via\\s+([^ \t]+)\\s", Pattern.CASE_INSENSITIVE);

  public abstract static class GatewayReceiver extends RawReceiver {
    @Override
    public void onNewLine(String line) {
      Matcher matcher = defaultRoutePattern.matcher(line);
      if(matcher.find()) {
        onGatewayFound(matcher.group(1));
      }
    }

    public abstract void onGatewayFound(String gateway);
  }

  public Child getGatewayForIface(String iface, GatewayReceiver receiver) throws ChildManager.ChildNotStartedException {
    return super.async("ip route show table " + iface, receiver);
  }
}
