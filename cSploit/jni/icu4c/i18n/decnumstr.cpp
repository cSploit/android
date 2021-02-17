/*
******************************************************************************
*
*   Copyright (C) 2010, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
******************************************************************************
*
* File decnumstr.cpp
*
*/

#include "unicode/utypes.h"
#include "decnumstr.h"
#include "cmemory.h"
#include "uassert.h"

U_NAMESPACE_BEGIN

DecimalNumberString::DecimalNumberString() {
    fLength  = 0;
    fText[0] = 0;
}

DecimalNumberString::~DecimalNumberString() {
}

DecimalNumberString::DecimalNumberString(const StringPiece &source, UErrorCode &status) {
    fLength = 0;
    fText[0] = 0;
    append(source, status);
}

DecimalNumberString & DecimalNumberString::append(char c, UErrorCode &status) {
    if (ensureCapacity(fLength + 2, status) == FALSE) {
        return *this;
    }
    fText[fLength++] = c;
    fText[fLength] = 0;
    return *this;
}

DecimalNumberString &DecimalNumberString::append(const StringPiece &str, UErrorCode &status) {
    int32_t sLength = str.length();
    if (ensureCapacity(fLength + sLength + 1, status) == FALSE) {
        return *this;
    }
    uprv_memcpy(&fText[fLength], str.data(), sLength);
    fLength += sLength;
    fText[fLength] = 0;
    return *this;
}

char & DecimalNumberString::operator [] (int32_t index) {
    U_ASSERT(index>=0 && index<fLength);
    return fText[index];
}

const char & DecimalNumberString::operator [] (int32_t index) const {
    U_ASSERT(index>=0 && index<fLength);
    return fText[index];
}

int32_t DecimalNumberString::length() const {
    return fLength;
}

void DecimalNumberString::setLength(int32_t length, UErrorCode &status) {
    if (ensureCapacity(length+1, status) == FALSE) {
        return;
    }
    if (length > fLength) {
        uprv_memset(&fText[fLength], length - fLength, 0);
    }
    fLength = length;
    fText[fLength] = 0;
}

DecimalNumberString::operator StringPiece() const {
    return StringPiece(fText, fLength);
}

UBool DecimalNumberString::ensureCapacity(int32_t neededSize, UErrorCode &status) {
    if (U_FAILURE(status)) {
        return FALSE;
    }
    if (fText.getCapacity() < neededSize) {
        char *newBuf = fText.resize(neededSize, fText.getCapacity());
        if (newBuf == NULL) {
            status = U_MEMORY_ALLOCATION_ERROR;
            return FALSE;
        }
        U_ASSERT(fText.getCapacity() >= neededSize);
        U_ASSERT(fText.getAlias() == newBuf);
    }
    return TRUE;
}

U_NAMESPACE_END

