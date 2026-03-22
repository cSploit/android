/*
 * This file is part of the cSploit.
 *
 * cSploit is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * cSploit is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with cSploit.  If not, see <http://www.gnu.org/licenses/>.
 */
package org.csploit.android.core;
import org.csploit.android.helpers.LoggingHelper;

import android.content.Context;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.io.PrintWriter;
import java.io.StringWriter;
import java.time.LocalDateTime;
import java.time.format.DateTimeFormatter;
import java.util.Locale;

/**
 * Centralized error and exception logging for the application.
 * Handles writing exceptions to log files and console.
 */
public class ErrorLogger {
    private static final String ERROR_LOG_FILE = "error_log.txt";
    // DateTimeFormatter is immutable and thread-safe (unlike SimpleDateFormat)
    private static final DateTimeFormatter DATE_FORMAT =
        DateTimeFormatter.ofPattern("yyyy-MM-dd HH:mm:ss", Locale.US);
    private static final int MAX_LOG_SIZE = 1024 * 1024; // 1 MB

    private String mLogsPath;

    /**
     * Creates an ErrorLogger instance
     *
     * @param logsPath Directory where log files are stored
     */
    public ErrorLogger(String logsPath) {
        this.mLogsPath = logsPath;
    }

    /**
     * Log an exception to file and console
     *
     * @param throwable The exception to log
     */
    public void logError(Throwable throwable) {
        if (throwable == null) return;

        String errorMessage = formatException(throwable);
        LoggingHelper.error(errorMessage);

        try {
            writeToErrorLog(errorMessage);
        } catch (IOException e) {
            LoggingHelper.error("Failed to write error to log file: " + e.getMessage());
        }
    }

    /**
     * Log an error message
     *
     * @param message The error message to log
     */
    public void logError(String message) {
        LoggingHelper.error(message);

        try {
            writeToErrorLog(message);
        } catch (IOException e) {
            LoggingHelper.error("Failed to write error to log file: " + e.getMessage());
        }
    }

    /**
     * Log an error with message and exception
     *
     * @param message The error message
     * @param throwable The exception
     */
    public void logError(String message, Throwable throwable) {
        String fullMessage = message + "\n" + formatException(throwable);
        LoggingHelper.error(fullMessage);

        try {
            writeToErrorLog(fullMessage);
        } catch (IOException e) {
            LoggingHelper.error("Failed to write error to log file: " + e.getMessage());
        }
    }

    /**
     * Format an exception with timestamp
     *
     * @param throwable The exception to format
     * @return Formatted exception string
     */
    private String formatException(Throwable throwable) {
        String timestamp = LocalDateTime.now().format(DATE_FORMAT);

        StringWriter sw = new StringWriter();
        PrintWriter pw = new PrintWriter(sw);
        pw.println("[" + timestamp + "]");
        throwable.printStackTrace(pw);
        pw.close();

        return sw.toString();
    }

    /**
     * Write error message to the log file
     *
     * @param message The message to write
     * @throws IOException if file operations fail
     */
    private synchronized void writeToErrorLog(String message) throws IOException {
        if (mLogsPath == null) return;

        File logFile = new File(mLogsPath, ERROR_LOG_FILE);
        File logsDir = logFile.getParentFile();

        // Ensure directory exists
        if (!logsDir.exists() && !logsDir.mkdirs()) {
            throw new IOException("Failed to create logs directory");
        }

        // Rotate log if it's too large
        if (logFile.exists() && logFile.length() > MAX_LOG_SIZE) {
            File rotatedFile = new File(mLogsPath, ERROR_LOG_FILE + ".1");
            if (rotatedFile.exists()) {
                rotatedFile.delete();
            }
            logFile.renameTo(rotatedFile);
        }

        // Append to log file
        try (BufferedWriter writer = new BufferedWriter(new FileWriter(logFile, true))) {
            writer.write(message);
            writer.newLine();
            writer.flush();
        }
    }

    /**
     * Clear the error log file
     *
     * @throws IOException if file operations fail
     */
    public void clearLog() throws IOException {
        if (mLogsPath == null) return;

        File logFile = new File(mLogsPath, ERROR_LOG_FILE);
        if (logFile.exists() && !logFile.delete()) {
            throw new IOException("Failed to delete log file");
        }
    }

    /**
     * Get the path to the error log file
     *
     * @return Path to error log file
     */
    public String getLogFilePath() {
        if (mLogsPath == null) return null;
        return new File(mLogsPath, ERROR_LOG_FILE).getAbsolutePath();
    }
}
