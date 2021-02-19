package org.csploit.android.events;

/**
 * hydra attempts status
 */
public class Attempts implements Event {
    public final int rate;
    public final int sent;
    public final int left;

    public Attempts(int rate, int sent, int left) {
        this.rate = rate;
        this.sent = sent;
        this.left = left;
    }
    public String toString() {
        return String.format("Attempts: { rate='%d', sent='%d', left='%d' }", rate, sent, left);
    }
}
