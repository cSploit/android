/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$
 */

package collections.ship.index;

import com.sleepycat.bind.EntryBinding;
import com.sleepycat.bind.serial.ClassCatalog;
import com.sleepycat.bind.serial.SerialBinding;
import com.sleepycat.collections.StoredEntrySet;
import com.sleepycat.collections.StoredSortedMap;

/**
 * SampleViews defines the data bindings and collection views for the sample
 * database.
 *
 * @author Mark Hayes
 */
public class SampleViews {

    private StoredSortedMap partMap;
    private StoredSortedMap supplierMap;
    private StoredSortedMap shipmentMap;
    private StoredSortedMap shipmentByPartMap;
    private StoredSortedMap shipmentBySupplierMap;
    private StoredSortedMap supplierByCityMap;

    /**
     * Create the data bindings and collection views.
     */
    public SampleViews(SampleDatabase db) {

        // Create the data bindings.
        // In this sample, the stored key and data entries are used directly
        // rather than mapping them to separate objects. Therefore, no binding
        // classes are defined here and the SerialBinding class is used.
        //
        ClassCatalog catalog = db.getClassCatalog();
        EntryBinding partKeyBinding =
            new SerialBinding(catalog, PartKey.class);
        EntryBinding partDataBinding =
            new SerialBinding(catalog, PartData.class);
        EntryBinding supplierKeyBinding =
            new SerialBinding(catalog, SupplierKey.class);
        EntryBinding supplierDataBinding =
            new SerialBinding(catalog, SupplierData.class);
        EntryBinding shipmentKeyBinding =
            new SerialBinding(catalog, ShipmentKey.class);
        EntryBinding shipmentDataBinding =
            new SerialBinding(catalog, ShipmentData.class);
        EntryBinding cityKeyBinding =
            new SerialBinding(catalog, String.class);

        // Create map views for all stores and indices.
        // StoredSortedMap is not used since the stores and indices are
        // ordered by serialized key objects, which do not provide a very
        // useful ordering.
        //
        partMap =
            new StoredSortedMap(db.getPartDatabase(),
                                partKeyBinding, partDataBinding, true);
        supplierMap =
            new StoredSortedMap(db.getSupplierDatabase(),
                                supplierKeyBinding, supplierDataBinding, true);
        shipmentMap =
            new StoredSortedMap(db.getShipmentDatabase(),
                                shipmentKeyBinding, shipmentDataBinding, true);
        shipmentByPartMap =
            new StoredSortedMap(db.getShipmentByPartDatabase(),
                                partKeyBinding, shipmentDataBinding, true);
        shipmentBySupplierMap =
            new StoredSortedMap(db.getShipmentBySupplierDatabase(),
                                supplierKeyBinding, shipmentDataBinding, true);
        supplierByCityMap =
            new StoredSortedMap(db.getSupplierByCityDatabase(),
                                cityKeyBinding, supplierDataBinding, true);
    }

    // The views returned below can be accessed using the java.util.Map or
    // java.util.Set interfaces, or using the StoredSortedMap and
    // StoredEntrySet classes, which provide additional methods.  The entry
    // sets could be obtained directly from the Map.entrySet() method, but
    // convenience methods are provided here to return them in order to avoid
    // down-casting elsewhere.

    /**
     * Return a map view of the part storage container.
     */
    public final StoredSortedMap getPartMap() {

        return partMap;
    }

    /**
     * Return a map view of the supplier storage container.
     */
    public final StoredSortedMap getSupplierMap() {

        return supplierMap;
    }

    /**
     * Return a map view of the shipment storage container.
     */
    public final StoredSortedMap getShipmentMap() {

        return shipmentMap;
    }

    /**
     * Return an entry set view of the part storage container.
     */
    public final StoredEntrySet getPartEntrySet() {

        return (StoredEntrySet) partMap.entrySet();
    }

    /**
     * Return an entry set view of the supplier storage container.
     */
    public final StoredEntrySet getSupplierEntrySet() {

        return (StoredEntrySet) supplierMap.entrySet();
    }

    /**
     * Return an entry set view of the shipment storage container.
     */
    public final StoredEntrySet getShipmentEntrySet() {

        return (StoredEntrySet) shipmentMap.entrySet();
    }

    /**
     * Return a map view of the shipment-by-part index.
     */
    public StoredSortedMap getShipmentByPartMap() {

        return shipmentByPartMap;
    }

    /**
     * Return a map view of the shipment-by-supplier index.
     */
    public StoredSortedMap getShipmentBySupplierMap() {

        return shipmentBySupplierMap;
    }

    /**
     * Return a map view of the supplier-by-city index.
     */
    public final StoredSortedMap getSupplierByCityMap() {

        return supplierByCityMap;
    }
}
