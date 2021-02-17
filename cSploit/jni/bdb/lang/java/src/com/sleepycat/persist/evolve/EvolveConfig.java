/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */

package com.sleepycat.persist.evolve;

import java.util.Collections;
import java.util.HashSet;
import java.util.Set;

import com.sleepycat.persist.EntityStore;

/**
 * Configuration properties for eager conversion of unevolved objects.  This
 * configuration is used with {@link EntityStore#evolve EntityStore.evolve}.
 *
 * @see com.sleepycat.persist.evolve Class Evolution
 * @author Mark Hayes
 */
public class EvolveConfig implements Cloneable {

    private Set<String> classesToEvolve;
    private EvolveListener evolveListener;

    /**
     * Creates an evolve configuration with default properties.
     */
    public EvolveConfig() {
        classesToEvolve = new HashSet<String>();
    }

    /**
     * Returns a shallow copy of the configuration.
     *
     * @deprecated As of JE 4.0.13, replaced by {@link
     * EvolveConfig#clone()}.</p>
     */
    public EvolveConfig cloneConfig() {
        try {
            return (EvolveConfig) super.clone();
        } catch (CloneNotSupportedException cannotHappen) {
            return null;
        }
    }

    /**
     * Returns a shallow copy of the configuration.
     */
    @Override
    public EvolveConfig clone() {
        try {
            return (EvolveConfig) super.clone();
        } catch (CloneNotSupportedException cannotHappen) {
            return null;
        }
    }

    /**
     * Adds an entity class for a primary index to be converted.  If no classes
     * are added, all indexes that require evolution will be converted.
     */
    public EvolveConfig addClassToEvolve(String entityClass) {
        classesToEvolve.add(entityClass);
        return this;
    }

    /**
     * Returns an unmodifiable set of the entity classes to be evolved.
     */
    public Set<String> getClassesToEvolve() {
        return Collections.unmodifiableSet(classesToEvolve);
    }

    /**
     * Sets a progress listener that is notified each time an entity is read.
     */
    public EvolveConfig setEvolveListener(EvolveListener listener) {
        setEvolveListenerVoid(listener);
        return this;
    }
    
    /**
     * The void return setter for use by Bean editors.
     */
    public void setEvolveListenerVoid(EvolveListener listener) {
        this.evolveListener = listener;
    }

    /**
     * Returns the progress listener that is notified each time an entity is
     * read.
     */
    public EvolveListener getEvolveListener() {
        return evolveListener;
    }
}
