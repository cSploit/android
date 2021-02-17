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
    [Serializable]
    public class Vendor {
        public string City;             /* City */
        public string PhoneNumber;      /* Vendor phone number */
        public string Name;             /* Vendor name */
        public string SalesRep;         /* Name of sales rep */
        public string SalesRepPhone;    /* Sales rep's phone number */
        public string State;            /* Two-digit US state code */
        public string Street;           /* Street name and number */
        public string Zipcode;          /* US zipcode */

        /* 
         * Marshall class data members into a single
         * contiguous memory location for the purpose of
         * storing the data in a database.
         */
        public byte[] GetBytes () {
            MemoryStream memStream = new MemoryStream();
            BinaryFormatter formatter = new BinaryFormatter();
            formatter.Serialize(memStream, this);
            byte [] bytes = memStream.GetBuffer();
            memStream.Close();
            return bytes;
        }
    }
}
