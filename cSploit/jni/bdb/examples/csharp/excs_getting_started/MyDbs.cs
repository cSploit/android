/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */
using System;
using System.Collections.Generic;
using System.Collections;
using System.Diagnostics;
using System.IO;
using System.Runtime.Serialization.Formatters.Binary;
using System.Text;
using BerkeleyDB;

namespace excs_getting_started {
    public class MyDbs {
        private string vDbName;
        private string iDbName;
        private string itemSDbName;

        private BTreeDatabase ibtreeDB, vbtreeDB;
        private BTreeDatabaseConfig btreeConfig;
        private SecondaryBTreeDatabase itemSecbtreeDB;
        private SecondaryBTreeDatabaseConfig itemSecbtreeConfig;

        public MyDbs(string databaseHome) {
            vDbName = "vendordb.db";
            iDbName = "inventorydb.db";
            itemSDbName = "itemname.sdb";

            if (databaseHome != null) {
                vDbName = databaseHome + "\\" + vDbName;
                iDbName = databaseHome + "\\" + iDbName;
                itemSDbName = databaseHome + "\\" + itemSDbName;
            }

            btreeConfig = new BTreeDatabaseConfig();
            btreeConfig.Creation = CreatePolicy.IF_NEEDED;
            btreeConfig.CacheSize = new CacheInfo(0, 64 * 1024, 1);
            btreeConfig.ErrorPrefix = "excs_getting_started";
            btreeConfig.PageSize = 8 * 1024;
            
            /* Optionally remove existing database files. */
            try {
                RemoveDbFile(vDbName);
                RemoveDbFile(iDbName);
                RemoveDbFile(itemSDbName);
            } catch (Exception e) {
                Console.WriteLine("Error deleting db.");
                Console.WriteLine(e.Message);
                throw e;
            }

            /* Create and open the Inventory and Vendor database files. */
            try {
                vbtreeDB = BTreeDatabase.Open(vDbName, btreeConfig);
            } catch (Exception e) {
                Console.WriteLine("Error opening {0}.", vDbName);
                Console.WriteLine(e.Message);
                throw e;
            }

            try {
                ibtreeDB = BTreeDatabase.Open(iDbName, btreeConfig);
            } catch (Exception e) {
                Console.WriteLine("Error opening {0}.", iDbName);
                Console.WriteLine(e.Message);
                throw e;
            }

            /* 
             * Open a secondary btree database associated with the 
             * Inventory database. 
             */
            try {
                itemSecbtreeConfig = new SecondaryBTreeDatabaseConfig(
                    ibtreeDB, new SecondaryKeyGenDelegate(
                    CreateSecondaryKey));

                itemSecbtreeConfig.Creation = CreatePolicy.IF_NEEDED;
                itemSecbtreeConfig.Duplicates = DuplicatesPolicy.UNSORTED;
                itemSecbtreeDB = SecondaryBTreeDatabase.Open(
                    itemSDbName, itemSecbtreeConfig);
            } catch (Exception e) {
                Console.WriteLine("Error opening secondary {0}", itemSDbName);
                Console.WriteLine(e.Message);
                throw e;
            }
        }

        ~MyDbs() {
            try {
                if (itemSecbtreeDB != null) {
                    itemSecbtreeDB.Close();
                }
                if (ibtreeDB != null) {
                    ibtreeDB.Close();
                }
                if (vbtreeDB != null) {
                    vbtreeDB.Close();
                }
            } catch (Exception e) {
                Console.WriteLine("Error closing DB");
                Console.WriteLine(e.Message);
                throw e;
            }
        }

        /* Declare a VDbName property of type string. */
        public string VDbName {
            get { return vDbName; }
            set { vDbName = value; }
        }

        /* Declare an IDbName property of type string. */
        public string IDbName {
            get { return iDbName; }
            set { iDbName = value; }
        }

        /* Declare an ItemSDbName property of type string. */
        public string ItemSDbName {
            get { return itemSDbName; }
            set { itemSDbName = value; }
        }

        public BTreeDatabase VendorDB {
            get { return vbtreeDB; }
        }

        public BTreeDatabase InventoryDB {
            get { return ibtreeDB; }
        }

        public SecondaryBTreeDatabase ItemSecbtreeDB {
            get { return itemSecbtreeDB; }
        }

        /* 
         * Used to extract an inventory item's name from an
         * inventory database record. This function is used to create
         * keys for secondary database records. Note key is the primary
         * key, and data is the primary data, the return is the secondary key.
         */
        public DatabaseEntry CreateSecondaryKey(
            DatabaseEntry key, DatabaseEntry data) {

            DatabaseEntry SecKeyDbt = new DatabaseEntry();
            Inventory inventory = new Inventory(data.Data);

            SecKeyDbt.Data = System.Text.Encoding.ASCII.GetBytes(
                inventory.Itemname);
            return SecKeyDbt;
        }

        private void RemoveDbFile(string dbName) {
            string buff;

            if (File.Exists(dbName)) {
                while (true) {
                    Console.Write("{0} exists.  Delete it? (y/n)", dbName);
                    buff = Console.ReadLine().ToLower();
                    if (buff == "y" || buff == "n")
                        break;
                }
                if (buff == "y") {
                    try {
                        File.Delete(dbName);
                    } catch (Exception e) {
                        Console.WriteLine("Unable to delete {0}.", dbName);
                        Console.WriteLine(e.Message);
                        throw e;
                    }
                }
            }
        }
    }
}
