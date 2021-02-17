/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2011, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */
using System;
using System.Collections.Generic;
using System.Text;

namespace BerkeleyDB {
    /// <summary>
    /// Statistical information about a QueueDatabase
    /// </summary>
    public class HeapStats {
        private Internal.HeapStatStruct st;
        internal HeapStats(Internal.HeapStatStruct stats) {
            st = stats;
        }

        /// <summary>
        /// Magic number that identifies the file as a Heap file. 
        /// </summary>
        public uint MagicNumber { get { return st.heap_magic; } }
        /// <summary>
        /// Reports internal flags. For internal use only. 
        /// </summary>
        public uint MetadataFlags { get { return st.heap_metaflags; } }
        /// <summary>
        /// Number of blob records.
        /// </summary>
        public uint nBlobRecords { get { return st.heap_nblobs; } }
        /// <summary>
        /// The number of pages in the database.
        /// </summary>
        public uint nPages { get { return st.heap_pagecnt; } }
        /// <summary>
        /// Reports the number of records in the Heap database.
        /// </summary>
        public uint nRecords { get { return st.heap_nrecs; } }
        /// <summary>
        /// The number of regions in the Heap database.
        /// </summary>
        public uint nRegions { get { return st.heap_nregions; } }
        /// <summary>
        /// The underlying database page (and bucket) size, in bytes.
        /// </summary>
        public uint PageSize { get { return st.heap_pagesize; } }
       /// <summary>
       /// The region size of the Heap database.
       /// </summary>
       public uint RegionSize { get { return st.heap_regionsize; } }
        /// <summary>
        /// The version of the Heap database. 
        /// </summary>
        public uint Version { get { return st.heap_version; } }

    }
}
