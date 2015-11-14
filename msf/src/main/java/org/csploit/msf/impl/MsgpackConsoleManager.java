package org.csploit.msf.impl;

import org.csploit.msf.api.Console;
import org.csploit.msf.api.ConsoleManager;
import org.csploit.msf.api.MsfException;
import org.csploit.msf.api.events.ConsoleEvent;
import org.csploit.msf.api.events.ConsoleOutputEvent;
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

  @Override
  public List<? extends Console> getConsoles() throws IOException, MsfException {
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
          boolean changed = info.busy != console.isBusy();
          changed |= info.prompt != null ?
                  !info.prompt.equals(console.getPrompt()) : console.getPrompt() != null;
          MsgpackLoader.fillConsole(info, console);

          if(changed) {
            notifyConsoleChanged(console);
          }
        } else {
          console = buildConsole(info);
          map.put(info.id, console); // log(m)
          notifyConsoleOpened(console);
        }
      }

      return new ArrayList<>(map.values());
    }
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
          notifyConsoleOutput(c, output);
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
    final int id = console.getId();

    executor.execute(new Runnable() {
      @Override
      public void run() {
        synchronized (listeners) {
          for (ConsoleListener listener : listeners) {
            listener.onConsoleOpened(new ConsoleEventImpl(id));
          }
        }
      }
    });
  }

  private void notifyConsoleChanged(Console console) {
    final int id = console.getId();

    executor.execute(new Runnable() {
      @Override
      public void run() {
        synchronized (listeners) {
          for (ConsoleListener listener : listeners) {
            listener.onConsoleChanged(new ConsoleEventImpl(id));
          }
        }
      }
    });
  }

  private void notifyConsoleOutput(Console console, final String output) {
    final int id = console.getId();

    executor.execute(new Runnable() {
      @Override
      public void run() {
        synchronized (listeners) {
          for(ConsoleListener listener : listeners) {
            listener.onConsoleOutput(new ConsoleOutputEventImpl(id, output));
          }
        }
      }
    });
  }

  private void notifyConsoleClosed(Console console) {
    final int id = console.getId();

    executor.execute(new Runnable() {
      @Override
      public void run() {
        synchronized (listeners) {
          for(ConsoleListener listener : listeners) {
            listener.onConsoleClosed(new ConsoleEventImpl(id));
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

  private class ConsoleEventImpl implements ConsoleEvent {
    private final int id;

    public ConsoleEventImpl(int id) {
      this.id = id;
    }

    @Override
    public int getId() {
      return id;
    }
  }

  private class ConsoleOutputEventImpl implements ConsoleOutputEvent {
    private final int id;
    private final String output;

    public ConsoleOutputEventImpl(int id, String output) {
      this.id = id;
      this.output = output;
    }

    @Override
    public int getId() {
      return id;
    }

    @Override
    public String getOutput() {
      return output;
    }
  }
}
