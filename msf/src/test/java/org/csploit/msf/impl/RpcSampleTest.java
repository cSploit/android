package org.csploit.msf.impl;

import static org.junit.Assert.*;
import static org.hamcrest.CoreMatchers.*;

import org.csploit.msf.api.License;
import org.csploit.msf.impl.module.Reference;
import org.csploit.msf.impl.module.SiteReference;
import org.csploit.msf.impl.module.Target;
import org.csploit.msf.impl.options.AddressOption;
import org.csploit.msf.impl.options.BooleanOption;
import org.csploit.msf.impl.options.EnumOption;
import org.junit.Test;

import java.util.HashMap;
import java.util.Map;

/**
 * Test msf library usage for rpc usage
 */
public class RpcSampleTest {

  private final static class TestOptionInfoFactory {

    public static MsgpackClient.OptionInfo create(String type, String description, boolean required, boolean advanced, boolean evasion, Object defaultValue, String[] enums) {
      MsgpackClient.OptionInfo result = new MsgpackClient.OptionInfo();
      result.type = type;
      result.description = description;
      result.required = required;
      result.advanced = advanced;
      result.evasion = evasion;
      result.defaultValue = defaultValue;
      result.enums = enums;

      return result;
    }

    public static MsgpackClient.OptionInfo create(String type, String description, boolean required, boolean advanced, boolean evasion, Object defaultValue) {
      return create(type, description, required, advanced, evasion, defaultValue, null);
    }

    public static MsgpackClient.OptionInfo create(String type, String description, boolean required, boolean advanced, boolean evasion) {
      return create(type, description, required, advanced, evasion, null, null);
    }
  }

  @Test
  public void putExploitTest() {

    MsgpackClient.ModuleInfo info = new MsgpackClient.ModuleInfo();

    info.name = "A funny exploit";
    info.description = "a very long description";
    info.license = License.GPL.toString();
    info.filepath = "whocares";
    info.rank = 300;
    info.references = new String[][] {
            {"URL", "http://nowhere.com/index.html"},
            {"OSVDB", "12345"}
    };
    info.authors = new String[] {
            "Someone <someone@example.com>"
    };
    info.targets = new HashMap<Integer, String>() {{
            put(0, "IE / RealOne Player 2");
            put(1, "IE / RealOne Player 3");
    }};
    info.defaultTarget = 0;

    Framework framework = new Framework();

    ModuleManager mgr = framework.getModuleManager();

    // deserialize rpc stuff

    Exploit exploit = new Exploit("test");

    MsgpackLoader.fillModule(exploit, info);

    mgr.put(exploit);

    assertThat(framework, is(exploit.getFramework()));
    assertThat(exploit.license, is(License.GPL));
    assertThat(exploit.getTarget(), is(exploit.getTargets()[0]));

    // receive exploit options
    Map<String, MsgpackClient.OptionInfo> options = new HashMap<>();

    options.put("NTLM::SendNTLM",
            TestOptionInfoFactory.create(
                    "bool", "A", true, true, false, true
            ));
    options.put("HTTP::uri_encode_mode",
            TestOptionInfoFactory.create(
                    "enum", "B", false, false, true, "hex-normal", new String[] {
                    "none", "hex-normal", "hex-noslashes", "hex-random",
                    "hex-all", "u-normal", "u-all", "u-random"
            }
            ));
    options.put("RHOST",
            TestOptionInfoFactory.create(
                    "address", "C", true, false, false
            ));

    // put them into the exploit
    MsgpackLoader.fillModuleOptions(exploit, options);

    // let's now try to edit an option

    // first in the local datastore
    exploit.getDataStore().put("TARGET", "1");
    assertThat(exploit.getTarget(), is(exploit.getTargets()[1]));
    exploit.getDataStore().remove("TARGET");
    assertThat(exploit.getTarget(), is(exploit.getTargets()[0]));

    // from global datastore
    framework.setGlobalOption("TARGET", "1");
    assertThat(exploit.getTarget(), is(exploit.getTargets()[1]));
    framework.getDataStore().remove("TARGET");
    assertThat(exploit.getTarget(), is(exploit.getTargets()[0]));

    // exploit settings should not be valid without the required RHOST options

    assertThat(exploit.getInvalidOptions().isEmpty(), is(false));

    framework.getDataStore().put("RHOST", "invalid address");

    assertThat(exploit.getInvalidOptions().isEmpty(), is(false));

    framework.getDataStore().put("RHOST", "127.0.0.1");

    assertThat(exploit.getInvalidOptions().isEmpty(), is(true));
  }
}
