package it.evilsocket.dsploit.events;

/**
 * hydra attempts status
 */
public class Attempts implements Event {
  public final long sent;
  public final long left;
  public final long rate;
  public final long elapsed;
  public final long eta;

  public Attempts(long sent, long left, long rate, long elapsed, long eta) {
    this.sent = sent;
    this.left = left;
    this.rate = rate;
    this.elapsed = elapsed;
    this.eta = eta;
  }
}
