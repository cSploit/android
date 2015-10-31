package org.csploit.msf.impl;

import org.csploit.msf.api.License;
import org.csploit.msf.api.Exploit;
import org.csploit.msf.api.module.Rank;
import org.csploit.msf.api.module.Reference;
import org.csploit.msf.api.module.SiteReference;
import org.csploit.msf.api.module.Target;
import org.junit.Test;

import java.util.Date;
import java.util.HashMap;
import java.util.Map;

import static org.junit.Assert.*;
import static org.hamcrest.CoreMatchers.*;

/**
 * Test msgpack loader
 */
public class MsgpackLoaderTest {
  private static MsgpackClient.ModuleInfo sampleModuleInfo = new MsgpackClient.ModuleInfo() {{
    name = "name";
    description = "description";
    license = License.MSF.toString();
    filepath = "whocares";
    rank = Rank.NORMAL.getValue();
    references = new String[][]{
            new String[]{"CVE", "12345"},
            new String[]{"OSVDB", "12345"}
    };
    authors = new String[] {
            "me <me@nowhere.org>",
            "you <you@somewhere.org>"
    };
    targets = new HashMap<Integer, String>() {{
      put(0, "target 0");
      put(1, "target 1");
    }};
    defaultTarget = 0;
  }};
  
  private static MsgpackClient.SessionInfo sampleSessionInfo = new MsgpackClient.SessionInfo() {{
    type = "shell";
    tunnelLocal = "hello";
    tunnelPeer = "world";
    viaExploit = "test";
    viaPayload = "test";
    description = "description";
    info = "info";
    workspace = "workspace";
    sessionHost = "session_host";
    sessionPort = 31337;
    targetHost = "target_host";
    username = "username";
    uuid = "uuid";
    exploitUuid = "whocares";
    routes = new String[] {
            "whocares", "about", "routes"
    };
  }};

  private static MsgpackClient.SessionInfo sampleMeterpreterSessionInfo = new MsgpackClient.SessionInfo() {{
    type = "meterpreter";
    tunnelLocal = sampleSessionInfo.tunnelLocal;
    tunnelPeer = sampleSessionInfo.tunnelPeer;
    viaExploit = sampleSessionInfo.viaExploit;
    viaPayload = sampleSessionInfo.viaPayload;
    description = sampleSessionInfo.description;
    info = sampleSessionInfo.info;
    workspace = sampleSessionInfo.workspace;
    sessionHost = sampleSessionInfo.sessionHost;
    sessionPort = sampleSessionInfo.sessionPort;
    targetHost = sampleSessionInfo.targetHost;
    username = sampleSessionInfo.username;
    uuid = sampleSessionInfo.uuid;
    exploitUuid = sampleSessionInfo.exploitUuid;
    routes = sampleSessionInfo.routes;
    platform = "windows";
  }};

  private static Map<String, MsgpackClient.OptionInfo> sampleOptionsList = new HashMap<String, MsgpackClient.OptionInfo>() {{
    put("StringOption", new MsgpackClient.OptionInfo() {{
      type = "string";
      required = false;
      advanced = false;
      evasion = false;
      description = "description";
      defaultValue = "hello world";
    }});

    put("AddressOption", new MsgpackClient.OptionInfo() {{
      type = "address";
      required = false;
      advanced = false;
      evasion = false;
      description = "description";
      defaultValue = "127.0.0.1";
    }});

    put("AddressRangeOption", new MsgpackClient.OptionInfo() {{
      type = "addressrange";
      required = false;
      advanced = false;
      evasion = false;
      description = "description";
    }});

    put("EnumOption", new MsgpackClient.OptionInfo() {{
      type = "enum";
      required = false;
      advanced = false;
      evasion = false;
      description = "description";
      defaultValue = "B";
      enums = new String[]{"A", "B", "C"};
    }});

    put("BooleanOption", new MsgpackClient.OptionInfo() {{
      type = "bool";
      required = false;
      advanced = false;
      evasion = false;
      description = "description";
      defaultValue = false;
    }});

    put("PortOption", new MsgpackClient.OptionInfo() {{
      type = "port";
      required = false;
      advanced = false;
      evasion = false;
      description = "description";
      defaultValue = 4000;
    }});

    put("PathOption", new MsgpackClient.OptionInfo() {{
      type = "path";
      required = false;
      advanced = false;
      evasion = false;
      description = "description";
      defaultValue = "/etc/passwd";
    }});

    put("IntegerOption", new MsgpackClient.OptionInfo() {{
      type = "integer";
      required = false;
      advanced = false;
      evasion = false;
      description = "description";
      defaultValue = 123;
    }});

    put("RegexpOption", new MsgpackClient.OptionInfo() {{
      type = "regexp";
      required = false;
      advanced = false;
      evasion = false;
      description = "description";
      defaultValue = "[0-9A-Za-z]{16,32}";
    }});

    put("RawOption", new MsgpackClient.OptionInfo() {{
      type = "raw";
      required = false;
      advanced = false;
      evasion = false;
      description = "description";
    }});
  }};

