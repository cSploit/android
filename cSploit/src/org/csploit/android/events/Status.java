package org.csploit.android.events;

/**
 * hydra attempts status
 */
public class Status implements Event {
    public final int rate;
    public final int sent;
    public final int elapsed;
    public final int left;
    public final int eta;

    public Status(int rate, int sent, int elapsed, int left, int eta) {
        this.rate = rate;
        this.sent = sent;
        this.elapsed = elapsed;
        this.left = left;
        this.eta = eta;
    }
    @Override
    public String toString() {
        return String.format("Attempts: { rate='%d', sent='%d', elapsed='%d', left='%d', eta='%s' }", rate, sent, elapsed, left, eta);
    }
}
