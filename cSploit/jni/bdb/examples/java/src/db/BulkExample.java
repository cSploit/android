/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1997, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$
 */

package db;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.nio.ByteBuffer;

import com.sleepycat.db.*;

public class BulkExample {
    private static final String dbFileName = "BulkExample.db";
    private static final String pDbName = "primary";
    private static final String sDbName = "secondary";
    private static final String home = "BulkExample";
    private static final int buffLen = 1024 * 1024;

    /* Bulk methods demonstrated in the example */
    private enum Operations {BULK_DELETE, BULK_READ, BULK_UPDATE}

    private Database pdb;
    private Environment env;
    private Operations opt = Operations.BULK_READ;
    private SecondaryDatabase sdb;
    private int cachesize = 0, dups = 0, pagesize = 0;
    private int num = 1000000;
    private boolean secondary = false;

    public static void main(String[] args) {
        long startTime, endTime;
        int count;

        BulkExample app = new BulkExample();

        /* Parse argument */
        for (int i = 0; i < args.length; i++) {
            if (args[i].compareTo("-c") == 0) {
                if (i == args.length - 1)
                    usage();
                i++;
                app.cachesize = Integer.parseInt(args[i]);
                System.out.println("[CONFIG] cachesize " + app.cachesize);
            } else if (args[i].compareTo("-d") == 0) {
                if (i == args.length - 1)
                    usage();
                i++;
                app.dups = Integer.parseInt(args[i]);
                System.out.println(
                    "[CONFIG] number of duplicates " + app.dups);
            } else if (args[i].compareTo("-n") == 0) {
                if (i == args.length - 1)
                    usage();
                i++;
                app.num = Integer.parseInt(args[i]);
                System.out.println("[CONFIG] number of keys " + app.num);
            } else if (args[i].compareTo("-p") == 0) {
                if (i == args.length - 1)
                    usage();
                i++;
                app.pagesize = Integer.parseInt(args[i]);
                System.out.println("[CONFIG] pagesize " + app.pagesize);
            } else if (args[i].compareTo("-D") == 0) {
                app.opt = Operations.BULK_DELETE;
            } else if (args[i].compareTo("-R") == 0) {
                app.opt = Operations.BULK_READ;
            } else if (args[i].compareTo("-S") == 0) {
                app.secondary = true;
            } else if (args[i].compareTo("-U") == 0) {
                app.opt = Operations.BULK_UPDATE;
            }else
                usage();
        }

        /* Open environment and database(s) */
        try {
            /* If perform bulk update or delete, clean environment home */
            app.cleanHome(app.opt != Operations.BULK_READ);
            app.initDbs();
        }catch (Exception e) {
            e.printStackTrace();
            app.closeDbs();
        }

        try {
            /* Perform bulk read from existing primary db */
            if (app.opt == Operations.BULK_READ) {
                startTime = System.currentTimeMillis();
                count = app.bulkRead();
                endTime = System.currentTimeMillis();
                app.printBulkOptStat(
                    Operations.BULK_READ, count, startTime, endTime);
            } else {
                /* Perform bulk update to populate the db */
                startTime = System.currentTimeMillis();
                app.bulkUpdate();
                endTime = System.currentTimeMillis();
                count = (app.dups == 0) ? app.num : app.num * app.dups;
                app.printBulkOptStat(
                    Operations.BULK_UPDATE, count, startTime, endTime);

                /* Perform bulk delete in primary db or secondary db */
                if (app.opt == Operations.BULK_DELETE) {
                    startTime = System.currentTimeMillis();
                    if (app.secondary == false) {
                        /*
                         * Delete from the first key to a random one in
                         * primary db.
                         */
                        count = new java.util.Random().nextInt(app.num);
                        count = app.bulkDelete(count);
                    } else {
                        /*
                         * Delete from the first key to a random one in
                         * secondary db.
                         */
                        count = new java.util.Random().nextInt(
                            DataVal.tstring.length());
                        count = app.bulkSecondaryDelete(
                            (DataVal.tstring.getBytes())[count], 
                            true);
                    } 
                    endTime = System.currentTimeMillis();

                    app.printBulkOptStat(Operations.BULK_DELETE, 
                        count, startTime, endTime);
                }
            }
        } catch (DatabaseException e) {
            e.printStackTrace();
        } catch (IOException e) {
            e.printStackTrace();
        } finally {
            /* Close dbs */
            app.closeDbs();
        }
    }