  private static MsgpackClient.JobInfo sampleJobInfo = new MsgpackClient.JobInfo() {{
    id = 1;
    name = "name";
    startTime = 0; // A long time ago, in a galaxy far, far away...
  }};

  private MsgpackLoader loader = new MsgpackLoader(null, new Framework());

  @Test
  public void testFillModule() throws Exception {
    InternalModule module = MsgpackProxy.Factory.newModule("exploit", "test");

    MsgpackLoader.fillModule(module, sampleModuleInfo);

    assertThat(module.getName(), is("name"));
    assertThat(module.getDescription(), is("description"));
    assertThat(module.getLicense(), is(License.MSF));
    assertThat(module.getRank(), is(Rank.NORMAL));
    assertThat(module.getReferences().length, is(2));
    Reference ref = module.getReferences()[0];
    assertThat(ref, instanceOf(SiteReference.class));
    ref = module.getReferences()[1];
    assertThat(ref, instanceOf(SiteReference.class));
    assertThat(module.getAuthors().length, is(2));
    assertThat(module.getAuthors()[0].toString(), is("me <me@nowhere.org>"));
    assertThat(module.getAuthors()[1].toString(), is("you <you@somewhere.org>"));
    Target[] targets = ((Exploit)module).getTargets();
    assertThat(targets.length, is(2));
    assertThat(targets[0].getName(), is("target 0"));
    assertThat(targets[1].getName(), is("target 1"));
    assertThat(((Exploit)module).getTarget(), is(targets[0]));
  }

  @Test
  public void testBuildSession() throws Exception {
    Session shell, meterpreter;

    shell = loader.buildSession(1, sampleSessionInfo);
    meterpreter = loader.buildSession(2, sampleMeterpreterSessionInfo);

    assertThat(shell, instanceOf(MsgpackProxy.ShellProxy.class));
    assertThat(meterpreter, instanceOf(MsgpackProxy.MeterpreterProxy.class));

    assertThat(shell.getId(), is(1));
    assertThat(shell.getDescription(), is("description"));
    assertThat(shell.getInfo(), is("info"));
    assertThat(shell.getLocalTunnel(), is("hello"));
    assertThat(shell.getPeerTunnel(), is("world"));
    assertThat(shell.getTargetHost(), is("target_host"));
    assertThat(shell.getWorkspace(), is("workspace"));
    assertThat(shell.getSessionHost(), is("session_host"));
    assertThat(shell.getSessionPort(), is(31337));
    assertThat(shell.getUuid(), is("uuid"));

    assertThat(meterpreter.getId(), is(2));
    assertThat(meterpreter.getDescription(), is(shell.getDescription()));
    assertThat(meterpreter.getInfo(), is(shell.getInfo()));
    assertThat(meterpreter.getLocalTunnel(), is(shell.getLocalTunnel()));
    assertThat(meterpreter.getPeerTunnel(), is(shell.getPeerTunnel()));
    assertThat(meterpreter.getTargetHost(), is(shell.getTargetHost()));
    assertThat(meterpreter.getWorkspace(), is(shell.getWorkspace()));
    assertThat(meterpreter.getSessionHost(), is(shell.getSessionHost()));
    assertThat(meterpreter.getSessionPort(), is(shell.getSessionPort()));
    assertThat(meterpreter.getUuid(), is(shell.getUuid()));
  }

  @Test
  public void testFillJob() throws Exception {

    Job job = new Job(2, "wrong_name");

    loader.fillJob(job, sampleJobInfo);

    assertThat(job.getId(), is(2));
    assertThat(job.getName(), is("name"));
    assertThat(job.getStartTime(), is(new Date(0)));
  }

  @Test
  public void testFillModuleOptions() throws Exception {
    InternalModule sampleModule = MsgpackProxy.Factory.newModule("exploit", "test");

    assertThat(sampleModule.getOptions().isEmpty(), is(true));

    MsgpackLoader.fillModuleOptions(sampleModule, sampleOptionsList);

    assertThat(sampleModule.getOptions().size(), is(sampleOptionsList.size()));

    for(Option o : sampleModule.getOptions()) {
      assertThat(o.getName(), is(o.getClass().getSimpleName()));
    }
  }
}