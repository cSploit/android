package org.csploit.android.core;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.concurrent.Executor;
import java.util.concurrent.Executors;

import org.csploit.android.core.Child.*;
import org.csploit.android.events.ChildDied;
import org.csploit.android.events.ChildEnd;
import org.csploit.android.events.Event;
import org.csploit.android.events.Newline;
import org.csploit.android.events.StderrNewline;

/**
 * a class that manage spawned commands
 *
 * it take care of handle ChildEnd, ChildDied events.
 * also give some key methods that add an extra abstraction layer between callers and the dSploitd client.
 */
public class ChildManager {

  private static final List<Child> children = new ArrayList<Child>(25);

  public static List<String> handlers = null;

  private static Executor eventExecutor = Executors.newCachedThreadPool();

  /**
   * wait for a child termination
   * @param c  the child to wait for
   * @return the child exit code
   * @throws InterruptedException if current thread gets interrupted while waiting
   * @throws ChildDiedException if child has been killed by a signal
   */
  public static int wait(Child c) throws InterruptedException, ChildDiedException {
    synchronized (children) {
      while (c.running)
        children.wait();
    }

    if(c.signal >= 0) {
      throw new ChildDiedException(c.signal);
    }
    return c.exitValue;
  }

  /**
   * execute a command
   * @param handler name of the handler
   * @param env     custom environment variables
   * @param cmd     arguments for the child
   * @param receiver the {@link org.csploit.android.core.Child.EventReceiver} that will receive child events
   * @return the process exit value
   * @throws InterruptedException if current thread get interrupted
   * @throws ChildDiedException if child get killed
   */
  public static int exec(String handler, String cmd, String[] env, EventReceiver receiver) throws InterruptedException, ChildDiedException, ChildNotStartedException {
    return wait(async(handler, cmd, env, receiver));
  }

  public static int exec(String handler, String cmd) throws InterruptedException, ChildDiedException, ChildNotStartedException {
    return exec(handler, cmd, null, null);
  }

  static int exec(String cmd) throws InterruptedException, ChildDiedException, ChildNotStartedException {
    return exec("blind", cmd, null, null);
  }

  /**
   * spawn a child asynchronously
   *
   * @param handler   name of the handler
   * @param cmd       arguments for the child
   * @param env       custom environment variables
   * @param receiver  receiver for events fired by the child
   * @return the spawned {@link org.csploit.android.core.Child}
   * @throws ChildNotStartedException when cannot start a child.
   */
  public static Child async(String handler, String cmd, String[] env, EventReceiver receiver) throws ChildNotStartedException {
    Child c;

    c = new Child();

    c.id = Client.StartCommand(handler, cmd, env);
    if (c.id == -1) {
      Logger.debug(String.format("{ handler='%s', cmd='%s' } => FAILED", handler, cmd));
      throw new ChildNotStartedException();
    }

    Logger.debug(String.format("{ handler='%s', cmd='%s' } => %d", handler, cmd, c.id));

    c.running = true;
    c.receiver = receiver;

    if(c.receiver!=null)
      c.receiver.onStart(cmd);

    synchronized (children) {
      children.add(c);
      children.notifyAll();
    }

    return c;
  }

  public static Child async(String handler, String cmd) throws ChildNotStartedException {
    return async(handler, cmd, null, null);
  }

  public static Child async(String cmd) throws ChildNotStartedException {
    return async("blind", cmd, null, null);
  }

  public static void join(Child c) throws InterruptedException {

    synchronized (children) {
      while(children.contains(c)) {
        children.wait();
      }
    }
  }

  public static void storeHandlers() {
    String [] result = Client.getHandlers();

    if(result==null) {
      handlers = null;
      return;
    }

    handlers = Arrays.asList(result);
  }

  /**
   * get a child by it's ID
   *
   * @param childID the id of the searched child
   * @return the searched {@link org.csploit.android.core.Child}
   *
   * WARNING: you must own the {@code children} lock ( synchronized on it )
   */
  private static Child getChildByID(int childID) {
    Child found = null;

    for(Child c : children) {
      if(c.id == childID) {
        found = c;
        break;
      }
    }

    return found;
  }

  private static void dispatchEvent(Child c, Event event) {
    boolean crash = false;
    boolean terminated = false;

    if(event instanceof ChildEnd) {
      c.exitValue = ((ChildEnd) event).exit_status;
      Logger.debug("Child #" + c.id + " exited ( exitValue=" + c.exitValue + " )");
      if(c.receiver != null)
        c.receiver.onEnd(c.exitValue);
      terminated = true;
      crash = c.exitValue > 128 && c.exitValue != 130 && c.exitValue != 137 && c.exitValue < 150;
      if(crash) {
        c.signal = c.exitValue - 128;
      }
    } else  if(event instanceof ChildDied) {
      c.signal = ((ChildDied) event).signal;
      Logger.debug("Child #" + c.id + " died ( signal=" + c.signal + " )");
      if(c.receiver != null)
        c.receiver.onDeath(c.signal);
      terminated = true;
      crash = c.signal != 2 && c.signal != 9;
    } else if(event instanceof StderrNewline) {
      if(c.receiver != null)
        c.receiver.onStderr(((StderrNewline) event).line);
    } else if(c.receiver != null) {
      c.receiver.onEvent(event);
    }

    if(terminated) {
      synchronized (children) {
        c.running = false;
        children.remove(c);
        children.notifyAll();
      }
    }

    if(crash) {
      CrashReporter.notifyChildCrashed(c.id, c.signal);
    }
  }

  /**
   * handle an incoming event from a child
   * @param childID the child that generated this event
   * @param event the generated {@link org.csploit.android.events.Event}
   *
   * this function is the main entry point for generated events.
   */
  public static void onEvent(final int childID, final Event event) {
    Child c;

    if(!(event instanceof Newline)) {
      Logger.debug("received an event: " + event);
    }

    synchronized (children) {
      while((c = getChildByID(childID)) == null) {
        try {
          children.wait();
        } catch (InterruptedException e) {
          Logger.error(e.getMessage());
          Logger.error("event lost: " + event);
          return;
        }
      }
    }

    final Child fc = c;

    eventExecutor.execute(new Runnable() {
      @Override
      public void run() {
        dispatchEvent(fc, event);
      }
    });
  }

  public static class ChildDiedException extends Exception {
    public ChildDiedException(int signal) {super("process terminated by signal " + signal);}
  }

  public static class ChildNotStartedException extends Exception {
    public ChildNotStartedException() {
      super("cannot start commands");
    }
  }
}
