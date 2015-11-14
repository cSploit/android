package org.csploit.msf.impl;

import static org.junit.Assert.*;
import static org.hamcrest.CoreMatchers.*;

import org.csploit.msf.api.Console;
import org.csploit.msf.api.ConsoleManager;
import org.csploit.msf.api.Exploit;
import org.csploit.msf.api.Framework;
import org.csploit.msf.api.Module;
import org.csploit.msf.api.Payload;
import org.csploit.msf.api.Post;
import org.csploit.msf.api.Session;
import org.csploit.msf.api.Job;
import org.csploit.msf.api.events.ConsoleEvent;
import org.csploit.msf.api.events.ConsoleOutputEvent;
import org.csploit.msf.api.listeners.ConsoleListener;
import org.junit.Test;

import java.util.ArrayList;
import java.util.List;

/**
 * Test msf library usage for messagepack usage
 */
public class MsgpackTest {

  private static final String host = "127.0.0.1";
  private static final String username = "msf";
  private static final String password = "msf";
  private static final int port = 55553;
  private static final boolean ssl = false;

  private static class DummyConsoleListener implements ConsoleListener {

    public boolean open, changed, output; // default to false

    @Override
    public synchronized void onConsoleOpened(ConsoleEvent event) {
      open = true;
      notifyAll();
    }

    @Override
    public synchronized void onConsoleChanged(ConsoleEvent event) {
      changed = true;
      notifyAll();
    }

    @Override
    public synchronized void onConsoleOutput(ConsoleOutputEvent event) {
      output = true;
      notifyAll();
    }

    @Override
    public synchronized void onConsoleClosed(ConsoleEvent event) {
      open = false;
      notifyAll();
    }
  }

  private static Framework createMsgpackFramework() throws Exception {
    return FrameworkFactory.newMsgpackFramework(host, username, password, port, ssl);
  }

  private static ConsoleManager createMsgpackConsoleManager() throws Exception {
    return ConsoleManagerFactory.createMsgpackConsoleManager(host, username, password, port, ssl);
  }

  @Test
  public void testSessionsList() throws Exception {
    Framework framework = createMsgpackFramework();

    framework.getSessions();
  }

  @Test
  public void testSessionCompatibleModules() throws Exception {
    Framework framework = createMsgpackFramework();

    for(Session s : framework.getSessions()) {
      s.getCompatibleModules();
    }
  }

  @Test
  public void testJobList() throws Exception {
    Framework framework = createMsgpackFramework();

    framework.getJobs();
  }

  @Test
  public void testGetJob() throws Exception {
    Framework framework = createMsgpackFramework();

    for(Job j : framework.getJobs()) {
      framework.getJob(j.getId());
    }
  }

  @Test
  public void testGetExploits() throws Exception {
    Framework framework = createMsgpackFramework();

    List<? extends Exploit> result = framework.getExploits();

    assertThat(result.isEmpty(), is(false));

    Exploit first = result.get(0);

    assertThat(MsgpackLoader.isModuleAlreadyFetched(first), is(false));

    first = (Exploit) framework.getModule(first.getFullName());

    assertThat(MsgpackLoader.isModuleAlreadyFetched(first), is(true));
  }

  @Test
  public void testGetModuleOptions() throws Exception {
    Framework framework = createMsgpackFramework();

    List<Module> allModules = new ArrayList<>();

    allModules.addAll(framework.getExploits());
    allModules.addAll(framework.getPayloads());
    allModules.addAll(framework.getPosts());

    for(Module m : allModules) {
      assertThat(m.getOptions().isEmpty(), is(false));
    }
  }

  @Test
  public void testGetPayloads() throws Exception {
    Framework framework = createMsgpackFramework();

    List<? extends Payload> result = framework.getPayloads();

    assertThat(result.isEmpty(), is(false));

    Payload first = result.get(0);

    assertThat(MsgpackLoader.isModuleAlreadyFetched(first), is(false));

    first = (Payload) framework.getModule(first.getFullName());

    assertThat(MsgpackLoader.isModuleAlreadyFetched(first), is(true));
  }

  @Test
  public void testGetPosts() throws Exception {
    Framework framework = createMsgpackFramework();

    List<? extends Post> result = framework.getPosts();

    assertThat(result.isEmpty(), is(false));

    Post first = result.get(0);

    assertThat(MsgpackLoader.isModuleAlreadyFetched(first), is(false));

    first = (Post) framework.getModule(first.getFullName());

    assertThat(MsgpackLoader.isModuleAlreadyFetched(first), is(true));
  }

  @Test(timeout = 10000)
  public void testConsoles() throws Exception {
    ConsoleManager consoleManager = createMsgpackConsoleManager();
    DummyConsoleListener listener = new DummyConsoleListener();

    consoleManager.start();

    consoleManager.addSubscriber(listener);

    Console console = consoleManager.create();
    console.interact();

    synchronized (listener) {
      while (!listener.open) {
        listener.wait();
      }
    }

    assertThat(consoleManager.getConsoles().contains(console), is(true));

    console.write("help\n");

    synchronized (listener) {
      while(!listener.output) {
        listener.wait();
      }
    }

    console.complete();

    synchronized (listener) {
      while (listener.open) {
        listener.wait();
      }
    }

    assertThat(consoleManager.getConsoles().contains(console), is(false));
  }
}
