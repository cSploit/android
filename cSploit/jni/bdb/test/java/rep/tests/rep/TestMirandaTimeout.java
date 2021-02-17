/*-
 * See the file LICENSE for redistribution information.
 * 
 * Copyright (c) 2010, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */

package rep;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

import com.sleepycat.db.*;

import repmgrtests.Util;
import org.junit.Test;
import java.util.*;

public class TestMirandaTimeout {
    class /* struct */ Message {
        DatabaseEntry ctl, rec;
        int sourceEID;
        boolean perm;
        Message(DatabaseEntry ctl, DatabaseEntry rec, int eid, boolean p) {
            this.ctl = ctl;
            this.rec = rec;
            sourceEID = eid;
            perm = p;
        }
    }

    List<Queue<Message>> queues = new ArrayList<Queue<Message>>();
    Environment[] envs = new Environment[2];

    public static final int SELF = Integer.MAX_VALUE;

    @Test public void deferredArrival() throws Exception {
        EnvironmentConfig ec = new EnvironmentConfig();
        ec.setAllowCreate(true);
        ec.setInitializeCache(true);
        ec.setInitializeLocking(true);
        ec.setInitializeLogging(true);
        ec.setInitializeReplication(true);
        ec.setTransactional(true);
        ec.setThreaded(true);
        ec.setReplicationNumSites(2);
        if (Boolean.getBoolean("VERB_REPLICATION"))
            ec.setVerbose(VerboseConfig.REPLICATION, true);

        // The EID serves double duty as an index into the array of
        // ArrayLists which are used as the message queues.  Master is
        // EID 0 and client is EID 1.
        //
        queues.add(new LinkedList<Message>());
        queues.add(new LinkedList<Message>());
        ec.setReplicationTransport(SELF, new Transport(0));
        Environment master = new Environment(Util.mkdir("master"), ec);
        envs[0] = master;
        ec.setReplicationTransport(SELF, new Transport(1));
        Environment client = new Environment(Util.mkdir("client"), ec);
        envs[1] = client;

        master.startReplication(null, true);
        client.startReplication(null, false);

        processMessages();

        DatabaseConfig dc = new DatabaseConfig();
        dc.setTransactional(true);
        dc.setAllowCreate(true);
        dc.setType(DatabaseType.BTREE);
        Database db = master.openDatabase(null, "test.db", null, dc);
        processMessages();

        Transaction txn = master.beginTransaction(null, null);
        DatabaseEntry key = new DatabaseEntry();
        DatabaseEntry value = new DatabaseEntry();
        key.setData("mykey".getBytes());
        value.setData("myvalue".getBytes());
        db.put(txn, key, value);
        txn.commit();
        byte[] token1 = txn.getCommitToken();

        txn = master.beginTransaction(null, null);
        key = new DatabaseEntry();
        value = new DatabaseEntry();
        key.setData("one,two".getBytes());
        value.setData("buckle my shoe".getBytes());
        db.put(txn, key, value);
        txn.commit();
        byte[] token2 = txn.getCommitToken();

        // Since we haven't sent pending msgs to the client yet, the
        // transaction shouldn't be there yet.
        // 
        assertEquals(TransactionStatus.TIMEOUT,
                     client.isTransactionApplied(token1, 0));

        
        // Start 2 client threads to wait for the transactions in
        // reverse order, to make sure the waiting completes in the
        // correct order.
        // 
        Client clientTh2 = new Client(client, token2);
        Thread t2 = new Thread(clientTh2, "clientThread2");
        t2.start();
        Thread.sleep(5000);

        Client clientTh1 = new Client(client, token1);
        Thread t1 = new Thread(clientTh1, "clientThread1");
        t1.start();
        Thread.sleep(5000);

        processOnePerm();
        Thread.sleep(5000);
        processMessages();

        t1.join();
        t2.join();

        assertEquals(TransactionStatus.APPLIED, clientTh1.getResult());
        assertTrue(clientTh1.getDuration() > 1000 &&
                   clientTh1.getDuration() < 100000);
        assertEquals(TransactionStatus.APPLIED, clientTh2.getResult());

        // We started thread2 (way) before starting thread1, so its
        // start time should be less (assuming the system isn't
        // ridiculously busy).  But thread1 still should have
        // completed (way) before thread2, because its transaction was
        // replicated first (before a pause).
        // 
        assertTrue(clientTh2.getStartTime() < clientTh1.getStartTime());
        assertTrue(clientTh1.getEndTime() < clientTh2.getEndTime());

        db.close();
        client.close();
        master.close();
    }

    class Client implements Runnable {
        private Environment env;
        private byte[] token;
        private TransactionStatus result;
        private long duration, endTime, startTime;
        public void run() {
            try {
                startTime = System.currentTimeMillis();
                result = env.isTransactionApplied(token, 100000000);
                endTime = System.currentTimeMillis();
                duration = endTime - startTime;
            } catch (Exception e) {
                // if an exception happens, we leave "result"
                // unset, which will eventually cause a test failure
                // in the parent thread.
                e.printStackTrace();
            }
        }

        Client(Environment env, byte[] token) {
            this.env = env;
            this.token = token;
        }

        long getDuration() { return duration; }
        long getStartTime() { return startTime; }
        long getEndTime() { return endTime; }
        TransactionStatus getResult() { return result; }
    }
            

    class Transport implements ReplicationTransport {
        private int myEID;
        Transport(int eid) { myEID = eid; }
        public int send(Environment env, DatabaseEntry ctl, DatabaseEntry rec,
                        LogSequenceNumber lsn, int eid, boolean noBuf,
                        boolean perm, boolean anywhere, boolean isRetry) {
            int target = 1 - myEID;
            queues.get(target).add(new Message(ctl, rec, myEID, perm));
            return (0);
        }
    }

    void processOnePerm() throws Exception {
        Queue<Message> q = queues.get(1); // the client is site 1
        while (!q.isEmpty()) {
            Message m = q.remove();
            envs[1].processReplicationMessage(m.ctl, m.rec, m.sourceEID);
            if (m.perm)
                return;
        }
    }

    void processMessages() throws Exception {
        boolean done = false;
        while (!done) {
            boolean got = false;
            for (int eid = 0; eid < 2; eid++) {
                Queue<Message> q = queues.get(eid);
                while (!q.isEmpty()) {
                    got = true;
                    Message m = q.remove();
                    envs[eid].processReplicationMessage(m.ctl, m.rec, m.sourceEID);
                }
            }
            if (!got)
                done = true;
        }
    }
}
