/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2013, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */
using System;
using System.Collections;
using System.Collections.Generic;
using System.Text;
using BerkeleyDB.Internal;

namespace BerkeleyDB {
    /// <summary>
    /// A class representing a database stream,
    /// which allows streaming access to blobs.
    /// </summary>
    public class DatabaseStream : IDisposable{
        /// <summary>
        /// The underlying DB_STREAM handle
        /// </summary>
        internal DB_STREAM dbs;
        private bool isOpen;
        private DatabaseStreamConfig config;
        static internal DB_STREAM getDBSTREAM(DatabaseStream dbstream) {
            return dbstream == null ? null : dbstream.dbs;
        }

        internal DatabaseStream(DB_STREAM dbstream,
            DatabaseStreamConfig config) {
            this.dbs = dbstream;
            isOpen = true;
            this.config = config;
        }

        /// <summary>
        /// Return the database stream configuration.
        /// </summary>
        /// <returns>
        /// Return the database stream configuration.
        /// </returns>
        public DatabaseStreamConfig GetConfig {
            get {
                return config;
            }
        }

        /// <summary>
        /// <para>
        /// Discard the database stream.
        /// </para>
        /// <para>
        /// It is possible for the Close() method to throw a
        /// <see cref="DatabaseException"/> if there is any failure.
        /// </para>
        /// <para>
        /// After Close has been called, regardless of its result, the database
        /// stream handle cannot be used again.
        /// Always close the stream when you have finished accessing the BLOB.
        /// </para>
        /// </summary>
        /// <exception cref="DatabaseException"></exception>
        public void Close() {
            dbs.close(0);

            isOpen = false;
        }

        /// <summary>
        /// Release the resources held by this object, and close the database
        /// stream if it is still open.
        /// </summary>
        public void Dispose() {
            try {
                if (isOpen)
                    Close();
            } catch {
                /* 
                 * Errors here are likely because our cursor has been closed out
                 * from under us.  Not much we can do, just move on. 
                 */
            }
            dbs.Dispose();
            GC.SuppressFinalize(this);
        }

        /// <summary>
        /// Read from the blob accessed by this database stream.
        /// </summary>
        /// <param name="offset">
        /// The position in bytes in the blob where the reading starts.
        /// </param>
        /// <param name="size">
        /// The number of bytes to read.
        /// </param>
        /// <returns>
        /// <see cref="DatabaseEntry"/> which contains the data read from
        /// the database stream.
        /// </returns>
        /// <exception cref="DatabaseException">
        /// Thrown if there is any failure.
        /// </exception>
        public DatabaseEntry Read(Int64 offset, uint size) {
            DatabaseEntry data = new DatabaseEntry();
            dbs.read(data, offset, size, 0);
            return data;
        }

        /// <summary>
        /// Return the size of the blob in bytes accessed by the database
        /// stream.
        /// </summary>
        /// <returns>
        /// Return the size of the blob in bytes accessed by the database
        /// stream.
        /// </returns>
        public Int64 Size() {
            Int64 ret = 0;
            dbs.size(ref ret, 0);
            return ret;
        }

        /// <summary>
        /// Write to the blob accessed by the database stream.
        /// </summary>
        /// <param name="data">
        /// The <see cref="DatabaseEntry"/> containing the data to write.
        /// into the blob.
        /// </param>
        /// <param name="offset">
        /// The position in bytes in the blob where the writing starts.
        /// </param>
        /// <returns>
        /// True if the writing succeeds and false otherwise.
        /// </returns>
        /// <exception cref="DatabaseException">
        /// Thrown if there is any failure.
        /// </exception>
        public bool Write(DatabaseEntry data, Int64 offset) {
            bool ret = true;
            if (dbs.write(data, offset, 0) != 0)
                ret = false;
            return ret;
        }
    }
}
