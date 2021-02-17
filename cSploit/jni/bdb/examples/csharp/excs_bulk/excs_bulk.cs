/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */
using System;
using System.Collections.Generic;
using System.IO;
using System.Runtime.Serialization.Formatters.Binary;
using System.Text;
using BerkeleyDB;

namespace excs_bulk {
    public class excs_bulk {
        private const String dbFileName = "BulkExample.db";
        private const String pDbName = "primary";
        private const String sDbName = "secondary";
        private const String home = "BulkExample";
        private const int buffLen = 1024 * 1024;

        private BTreeDatabase pdb;
        private DatabaseEnvironment env;
        private Operations opt = Operations.BULK_READ;
        private SecondaryBTreeDatabase sdb;
        private uint cachesize = 0, pagesize = 0;
        private int num = 1000000, dups = 0;
        private bool secondary = false;

        /* Bulk methods demonstrated in the example */
        public enum Operations {
            BULK_DELETE,
            BULK_READ,
            BULK_UPDATE
        }

        public static void Main(String[] args) {
            DateTime startTime, endTime;
            int count;

            try {
                String pwd = Environment.CurrentDirectory;
                pwd = Path.Combine(pwd, "..");
                pwd = Path.Combine(pwd, "..");
                if (IntPtr.Size == 4)
                    pwd = Path.Combine(pwd, "Win32");
                else
                    pwd = Path.Combine(pwd, "x64");
#if DEBUG
                pwd = Path.Combine(pwd, "Debug");
#else
                pwd = Path.Combine(pwd, "Release");
#endif
                pwd += ";" + Environment.GetEnvironmentVariable("PATH");
                Environment.SetEnvironmentVariable("PATH", pwd);
            } catch (Exception e) {
                Console.WriteLine(
                    "Unable to set the PATH environment variable.");
                Console.WriteLine(e.Message);
                return;
            }

            excs_bulk app = new excs_bulk();

            /* Parse argument */
            for (int i = 0; i < args.Length; i++) {
                if (args[i].CompareTo("-c") == 0) {
                    if (i == args.Length - 1)
                        Usage();
                    i++;
                    app.cachesize = uint.Parse(args[i]);
                    Console.WriteLine("[CONFIG] cachesize {0}",
                        app.cachesize);
                } else if (args[i].CompareTo("-d") == 0) {
                    if (i == args.Length - 1)
                        Usage();
                    i++;
                    app.dups = int.Parse(args[i]);
                    Console.WriteLine("[CONFIG] number of duplicates {0}",
                        app.dups);
                } else if (args[i].CompareTo("-n") == 0) {
                    if (i == args.Length - 1)
                        Usage();
                    i++;
                    app.num = int.Parse(args[i]);
                    Console.WriteLine("[CONFIG] number of keys {0}", app.num);
                } else if (args[i].CompareTo("-p") == 0) {
                    if (i == args.Length - 1)
                        Usage();
                    i++;
                    app.pagesize = uint.Parse(args[i]);
                    Console.WriteLine("[CONFIG] pagesize {0}", app.pagesize);
                } else if (args[i].CompareTo("-D") == 0) {
                    app.opt = Operations.BULK_DELETE;
                } else if (args[i].CompareTo("-R") == 0) {
                    app.opt = Operations.BULK_READ;
                } else if (args[i].CompareTo("-S") == 0) {
                    app.secondary = true;
                } else if (args[i].CompareTo("-U") == 0) {
                    app.opt = Operations.BULK_UPDATE;
                } else
                    Usage();
            }

            /* Open environment and database(s) */
            try {
                /* 
                 * If perform bulk update or delete, clean environment 
                 * home 
                 */
                app.CleanHome(app.opt != Operations.BULK_READ);
                app.InitDbs();
            } catch (Exception e) {
                Console.WriteLine(e.StackTrace);
                app.CloseDbs();
            }

            try {
                /* Perform bulk read from existing primary db */
                if (app.opt == Operations.BULK_READ) {
                    startTime = DateTime.Now;
                    count = app.BulkRead();
                    endTime = DateTime.Now;
                    app.PrintBulkOptStat(
                        Operations.BULK_READ, count, startTime, endTime);
                } else {
                    /* Perform bulk update to populate the db */
                    startTime = DateTime.Now;
                    app.BulkUpdate();
                    endTime = DateTime.Now;
                    count = (app.dups == 0) ? app.num : app.num * app.dups;
                    app.PrintBulkOptStat(
                        Operations.BULK_UPDATE, count, startTime, endTime);

                    /* Perform bulk delete in primary db or secondary db */
                    if (app.opt == Operations.BULK_DELETE) {
                        startTime = DateTime.Now;
                        if (app.secondary == false) {
                            /*
                             * Delete from the first key to a random one in
                             * primary db.
                             */
                            count = new Random().Next(app.num);
                            app.BulkDelete(count);
                        } else {
                            /*
                             * Delete a set of keys or specified key/data 
                             * pairs in secondary db. If delete a set of keys,
                             * delete from the first key to a random one in
                             * secondary db. If delete a set of specified 
                             * key/data pairs, delete a random key and all
                             * its duplicate records.
                             */
                            int deletePair = new Random().Next(1);
                            count = new Random().Next(DataVal.tstring.Length);
                            count = app.BulkSecondaryDelete(
                                ASCIIEncoding.ASCII.GetBytes(
                                DataVal.tstring)[count],
                                deletePair);
                        }
                        endTime = DateTime.Now;

                        app.PrintBulkOptStat(Operations.BULK_DELETE,
                            count, startTime, endTime);
                    }
                }
            } catch (DatabaseException e) {
                Console.WriteLine(e.StackTrace);
            } catch (IOException e) {
                Console.WriteLine(e.StackTrace);
            } finally {
                /* Close dbs */
                app.CloseDbs();
            }
        }

