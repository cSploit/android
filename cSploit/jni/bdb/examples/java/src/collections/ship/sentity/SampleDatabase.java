/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$
 */

package collections.ship.sentity;

import java.io.File;
import java.io.FileNotFoundException;

import com.sleepycat.bind.serial.ClassCatalog;
import com.sleepycat.bind.serial.StoredClassCatalog;
import com.sleepycat.bind.serial.TupleSerialKeyCreator;
import com.sleepycat.bind.tuple.TupleInput;
import com.sleepycat.bind.tuple.TupleOutput;
import com.sleepycat.db.Database;
import com.sleepycat.db.DatabaseConfig;
import com.sleepycat.db.DatabaseException;
import com.sleepycat.db.DatabaseType;
import com.sleepycat.db.Environment;
import com.sleepycat.db.EnvironmentConfig;
import com.sleepycat.db.ForeignKeyDeleteAction;
import com.sleepycat.db.SecondaryConfig;
import com.sleepycat.db.SecondaryDatabase;

/**
 * SampleDatabase defines the storage containers, indices and foreign keys
 * for the sample database.
 *
 * @author Mark Hayes
 */
public class SampleDatabase {

    private static final String CLASS_CATALOG = "java_class_catalog";
    private static final String SUPPLIER_STORE = "supplier_store";
    private static final String PART_STORE = "part_store";
    private static final String SHIPMENT_STORE = "shipment_store";
    private static final String SHIPMENT_PART_INDEX = "shipment_part_index";
    private static final String SHIPMENT_SUPPLIER_INDEX =
	"shipment_supplier_index";
    private static final String SUPPLIER_CITY_INDEX = "supplier_city_index";

    private Environment env;
    private Database partDb;
    private Database supplierDb;
    private Database shipmentDb;
    private SecondaryDatabase supplierByCityDb;
    private SecondaryDatabase shipmentByPartDb;
    private SecondaryDatabase shipmentBySupplierDb;
    private StoredClassCatalog javaCatalog;

    /**
     * Open all storage containers, indices, and catalogs.
     */
    public SampleDatabase(String homeDirectory)
        throws DatabaseException, FileNotFoundException {

        // Open the Berkeley DB environment in transactional mode.
        //
        System.out.println("Opening environment in: " + homeDirectory);
        EnvironmentConfig envConfig = new EnvironmentConfig();
        envConfig.setTransactional(true);
        envConfig.setAllowCreate(true);
        envConfig.setInitializeCache(true);
        envConfig.setInitializeLocking(true);
        env = new Environment(new File(homeDirectory), envConfig);

        // Set the Berkeley DB config for opening all stores.
        //
        DatabaseConfig dbConfig = new DatabaseConfig();
        dbConfig.setTransactional(true);
        dbConfig.setAllowCreate(true);
        dbConfig.setType(DatabaseType.BTREE);

        // Create the Serial class catalog.  This holds the serialized class
        // format for all database records of serial format.
        //
        Database catalogDb = env.openDatabase(null, CLASS_CATALOG, null,
                                              dbConfig);
        javaCatalog = new StoredClassCatalog(catalogDb);

        // Open the Berkeley DB database for the part, supplier and shipment
        // stores.  The stores are opened with no duplicate keys allowed.
        //
        partDb = env.openDatabase(null, PART_STORE, null, dbConfig);

        supplierDb = env.openDatabase(null, SUPPLIER_STORE, null, dbConfig);

        shipmentDb = env.openDatabase(null, SHIPMENT_STORE, null, dbConfig);

        // Open the SecondaryDatabase for the city index of the supplier store,
        // and for the part and supplier indices of the shipment store.
        // Duplicate keys are allowed since more than one supplier may be in
        // the same city, and more than one shipment may exist for the same
        // supplier or part.  A foreign key constraint is defined for the
        // supplier and part indices to ensure that a shipment only refers to
        // existing part and supplier keys.  The CASCADE delete action means
        // that shipments will be deleted if their associated part or supplier
        // is deleted.
        //
        SecondaryConfig secConfig = new SecondaryConfig();
        secConfig.setTransactional(true);
        secConfig.setAllowCreate(true);
        secConfig.setType(DatabaseType.BTREE);
        secConfig.setSortedDuplicates(true);

        secConfig.setKeyCreator(new SupplierByCityKeyCreator(javaCatalog,
                                                             Supplier.class));
        supplierByCityDb = env.openSecondaryDatabase(null, SUPPLIER_CITY_INDEX,
                                                     null, supplierDb,
                                                     secConfig);

        secConfig.setForeignKeyDatabase(partDb);
        secConfig.setForeignKeyDeleteAction(ForeignKeyDeleteAction.CASCADE);
        secConfig.setKeyCreator(new ShipmentByPartKeyCreator(javaCatalog,
                                                             Shipment.class));
        shipmentByPartDb = env.openSecondaryDatabase(null, SHIPMENT_PART_INDEX,
                                                     null, shipmentDb,
                                                     secConfig);

        secConfig.setForeignKeyDatabase(supplierDb);
        secConfig.setForeignKeyDeleteAction(ForeignKeyDeleteAction.CASCADE);
        secConfig.setKeyCreator(new ShipmentBySupplierKeyCreator(javaCatalog,
                                                              Shipment.class));
        shipmentBySupplierDb = env.openSecondaryDatabase(null,
                                                     SHIPMENT_SUPPLIER_INDEX,
                                                     null, shipmentDb,
                                                     secConfig);
    }

