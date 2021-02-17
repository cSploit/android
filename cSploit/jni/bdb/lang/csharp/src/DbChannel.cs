/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2011, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */
using System;
using System.Collections.Generic;
using System.Text;
using BerkeleyDB.Internal;

namespace BerkeleyDB {
    /// <summary>
    /// A class representing a channel in Berkeley DB HA replication manager.
    /// </summary>
    public class DbChannel : IDisposable {
        private bool isOpen;
        private DB_CHANNEL channel;
        private uint timeout;

        /// <summary>
        /// Close the channel.
        /// </summary>
        /// <remarks>
        /// <para>
        /// All channels must be closed before the encompassing environment is
        /// closed. Also, all on-going messaging operations on the channel
        /// should be allowed to complete before attempting to close the 
        /// channel.
        /// </para>
        /// <para>
        /// After close, regardless of its return, the DbChannel may not be 
        /// accessed again.
        /// </para>
        /// </remarks>
        public void Close() {
            isOpen = false;
            channel.close(0);
        }

        /// <summary>
        /// Release the resources held by this object, and close the channel if
        /// it's still open.
        /// </summary>
        public void Dispose() {
            if (isOpen)
                channel.close(0);
            channel.Dispose();
            GC.SuppressFinalize(this);
        }

        /// <summary>
        /// Send a message on the message channel. The message is sent 
        /// asynchronously. The method does not wait for a response before
        /// returning. It usually completes quickly because it only waits for
        /// local TCP implementation to accept the bytes into its network data 
        /// buffer. However, this message could block briefly for longer
        /// messages, and/or if the network data buffer is nearly full. 
        /// It could even block indefinitely if the remote site is slow
        /// to read.
        /// </summary>
        /// <remarks>
        /// <para>
        /// To block while waiting for a response from a remote site, use 
        /// <see cref="SendRequest"/> instead of this method.
        /// </para>
        /// <para>
        /// The sent message is received and handled at remote sites using a
        /// message dispatch callback, which is configured using
        /// <see cref="DatabaseEnvironment.RepMessageDispatch"/>. Note that this
        /// method may be used within the the message dispatch callback on the
        /// remote site to send a reply or acknowledgement for messages that it
        /// receives and is handling.
        /// </para>
        /// <para>
        /// This method may be used on channels opened to any destination. See
        /// <see cref="DatabaseEnvironment.RepMgrChannel"/> for a list of 
        /// potential destinations.
        /// </para>
        /// </remarks>
        /// <param name="msg">
        /// An array of DatabaseEntry objects. Any flags for the DatabaseEntry
        /// objects are ignored.
        /// </param>
        public void SendMessage(DatabaseEntry[] msg) {
            int size = msg.Length;
            IntPtr[] dbts = new IntPtr[size];
            for (int i = 0; i < size; i++)
                dbts[i] = DBT.getCPtr(DatabaseEntry.getDBT(msg[i])).Handle;
            channel.send_msg(dbts, (uint)size, 0); 
        }

        /// <summary>
        /// Send a message on the message channel. The message is sent 
        /// synchronously. The method blocks waiting for a response before
        /// returning. If a response is not received within the timeout value
        /// configured for this request, this method returns with an error
        /// condition.
        /// </summary>
        /// <remarks>
        /// <para>
        /// To avoid block while waiting for a response from a remote site,
        /// use <see cref="SendMessage"/>
        /// </para>
        /// <para>
        /// The message sent by this method is received and handled at remote
        /// sites using a message dispatch callback, which is configured using
        /// <see cref="DatabaseEnvironment.RepMessageDispatch"/>
        /// </para>
        /// </remarks>
        /// <param name="request">
        /// DatabaseEntry objects array. Any flags for the DatabaseEntry objects
        /// are ignored.
        /// </param>
        /// <param name="timeout">
        /// The amount of time that may elapse while this method waits for a 
        /// response from the remote site. The timeout value must be specified
        /// as an unsigned 32-bit number of microseconds, limiting the maximum
        /// timeout to roughly 71 minutes. A timeout value of 0 indicates that
        /// the channel's default timeout value should be used. This default is
        /// configured using <see cref="Timeout"/>.
        /// </param>
        /// <returns>The response from remote site</returns>
        public DatabaseEntry SendRequest(
            DatabaseEntry[] request, uint timeout) {
            int size = request.Length;
            IntPtr[] dbts = new IntPtr[size];
            for (int i = 0; i < size; i++)
                dbts[i] = DBT.getCPtr(DatabaseEntry.getDBT(request[i])).Handle;
            DatabaseEntry response = new DatabaseEntry();
            channel.send_request(dbts, (uint)size, response, timeout, 0);
            return response;
        }

        /// <summary>
        /// Send a message on the message channel. The message is sent 
        /// synchronously. The method blocks waiting for a response before
        /// returning. If a response is not received within the timeout value
        /// configured for this request, this method returns with an error
        /// condition.
        /// </summary>
        /// <remarks>
        /// <para>
        /// To avoid block while waiting for a response from a remote site,
        /// use <see cref="SendMessage"/>
        /// </para>
        /// <para>
        /// The message sent by this method is received and handled at remote
        /// sites using a message dispatch callback, which is configured using
        /// <see cref="DatabaseEnvironment.RepMessageDispatch"/>
        /// </para>
        /// </remarks>
        /// <param name="request">
        /// DatabaseEntry objects array. Any flags for the DatabaseEntry objects
        /// are ignored.
        /// </param>
        /// <param name="bufferSize">Size of bulk buffer</param>
        /// <param name="timeout">
        /// The amount of time that may elapse while this method waits for a 
        /// response from the remote site. The timeout value must be specified
        /// as an unsigned 32-bit number of microseconds, limiting the maximum
        /// timeout to roughly 71 minutes. A timeout value of 0 indicates that
        /// the channel's default timeout value should be used. This default is
        /// configured using <see cref="Timeout"/>.
        /// </param>
        /// <returns>Multiple responses from the remote site.</returns>
        public MultipleDatabaseEntry SendRequest(
            DatabaseEntry[] request, int bufferSize, uint timeout) {
            int size = request.Length;
            IntPtr[] dbts = new IntPtr[size];
            for (int i = 0; i < size; i++)
                dbts[i] = DBT.getCPtr(DatabaseEntry.getDBT(request[i])).Handle;
            DatabaseEntry data = new DatabaseEntry();
            data.UserData = new byte[bufferSize];
            channel.send_request(dbts, (uint)size, data, timeout,
                DbConstants.DB_MULTIPLE);
            MultipleDatabaseEntry response = new MultipleDatabaseEntry(data);
            return response;
        }

        /// <summary>
        /// Set the default timeout value. It is used by
        /// <see cref="SendRequest"/>.
        /// </summary>
        /// <remarks>
        /// <para>
        /// The timeout is the amount of time that may elapse while 
        /// <see cref="SendRequest"/> waits for a message response. It must be
        /// specified as an unsigned 32-bit number of microseconds, limiting the
        ///  maximum timeout to roughly 71 minutes. 
        /// </para>
        /// </remarks>
        public uint Timeout {
            get {
                return timeout;
            }
            set {
                channel.set_timeout(timeout);
            }
        }

        internal DbChannel(DB_CHANNEL channel) {
            this.channel = channel;
            isOpen = true;
            timeout = 0;
        }
    }
}
