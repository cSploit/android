/******************************************************************************
 *   Copyright (C) 2009, International Business Machines
 *   Corporation and others.  All Rights Reserved.
 *******************************************************************************
 */

#include "flagparser.h"
#include "filestrm.h"

#define LARGE_BUFFER_MAX_SIZE 2048

static void extractFlag(char* buffer, int32_t bufferSize, char* flag, int32_t flagSize, UErrorCode *status);
static int32_t getFlagOffset(const char *buffer, int32_t bufferSize);

/*
 * Opens the given fileName and reads in the information storing the data in flagBuffer.
 */
U_CAPI void U_EXPORT2
parseFlagsFile(const char *fileName, char **flagBuffer, int32_t flagBufferSize, int32_t numOfFlags, UErrorCode *status) {
    char buffer[LARGE_BUFFER_MAX_SIZE];
    int32_t i;

    FileStream *f = T_FileStream_open(fileName, "r");
    if (f == NULL) {
        *status = U_FILE_ACCESS_ERROR;
        return;
    }

    for (i = 0; i < numOfFlags; i++) {
        if (T_FileStream_readLine(f, buffer, LARGE_BUFFER_MAX_SIZE) == NULL) {
            *status = U_FILE_ACCESS_ERROR;
            break;
        }

        extractFlag(buffer, LARGE_BUFFER_MAX_SIZE, flagBuffer[i], flagBufferSize, status);
        if (U_FAILURE(*status)) {
            break;
        }
    }

    T_FileStream_close(f);
}


/*
 * Extract the setting after the '=' and store it in flag excluding the newline character.
 */
static void extractFlag(char* buffer, int32_t bufferSize, char* flag, int32_t flagSize, UErrorCode *status) {
    int32_t i;
    char *pBuffer;
    int32_t offset;
    UBool bufferWritten = FALSE;

    if (buffer[0] != 0) {
        /* Get the offset (i.e. position after the '=') */
        offset = getFlagOffset(buffer, bufferSize);
        pBuffer = buffer+offset;
        for(i = 0;;i++) {
            if (i >= flagSize) {
                *status = U_BUFFER_OVERFLOW_ERROR;
                return;
            }
            if (pBuffer[i+1] == 0) {
                /* Indicates a new line character. End here. */
                flag[i] = 0;
                break;
            }

            flag[i] = pBuffer[i];
            if (i == 0) {
                bufferWritten = TRUE;
            }
        }
    }

    if (!bufferWritten) {
        flag[0] = 0;
    }
}

/*
 * Get the position after the '=' character.
 */
static int32_t getFlagOffset(const char *buffer, int32_t bufferSize) {
    int32_t offset = 0;

    for (offset = 0; offset < bufferSize;offset++) {
        if (buffer[offset] == '=') {
            offset++;
            break;
        }
    }

    if (offset == bufferSize || (offset - 1) == bufferSize) {
        offset = 0;
    }

    return offset;
}