        /* Perform bulk delete in primary db */
        public void BulkDelete(int value) {
            Transaction txn = env.BeginTransaction();

            try {
                if (dups == 0) {
                    /* Delete a set of key/data pairs */
                    List<KeyValuePair<DatabaseEntry, DatabaseEntry>> pList =
                    new List<KeyValuePair<DatabaseEntry, DatabaseEntry>>();
                    for (int i = 0; i < value; i++)
                        pList.Add(
                            new KeyValuePair<DatabaseEntry, DatabaseEntry>(
                            new DatabaseEntry(BitConverter.GetBytes(i)),
                            new DatabaseEntry(getBytes(new DataVal(i)))));

                    MultipleKeyDatabaseEntry pairs =
                        new MultipleKeyDatabaseEntry(pList, false);
                    pdb.Delete(pairs, txn);
                } else {
                    /* Delete a set of keys */
                    List<DatabaseEntry> kList = new List<DatabaseEntry>();
                    for (int i = 0; i < value; i++)
                        kList.Add(new DatabaseEntry(
                            BitConverter.GetBytes(i)));
                    MultipleDatabaseEntry keySet =
                        new MultipleDatabaseEntry(kList, false);
                    pdb.Delete(keySet, txn);
                }
                txn.Commit();
            } catch (DatabaseException e) {
                txn.Abort();
                throw e;
            }
        }

