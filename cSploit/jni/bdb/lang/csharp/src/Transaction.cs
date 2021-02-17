/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */
using System;
using System.Collections.Generic;
using System.Text;
using BerkeleyDB.Internal;

namespace BerkeleyDB {
    /// <summary>
    /// A class representing Berkeley DB transactions
    /// </summary>
    /// <remarks>
    /// <para>
    /// Calling <see cref="Transaction.Abort"/>,
    /// <see cref="Transaction.Commit"/> or
    /// <see cref="Transaction.Discard"/> will release the resources held by
    /// the created object.
    /// </para>
    /// <para>
    /// Transactions may only span threads if they do so serially; that is,
    /// each transaction must be active in only a single thread of control
    /// at a time. This restriction holds for parents of nested transactions
    /// as well; no two children may be concurrently active in more than one
    /// thread of control at any one time.
    /// </para>
    /// <para>
    /// Cursors may not span transactions; that is, each cursor must be
    /// opened and closed within a single transaction.
    /// </para>
    /// <para>
    /// A parent transaction may not issue any Berkeley DB operations
    /// except for <see cref="DatabaseEnvironment.BeginTransaction"/>, 
    /// <see cref="Transaction.Abort"/>and <see cref="Transaction.Commit"/>
    /// while it has active child transactions (child transactions that
    /// have not yet been committed or aborted).
    /// </para>
    /// </remarks>
    public class Transaction {
        /// <summary>
        /// The size of the global transaction ID
        /// </summary>
        public static uint GlobalIdLength = DbConstants.DB_GID_SIZE;

        internal DB_TXN dbtxn;
        internal DB_TXN_TOKEN dbtoken;
        
        internal void SetCommitToken() {
            if (dbtxn.is_commit_token_enabled()) {
                dbtoken = new DB_TXN_TOKEN();
                dbtxn.set_commit_token(dbtoken);
            }            
        }

        internal Transaction(DB_TXN txn) {
            dbtxn = txn;
            dbtoken = null;            
        }

        private bool idCached = false;
        private uint _id;
        private bool isCommitted = false;
        /// <summary>
        /// The unique transaction id associated with this transaction.
        /// </summary>
        public uint Id {
            get {
                if (!idCached) {
                    _id = dbtxn.id();
                    idCached = true;
                }
                return _id;
            }
        }

        /// <summary>
        /// The commit token generated at the commit time of this transaction.
        /// This operation can only be performed after this transaction has committed.
        /// </summary>
        public byte[] CommitToken {
            /*
             *If iscommitted is true, but dbtoken is null, then it's one
             * of below cases:
             * 1) either this transaction is a nested txn; or
             * 2) this environment didn't enable logging; or
             * 3) the user calls this function on a rep client node.\
             */
            get {
                if (isCommitted)
                    if (dbtoken != null)
                        return dbtoken.buf;
                    else throw new ArgumentException("Cannot get token because of improper environment settings.");
                else throw new InvalidOperationException("Cannot get token before transaction commit.");
            }
        }

        /// <summary>
        /// The deadlock priority for this transaction.  The deadlock detector
        /// will reject lock requests from lower priority transactions before
        /// those from higher priority transactions.
        /// </summary>
        public uint Priority {
            get {
                uint ret = 0;
                dbtxn.get_priority(ref ret);
                return ret;
            }
            set {
                dbtxn.set_priority(value);
            }
        }

        /// <summary>
        /// Cause an abnormal termination of the transaction.
        /// </summary>
        /// <remarks>
        /// <para>
        /// Before Abort returns, any locks held by the transaction will have
        /// been released.
        /// </para>
        /// <para>
        /// In the case of nested transactions, aborting a parent transaction
        /// causes all children (unresolved or not) of the parent transaction to
        /// be aborted.
        /// </para>
		/// <para>
        /// All cursors opened within the transaction must be closed before the 
        ///	transaction is aborted. This method closes all open cursor handles,
        /// if any. And if a close operation fails, the rest of
		///	the cursors are closed, and the database environment
		/// is set to the panic state.
        /// </para>
        /// </remarks>
        public void Abort() {
            dbtxn.abort();
        }
        
        /// <summary>
        /// End the transaction.
        /// </summary>
        /// <overloads>
        /// <para>
        /// In the case of nested transactions, if the transaction is a parent
        /// transaction, committing the parent transaction causes all unresolved
        /// children of the parent to be committed. In the case of nested
        /// transactions, if the transaction is a child transaction, its locks
        /// are not released, but are acquired by its parent. Although the
        /// commit of the child transaction will succeed, the actual resolution
        /// of the child transaction is postponed until the parent transaction
        /// is committed or aborted; that is, if its parent transaction commits,
        /// it will be committed; and if its parent transaction aborts, it will
        /// be aborted.
        /// </para>
		/// <para>
        /// All cursors opened within the transaction must be closed before the
        /// transaction is committed. If there are cursor handles
	/// open when this method is called, they are all closed inside this 
	/// method. And if there are errors when closing the cursor handles, 
	/// the transaction is aborted and the first such error is returned.
        /// </para>
        /// </overloads>
        public void Commit() {
            SetCommitToken();
            dbtxn.commit(0);
            isCommitted = true;
        }
        /// <summary>
        /// End the transaction.
        /// </summary>
        /// <remarks>
        /// Synchronously flushing the log is the default for Berkeley DB
        /// environments unless
        /// <see cref="DatabaseEnvironmentConfig.TxnNoSync"/> was specified.
        /// Synchronous log flushing may also be set or unset for a single
        /// transaction using
        /// <see cref="DatabaseEnvironment.BeginTransaction"/>. The
        /// value of <paramref name="syncLog"/> overrides both of those
        /// settings.
        /// </remarks>
        /// <param name="syncLog">If true, synchronously flush the log.</param>
        public void Commit(bool syncLog) {
            SetCommitToken();
            dbtxn.commit(
                syncLog ? DbConstants.DB_TXN_SYNC : DbConstants.DB_TXN_NOSYNC);
            isCommitted = true;
        }

