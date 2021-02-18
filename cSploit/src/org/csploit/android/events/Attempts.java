package org.csploit.android.events;

/**
 * hydra attempts status
 */
public class Attempts implements Event {
    public final int rate;
    public final int sent;
    public final int elapsed;
    public final int left;
    public final long eta;

    public Attempts(int rate, int sent, int elapsed, int left, int eta) {
        this.rate = rate;
        this.sent = sent;
        this.elapsed = elapsed;
        this.left = left;
        this.eta = eta;
    }
    public String toString() {
        return String.format("AttemptsEvent: { rate=%d, sent=%d, elapsed=%d, left=%d, eta=%d }", rate, sent, elapsed, left, eta);
    }
}