        /* Perform bulk delete in secondary db */
        public int BulkSecondaryDelete(byte value, int deletePair) {
            DatabaseEntry key, data;
            byte[] tstrBytes = ASCIIEncoding.ASCII.GetBytes(DataVal.tstring);
            int i = 0;
            int count = 0;

            Transaction txn = env.BeginTransaction();

            try {
                if (deletePair == 1) {
                    /* 
                     * Delete the given key and all keys prior to it, 
                     * together with their duplicate data.
                     */
                    List<KeyValuePair<DatabaseEntry, DatabaseEntry>> pList =
                        new List<KeyValuePair<DatabaseEntry, DatabaseEntry>>();
                    do {
                        int j = 0;
                        int idx = 0;
                        key = new DatabaseEntry();
                        key.Data = new byte[1];
                        key.Data[0] = tstrBytes[i];

                        while (j < this.num / DataVal.tstring.Length) {
                            idx = j * DataVal.tstring.Length + i;
                            data = new DatabaseEntry(
                                getBytes(new DataVal(idx)));
                            pList.Add(new KeyValuePair<
                                DatabaseEntry, DatabaseEntry>(key, data));
                            j++;
                        }
                        if (i < this.num % DataVal.tstring.Length) {
                            idx = j * DataVal.tstring.Length + i;
                            data = new DatabaseEntry(
                                getBytes(new DataVal(idx)));
                            pList.Add(new KeyValuePair<
                                DatabaseEntry, DatabaseEntry>(
                                key, data));
                            j++;
                        }
                        count += j;
                    } while (value != tstrBytes[i++]);
                    MultipleKeyDatabaseEntry pairs =
                        new MultipleKeyDatabaseEntry(pList, false);
                    sdb.Delete(pairs, txn);
                } else {
                    List<DatabaseEntry> kList = new List<DatabaseEntry>();
                    /* Delete the given key and all keys prior to it */
                    do {
                        key = new DatabaseEntry();
                        key.Data = new byte[1];
                        key.Data[0] = tstrBytes[i];
                        kList.Add(key);
                    } while (value != tstrBytes[i++]);
                    MultipleDatabaseEntry keySet =
                        new MultipleDatabaseEntry(kList, false);
                    sdb.Delete(keySet, txn);
                    count = this.num / DataVal.tstring.Length * i;
                    if (i < this.num % DataVal.tstring.Length)
                        count += i;
                }
                txn.Commit();
                return count;
            } catch (DatabaseException e) {
                txn.Abort();
                throw e;
            }
        }

        /* Perform bulk read in primary db */
        public int BulkRead() {
            BTreeCursor cursor;
            int count = 0;

            Transaction txn = env.BeginTransaction();

            try {
                cursor = pdb.Cursor(txn);
            } catch (DatabaseException e) {
                txn.Abort();
                throw e;
            }

            try {
                if (dups == 0) {
                    /* Get all records in a single key/data buffer */
                    while (cursor.MoveNextMultipleKey()) {
                        MultipleKeyDatabaseEntry pairs =
                            cursor.CurrentMultipleKey;
                        foreach (KeyValuePair<DatabaseEntry, DatabaseEntry> p
                            in pairs) {
                            count++;
                        }
                    }
                } else {
                    /*
                     * Get all key/data pairs in two buffers, one for all
                     * keys, the other for all data.
                     */
                    while (cursor.MoveNextMultiple()) {
                        KeyValuePair<DatabaseEntry, MultipleDatabaseEntry>
                            pairs = cursor.CurrentMultiple;
                        foreach (DatabaseEntry d in pairs.Value) {
                            count++;
                        }
                    }
                }
                cursor.Close();
                txn.Commit();
                return count;
            } catch (DatabaseException e) {
                cursor.Close();
                txn.Abort();
                throw e;
            }
        }


