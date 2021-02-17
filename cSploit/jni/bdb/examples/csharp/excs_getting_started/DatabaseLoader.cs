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
using System.Text;
using System.Runtime.Serialization.Formatters.Binary;
using BerkeleyDB; 

namespace excs_getting_started {
    public class DatabaseLoader {
        private MyDbs myDbs = null;

        public DatabaseLoader(MyDbs dbs) {
            myDbs = dbs;
        }

        public void LoadInventoryDB(string dataDir) {
            DatabaseEntry key, data;
            string inventory_text_file = dataDir + "\\" + "inventory.txt";

            if (!File.Exists(inventory_text_file)) {
                Console.WriteLine("{0} does not exist.", inventory_text_file);
                return;
            }
 
            using (StreamReader sr = File.OpenText(inventory_text_file)) {
                Inventory inventory = new Inventory();
                string input;

                /* Text file fields are delimited by #, just read them in. */
                while ((input=sr.ReadLine())!=null) {
                    char [] delimiterPound = {'#'};
                    string [] fields = input.Split(delimiterPound);
#if TEST_DEBUG
                    System.Console.WriteLine(input);
#endif
                    inventory.Itemname = fields[0];
                    inventory.Sku = fields[1];
                    inventory.Price = float.Parse(fields[2]);
                    inventory.Quantity = int.Parse(fields[3]);
                    inventory.Category = fields[4];
                    inventory.Vendor = fields[5];

                    /* Insert key/data pairs into database. */
                    key = new DatabaseEntry();
                    key.Data = System.Text.Encoding.ASCII.GetBytes(
                        inventory.Sku);

                    byte [] bytes = inventory.getBytes();
                    data = new DatabaseEntry(bytes);

                    try {
                        myDbs.InventoryDB.Put(key, data);
                    } catch(Exception e) {
                        Console.WriteLine("LoadInventoryDB Error.");
                        Console.WriteLine(e.Message);
                        throw e;
                    } 
                }
            }
        } 

        public void LoadVendorDB(string dataDir) {
            DatabaseEntry key;
            DatabaseEntry data;
            string vendor_text_file = dataDir + "\\" + "vendors.txt";

            if (!File.Exists(vendor_text_file)) {
                Console.WriteLine("{0} does not exist.", vendor_text_file);
                return;
            }

            using (StreamReader sr = File.OpenText(vendor_text_file)) {
                Vendor vendor = new Vendor();
                string input;

                /* Text file fields are delimited by #, just read them in. */
                while ((input = sr.ReadLine()) != null) {
                    char[] delimiterPound = { '#' };
                    string[] fields = input.Split(delimiterPound);
#if TEST_DEBUG
                    System.Console.WriteLine(input);
#endif

                    vendor.Name = fields[0];
                    vendor.Street = fields[1];
                    vendor.City = fields[2];
                    vendor.State = fields[3];
                    vendor.Zipcode = fields[4];
                    vendor.PhoneNumber = fields[5];
                    vendor.SalesRep = fields[6];
                    vendor.SalesRepPhone = fields[7];

                    /* Insert key/data pairs into database. */
                    key = new DatabaseEntry();
                    key.Data = System.Text.Encoding.ASCII.GetBytes(vendor.Name);

                    byte [] bytes = vendor.GetBytes();
                    data = new DatabaseEntry(bytes);

                    try {
                        this.myDbs.VendorDB.Put(key, data);
                    }
                    catch (Exception e) {
                        Console.WriteLine("LoadVendorDB Error.");
                        Console.WriteLine(e.Message);
                        throw e;
                    }
                }
            }
        }
    }
}
