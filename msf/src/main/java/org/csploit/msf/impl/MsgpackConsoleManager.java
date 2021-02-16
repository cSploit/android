package org.csploit.msf.impl;

import org.csploit.msf.api.Console;
import org.csploit.msf.api.ConsoleManager;
import org.csploit.msf.api.MsfException;
import org.csploit.msf.api.events.ConsoleChangedEvent;
import org.csploit.msf.api.events.ConsoleClosedEvent;
import org.csploit.msf.api.events.ConsoleOpenedEvent;
import org.csploit.msf.api.events.ConsoleOutputEvent;
import org.csploit.msf.api.exceptions.ResourceNotFoundException;
import org.csploit.msf.api.listeners.ConsoleListener;

import java.io.IOException;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.TreeMap;
import java.util.TreeSet;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

/**
 * A console Manager that use a Msgpack RPC connection to deal with the framework
 */
class MsgpackConsoleManager implements ConsoleManager, Runnable {
  private static final int SLEEP_INTERVAL = 3000;
  private static final int WORK_INTERVAL = 300;

  private final MsgpackClient client;
  private final Map<Integer, MsgpackConsole> map = new TreeMap<>();
  private final Set<ConsoleListener> listeners = new HashSet<>();
  private final ExecutorService executor;
  private Thread thread;
  private boolean running;

  public MsgpackConsoleManager(MsgpackClient client) {
    this.client = client;
    executor = Executors.newSingleThreadExecutor();
  }

  private void updateConsoles() throws IOException, MsfException {
    MsgpackClient.ConsoleInfo[] remote = client.getConsoles(); // n
    List<Integer> remoteIds = new LinkedList<>();
    Set<Integer> toRemove;

    // MAX(n, m, n * log(m), m * log(m), 2n * log(m)) => log(m) * MAX(2n, m)

    for (MsgpackClient.ConsoleInfo info : remote) { // n
      remoteIds.add(info.id); // 1
    }

    synchronized (map) {
      // toRemove = local - remote
      toRemove = new TreeSet<>(map.keySet()); // m

      for (Integer id : remoteIds) { // n
        toRemove.remove(id); // log(m)
      }

      for (Integer id : toRemove) { // m
        Console c = map.remove(id); // log(m)
        notifyConsoleClosed(c);
      }

      for (MsgpackClient.ConsoleInfo info : remote) { // n
        MsgpackConsole console = map.get(info.id); // log(m)
        if (console != null) {
          MsgpackLoader.fillConsole(info, console);

          if (console.hasChanged()) {
            notifyConsoleChanged(console);
            console.clearChanged();
          }
        } else {
          console = buildConsole(info);
          map.put(info.id, console); // log(m)
          notifyConsoleOpened(console);
        }
      }
    }
  }

  @Override
  public List<? extends Console> getConsoles() throws IOException, MsfException {

    updateConsoles();

    synchronized (map) {
      return new ArrayList<>(map.values());
    }
  }

  @Override
  public Console getConsole(int id) throws IOException, MsfException {
    Console result;
    synchronized (map) {
      result = map.get(id);
      if(result != null) {
        return result;
      }
    }

    updateConsoles();

    synchronized (map) {
      result = map.get(id);
    }

    if(result == null) {
      throw new ResourceNotFoundException("console #" + id + " not found");
    }

    return result;
  }

  private MsgpackConsole addConsole(MsgpackClient.ConsoleInfo info) throws IOException, MsfException {
    MsgpackConsole console;
    synchronized (map) {
      console = map.remove(info.id);
      if(console != null) {
        console.complete();
      }
      console = buildConsole(info);
      map.put(info.id, console);
    }

    notifyConsoleOpened(console);
    return console;
  }

  @Override
  public Console create(boolean allowCommandPassthru, boolean realReadline, String histFile, String[] resources, boolean skipDatabaseInit) throws IOException, MsfException {
    MsgpackClient.ConsoleInfo info = client.createConsole(allowCommandPassthru, realReadline, histFile, resources, skipDatabaseInit);

    return addConsole(info);
  }

  @Override
  public Console create() throws IOException, MsfException {
    MsgpackClient.ConsoleInfo info = client.createConsole();

    return addConsole(info);
  }

  @Override
  public void addSubscriber(ConsoleListener listener) {
    synchronized (listeners) {
      listeners.add(listener);
    }
  }

  @Override
  public void removeSubscriber(ConsoleListener listener) {
    synchronized (listeners) {
      listeners.remove(listener);
    }
  }

  @Override
  public void run() {
    running = true;

    while(running) {

      Set<InternalConsole> copy;

      synchronized (map) {
        copy = new HashSet<InternalConsole>(map.values());
      }

      boolean interacting = false;

      for(InternalConsole c : copy) {
        if(!interacting) {
          try {
            interacting = c.isInteracting();
          } catch (IOException | MsfException e) {
            // ignored
          }
        }

        try {
          String output = c.read();

          if(output != null && output.length() > 0) {
            notifyConsoleOutput(c, output);
          }

          if(c.hasChanged()) {
            c.clearChanged();
            notifyConsoleChanged(c);
          }
        } catch (IOException | MsfException e) {
          // ignored
        }
      }

      try {
        getConsoles();
      } catch (IOException | MsfException e) {
        // ignored
      }

      long interval = interacting ? WORK_INTERVAL : SLEEP_INTERVAL;

      try {
        Thread.sleep(interval);
      } catch (InterruptedException e) {
        running = false;
      }
    }
  }

  public void start() {
    stop();
    thread = new Thread(this, getClass().getSimpleName());
    thread.start();
  }

  public void stop() {
    if(thread != null && thread.isAlive()) {
      running = false;
      try {
        thread.join();
      } catch (InterruptedException e) {
        // ignored
      }
    }
  }

  private MsgpackConsole buildConsole(MsgpackClient.ConsoleInfo info) {
    MsgpackConsole console = new MsgpackConsole(client, info.id);
    MsgpackLoader.fillConsole(info, console);
    return console;
  }

  private void notifyConsoleOpened(Console console) {
    final ConsoleOpenedEvent event = new ConsoleOpenedEvent(this, console);

    executor.execute(new Runnable() {
      @Override
      public void run() {
        synchronized (listeners) {
          for (ConsoleListener listener : listeners) {
            listener.onConsoleOpened(event);
          }
        }
      }
    });
  }

  private void notifyConsoleChanged(Console console) {
    final ConsoleChangedEvent event = new ConsoleChangedEvent(this, console);

    executor.execute(new Runnable() {
      @Override
      public void run() {
        synchronized (listeners) {
          for (ConsoleListener listener : listeners) {
            listener.onConsoleChanged(event);
          }
        }
      }
    });
  }

  private void notifyConsoleOutput(Console console, String output) {
    final ConsoleOutputEvent event = new ConsoleOutputEvent(this, console, output);

    executor.execute(new Runnable() {
      @Override
      public void run() {
        synchronized (listeners) {
          for(ConsoleListener listener : listeners) {
            listener.onConsoleOutput(event);
          }
        }
      }
    });
  }

  private void notifyConsoleClosed(Console console) {
    final ConsoleClosedEvent event = new ConsoleClosedEvent(this, console);

    executor.execute(new Runnable() {
      @Override
      public void run() {
        synchronized (listeners) {
          for(ConsoleListener listener : listeners) {
            listener.onConsoleClosed(event);
          }
        }
      }
    });
  }

  @Override
  protected void finalize() throws Throwable {
    super.finalize();
    stop();
  }
}