    /* Perform bulk delete in primary db */
    public int bulkDelete(int value) throws DatabaseException, IOException {
        DatabaseEntry key = new DatabaseEntry();
        Transaction txn = env.beginTransaction(null, null);
        int count = 0;

        try {
            if (dups != 0) {
                /* Delete a set of keys */
                MultipleDataEntry keySet = new MultipleDataEntry();
                keySet.setData(new byte[buffLen]);
                keySet.setUserBuffer(buffLen, true);
                for (int i = 0; i < value; i++) {
                    key.setData(intToByte(i));
                    keySet.append(key);
                }
                if (pdb.deleteMultiple(txn, keySet) != 
                    OperationStatus.SUCCESS)
                    count = 0;
                else
                    count = value * dups;
            } else {
                /* Delete a set of key/value pairs */
                MultipleKeyDataEntry pairSet = new MultipleKeyDataEntry();
                pairSet.setData(new byte[buffLen * 2]);
                pairSet.setUserBuffer(buffLen * 2, true);
                DatabaseEntry data = new DatabaseEntry(); 
                for (int i = 0; i < value; i++) {
                    key.setData(intToByte(i));
                    DataVal dataVal = new DataVal(i);
                    data.setData(dataVal.getBytes());
                    pairSet.append(key, data);
                }
                if (pdb.deleteMultipleKey(txn, pairSet) != 
                    OperationStatus.SUCCESS)
                    count = 0;
                else
                    count = value;
            }

            txn.commit();
            return count;
        } catch (DatabaseException e) {
            txn.abort();
            throw e;
        }
    }

    /* Perform bulk read in primary db */
    public int bulkRead() throws DatabaseException {
        ByteBuffer buffer1, buffer2;
        Cursor cursor;
        DatabaseEntry key, data;
        MultipleKeyNIODataEntry pairs;
        MultipleNIODataEntry keySet, dataSet;
        int count;

        key = new DatabaseEntry();
        data = new DatabaseEntry();
        count = 0;
        
        Transaction txn = env.beginTransaction(null, null);

        try {
            cursor = pdb.openCursor(txn, null);
        } catch (DatabaseException e) {
            txn.abort();
            return 0;
        }

        try {    
            if (dups == 0){
                /* Get all records in a single key/data buffer */
                pairs = new MultipleKeyNIODataEntry();
                buffer1 = ByteBuffer.allocateDirect(buffLen * 2);
                pairs.setDataNIO(buffer1);
                pairs.setUserBuffer(buffLen * 2, true);
                while (cursor.getNext(key, pairs, null) ==
                    OperationStatus.SUCCESS) {
                    while(pairs.next(key, data))
                        count++;
                    key = new DatabaseEntry();
                }
            } else {
                /*
                 * Get all key/data pairs in two buffers, one for all keys,
                 * the other for all data.
                 */
                keySet = new MultipleNIODataEntry();
                buffer1 = ByteBuffer.allocateDirect(buffLen);
                keySet.setDataNIO(buffer1);
                keySet.setUserBuffer(buffLen, true);
                dataSet = new MultipleNIODataEntry();
                buffer2 = ByteBuffer.allocateDirect(buffLen);
                dataSet.setDataNIO(buffer2);
                dataSet.setUserBuffer(buffLen, true);
                while (cursor.getNext(keySet, dataSet, null) == 
                    OperationStatus.SUCCESS)
                    while(keySet.next(key) && dataSet.next(data))
                        count++;
            }
            cursor.close();
            txn.commit();
            return count;
        } catch (DatabaseException e) {
            cursor.close();
            txn.abort();
            throw e;
        }
    }

    /* Perform bulk delete in secondary db */
    public int bulkSecondaryDelete(byte value, boolean deletePair)
        throws DatabaseException, IOException {
        DatabaseEntry key = new DatabaseEntry();
        byte[] valueBytes = new byte[1];
        byte[] tstrBytes = DataVal.tstring.getBytes();
        int i = 0;
        int count = 0;

        Transaction txn = env.beginTransaction(null, null);

        try {
            /* Prepare the buffer for a set of keys */
            MultipleDataEntry keySet = new MultipleDataEntry();
            keySet.setData(new byte[buffLen]);
            keySet.setUserBuffer(buffLen, true);

            /* Delete the given key and all keys prior to it */
            do {
                valueBytes[0] = tstrBytes[i];
                key.setData(valueBytes);
                keySet.append(key);
            } while (value != tstrBytes[i++]);
            if (sdb.deleteMultiple(txn, keySet) != OperationStatus.SUCCESS)
                count = 0;
            else
                count = this.num / DataVal.tstring.length() * i;
            if (i < this.num % DataVal.tstring.length())
                count += i;

            txn.commit();
            return count;
        } catch (DatabaseException e) {
            txn.abort();
            throw e;
        }
    }

