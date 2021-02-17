/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */
using System;
using System.Collections;
using System.Collections.Generic;
using System.Text;
using BerkeleyDB.Internal;

namespace BerkeleyDB {
    /// <summary>
    /// A class providing access to multiple key/data pairs.
    /// </summary>
    public class MultipleKeyDatabaseEntry : DatabaseEntry,
        IEnumerable<KeyValuePair<DatabaseEntry, DatabaseEntry>> {
        private bool _recno = false;

        internal MultipleKeyDatabaseEntry(
            DatabaseEntry dbt, bool recno) {
            _data = dbt.UserData;
            ulen = dbt.ulen;
            _recno = recno;
        }

        /// <summary>
        /// Construct a new bulk buffer for key/data pairs with a formatted
        /// byte array.
        /// </summary>
        /// <param name="data">The formatted byte array</param>
        /// <param name="recno">
        /// Whether the bulk is for recno/queue database.
        /// </param>
        public MultipleKeyDatabaseEntry(byte[] data, bool recno) {
            UserData = data;
            _data = UserData;
            _recno = recno;
            flags |= DbConstants.DB_DBT_BULK;
        }

        /// <summary>
        /// Construct a new bulk buffer for key/data pairs.
        /// </summary>
        /// <param name="recno">
        /// Whether the bulk buffer is for queue/recno database
        /// </param>
        /// <param name="data">Enumerable key/data pairs</param>            
        public MultipleKeyDatabaseEntry(
            IEnumerable<KeyValuePair<DatabaseEntry, DatabaseEntry>> data,
            bool recno) {
            List<byte> list = new List<byte>();
            byte[] arr;
            int i;
            uint pos = 0;
            uint size;

            if (recno) {
                /*
                 * For recno database, data value is written to the beginning 
                 * of the array, but the record number key is stored in the 
                 * "header" section, along with the offset and length of the
                 * data value.  Offset and length are not stored for the 
                 * recno key, since they are always 4 bytes.  The list is 
                 * zero terminated (since -1 is a valid record number):
                 * -------------------------------------------------------------
                 * |d1|..|dN|0|lengthN|offsetN|recnoN|..|length1|offset1|recno1|
                 * -------------------------------------------------------------
                 */
                List<byte[]> metadata = new List<byte[]>();
                foreach (KeyValuePair<DatabaseEntry, DatabaseEntry> pair
                    in data) {
                    size = pair.Value.size;
                    arr = pair.Value.Data;
                    for (i = 0; i < size; i++)
                        list.Add(arr[i]);
                    metadata.Add(pair.Key.Data);
                    metadata.Add(BitConverter.GetBytes(pos));
                    metadata.Add(BitConverter.GetBytes(size));
                    pos += size;
                }
                for (i = 0; i < 4; i++)
                    list.Add((byte)0);
                for (i = metadata.Count; i > 0; i--) {
                    for (int j = 0; j < 4; j++)
                        list.Add(metadata[i - 1][j]);
                }
            } else {
                /*
                 * For non-recno database, the items are written to the
                 * array in key/data pairs, but use the same layout as in
                 * MultipleDatabaseEntry, with DBT.data values at the front
                 * of the array, and header information at the end of the array.
                 * --------------------------------------------------------
                 * |key1|data1|...|keyN|dataN|-1|lenDataN|offDataN| (linewrap)
                 * --------------------------------------------------------
                 * |lenKeyN|offKeyN|...|lenData1|offData1||lenKey1|offKey1|
                 * --------------------------------------------------------
                 */
                List<uint> metadata = new List<uint>();
                foreach (KeyValuePair<DatabaseEntry, DatabaseEntry> pair
                    in data) {
                    size = pair.Key.size;
                    arr = pair.Key.Data;
                    metadata.Add(pos);
                    metadata.Add(size);
                    for (i = 0; i < (int)size; i++)
                        list.Add(arr[i]);
                    pos += size;

                    size = pair.Value.size;
                    arr = pair.Value.Data;
                    metadata.Add(pos);
                    metadata.Add(size);
                    for (i = 0; i < (int)size; i++)
                        list.Add(arr[i]);
                    pos += size;
                }
                arr = BitConverter.GetBytes(-1);
                for (i = 0; i < 4; i++)
                    list.Add(arr[i]);
                for (i = metadata.Count; i > 0; i--) {
                    arr = BitConverter.GetBytes(metadata[i - 1]);
                    for (int j = 0; j < 4; j++)
                        list.Add(arr[j]);
                }
            }

            UserData = list.ToArray();
            _data = UserData;
            _recno = recno;
            flags |= DbConstants.DB_DBT_BULK;
        }

        /// <summary>
        /// Whether the bulk buffer is for recno or queue database.
        /// </summary>
        public bool Recno {
            get { return _recno; }
        }

        IEnumerator IEnumerable.GetEnumerator() {
            return GetEnumerator();
        }

        /// <summary>
        /// Return an enumerator which iterates over all
        /// <see cref="DatabaseEntry"/> pairs represented by the 
        /// <see cref="MultipleKeyDatabaseEntry"/>.
        /// </summary>
        /// <returns>
        /// An enumerator for the <see cref="MultipleDatabaseEntry"/>
        /// </returns>
        public IEnumerator<KeyValuePair<DatabaseEntry, DatabaseEntry>>
            GetEnumerator() {
            byte[] darr, karr;
            int off, sz;
            uint pos, recno;

            pos = ulen - 4;

            if (_recno) {
                recno = BitConverter.ToUInt32(_data, (int)pos);
                for (int i = 0; recno > 0;
                    recno = BitConverter.ToUInt32(_data, (int)pos), i++) {
                    pos -= 4;
                    off = BitConverter.ToInt32(_data, (int)pos);
                    pos -= 4;
                    sz = BitConverter.ToInt32(_data, (int)pos);
                    darr = new byte[sz];
                    Array.Copy(_data, off, darr, 0, sz);
                    pos -= 4;
                    yield return new KeyValuePair<DatabaseEntry, DatabaseEntry>(
                        new DatabaseEntry(BitConverter.GetBytes(recno)),
                        new DatabaseEntry(darr));
                }
            } else {
                off = BitConverter.ToInt32(_data, (int)pos);
                for (int i = 0; off >= 0;
                    off = BitConverter.ToInt32(_data, (int)pos), i++) {
                    pos -= 4;
                    sz = BitConverter.ToInt32(_data, (int)pos);
                    karr = new byte[sz];
                    Array.Copy(_data, off, karr, 0, sz);
                    pos -= 4;
                    off = BitConverter.ToInt32(_data, (int)pos);
                    pos -= 4;
                    sz = BitConverter.ToInt32(_data, (int)pos);
                    darr = new byte[sz];
                    Array.Copy(_data, off, darr, 0, sz);
                    pos -= 4;
                    yield return new KeyValuePair<DatabaseEntry, DatabaseEntry>(
                        new DatabaseEntry(karr), new DatabaseEntry(darr));
                }
            } 
        }

        // public byte[][] Data;
        /* No Public Constructor */
        //internal MultipleDatabaseEntry(DatabaseEntry dbt) {
        //    byte[] dat = dbt.UserData;
        //    List<byte[]> tmp = new List<byte[]>();
        //    uint pos = dbt.ulen - 4;
        //    int off = BitConverter.ToInt32(dat, (int)pos);
        //    for (int i = 0; off > 0; off = BitConverter.ToInt32(dat, (int)pos), i++) {
        //        pos -= 4;
        //        int sz = BitConverter.ToInt32(dat, (int)pos);
        //        tmp.Add(new byte[sz]);
        //        Array.Copy(dat, off, tmp[i], 0, sz);
        //        pos -= 4;
        //    }
        //    Data = tmp.ToArray();
        //}


    }
}