        /* Perform bulk update in primary db */
        public void BulkUpdate() {
            Transaction txn = env.BeginTransaction();

            try {
                if (this.dups == 0) {
                    /* 
                     * Bulk insert non-duplicate key/data pairs. Those pairs
                     * are filled into one single buffer.
                     */
                    List<KeyValuePair<DatabaseEntry, DatabaseEntry>> pList =
                        new List<KeyValuePair<DatabaseEntry, DatabaseEntry>>();
                    for (int i = 0; i < this.num; i++) {
                        DataVal dataVal = new DataVal(i);
                        pList.Add(
                            new KeyValuePair<DatabaseEntry, DatabaseEntry>(
                            new DatabaseEntry(BitConverter.GetBytes(i)),
                            new DatabaseEntry(getBytes(dataVal))));
                    }
                    pdb.Put(new MultipleKeyDatabaseEntry(pList, false), txn);
                } else {
                    /* Bulk insert duplicate key/data pairs. All keys are  
                     * filled into a buffer, and their data is filled into 
                     * another buffer.
                     */
                    List<DatabaseEntry> kList = new List<DatabaseEntry>();
                    List<DatabaseEntry> dList = new List<DatabaseEntry>();
                    for (int i = 0; i < this.num; i++) {
                        int j = 0;
                        DataVal dataVal = new DataVal(i);
                        do {
                            kList.Add(
                                new DatabaseEntry(BitConverter.GetBytes(i)));
                            dataVal.DataId = j;
                            dList.Add(new DatabaseEntry(getBytes(dataVal)));
                        } while (++j < this.dups);
                    }
                    pdb.Put(new MultipleDatabaseEntry(kList, false),
                        new MultipleDatabaseEntry(dList, false), txn);
                }

                txn.Commit();
            } catch (DatabaseException e1) {
                txn.Abort();
                throw e1;
            } catch (IOException e2) {
                txn.Abort();
                throw e2;
            }
        }

        /* Clean the home directory */
        public void CleanHome(bool clean) {
            try {
                if (!Directory.Exists(home)) {
                    Directory.CreateDirectory(home);
                } else if (clean == true) {
                    string[] files = Directory.GetFiles(home);
                    for (int i = 0; i < files.Length; i++)
                        File.Delete(files[i]);
                    Directory.Delete(home);
                    Directory.CreateDirectory(home);
                }
            } catch (Exception e) {
                Console.WriteLine(e.StackTrace);
                System.Environment.Exit(1);
            }
        }

        /* Close database(s) and environment */
        public void CloseDbs() {
            try {
                if (this.secondary && sdb != null)
                    sdb.Close();
                if (pdb != null)
                    pdb.Close();
                if (env != null)
                    env.Close();
            } catch (DatabaseException e) {
                Console.WriteLine(e.StackTrace);
                System.Environment.Exit(1);
            }
        }

        /* Print statistics in bulk operations */
        public void PrintBulkOptStat(
            Operations opt, int count, DateTime startTime, DateTime endTime) {
            Console.Write("[STAT] ");
            if (opt == Operations.BULK_DELETE && !secondary)
                Console.Write("Bulk delete ");
            else if (opt == Operations.BULK_DELETE && secondary)
                Console.Write("Bulk secondary delete ");
            else if (opt == Operations.BULK_READ)
                Console.Write("Bulk read ");
            else
                Console.Write("Bulk update ");

            Console.Write("{0} records", count);
            Console.Write(" in {0} ms",
                ((TimeSpan)(endTime - startTime)).Milliseconds);
            Console.WriteLine(", that is {0} records/second",
                (int)((double)1000 /
                ((TimeSpan)(endTime - startTime)).Milliseconds *
                (double)count));
        }