    /* Perform bulk update in primary db */
    public void bulkUpdate() throws DatabaseException, IOException {
        DatabaseEntry key, data;
        MultipleDataEntry keySet, dataSet;
        MultipleKeyDataEntry pairSet;
        DataVal dataVal;
        int i, j;

        Transaction txn = env.beginTransaction(null, null);
        key = new DatabaseEntry();
        data = new DatabaseEntry();

        try {
            if (this.dups == 0) {
                /* 
                 * Bulk insert non-duplicate key/data pairs. Those pairs
                 * are filled into one single buffer.
                 */ 
                pairSet = new MultipleKeyDataEntry();
                pairSet.setData(new byte[buffLen * 2]);
                pairSet.setUserBuffer(buffLen * 2, true);
                for (i = 0; i < this.num; i++) {
                    dataVal = new DataVal(i);
                    key.setData(intToByte(i));
                    data.setData(dataVal.getBytes());
                    pairSet.append(key, data);
                }
                pdb.putMultipleKey(txn, pairSet, true);
            } else {
                /* Bulk insert duplicate key/data pairs. All keys are  
                 * filled into a buffer, and their data is filled into 
                 * another buffer.
                 */ 
                keySet = new MultipleDataEntry();
                keySet.setData(new byte[buffLen]);
                keySet.setUserBuffer(buffLen, true);
                dataSet = new MultipleDataEntry();
                dataSet.setData(new byte[buffLen]);
                dataSet.setUserBuffer(buffLen, true);

                for (i = 0; i < this.num; i++) {
                    j = 0;
                    dataVal = new DataVal(i);
                    do {
                        key.setData(intToByte(i));
                        keySet.append(key);
                        dataVal.setDataId(j);
                        data.setData(dataVal.getBytes());
                        dataSet.append(data);
                    } while(++j < this.dups);
                }
                pdb.putMultiple(txn, keySet, dataSet, true);
            }

            txn.commit();
        }catch (DatabaseException e1) {
            txn.abort();
            throw e1;
        }catch (IOException e2) {
            txn.abort();
            throw e2;
        }
    }

    /* Clean the home directory */
    public void cleanHome(boolean clean) {
        File file = new File(home);
        /* 
         * If the home directory doesn't exist, create the directory. 
         * Unless, if it is requested to be cleaned, delete all and 
         * create a new one.
         */ 
        if (!file.exists())
            file.mkdir();
        else if (clean == true) {
            File[] files = file.listFiles();
            for(int i=0; i< files.length; i++) {
                files[i].delete();
            }
            file.delete();
            file.mkdir();
        }
    }

    /* Close database(s) and environment */
    public void closeDbs() {
        try {
            if (this.secondary)
                sdb.close();
            pdb.close();
            env.close();
        } catch (DatabaseException e) {
            System.exit(1);
        }
    }

    /* Print statistics in bulk operations */
    public void printBulkOptStat(
        Operations opt, int count, long startTime, long endTime) {
        System.out.print("[STAT] "); 
        if (opt == Operations.BULK_DELETE && !secondary)
            System.out.print("Bulk delete ");
        else if (opt == Operations.BULK_DELETE && secondary)
            System.out.print("Bulk secondary delete ");
        else if (opt == Operations.BULK_READ)
            System.out.print("Bulk read ");
        else
            System.out.print("Bulk update ");
            
        System.out.print(count + " records");
        System.out.print(" in " + (endTime - startTime) + " ms");
        System.out.println(", that is " + 
            (int)((double)1000 / (endTime - startTime) * (double)count) +
            " records/second");
    }

