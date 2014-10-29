package it.evilsocket.dsploit.core;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import it.evilsocket.dsploit.core.Child.*;
import it.evilsocket.dsploit.events.ChildDied;
import it.evilsocket.dsploit.events.ChildEnd;
import it.evilsocket.dsploit.events.Event;
import it.evilsocket.dsploit.events.StderrNewline;

/**
 * a class that manage spawned commands
 *
 * it take care of handle ChildEnd, ChildDied events.
 * also give some key methods that add an extra abstraction layer between callers and the dSploitd client.
 */
public class ChildManager {

  private static final List<Child> children = new ArrayList<Child>(25);

  public static List<String> handlers = null;

  /**
   * execute a command
   * @param handler the command handler
   * @param cmd the command to execute
   * @param receiver the {@link it.evilsocket.dsploit.core.Child.EventReceiver} that will receive child events
   * @return the process exit value
   * @throws InterruptedException if current thread get interrupted
   * @throws ChildDiedException if child get killed
   */
  public static int exec(String handler, String cmd, EventReceiver receiver) throws InterruptedException, ChildDiedException, ChildNotStartedException {
    Child c;

    c = async(handler, cmd, receiver);

    synchronized (children) {
      while (c.running)
        children.wait();
    }

    if(c.signal >= 0) {
      throw new ChildDiedException(c.exitValue);
    }
    return c.exitValue;
  }

  public static int exec(String handler, String cmd) throws InterruptedException, ChildDiedException, ChildNotStartedException {
    return exec(handler, cmd, null);
  }

  static int exec(String cmd) throws InterruptedException, ChildDiedException, ChildNotStartedException {
    return exec("blind", cmd, null);
  }

  public static Child async(String handler, String cmd, EventReceiver receiver) throws ChildNotStartedException {
    Child c;

    c = new Child();

    c.id = Client.StartCommand(handler, cmd);
    if (c.id == -1)
      throw new ChildNotStartedException();

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
    return async(handler, cmd, null);
  }

  public static Child async(String cmd) throws ChildNotStartedException {
    return async("blind", cmd, null);
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
   * @return the searched {@link it.evilsocket.dsploit.core.Child}
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

  /**
   * handle an incoming event from a child
   * @param childID the child that generated this event
   * @param event the generated {@link it.evilsocket.dsploit.events.Event}
   *
   * this function is the main entry point for generated events.
   */
  public static void onEvent(int childID, Event event) {
    Child c;
    boolean end, died, stderr;

    died = stderr = false;

    end = event instanceof ChildEnd;
    if(!end) {
      died = event instanceof ChildDied;
      if(!died) {
        stderr = event instanceof StderrNewline;
      }
    }

    Logger.debug("received an event: " + event);

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

    if(c.receiver != null) {
      if(end) {
        c.exitValue = ((ChildEnd) event).exit_status;
        Logger.debug("Child #" + c.id + " exited ( exitValue=" + c.exitValue + " )");
        c.receiver.onEnd(c.exitValue);
      } else if(died) {
        c.signal = ((ChildDied) event).signal;
        Logger.debug("Child #" + c.id + " died ( signal=" + c.signal + " )");
        c.receiver.onDeath(c.signal);
      } else if(stderr) {
        Logger.warning("Child #" + c.id + " sent '" + ((StderrNewline) event).line + "' to stderr");
        c.receiver.onStderr(((StderrNewline) event).line);
      } else {
        c.receiver.onEvent(event);
      }
    }

    if(end || died) {
      synchronized (children) {
        c.running = false;
        children.remove(c);
        children.notifyAll();
      }
    }
  }

  public static class ChildDiedException extends Exception {
    public ChildDiedException(int signal) {super("process terminated by signal " + signal);}
  }

  public static class ChildNotFoundException extends Exception {
    public ChildNotFoundException(int id) { super("child #" + id + " not found"); }
  }

  public static class ChildNotStartedException extends Exception {
    public ChildNotStartedException() {
      super("cannot start commands");
    }

  }
}
