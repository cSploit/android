/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */

package com.sleepycat.util;

import java.beans.BeanDescriptor;
import java.beans.EventSetDescriptor;
import java.beans.IntrospectionException;
import java.beans.PropertyDescriptor;
import java.beans.SimpleBeanInfo;
import java.lang.reflect.Method;
import java.util.ArrayList;

/*
 * If someone add a property in some FooConfig.java,
 *   (1) If the setter/getter methods are setFoo/getFoo, the name of the
 *       property should be "foo", which means the first letter of the property
 *       name should be lower case.
 *   (2) The setter method for this property setProperty should return "this",
 *       and setPropertyVoid method which returns void value must be added.
 *       The return type of the getter method should be the same as the
 *       parameter of the setter method.
 *   (3) The setter method and getter method must be added into
 *       FooConfigBeanInfo;
 *   (4) If for some of the setter methods in the FooConfig.java, setterVoid
 *       methods are not necessary, then add the name of such setter methods
 *       into the ArrayList ignoreMethods within the corresponding
 *       FooConfigBeanInfo.getPropertyDescriptors method. For example,
 *       setMaxSeedTestHook method in DiskOrderedCursorConfig.java is only used
 *       for unit tests, so "setMaxSeedTestHook" is added into ignoreMethods
 *       list within DiskOrderedCursorConfigBeanInfo.getPropertyDescriptors.
 *
 *
 * If someone adds a new FooConfig.java,
 *   (1) The definition of setter/getter mehods and the names of the properties
 *       should follow the rules described above.
 *   (2) There must be FooConfigBeanInfo.java. You can write it according to
 *       the current beaninfo classes.
 *   (3) "PackagePath.FooConfig" must be added into the unit test:
 *       com.sleepycat.db.ConfigBeanInfoTest.
 *
 * If someond remove an existing FooConfig.java, then "PackagePath.FooConfig"
 * must be deleted in the unit test com.sleepycat.db.ConfigBeanInfoTest.
 */
public class ConfigBeanInfoBase extends SimpleBeanInfo {
    private static java.awt.Image iconColor16 = null;
    private static java.awt.Image iconColor32 = null;
    private static java.awt.Image iconMono16 = null;
    private static java.awt.Image iconMono32 = null;
    private static String iconNameC16 = null;
    private static String iconNameC32 = null;
    private static String iconNameM16 = null;
    private static String iconNameM32 = null;

    private static final int defaultPropertyIndex = -1;
    private static final int defaultEventIndex = -1;

    protected static ArrayList<String> propertiesName = new ArrayList<String>();
    protected static ArrayList<String>
        getterAndSetterMethods = new ArrayList<String>();

    protected static ArrayList<String> ignoreMethods = new ArrayList<String>();

    /*
     * Get the propertis' infomation, including all the properties's names
     * and their getter/setter methods.
     */
    protected static void getPropertiesInfo(Class cls) {
        propertiesName.clear();
        getterAndSetterMethods.clear();
        try {

            /* Get all of the public methods. */
            ArrayList<String> allMethodNames = new ArrayList<String>();
            Method[] methods = cls.getMethods();
            for (int i = 0; i < methods.length; i++) {
                allMethodNames.add(methods[i].getName());
            }
            for (int i = 0; i < allMethodNames.size(); i++) {
                String name = allMethodNames.get(i);
                String subName = name.substring(0, 3);

                /* If it is a setter method. */
                if (subName.equals("set")) {
                    if (isIgnoreMethods(name)) {
                        continue;
                    }
                    String propertyName = name.substring(3);
                    Method getterMethod = null;
                    try {
                        getterMethod = cls.getMethod("get" + propertyName);
                    } catch (NoSuchMethodException e) {
                        getterMethod = null;
                    }
                    if (getterMethod != null) {
                        getterAndSetterMethods.add("get" + propertyName);
                        getterAndSetterMethods.add(name + "Void");

                        /*
                         * Add the real property name into propertiesName.
                         * if the names of setter/getter methods are
                         * setFoo/getFoo, the name of the property should be
                         * "foo".
                         */
                        propertiesName.add
                            (propertyName.substring(0, 1).toLowerCase() +
                             propertyName.substring(1));
                    }
                }
            }
        } catch (SecurityException e) {
            e.printStackTrace();
        }
    }

    private static boolean isIgnoreMethods(String methodName) {
        for (int i = 0; i < ignoreMethods.size(); i++) {
            if (ignoreMethods.get(i).equals(methodName)) {
                return true;
            }
        }
        return false;
    }