    /* Initialize environment and database (s) */
    public void initDbs() throws DatabaseException, FileNotFoundException {
        /* Open transactional environment */
        EnvironmentConfig envConfig = new EnvironmentConfig();
        envConfig.setAllowCreate(true);
        envConfig.setInitializeCache(true);
        envConfig.setInitializeLocking(true);
        envConfig.setInitializeLogging(true);
        envConfig.setTransactional(true);
        File envFile = new File(home);
        try {
            env = new Environment(envFile, envConfig);
        } catch (Exception e) {
            System.exit(1);
        }

        Transaction txn = env.beginTransaction(null, null);

        try {
            /* Open primary db in transaction */
            DatabaseConfig dbConfig = new DatabaseConfig();
            dbConfig.setTransactional(true);
            dbConfig.setAllowCreate(true);
            dbConfig.setCacheSize(this.cachesize);
            dbConfig.setPageSize(this.pagesize);
            dbConfig.setType(DatabaseType.BTREE);
            if (this.dups != 0)
                dbConfig.setUnsortedDuplicates(true);
            pdb = env.openDatabase(txn, dbFileName, pDbName, dbConfig);

            /* Open secondary db in transaction */
            if (this.secondary) {
                SecondaryConfig sdbConfig = new SecondaryConfig();
                sdbConfig.setTransactional(true);
                sdbConfig.setAllowCreate(true);
                sdbConfig.setCacheSize(this.cachesize);
                sdbConfig.setPageSize(this.pagesize);
                sdbConfig.setType(DatabaseType.BTREE);
                sdbConfig.setKeyCreator(new FirstStrKeyCreator());
                sdbConfig.setSortedDuplicates(true);
                sdb = env.openSecondaryDatabase(
                    txn, dbFileName, sDbName, pdb, sdbConfig);
            }

            txn.commit();
        } catch (DatabaseException e1) {
            txn.abort();
            throw e1;
        } catch (FileNotFoundException e2) {
            txn.abort();
            throw e2;
        }
    }

    /* Convert integer to byte array */
    public byte[] intToByte(int i) {
        byte[] dword = new byte[4];
        dword[0] = (byte) (i & 0x00FF);
        dword[1] = (byte) ((i >> 8) & 0x000000FF);
        dword[2] = (byte) ((i >> 16) & 0x000000FF);
        dword[3] = (byte) ((i >> 24) & 0x000000FF);
        return dword;
    }

    /* Usage of BulkExample */
    private static void usage() {
        System.out.println("Usage: BulkExample \n" +
            " -c cachesize [1000 * pagesize] \n" + 
            " -d number of duplicates [none] \n" + 
            " -n number of keys [1000000] \n" + 
            " -p pagesize [65536] \n" +
            " -D perform bulk delete \n" +
            " -R perform bulk read \n" + 
            " -S perform bulk read in secondary database \n" + 
            " -U perform bulk update \n");
        System.exit(1);
    }

    class DataVal {
        /* String template */
        public static final String tstring = "0123456789abcde";

        private String dataString;
        private int dataId;

        /* 
         * Construct DataVal with given kid and id. The dataString is the
         * rotated tstring from the kid position. The dataId is the id.
         */
        public DataVal(int kid) {
            int rp = kid % tstring.length();
            if (rp == 0)
                this.dataString = tstring;
            else
                this.dataString = tstring.substring(rp) +
                    tstring.substring(0, rp - 1);

            this.dataId = 0;
        }

        /* Parse the object including dataId and dataString to byte array. */
        public byte[] getBytes() throws IOException {
            int i;

            byte[] strBytes = dataString.getBytes();
            byte[] intBytes = intToByte(dataId);
            byte[] bytes = new byte[4 + strBytes.length];
            for (i = 0; i < 4 + strBytes.length; i++) {
                if (i < 4)
                    bytes[i] = intBytes[i];
                else
                    bytes[i] = strBytes[i - 4];
            }

            return bytes;
        }

        public int getDataId() {
            return dataId;
        }
            
        public void setDataId(int id) {
            dataId = id;
        }

        public String getDataString() {
            return dataString;
        }
    }

    /* Secondary key creator */
    class FirstStrKeyCreator implements SecondaryKeyCreator {
        public boolean createSecondaryKey(SecondaryDatabase secondary,
                DatabaseEntry key, DatabaseEntry data, DatabaseEntry result)
                throws DatabaseException {
            /*
             * Create the secondary key. It would be the first character in
             * dataString of DataVal, that is the 5th byte in data. 
             */
            byte[] firstStr = new byte[1];
            firstStr[0] = (data.getData())[4];
            result.setData(firstStr);
            result.setSize(1);
            return true;
        }
    }
}
