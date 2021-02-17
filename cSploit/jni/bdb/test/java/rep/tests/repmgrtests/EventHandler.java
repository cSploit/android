/*-
 * See the file LICENSE for redistribution information.
 * 
 * Copyright (c) 2011, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */

package repmgrtests;

import com.sleepycat.db.EventHandlerAdapter;

class EventHandler extends EventHandlerAdapter {
    private boolean done;
    private boolean panic;
    private boolean gotNewmaster;
    private boolean iAmMaster;
    private int permFailCount;
    	
    @Override synchronized public void handleRepStartupDoneEvent() {
        done = true;
        notifyAll();
    }

    @Override synchronized public void handleRepMasterEvent() {
        iAmMaster = true;
    }

    @Override synchronized public void handleRepClientEvent() {
        iAmMaster = false;
    }

    @Override synchronized public void handleRepNewMasterEvent(int eid) {
        gotNewmaster = true;
        notifyAll();
    }

    @Override synchronized public void handleRepPermFailedEvent() {
            permFailCount++;
    }

    synchronized public boolean isMaster() { return iAmMaster; }

    @Override synchronized public void handlePanicEvent() {
        panic = true;
        notifyAll();
    }
    
    synchronized void await() throws Exception {
        while (!done) {
            wait();
            checkPanic();
        }
    }

    synchronized void awaitNewmaster(long maxWait) throws Exception {
        long deadline = System.currentTimeMillis() + maxWait;
        
        while (!gotNewmaster) {
            long now = System.currentTimeMillis();
            if (now >= deadline)
                throw new Exception("aborted by timeout");
            long waitTime = deadline-now;
            wait(waitTime);
            checkPanic();
        }
    }

    synchronized void awaitNewmaster() throws Exception {
        while (!gotNewmaster) {
            wait();
            checkPanic();
        }
    }

    synchronized public int getPermFailCount() { return permFailCount; }

    private void checkPanic() throws Exception {
        if (panic)
            throw new Exception("aborted by DB panic");
    }
}