    protected static PropertyDescriptor[] getPdescriptor(Class cls) {
        getPropertiesInfo(cls);
        final int propertyNum = propertiesName.size();
        assert propertyNum * 2 == getterAndSetterMethods.size();
        PropertyDescriptor[] properties = new PropertyDescriptor[propertyNum];
        try {
            for (int i = 0, j = 0; i < propertyNum; i += 1, j += 2) {
                properties[i] = new PropertyDescriptor
                    (propertiesName.get(i), cls, getterAndSetterMethods.get(j),
                     getterAndSetterMethods.get(j + 1));
            }
        } catch(IntrospectionException e) {
            e.printStackTrace();
        }
        return properties;
    }

    protected static BeanDescriptor getBdescriptor(Class cls) {
        BeanDescriptor beanDescriptor = new BeanDescriptor(cls, null);
        return beanDescriptor;
    }

    /**
     * Gets the bean's <code>BeanDescriptor</code>s.
     *
     * @return BeanDescriptor describing the editable
     * properties of this bean. May return null if the
     * information should be obtained by automatic analysis.
     */
    public BeanDescriptor getBeanDescriptor(Class cls) {
        return null;
    }

    /**
     * Gets the bean's <code>PropertyDescriptor</code>s.
     *
     * @return An array of PropertyDescriptors describing the editable
     * properties supported by this bean. May return null if the
     * information should be obtained by automatic analysis.
     * <p>
     * If a property is indexed, then its entry in the result array will
     * belong to the IndexedPropertyDescriptor subclass of PropertyDescriptor.
     * A client of getPropertyDescriptors can use "instanceof" to check
     * if a given PropertyDescriptor is an IndexedPropertyDescriptor.
     */
    public PropertyDescriptor[] getPropertyDescriptors(Class cls) {
        return null;
    }

    /**
     * Gets the bean's <code>EventSetDescriptor</code>s.
     *
     * @return An array of EventSetDescriptors describing the kinds of
     * events fired by this bean. May return null if the information
     * should be obtained by automatic analysis.
     */
    public EventSetDescriptor[] getEventSetDescriptors() {
        EventSetDescriptor[] eventSets = new EventSetDescriptor[0];
        return eventSets;
    }

    /**
     * A bean may have a "default" property that is the property that will
     * mostly commonly be initially chosen for update by human's who are
     * customizing the bean.
     * @return Index of default property in the PropertyDescriptor array
     * returned by getPropertyDescriptors.
     * <p> Returns -1 if there is no default property.
     */
    public int getDefaultPropertyIndex() {
        return defaultPropertyIndex;
    }

    /**
     * A bean may have a "default" event that is the event that will
     * mostly commonly be used by human's when using the bean.
     * @return Index of default event in the EventSetDescriptor array
     * returned by getEventSetDescriptors.
     * <p> Returns -1 if there is no default event.
     */
    public int getDefaultEventIndex() {
        return defaultEventIndex;
    }

    /**
     * This method returns an image object that can be used to
     * represent the bean in toolboxes, toolbars, etc. Icon images
     * will typically be GIFs, but may in future include other formats.
     * <p>
     * Beans aren't required to provide icons and may return null from
     * this method.
     * <p>
     * There are four possible flavors of icons (16x16 color,
     * 32x32 color, 16x16 mono, 32x32 mono). If a bean choses to only
     * support a single icon we recommend supporting 16x16 color.
     * <p>
     * We recommend that icons have a "transparent" background
     * so they can be rendered onto an existing background.
     *
     * @param iconKind  The kind of icon requested. This should be
     * one of the constant values ICON_COLOR_16x16, ICON_COLOR_32x32,
     * ICON_MONO_16x16, or ICON_MONO_32x32.
     * @return An image object representing the requested icon. May
     * return null if no suitable icon is available.
     */
    public java.awt.Image getIcon(int iconKind) {
        switch (iconKind) {
        case ICON_COLOR_16x16:
            if (iconNameC16 == null) {
                return null;
            } else {
                if (iconColor16 == null) {
                    iconColor16 = loadImage(iconNameC16);
                }
                return iconColor16;
            }

        case ICON_COLOR_32x32:
            if (iconNameC32 == null) {
                return null;
            } else {
                if (iconColor32 == null) {
                    iconColor32 = loadImage(iconNameC32);
                }
                return iconColor32;
            }

        case ICON_MONO_16x16:
            if (iconNameM16 == null) {
                return null;
            } else {
                if (iconMono16 == null) {
                    iconMono16 = loadImage(iconNameM16);
                }
                return iconMono16;
            }

        case ICON_MONO_32x32:
            if (iconNameM32 == null) {
                return null;
            } else {
                if (iconMono32 == null) {
                    iconMono32 = loadImage(iconNameM32);
                }
                return iconMono32;
            }

        default:
            return null;
        }
    }
}
