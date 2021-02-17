/*
********************************************************************************
*   Copyright (C) 2010, International Business Machines
*   Corporation and others.  All Rights Reserved.
********************************************************************************
*
* File attiter.h
*
* Modification History:
*
*   Date        Name        Description
*   12/15/2009  dougfelt    Created
********************************************************************************
*/

#ifndef FPOSITER_H
#define FPOSITER_H

#include "unicode/utypes.h"
#include "unicode/uobject.h"

/**
 * \file
 * \brief C++ API: FieldPosition Iterator.
 */

#if UCONFIG_NO_FORMATTING

U_NAMESPACE_BEGIN

/*
 * Allow the declaration of APIs with pointers to FieldPositionIterator
 * even when formatting is removed from the build.
 */
class FieldPositionIterator;

U_NAMESPACE_END

#else

#include "unicode/fieldpos.h"
#include "unicode/umisc.h"

U_NAMESPACE_BEGIN

class UVector32;

/**
 * FieldPositionIterator returns the field ids and their start/limit positions generated
 * by a call to Format::format.  See Format, NumberFormat, DecimalFormat.
 * @draft ICU 4.4
 */
class U_I18N_API FieldPositionIterator : public UObject {
public:
    /**
     * Destructor.
     * @draft ICU 4.4
     */
    ~FieldPositionIterator();

    /**
     * Constructs a new, empty iterator.
     * @draft ICU 4.4
     */
    FieldPositionIterator(void);

    /**
     * Copy constructor.  If the copy failed for some reason, the new iterator will
     * be empty.
     * @draft ICU 4.4
     */
    FieldPositionIterator(const FieldPositionIterator&);

    /**
     * Return true if another object is semantically equal to this
     * one.
     * <p>
     * Return true if this FieldPositionIterator is at the same position in an
     * equal array of run values.
     * @draft ICU 4.4
     */
    UBool operator==(const FieldPositionIterator&) const;

    /**
     * Returns the complement of the result of operator==
     * @param rhs The FieldPositionIterator to be compared for inequality
     * @return the complement of the result of operator==
     * @draft ICU 4.4
     */
    UBool operator!=(const FieldPositionIterator& rhs) const { return !operator==(rhs); }

    /**
     * If the current position is valid, updates the FieldPosition values, advances the iterator,
     * and returns TRUE, otherwise returns FALSE.
     * @draft ICU 4.4
     */
    UBool next(FieldPosition& fp);

    /**
     * ICU "poor man's RTTI", returns a UClassID for this class.
     * @draft ICU 4.4
     */
    static UClassID U_EXPORT2 getStaticClassID();

    /**
     * ICU "poor man's RTTI", returns a UClassID for the actual class.
     * @draft ICU 4.4
     */
    virtual UClassID getDynamicClassID() const;

    /**
     * Returns the data.  If dest is null, returns the length of the data.
     * Otherwise, if capacity is insufficient, returns the negative of the
     * length of the data.  Otherwise, copies data into dest and returns
     * the length of the data.
     * @internal
     */
    int32_t getData(int32_t *dest, int32_t capacity) const;

private:
    friend class FieldPositionIteratorHandler;

    /**
     * Sets the data used by the iterator, and resets the position.
     * Returns U_ILLEGAL_ARGUMENT_ERROR in status if the data is not valid 
     * (length is not a multiple of 3, or start >= limit for any run).
     */
    void setData(UVector32 *adopt, UErrorCode& status);

    UVector32 *data;
    int32_t pos;
};

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */

#endif // FPOSITER_H
