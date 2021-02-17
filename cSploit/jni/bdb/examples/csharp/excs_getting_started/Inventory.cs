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
    [Serializable]
    public class Inventory {
        private string category;
        private string itemname;
        private float price;
        private int quantity;
        private string sku;
        private string vendor;

        /* Declare a Category property of type string. */
        public string Category {
            get { return category; }
            set { category = value; }
        }

        /* Declare an Itemname property of type string. */
        public string Itemname {
            get { return itemname; }
            set { itemname = value; }
        }

        /* Declare a Price property of type string. */
        public float Price {
            get { return price; }
            set { price = value; }
        }

        /* Declare a Quantity property of type string. */
        public int Quantity {
           get { return quantity; }
           set { quantity = value; }
        }

        /* Declare a Sku property of type string. */
        public string Sku {
            get { return sku; }
            set { sku = value; }
        }

        /* Declare a Vendor property of type string. */
        public string Vendor {
            get { return vendor; }
            set { vendor = value; }
        }

        /* Default constructor. */
        public Inventory() {
            itemname = System.String.Empty;
            category = System.String.Empty;
            price = 0.0F;
            quantity = 0;
            sku = System.String.Empty;
            vendor = System.String.Empty;
        }

        /* Constructor for use with data returned from a BDB get. */
        public Inventory (byte[] buffer) {
            /* Fill in the fields from the buffer. */
            BinaryFormatter formatter = new BinaryFormatter();
            MemoryStream memStream = new MemoryStream(buffer);
            Inventory tmp = (Inventory) formatter.Deserialize(memStream);

            this.itemname = tmp.itemname;
            this.sku = tmp.sku;
            this.price = tmp.price;
            this.quantity = tmp.quantity;
            this.category = tmp.category;
            this.vendor = tmp.vendor;
            memStream.Close();
        }

        /* 
         * Marshall class data members into a single contiguous memory 
         * location for the purpose of storing the data in a database.
         */
        public byte[] getBytes () { 
            BinaryFormatter formatter = new BinaryFormatter();
            MemoryStream memStream = new MemoryStream();
            formatter.Serialize(memStream, this);
            byte [] bytes = memStream.GetBuffer();
            memStream.Close();
            return bytes;
        }
    }
}
