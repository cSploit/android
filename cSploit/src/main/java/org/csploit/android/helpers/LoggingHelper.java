package org.csploit.android.helpers;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import android.util.Log;

import java.io.PrintWriter;
import java.io.StringWriter;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;

/**
 * Enhanced logging utility with structured logging capabilities.
 * Provides methods for consistent logging across the application.
 */
public final class LoggingHelper {

    private static final String TAG = "cSploit";
    private static final SimpleDateFormat dateFormat = 
        new SimpleDateFormat("yyyy-MM-dd HH:mm:ss.SSS", Locale.US);

    /**
     * Log level for categorizing messages
     */
    public enum LogLevel {
        VERBOSE(Log.VERBOSE, "V"),
        DEBUG(Log.DEBUG, "D"),
        INFO(Log.INFO, "I"),
        WARN(Log.WARN, "W"),
        ERROR(Log.ERROR, "E");

        public final int level;
        public final String letter;

        LogLevel(int level, String letter) {
            this.level = level;
            this.letter = letter;
        }
    }

    /**
     * Log a verbose message
     */
    public static void v(@NonNull String tag, @NonNull String message) {
        log(LogLevel.VERBOSE, tag, message, null);
    }

    /**
     * Log a debug message
     */
    public static void d(@NonNull String tag, @NonNull String message) {
        log(LogLevel.DEBUG, tag, message, null);
    }

    /**
     * Log an info message
     */
    public static void i(@NonNull String tag, @NonNull String message) {
        log(LogLevel.INFO, tag, message, null);
    }

    /**
     * Log a warning message
     */
    public static void w(@NonNull String tag, @NonNull String message) {
        log(LogLevel.WARN, tag, message, null);
    }

    /**
     * Log an error message
     */
    public static void e(@NonNull String tag, @NonNull String message) {
        log(LogLevel.ERROR, tag, message, null);
    }

    /**
     * Log an error with exception
     */
    public static void e(@NonNull String tag, @NonNull String message, @NonNull Throwable throwable) {
        log(LogLevel.ERROR, tag, message, throwable);
    }

    /**
     * Log an exception
     */
    public static void logException(@NonNull String tag, @NonNull Throwable throwable) {
        e(tag, "Exception occurred", throwable);
    }

    /**
     * Core logging method
     */
    private static void log(
            @NonNull LogLevel level,
            @NonNull String tag,
            @NonNull String message,
            @Nullable Throwable throwable) {

        String logMessage = String.format(
            "%s [%s] %s: %s",
            getTimestamp(),
            level.letter,
            tag,
            message
        );

        // Log to Android logger
        if (throwable != null) {
            Log.println(level.level, TAG, logMessage + "\n" + getStackTrace(throwable));
        } else {
            Log.println(level.level, TAG, logMessage);
        }
    }

    /**
     * Get current timestamp
     */
    @NonNull
    private static String getTimestamp() {
        return dateFormat.format(new Date());
    }

    /**
     * Get stack trace from exception
     */
    @NonNull
    private static String getStackTrace(@NonNull Throwable throwable) {
        StringWriter sw = new StringWriter();
        throwable.printStackTrace(new PrintWriter(sw));
        return sw.toString();
    }

    /**
     * Log method entry (for debugging)
     */
    public static void logMethodEntry(@NonNull String tag, @NonNull String methodName) {
        d(tag, ">>> " + methodName);
    }

    /**
     * Log method exit (for debugging)
     */
    public static void logMethodExit(@NonNull String tag, @NonNull String methodName) {
        d(tag, "<<< " + methodName);
    }

    /**
     * Log a value for debugging
     */
    public static void logValue(@NonNull String tag, @NonNull String key, @Nullable Object value) {
        d(tag, key + " = " + (value != null ? value.toString() : "null"));
    }

    /**
     * Log performance metric
     */
    public static void logPerformance(
            @NonNull String tag,
            @NonNull String operation,
            long durationMs) {
        
        d(tag, operation + " took " + durationMs + "ms");
    }

    /**
     * Create a formatted performance message
     */
    @NonNull
    public static String formatPerformance(@NonNull String operation, long durationMs) {
        return operation + " took " + durationMs + "ms";
    }

    /**
     * Safely log an object (null-safe)
     */
    public static void logObject(
            @NonNull String tag,
            @NonNull String name,
            @Nullable Object obj) {
        
        d(tag, name + " = " + (obj != null ? obj : "null"));
    }

    /**
     * Log collection size
     */
    public static void logCollectionSize(
            @NonNull String tag,
            @NonNull String name,
            @Nullable java.util.Collection<?> collection) {

        d(tag, name + " size = " + (collection != null ? collection.size() : 0));
    }

    /**
     * Log formatted message (like String.format)
     */
    public static void d(@NonNull String tag, @NonNull String format, Object... args) {
        try {
            d(tag, String.format(format, args));
        } catch (Exception e) {
            d(tag, format + " [formatting error]");
        }
    }

    /**
     * Log formatted warning
     */
    public static void w(@NonNull String tag, @NonNull String format, Object... args) {
        try {
            w(tag, String.format(format, args));
        } catch (Exception e) {
            w(tag, format + " [formatting error]");
        }
    }

    /**
     * Log formatted error
     */
    public static void e(@NonNull String tag, @NonNull String format, Object... args) {
        try {
            e(tag, String.format(format, args));
        } catch (Exception e) {
            e(tag, format + " [formatting error]");
        }
    }

    /**
     * Create a performance timer for measuring code execution
     */
    public static class PerformanceTimer {
        private final String tag;
        private final String operation;
        private final long startTime;

        public PerformanceTimer(@NonNull String tag, @NonNull String operation) {
            this.tag = tag;
            this.operation = operation;
            this.startTime = System.currentTimeMillis();
        }

        /**
         * Log the elapsed time since timer creation
         */
        public void logElapsed() {
            long elapsed = System.currentTimeMillis() - startTime;
            logPerformance(tag, operation, elapsed);
        }

        /**
         * Get elapsed time without logging
         */
        public long getElapsed() {
            return System.currentTimeMillis() - startTime;
        }
    }

    /**
     * Thread-aware logging that includes thread name
     */
    public static void logThread(@NonNull String tag, @NonNull String message) {
        String threadInfo = "Thread[" + Thread.currentThread().getName() + "]";
        d(tag, threadInfo + " " + message);
    }

    /**
     * Log with context (tag auto-generated from calling class)
     */
    public static void contextLog(@NonNull String message) {
        StackTraceElement ste = Thread.currentThread().getStackTrace()[2];
        String tag = ste.getClassName().substring(ste.getClassName().lastIndexOf('.') + 1);
        d(tag, message);
    }
}