        /* Initialize environment and database (s) */
        public void InitDbs() {
            /* Open transactional environment */
            DatabaseEnvironmentConfig envConfig =
                new DatabaseEnvironmentConfig();
            envConfig.Create = true;
            envConfig.UseMPool = true;
            envConfig.UseLocking = true;
            envConfig.UseLogging = true;
            envConfig.UseTxns = true;
            envConfig.LockSystemCfg = new LockingConfig();
            envConfig.LockSystemCfg.MaxLocks =
                (this.dups == 0) ?
                (uint)this.num :
                (uint)(this.num * this.dups);
            envConfig.LockSystemCfg.MaxObjects =
                envConfig.LockSystemCfg.MaxLocks;
            if (this.cachesize != 0) {
                envConfig.MPoolSystemCfg = new MPoolConfig();
                envConfig.MPoolSystemCfg.CacheSize =
                    new CacheInfo(0, this.cachesize, 1);
            }

            try {
                env = DatabaseEnvironment.Open(home, envConfig);
            } catch (Exception e) {
                Console.WriteLine(e.StackTrace);
                System.Environment.Exit(1);
            }

            Transaction txn = env.BeginTransaction();

            try {
                /* Open primary db in transaction */
                BTreeDatabaseConfig dbConfig = new BTreeDatabaseConfig();
                dbConfig.Env = env;
                dbConfig.Creation = CreatePolicy.IF_NEEDED;
                if (this.pagesize != 0)
                    dbConfig.PageSize = this.pagesize;
                if (this.dups != 0)
                    dbConfig.Duplicates = DuplicatesPolicy.UNSORTED;
                pdb = BTreeDatabase.Open(dbFileName, pDbName, dbConfig, txn);

                /* Open secondary db in transaction */
                if (this.secondary) {
                    SecondaryBTreeDatabaseConfig sdbConfig =
                        new SecondaryBTreeDatabaseConfig(
                        pdb, new SecondaryKeyGenDelegate(SecondaryKeyGen));
                    sdbConfig.Creation = CreatePolicy.IF_NEEDED;
                    if (this.pagesize != 0)
                        sdbConfig.PageSize = this.pagesize;
                    sdbConfig.Duplicates = DuplicatesPolicy.SORTED;
                    sdbConfig.Env = env;
                    sdb = SecondaryBTreeDatabase.Open(
                        dbFileName, sDbName, sdbConfig, txn);
                }

                txn.Commit();
            } catch (DatabaseException e1) {
                txn.Abort();
                throw e1;
            } catch (FileNotFoundException e2) {
                txn.Abort();
                throw e2;
            }
        }

        /* Usage of BulkExample */
        private static void Usage() {
            Console.WriteLine("Usage: BulkExample \n" +
                    " -c cachesize [1000 * pagesize] \n" +
                    " -d number of duplicates [none] \n" +
                    " -e use environment and transaction \n" +
                    " -i number of read iterations [1000000] \n" +
                    " -n number of keys [1000000] \n" +
                    " -p pagesize [65536] \n" +
                    " -D perform bulk delete \n" +
                    " -I just initialize the environment \n" +
                    " -R perform bulk read \n" +
                    " -S perform bulk read in secondary database \n" +
                    " -U perform bulk update \n");
            System.Environment.Exit(1);
        }

        public DatabaseEntry SecondaryKeyGen(
                DatabaseEntry key, DatabaseEntry data) {
            byte[] firstStr = new byte[1];
            DataVal dataVal = getDataVal(data.Data);
            firstStr[0] = ASCIIEncoding.ASCII.GetBytes(dataVal.DataString)[0];
            return new DatabaseEntry(firstStr);
        }

        public byte[] getBytes(DataVal dataVal) {
            BinaryFormatter formatter = new BinaryFormatter();
            MemoryStream ms = new MemoryStream();
            formatter.Serialize(ms, dataVal);
            return ms.GetBuffer();
        }

        public DataVal getDataVal(byte[] bytes) {
            BinaryFormatter formatter = new BinaryFormatter();
            MemoryStream ms = new MemoryStream(bytes);
            return (DataVal)formatter.Deserialize(ms);
        }

        [Serializable]
        public class DataVal {
            /* String template */
            public static String tstring = "0123456789abcde";

            private String dataString;
            private int dataId;

            /* 
             * Construct DataVal with given kid and id. The dataString is the
             * rotated tstring from the kid position. The dataId is the id.
             */
            public DataVal(int kid) {
                int rp = kid % tstring.Length;
                if (rp == 0)
                    this.dataString = tstring;
                else
                    this.dataString = tstring.Substring(rp) +
                        tstring.Substring(0, rp - 1);

                this.dataId = 0;
            }

            public int DataId {
                get {
                    return dataId;
                }
                set {
                    dataId = value;
                }
            }

            public String DataString {
                get {
                    return dataString;
                }
            }
        }
    }
}
