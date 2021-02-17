/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */

package com.sleepycat.persist.impl;

import java.lang.reflect.Array;
import java.util.Map;
import java.util.Set;

import com.sleepycat.persist.model.EntityModel;
import com.sleepycat.persist.model.PersistentProxy;
import com.sleepycat.persist.raw.RawObject;
import java.util.IdentityHashMap;

/**
 * Format for types proxied by a PersistentProxy.
 *
 * @author Mark Hayes
 */
public class ProxiedFormat extends Format {

    private static final long serialVersionUID = -1000032651995478768L;

    private Format proxyFormat;
    private transient String proxyClassName;

    ProxiedFormat(Catalog catalog, Class proxiedType, String proxyClassName) {
        super(catalog, proxiedType);
        this.proxyClassName = proxyClassName;
    }

    /**
     * Returns the proxy class name.  The proxyClassName field is non-null for
     * a constructed object and null for a de-serialized object.  Whenever the
     * proxyClassName field is null (for a de-serialized object), the
     * proxyFormat will be non-null.
     */
    private String getProxyClassName() {
        if (proxyClassName != null) {
            return proxyClassName;
        } else {
            assert proxyFormat != null;
            return proxyFormat.getClassName();
        }
    }

    /**
     * In the future if we implement container proxies, which support nested
     * references to the container, then we will return false if this is a
     * container proxy.  [#15815]
     */
    @Override
    boolean areNestedRefsProhibited() {
        return true;
    }

    @Override
    void collectRelatedFormats(Catalog catalog,
                               Map<String, Format> newFormats) {
        /* Collect the proxy format. */
        assert proxyClassName != null;
        catalog.createFormat(proxyClassName, newFormats);
    }

    @Override
    void initialize(Catalog catalog, EntityModel model, int initVersion) {
        /* Set the proxy format for a new (never initialized) format. */
        if (proxyFormat == null) {
            assert proxyClassName != null;
            proxyFormat = catalog.getFormat(proxyClassName);
        }
        /* Make the linkage from proxy format to proxied format. */
        proxyFormat.setProxiedFormat(this);
    }

    @Override
    Object newArray(int len) {
        return Array.newInstance(getType(), len);
    }

    @Override
    public Object newInstance(EntityInput input, boolean rawAccess)
        throws RefreshException {

        Reader reader = proxyFormat.getReader();
        if (rawAccess) {
            return reader.newInstance(null, true);
        } else {

            /* 
             * Note that the read object will not be a PersistentProxy if
             * a class converter mutation is used.  In this case, the reader 
             * will be ConverterReader. ConverterReader.readObject
             * will call ProxiedFormat.convertRawObject, which will call
             * PersistentProxy.convertProxy to convert the proxy. So we do not
             * need another call to the convertProxy method.  [#19312]
             */
            Object o = reader.readObject(reader.newInstance(null, false), 
                                         input, false);
            if (o instanceof PersistentProxy) {
                o = ((PersistentProxy) o).convertProxy();
            }
            return o;
        }
    }

    @Override
    public Object readObject(Object o, EntityInput input, boolean rawAccess)
        throws RefreshException {

        if (rawAccess) {
            o = proxyFormat.getReader().readObject(o, input, true);
        }
        /* Else, do nothing here -- newInstance reads the value. */
        return o;
    }

    @Override
    void writeObject(Object o, EntityOutput output, boolean rawAccess)
        throws RefreshException {

        if (rawAccess) {
            proxyFormat.writeObject(o, output, true);
        } else {
            PersistentProxy proxy =
                (PersistentProxy) proxyFormat.newInstance(null, false);
            proxy.initializeProxy(o);
            proxyFormat.writeObject(proxy, output, false);
        }
    }

    @Override
    Object convertRawObject(Catalog catalog,
                            boolean rawAccess,
                            RawObject rawObject,
                            IdentityHashMap converted)
        throws RefreshException {

        PersistentProxy proxy = (PersistentProxy) proxyFormat.convertRawObject
            (catalog, rawAccess, rawObject, converted);
        Object o = proxy.convertProxy();
        converted.put(rawObject, o);
        return o;
    }

    @Override
    void skipContents(RecordInput input)
        throws RefreshException {

        proxyFormat.skipContents(input);
    }

    @Override
    void copySecMultiKey(RecordInput input, Format keyFormat, Set results)
        throws RefreshException {

        CollectionProxy.copyElements(input, this, keyFormat, results);
    }

    @Override
    boolean evolve(Format newFormatParam, Evolver evolver) {
        if (!(newFormatParam instanceof ProxiedFormat)) {
            
            /* 
             * A workaround for reading the BigDecimal data stored by 
             * BigDecimal proxy before je4.1. 
             * 
             * The BigDecimal proxy has a proxied format for BigDecimal, which 
             * is a built-in SimpleType. We will evolve this ProxiedFormat of 
             * BigDecimal to the SimpleFormat. In other words, the conversion 
             * from a BigDecimal proxied format to a BigDecimal SimpleFormat is 
             * allowed, and the old format can be used as the reader of the old 
             * data.
             */
            if (newFormatParam.allowEvolveFromProxy()) {
                evolver.useEvolvedFormat(this, this, newFormatParam);
                return true;
            }
            evolver.addEvolveError
                (this, newFormatParam, null,
                 "A proxied class may not be changed to a different type");
            return false;
        }
        ProxiedFormat newFormat = (ProxiedFormat) newFormatParam;
        if (!evolver.evolveFormat(proxyFormat)) {
            return false;
        }
        Format newProxyFormat = proxyFormat.getLatestVersion();
        if (!newProxyFormat.getClassName().equals
                (newFormat.getProxyClassName())) {
            evolver.addEvolveError
                (this, newFormat, null,
                 "The proxy class for this type has been changed from: " +
                 newProxyFormat.getClassName() + " to: " +
                 newFormat.getProxyClassName());
            return false;
        }
        if (newProxyFormat != proxyFormat) {
            evolver.useEvolvedFormat(this, this, newFormat);
        } else {
            evolver.useOldFormat(this, newFormat);
        }
        return true;
    }
}
