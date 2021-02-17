/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */

package com.sleepycat.persist.impl;

/**
 * Thrown and handled internally when metadata must be refreshed on a Replica.
 *
 * There are several scenarios for refreshing DPL metadata:
 *
 * 1. Read entity record on Replica that has stale in-memory metadata.
 *
 *    When an entity record that references new metadata (for example, a never
 *    before encountered class) is written on the Master, the metadata is
 *    written and replicated prior to writing and replicating the entity
 *    record.  However, the Replica's in-memory cache of metadata (the
 *    PersistCatalog object) is not synchronously updated when the metadata is
 *    replicated.  When the entity record that references the newly replicated
 *    metadata is read on the Replica, the DPL must refresh the in-memory
 *    metadata cache by reading it from the catalog database.
 *
 *    Note that this scenario occurs even without class evolution/upgrade, for
 *    two reasons.  First, the Master does not write all metadata at once;
 *    metadata is added to the catalog incrementally as new persistent classes
 *    are encountered.  Second, even when all metadata is written intially by
 *    the Master, the Replica may read the catalog before the Master has
 *    completed metadata updates.
 *
 *    Implementation:
 *    + PersistCatalog.getFormat(int) throws RefreshException when the given
 *      format ID is not in the in-memory catalog.
 *    + The binding method that is calling getFormat catches RefreshException,
 *      calls RefreshException.refresh to read the updated metadata, and
 *      retries the operation.
 *
 *    Tests:
 *      c.s.je.rep.persist.test.UpgradeTest.testIncrementalMetadataChanges
 *      c.s.je.rep.persist.test.UpgradeTest.testUpgrade
 *
 * 2. Write entity record on Master that is in Replica Upgrade Mode.
 *
 *    When a Replica is upgraded with new persistent classes (see
 *    evolve/package.html doc) the DPL will evolve the existing metadata and
 *    update the in-memory metadata cache (PersistCatalog object), but will not
 *    write the metadata; instead, it will enter Replica Upgrade Mode.  In this
 *    mode, the Replica will convert old format data to new format data as
 *    records are read, using the in-memory evolved metadata.  This allows the
 *    Replica application to perform entity read operations with the new
 *    persistent classes, during the upgrade process.
 *
 *    When this Replica is elected Master, the application will begin writing
 *    entity records.  Note that the new metadata has not yet been written to
 *    the catalog database.  In Replica Upgrade Mode, the current in-memory
 *    metadata cannot be written to disk, since the catalog database may be
 *    stale, i.e., it have been updated by the Master after the Replica's
 *    in-memory metadata was evolved.  Therefore, before the first entity
 *    record is written, the newly elected Master must read the latest
 *    metadata, perform evolution of the metadata again, write the metadata,
 *    and then write the entity record.
 *
 *    Implementation:
 *    + The catalog enters Replica Upgrade Mode when a new or evolved format is
 *      added to the catalog, and a ReplicaWriteException occurs when
 *      attempting to write the metadata.  Replica Upgrade Mode is defined as
 *      when the number of in-memory formats is greater than the number of
 *      stored formats.
 *    + Before an entity record is written, PersistEntityBinding.objectToData
 *      is called to convert the entity object to record data.
 *    + objectToData calls PersistCatalog.checkWriteInReplicaUpgradeMode,
 *      which throws RefreshExeption in Replica Upgrade Mode.
 *    + objectToData catches RefreshException, calls RefreshException.refresh
 *      to read the updated metadata, and retries the operation.
 *
 *    Tests:
 *      c.s.je.rep.persist.test.UpgradeTest.testElectedMasterWithStaleMetadata
 *      c.s.je.rep.persist.test.UpgradeTest.testRefreshAfterFirstWrite
 *      c.s.je.rep.persist.test.UpgradeTest.testUpgrade
 *
 * 3. Write metadata on Master that is in Replica Upgrade Mode.
 *
 *    This third scenario is more unusual than the first two.  It occurs when
 *    a Replica with stale metadata is elected Master, but is not in Replica
 *    Upgrade Mode.  The new Master must refresh metadata before writing
 *    metadata.  See the test case for more information.
 *
 *    Implementation:
 *    + On a Master with stale metadata, the application tries to write an
 *      entity record that refers to a class that has not been encountered
 *      before.
 *    + Before the entity record is written, PersistEntityBinding.objectToData
 *      is called to convert the entity object to record data.
 *    + objectToData calls PersistCatalog.addNewFormat during serialization,
 *      which attempts to write metadata by by calling writeDataCheckStale.
 *    + writeDataCheckStale reads the existing metadata and detects that it
 *      has changed since metadata was last read, and throws RefreshExeption.
 *    + objectToData catches RefreshException, calls RefreshException.refresh
 *      to read the updated metadata, and retries the operation.
 *
 *    Tests: 
 *      c.s.je.rep.persist.test.UpgradeTest.testRefreshBeforeWrite
 */
public class RefreshException extends Exception {

    private final Store store;
    private final PersistCatalog catalog;
    private final int formatId;

    RefreshException(final Store store,
                     final PersistCatalog catalog,
                     final int formatId) {
        this.store = store;
        this.catalog = catalog;
        this.formatId = formatId;
    }

    /**
     * This method must be called to handle this exception in the binding
     * methods, after the stack has unwound.  The binding methods should retry
     * the operation once after calling this method.  If the operation fails
     * again, then corruption rather than stale metadata is the likely cause
     * of the problem, and an exception will be thrown to that effect.
     * [#16655]
     */
    public PersistCatalog refresh() {
        return store.refresh(catalog, formatId, this);
    }

    @Override
    public String getMessage() {
        return "formatId=" + formatId;
    }
}
