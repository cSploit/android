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
    /// Content used for the key in a Heap database record. Berkeley DB creates
    /// this value for you when you create a record in a Heap database. You
    /// should never create this structure yourself; Berkeley DB must create it
    /// for you.
    ///
    /// This structure is returned in the key DatabaseEntry parameter of the
    /// method that you use to add a record to the Heap database. 
    /// </summary>
    public class HeapRecordId {
        private uint pgno;
        private ushort indx;

        /// <summary>
        /// Construct a new record id, given a page number and index.
        /// </summary>
        /// <param name="pgno">The database page number where the record is
        /// stored. </param>
        /// <param name="indx">Index in the offset table where the record can 
        /// be found.</param>
        public HeapRecordId(uint pgno, ushort indx) {
            this.pgno = pgno;
            this.indx = indx;
        }

        /// <summary>
        /// The database page number where the record is stored. 
        /// </summary>
        public uint PageNumber {
            get { return pgno; }
            set { pgno = value; }
        }
        /// <summary>
        /// Index in the offset table where the record can be found.
        /// </summary>
        public ushort Index {
            get { return indx; }
            set { indx = value; }
        }

        /// <summary>
        /// Construct a HeapRecordId from a byte array, typically
        /// <see cref="DatabaseEntry.Data"/>.
        /// </summary>
        /// <param name="array">The array representing the record id</param>
        /// <returns>A new HeapRecordId</returns>
        public static HeapRecordId fromArray(byte[] array) {
            return fromArray(array, ByteOrder.MACHINE);
        }
        /// <summary>
        /// Construct a HeapRecordId from a byte array, typically
        /// <see cref="DatabaseEntry.Data"/>.
        /// </summary>
        /// <param name="array">The array representing the record id</param>
        /// <param name="order">The byte order of the data in
        /// <paramref name="array"/></param>
        /// <returns>A new HeapRecordId</returns>
        public static HeapRecordId fromArray(byte[] array, ByteOrder order) {
            if (array.Length != 6)
                throw new ArgumentException("Input array too small.");

            /*
             * If necessary, reverse the bytes in the array.  An RID is a
             * u_int32_t followed by a u_int16_t.  Note that we never convert if
             * the user gave us ByteOrder.MACHINE.
             */
            if ((BitConverter.IsLittleEndian &&
                order == ByteOrder.BIG_ENDIAN) ||
                (!BitConverter.IsLittleEndian &&
                order == ByteOrder.LITTLE_ENDIAN)) {
                Array.Reverse(array, 0, 4);
                Array.Reverse(array, 4, 2);
            }
            uint pgno = BitConverter.ToUInt32(array, 0);
            ushort indx = BitConverter.ToUInt16(array, 4);

            return new HeapRecordId(pgno, indx);
        }

        /// <summary>
        /// Return a byte array representing this record id.
        /// </summary>
        /// <returns>A byte array representing this record id.</returns>
        public byte[] toArray() {
            return toArray(ByteOrder.MACHINE);
        }
        /// <summary>
        /// Return a byte array representing this record id.
        /// </summary>
        /// <param name="order">The byte order to use when constructing the 
        /// array.</param>
        /// <returns>A byte array representing this record id.</returns>
        public byte[] toArray(ByteOrder order) {
            byte[] ret = new byte[6];
            byte[] pgnoArray = BitConverter.GetBytes(pgno);
            byte[] indxArray = BitConverter.GetBytes(indx);
            /*
             * If necessary, reverse the bytes in the array.  An RID is a
             * u_int32_t followed by a u_int16_t.  Note that we never convert if
             * the user gave us ByteOrder.MACHINE.
             */
            if ((BitConverter.IsLittleEndian &&
                order == ByteOrder.BIG_ENDIAN) ||
                (!BitConverter.IsLittleEndian &&
                order == ByteOrder.LITTLE_ENDIAN)) {
                Array.Reverse(pgnoArray);
                Array.Reverse(indxArray);
            }
            Buffer.BlockCopy(pgnoArray, 0, ret, 0, pgnoArray.Length);
            Buffer.BlockCopy(indxArray, 0, ret, 4, indxArray.Length);
            
            return ret;
        }
    }
}