        /// <summary>
        /// Free up all the per-process resources associated with the specified
        /// Transaction instance, neither committing nor aborting the
        /// transaction.
        /// </summary>
        /// <remarks>
	/// If there are cursor handles open when this method is called, they 
	/// are all closed inside this method. And if there are errors when 
	/// closing the cursor handles, the first such error is returned.
        /// This call may be used only after calls to
        /// <see cref="DatabaseEnvironment.Recover"/> when there are multiple
        /// global transaction managers recovering transactions in a single
        /// Berkeley DB environment. Any transactions returned by 
        /// <see cref="DatabaseEnvironment.Recover"/> that are not handled by
        /// the current global transaction manager should be discarded using 
        /// Discard.
        /// </remarks>
        public void Discard() {
            dbtxn.discard(0);
        }

        /// <summary>
        /// The transaction's name. The name is returned by
        /// <see cref="DatabaseEnvironment.TransactionSystemStats"/>
        /// and displayed by
        /// <see cref="DatabaseEnvironment.PrintTransactionSystemStats"/>.
        /// </summary>
        /// <remarks>
        /// If the database environment has been configured for logging and the
        /// Berkeley DB library was built in Debug mode (or with DIAGNOSTIC
        /// defined), a debugging log record is written including the
        /// transaction ID and the name.
        /// </remarks>
        public string Name {
            get {
                string ret = "";
                dbtxn.get_name(out ret);
                return ret;
            }
            set { dbtxn.set_name(value); }
        }

        /// <summary>
        /// Initiate the beginning of a two-phase commit.
        /// </summary>
        /// <remarks>
        /// <para>
        /// In a distributed transaction environment, Berkeley DB can be used as
        /// a local transaction manager. In this case, the distributed
        /// transaction manager must send prepare messages to each local
        /// manager. The local manager must then call Prepare and await its
        /// successful return before responding to the distributed transaction
        /// manager. Only after the distributed transaction manager receives
        /// successful responses from all of its prepare messages should it
        /// issue any commit messages.
        /// </para>
		/// <para>
        /// In the case of nested transactions, preparing the parent causes all
        /// unresolved children of the parent transaction to be committed. Child
        /// transactions should never be explicitly prepared. Their fate will be
        /// resolved along with their parent's during global recovery.
        /// </para>
        /// <para>
	/// If there are cursor handles open when this method is called, they 
	/// are all closed inside this method. And if there are errors when 
	/// closing the cursor handles, the first such error is returned.
	/// </para>
        /// </remarks>
        /// <param name="globalId">
        /// The global transaction ID by which this transaction will be known.
        /// This global transaction ID will be returned in calls to 
        /// <see cref="DatabaseEnvironment.Recover"/> telling the
        /// application which global transactions must be resolved.
        /// </param>
        public void Prepare(byte[] globalId) {
            if (globalId.Length != Transaction.GlobalIdLength)
                throw new ArgumentException(
                    "Global ID length must be " + Transaction.GlobalIdLength);
            dbtxn.prepare(globalId);
        }

        /// <summary>
        /// Set the timeout value for locks for this transaction. 
        /// </summary>
        /// <remarks>
        /// <para>
        /// Timeouts are checked whenever a thread of control blocks on a lock
        /// or when deadlock detection is performed. This timeout is for any
        /// single lock request. As timeouts are only checked when the lock
        /// request first blocks or when deadlock detection is performed, the
        /// accuracy of the timeout depends on how often deadlock detection is
        /// performed.
        /// </para>
		/// <para>
        /// Timeout values may be specified for the database environment as a
        /// whole. See <see cref="DatabaseEnvironment.LockTimeout"/> for more
        /// information. 
        /// </para>
        /// </remarks>
        /// <param name="timeout">
        /// An unsigned 32-bit number of microseconds, limiting the maximum
        /// timeout to roughly 71 minutes. A value of 0 disables timeouts for
        /// the transaction.
        /// </param>
        public void SetLockTimeout(uint timeout) {
            dbtxn.set_timeout(timeout, DbConstants.DB_SET_LOCK_TIMEOUT);
        }

        /// <summary>
        /// Set the timeout value for transactions for this transaction. 
        /// </summary>
        /// <remarks>
        /// <para>
        /// Timeouts are checked whenever a thread of control blocks on a lock
        /// or when deadlock detection is performed. This timeout is for the
        /// life of the transaction. As timeouts are only checked when the lock
        /// request first blocks or when deadlock detection is performed, the
        /// accuracy of the timeout depends on how often deadlock detection is
        /// performed. 
        /// </para>
		/// <para>
        /// Timeout values may be specified for the database environment as a
        /// whole. See <see cref="DatabaseEnvironment.TxnTimeout"/> for more
        /// information. 
        /// </para>
        /// </remarks>
        /// <param name="timeout">
        /// An unsigned 32-bit number of microseconds, limiting the maximum
        /// timeout to roughly 71 minutes. A value of 0 disables timeouts for
        /// the transaction.
        /// </param>
        public void SetTxnTimeout(uint timeout) {
            dbtxn.set_timeout(timeout, DbConstants.DB_SET_TXN_TIMEOUT);
        }

        static internal DB_TXN getDB_TXN(Transaction txn) {
            return txn == null ? null : txn.dbtxn;
        }
    }
}
