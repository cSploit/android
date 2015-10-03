package org.csploit.msf;

import static org.junit.Assert.*;
import static org.hamcrest.CoreMatchers.*;

import org.csploit.msf.module.ArchSet;
import org.csploit.msf.module.PlatformList;
import org.csploit.msf.module.Reference;
import org.csploit.msf.module.SiteReference;
import org.csploit.msf.module.Target;
import org.csploit.msf.options.AddressOption;
import org.csploit.msf.options.BooleanOption;
import org.csploit.msf.options.EnumOption;
import org.junit.Test;

import java.util.HashMap;
import java.util.Map;

/**
 * Test msf library usage for rpc usage
 */
public class RpcSampleTest {

  private class ModuleInfoBase {
    public String name,description,license,filepath;
    public int rank;
    public String[][] references;
    public String[] authors;
  }

  private class ExploitInfo extends ModuleInfoBase {
    public String[] targets;
    public int defaultTarget;
  }

  private class AuxInfo extends ModuleInfoBase {
    public String[] actions;
    public int defaultAction;
  }

  private class ModuleOptionInfo<T> {
    public String type, description;
    public boolean required, advanced, evasion;
    public T defaultValue;
    public String[] enums;

    public ModuleOptionInfo(String type, String description, boolean required, boolean advanced, boolean evasion, T defaultValue, String[] enums) {
      this.type = type;
      this.description = description;
      this.required = required;
      this.advanced = advanced;
      this.evasion = evasion;
      this.defaultValue = defaultValue;
      this.enums = enums;
    }

    public ModuleOptionInfo(String type, String description, boolean required, boolean advanced, boolean evasion, T defaultValue) {
      this(type, description, required, advanced, evasion, defaultValue, null);
    }

    public ModuleOptionInfo(String type, String description, boolean required, boolean advanced, boolean evasion) {
      this(type, description, required, advanced, evasion, null, null);
    }
  }

  @Test
  public void putExploitTest() {

    ExploitInfo info = new ExploitInfo();

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
    info.targets = new String[] {
            "IE / RealOne Player 2",
            "IE / RealOne Player 3"
    };
    info.defaultTarget = 0;


    Framework framework = new Framework();

    ModuleManager mgr = framework.getModuleManager();

    // deserialize rpc stuff

    Target[] targets = new Target[info.targets.length];
    Author[] authors = new Author[info.authors.length];
    Reference[] references = new Reference[info.references.length];
    License license = License.fromString(info.license);

    for(int i = 0; i < info.targets.length; i++) {
      targets[i] = new Target(info.targets[i]);
    }

    for(int i = 0; i < info.authors.length; i++) {
      authors[i] = Author.fromString(info.authors[i]);
    }

    for(int i = 0; i < info.references.length; i++) {
      String[] pair = info.references[i];
      String id = pair[0];
      String val = pair[1];
      references[i] = new SiteReference(id, val);
    }

    Exploit exploit = new Exploit(
            info.name, info.description, "0", authors,
            new ArchSet(), new PlatformList(), references,
            false, license, targets, 0);

    mgr.put(exploit);

    assertThat(exploit.getFramework(), is(framework));
    assertThat(exploit.license, is(License.GPL));
    assertThat(exploit.getTarget(), is(exploit.getTargets()[0]));

    // receive exploit options
    Map<String, ModuleOptionInfo> options = new HashMap<>();

    options.put("NTLM::SendNTLM",
            new ModuleOptionInfo<>(
                    "bool", "A", true, true, false, true
            ));
    options.put("HTTP::uri_encode_mode",
            new ModuleOptionInfo<>(
                    "enum", "B", false, false, true, "hex-normal", new String[] {
                    "none", "hex-normal", "hex-noslashes", "hex-random",
                    "hex-all", "u-normal", "u-all", "u-random"
            }
            ));
    options.put("RHOST",
            new ModuleOptionInfo<>(
                    "address", "C", true, false, false
            ));

    // put them into the exploit

    for (Map.Entry<String, ModuleOptionInfo> entry : options.entrySet()) {
      ModuleOptionInfo info1 = entry.getValue();
      String name = entry.getKey();

      exploit.registerOption(name, getOptionForInfo(name, info1), info1.advanced, info1.evasion);
    }

    // let's now try to edit an option

    // first in the local datastore
    exploit.getDataStore().put("TARGET", "1");
    assertThat(exploit.getTarget(), is(exploit.getTargets()[1]));
    exploit.getDataStore().remove("TARGET");
    assertThat(exploit.getTarget(), is(exploit.getTargets()[0]));

    // from global datastore
    framework.getDataStore().put("TARGET", "1");
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

  private static Option getOptionForInfo(String name, ModuleOptionInfo info) {
    switch (info.type) {
      case "bool":
        return new BooleanOption(name, info.required, info.description,
                ((ModuleOptionInfo<Boolean>)info).defaultValue);
      case "enum":
        return new EnumOption(name, info.required, info.description,
                ((ModuleOptionInfo<String>)info).defaultValue, info.enums);
      case "address":
        return new AddressOption(name, info.required, info.description,
                ((ModuleOptionInfo<String>)info).defaultValue);
    }
    return null;
  }
}