    /**
     * Return the storage environment for the database.
     */
    public final Environment getEnvironment() {

        return env;
    }

    /**
     * Return the class catalog.
     */
    public final StoredClassCatalog getClassCatalog() {

        return javaCatalog;
    }

    /**
     * Return the part storage container.
     */
    public final Database getPartDatabase() {

        return partDb;
    }

    /**
     * Return the supplier storage container.
     */
    public final Database getSupplierDatabase() {

        return supplierDb;
    }

    /**
     * Return the shipment storage container.
     */
    public final Database getShipmentDatabase() {

        return shipmentDb;
    }

    /**
     * Return the shipment-by-part index.
     */
    public final SecondaryDatabase getShipmentByPartDatabase() {

        return shipmentByPartDb;
    }

    /**
     * Return the shipment-by-supplier index.
     */
    public final SecondaryDatabase getShipmentBySupplierDatabase() {

        return shipmentBySupplierDb;
    }

    /**
     * Return the supplier-by-city index.
     */
    public final SecondaryDatabase getSupplierByCityDatabase() {

        return supplierByCityDb;
    }

    /**
     * Close all stores (closing a store automatically closes its indices).
     */
    public void close()
        throws DatabaseException {

        // Close secondary databases, then primary databases.
        supplierByCityDb.close();
        shipmentByPartDb.close();
        shipmentBySupplierDb.close();
        partDb.close();
        supplierDb.close();
        shipmentDb.close();
        // And don't forget to close the catalog and the environment.
        javaCatalog.close();
        env.close();
    }

    /**
     * The SecondaryKeyCreator for the SupplierByCity index.  This is an
     * extension of the abstract class TupleSerialKeyCreator, which implements
     * SecondaryKeyCreator for the case where the data keys are of the format
     * TupleFormat and the data values are of the format SerialFormat.
     */
    private static class SupplierByCityKeyCreator
        extends TupleSerialKeyCreator {

        /**
         * Construct the city key extractor.
         * @param catalog is the class catalog.
         * @param valueClass is the supplier value class.
         */
        private SupplierByCityKeyCreator(ClassCatalog catalog,
                                         Class valueClass) {

            super(catalog, valueClass);
        }

        /**
         * Extract the city key from a supplier key/value pair.  The city key
         * is stored in the supplier value, so the supplier key is not used.
         */
        public boolean createSecondaryKey(TupleInput primaryKeyInput,
                                          Object valueInput,
                                          TupleOutput indexKeyOutput) {

            Supplier supplier = (Supplier) valueInput;
            String city = supplier.getCity();
            if (city != null) {
                indexKeyOutput.writeString(supplier.getCity());
                return true;
            } else {
                return false;
            }
        }
    }

    /**
     * The SecondaryKeyCreator for the ShipmentByPart index.  This is an
     * extension of the abstract class TupleSerialKeyCreator, which implements
     * SecondaryKeyCreator for the case where the data keys are of the format
     * TupleFormat and the data values are of the format SerialFormat.
     */
    private static class ShipmentByPartKeyCreator
        extends TupleSerialKeyCreator {

        /**
         * Construct the part key extractor.
         * @param catalog is the class catalog.
         * @param valueClass is the shipment value class.
         */
        private ShipmentByPartKeyCreator(ClassCatalog catalog,
                                         Class valueClass) {
            super(catalog, valueClass);
        }

        /**
         * Extract the part key from a shipment key/value pair.  The part key
         * is stored in the shipment key, so the shipment value is not used.
         */
        public boolean createSecondaryKey(TupleInput primaryKeyInput,
                                          Object valueInput,
                                          TupleOutput indexKeyOutput) {

            String partNumber = primaryKeyInput.readString();
            // don't bother reading the supplierNumber
            indexKeyOutput.writeString(partNumber);
            return true;
        }
    }

    /**
     * The SecondaryKeyCreator for the ShipmentBySupplier index.  This is an
     * extension of the abstract class TupleSerialKeyCreator, which implements
     * SecondaryKeyCreator for the case where the data keys are of the format
     * TupleFormat and the data values are of the format SerialFormat.
     */
    private static class ShipmentBySupplierKeyCreator
        extends TupleSerialKeyCreator {

        /**
         * Construct the supplier key extractor.
         * @param catalog is the class catalog.
         * @param valueClass is the shipment value class.
         */
        private ShipmentBySupplierKeyCreator(ClassCatalog catalog,
                                             Class valueClass) {
            super(catalog, valueClass);
        }

        /**
         * Extract the supplier key from a shipment key/value pair.  The
         * supplier key is stored in the shipment key, so the shipment value is
         * not used.
         */
        public boolean createSecondaryKey(TupleInput primaryKeyInput,
                                          Object valueInput,
                                          TupleOutput indexKeyOutput) {

            primaryKeyInput.readString(); // skip the partNumber
            String supplierNumber = primaryKeyInput.readString();
            indexKeyOutput.writeString(supplierNumber);
            return true;
        }
    }
}
