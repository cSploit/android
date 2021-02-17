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
    public class DatabaseReader {
        private MyDbs myDbs = null;

        public DatabaseReader(MyDbs dbs) {
            myDbs = dbs;
        }

        public void showItem(string locateItem) {
            SecondaryCursor secCursor;

            /* searchKey is the key to find in the secondary db. */
            DatabaseEntry searchKey = new DatabaseEntry();
            searchKey.Data = System.Text.Encoding.ASCII.GetBytes(locateItem);

            /* Open a secondary cursor. */
            secCursor = this.myDbs.ItemSecbtreeDB.SecondaryCursor();

            /* 
             * Search for the secondary. The primary key/data pair 
             * associated with the given secondary key is stored in Current.
             * In the presence of duplicate key values, the first data
             * item in the set of duplicates is stored in Current.
             */
            if (secCursor.Move(searchKey, true)) {
                Inventory theInventory = new Inventory(
                    secCursor.Current.Value.Value.Data);

                /* Display the record. */
                displayInventory(theInventory);

                /* Get each duplicate. */
                while (secCursor.MoveNextDuplicate()) {
                    theInventory = new Inventory(
                        secCursor.Current.Value.Value.Data);
                    displayInventory(theInventory);
                }
            } else { 
                Console.WriteLine("Could not find itemname {0} ", locateItem);
                Console.WriteLine();
                Console.WriteLine();
                return;
            }

            /* Close the cursor and the duplicate cursor. */
            secCursor.Close();
        }

        public void showAllInventory() {
            BTreeCursor cursor = myDbs.InventoryDB.Cursor();
            Inventory theInventory;

            while (cursor.MoveNext()) {
                theInventory = new Inventory(cursor.Current.Value.Data);
                displayInventory(theInventory);
            }
            cursor.Close();
        }

        public void displayInventory(Inventory theInventory) {
            Console.WriteLine("Item: {0} ",theInventory.Itemname);
            Console.WriteLine("SKU: {0} ", theInventory.Sku);
	    Console.WriteLine("Price per unit: {0} ", theInventory.Price);
            Console.WriteLine("Quantity: {0} ", theInventory.Quantity);
            Console.WriteLine("Category: {0} ", theInventory.Category);
            Console.WriteLine("Contact: {0} ", theInventory.Vendor);

            /* Display the associated Vendor information. */
            displayVendor(theInventory);
        }

        private void displayVendor(Inventory theInventory) {
            BinaryFormatter formatter = new BinaryFormatter();
            DatabaseEntry foundVendor = new DatabaseEntry();
            MemoryStream memStream;
            Vendor theVendor;

            foundVendor.Data = System.Text.Encoding.ASCII.GetBytes(
                theInventory.Vendor);
            try {
                KeyValuePair<DatabaseEntry, DatabaseEntry> pair = 
                    new KeyValuePair<DatabaseEntry, DatabaseEntry>();
                string vendorData;

                pair = this.myDbs.VendorDB.Get(foundVendor);
                vendorData = System.Text.ASCIIEncoding.ASCII.GetString(
                    pair.Value.Data);

                memStream = new MemoryStream(pair.Value.Data.Length);
                memStream.Write(pair.Value.Data, 0, pair.Value.Data.Length);

                /* Read from the begining of the stream. */
                memStream.Seek(0, SeekOrigin.Begin);
                theVendor = (Vendor)formatter.Deserialize(memStream);

                memStream.Close();

                System.Console.WriteLine("Vendor: {0}", theVendor.Name);
                System.Console.WriteLine("Vendor Phone: {0}", 
                    theVendor.PhoneNumber);
                System.Console.Write("Vendor Address: {0}, ",theVendor.Street); 
                System.Console.Write("{0} ",theVendor.City); 
                System.Console.Write("{0} ",theVendor.State); 
                System.Console.WriteLine("{0} ",theVendor.Zipcode); 
                System.Console.WriteLine("Vendor Rep: {0}: {1}", 
                    theVendor.SalesRep, theVendor.SalesRepPhone);
                System.Console.WriteLine();
                System.Console.WriteLine();
            }
            catch (Exception e) {
                Console.WriteLine("displayVendor Error.");
                Console.WriteLine(e.Message);
                throw e;
            }
        }
    }
}
