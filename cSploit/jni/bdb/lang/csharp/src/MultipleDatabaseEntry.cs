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
    /// A class providing access to multiple <see cref="DatabaseEntry"/>
    /// objects.
    /// </summary>
    public class MultipleDatabaseEntry : DatabaseEntry, 
        IEnumerable<DatabaseEntry> {
        private bool _recno = false;

        internal MultipleDatabaseEntry(DatabaseEntry dbt) {
            _data = dbt.UserData;
            ulen = dbt.ulen;
        }

        /// <summary>
        /// Construct a new key or data bulk buffer with a formatted byte array.
        /// </summary>
        /// <param name="data">The formatted byte array</param>
        /// <param name="recno">
        /// Whether it is buffer for keys in <see cref="RecnoDatabase"/> or 
        /// <see cref="QueueDatabase"/>
        /// </param>
        public MultipleDatabaseEntry(byte[] data, bool recno) {
            UserData = data;
            _data = UserData;
            _recno = recno;
            flags |= DbConstants.DB_DBT_BULK;
        }

        /// <summary>
        /// Construct a new key or data bulk buffer with a bunch of record 
        /// numbers, as keys in <see cref="RecnoDatabase"/> or
        /// <see cref="QueueDatabase"/>
        /// </summary>
        /// <param name="recnos">Enumerable record numbers</param>
        public MultipleDatabaseEntry(IEnumerable<uint> recnos) {
            List<byte> list = new List<byte>();
            byte[] arr;
            byte[] lenArr = BitConverter.GetBytes(4);
            byte tmp;
            int i, j;
            uint pos = 0;

            /* 
             * The first record number is written at the end of the array, 
             * preceded by its offset in the array and the length of the record
             * number (the length is always 4 bytes.)  Each entry in the array
             * (record number, offset & length) is 12 bytes.
             *
             * 0 is an invalid record number and is used to mark the end of the
             * array of record numbers.  Since records are written from the
             * back of the array, the first 4 bytes of the array are zeros.
             * -----------------------------------------------------
             * |0|lengthN|offsetN|recnoN|...|length1|offset1|recno1|
             * -----------------------------------------------------
             */
            foreach (uint recno in recnos) {
                arr = BitConverter.GetBytes(recno);
                for (i = 0; i < 4; i++)
                    list.Add(arr[i]);
                arr = BitConverter.GetBytes(pos);
                for (i = 0; i < 4; i++)
                    list.Add(arr[i]);
                for (i = 0; i < 4; i++)
                    list.Add(lenArr[i]);
                pos += 12;
            }
            arr = BitConverter.GetBytes(0);
            for (i = 0; i < 4; i++)
                list.Add(arr[i]);
            for (i = 0; i < list.Count / (2 * 4); i++) {
                for (j = 0; j < 4; j++) {
                    tmp = list[i * 4 + j];
                    list[i * 4 + j] = list[list.Count - 4 + j - i * 4];
                    list[list.Count - 4 + j - i * 4] = tmp;
                }
            }
            UserData = list.ToArray();
            _data = UserData;
            _recno = true;
            flags |= DbConstants.DB_DBT_BULK;
        }

        /// <summary>
        /// Construct a new key or data bulk buffer with a bunch of 
        /// <see cref="DatabaseEntry"/> referring to <paramref name="recno"/>.
        /// </summary>
        /// <param name="data">Enumerable data</param>
        /// <param name="recno">
        /// Whether it is buffer for keys in <see cref="RecnoDatabase"/>
        /// or <see cref="QueueDatabase"/>
        /// </param>
        public MultipleDatabaseEntry(
            IEnumerable<DatabaseEntry> data, bool recno) {
            List<byte> list = new List<byte>();
            List<uint> metadata = new List<uint>();
            byte[] arr;
            byte[] lenArr = BitConverter.GetBytes(4);
            int i, j;
            uint pos = 0;
            uint size;
            byte tmp;

            if (recno) {
                /*
                 * For recno DatabaseEntry, everything is stored in the header,
                 * as described in the comment above.  The layout, again, is:
                 * -----------------------------------------------------
                 * |0|lengthN|offsetN|recnoN|...|length1|offset1|recno1|
                 * -----------------------------------------------------
                 */
                foreach (DatabaseEntry dbtObj in data) {
                    arr = dbtObj.Data;
                    for (i = 0; i < 4; i++)
                        list.Add(arr[i]);
                    arr = BitConverter.GetBytes(pos);
                    for (i = 0; i < 4; i++)
                        list.Add(arr[i]);
                    for (i = 0; i < 4; i++)
                        list.Add(lenArr[i]);
                    pos += 12;
                }
                arr = BitConverter.GetBytes(0);
                for (i = 0; i < 4; i++)
                    list.Add(arr[i]);
                for (i = 0; i < list.Count / (2 * 4); i++) {
                    for (j = 0; j < 4; j++) {
                        tmp = list[i * 4 + j];
                        list[i * 4 + j] = list[list.Count - 4 + j - i * 4];
                        list[list.Count - 4 + j - i * 4] = tmp;
                    }
                }
            } else {
                /* 
                 * For non-recno DatabaseEntry, the data values are written
                 * sequentially to the start of the array.  Header information
                 * is written at the end of the array.  Header information is
                 * the offset in the array at which the data value starts and
                 * the length of the data (each header entry occupies 8 bytes.)
                 * The data values are separated from the header information by
                 * a value of -1.  The layout is:
                 * ---------------------------------------------------------
                 * |data1|...|dataN||-1|lengthN|offsetN|...|length1|offset1|
                 * ---------------------------------------------------------
                 */
                foreach (DatabaseEntry dbtObj in data) {
                    size = dbtObj.size;
                    arr = dbtObj.Data;
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
                    for (j = 0; j < 4; j++)
                        list.Add(arr[j]);
                }
            }
            UserData = list.ToArray();
            _data = UserData;
            _recno = recno;
            flags |= DbConstants.DB_DBT_BULK;
        }

        /// <summary>
        /// Return the number of records in the DBT.
        /// </summary>
        internal int nRecs {
            get {
                uint pos = ulen - 4;
                int ret = 0;
                if (_recno) {
                    int recno;
                    while ((recno = BitConverter.ToInt32(_data, (int)pos)) != 0) {
                        ret++;
                        pos -= 12;
                    }
                } else {
                    int off;
                    while ((off = BitConverter.ToInt32(_data, (int)pos)) >= 0) {
                        ret++;
                        pos -= 8;
                    }
                }
                return ret;
            }
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
        /// <see cref="DatabaseEntry"/> objects represented by the 
        /// <see cref="MultipleDatabaseEntry"/>.
        /// </summary>
        /// <returns>
        /// An enumerator for the <see cref="MultipleDatabaseEntry"/>
        /// </returns>
        public IEnumerator<DatabaseEntry> GetEnumerator() {
            uint pos;
            if (_recno) {
                int recno;
                pos = ulen - 4;
                while ((recno = BitConverter.ToInt32(_data, (int)pos)) != 0) {
                    byte[] arr = new byte[4];
                    Array.Copy(_data, pos, arr, 0, 4);
                    pos -= 12;
                    yield return new DatabaseEntry(arr);
                }
            } else {
                pos = ulen - 4;
                int off = BitConverter.ToInt32(_data, (int)pos);
                for (int i = 0; off >= 0;
                    off = BitConverter.ToInt32(_data, (int)pos), i++) {
                    pos -= 4;
                    int sz = BitConverter.ToInt32(_data, (int)pos);
                    byte[] arr = new byte[sz];
                    Array.Copy(_data, off, arr, 0, sz);
                    pos -= 4;
                    yield return new DatabaseEntry(arr);
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