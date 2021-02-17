package org.csploit.android.core;

import java.io.IOException;

import org.csploit.android.events.Event;

/**
 * class to hold info about a spawned child
 */
public class Child {
  public int id;
  public int exitValue;
  public int signal;
  public EventReceiver receiver;
  public boolean running;

  public Child() {
    this.id = -1;
    this.exitValue = 0;
    this.receiver = null;
    this.running = false;
    this.signal = -1;
  }

  public static abstract class EventReceiver {
    /**
     * callback function called whence a child has been successfully started
     *
     * this method is called before any other in this class,
     * and before receiving any event from the associated child.
     * @param cmd the command that this child is executing
     */
    public void onStart(String cmd) { }

    /**
     * callback function called whence the child exit
     * @param exitValue the child exit value
     */
    public void onEnd(int exitValue) { }

    /**
     * callback function called whence the child get terminated by a signal
     * @param signal the signal that killed the child
     */
    public void onDeath(int signal) { }

    /**
     * callback function called whence the child print something on the stderr
     * @param line  the printed line
     */
    public void onStderr(String line) { }

    /**
     * callback function called whence the child generate an evant
     * @param e the generated {@link org.csploit.android.events.Event}
     */
    public abstract void onEvent(Event e);
  }

  /**
   * send bytes to this child's stdin
   * @param data the bytes to send
   */
  public synchronized void send(byte[] data) throws IOException {
    if(!Client.SendTo(this.id, data))
      throw new IOException("cannot send bytes to child");
  }

  /**
   * send a string to this child's stdin
   * @param s the string to send
   */
  public void send(String s) throws IOException {
    send(s.getBytes());
  }

  /**
   * send a signal to this child
   * @param signal the signal to send
   */
  public void kill(int signal) {
    Client.Kill(this.id, signal);
  }

  /**
   * kill this child by sending a SIGKILL
   */
  public void kill() {
    Client.Kill(this.id, 9);
  }

  /**
   * join a child by waiting it's termination
   */
  public void join() throws InterruptedException {
    ChildManager.join(this);
  }

  public boolean equals(Object o) {
    Child c;

    if(!(o instanceof Child))
      return false;

    c = (Child)o;

    return c.id == this.id;
  }
}
