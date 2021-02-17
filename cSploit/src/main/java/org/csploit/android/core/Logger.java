package org.csploit.android.core;

import android.util.Log;

public class Logger
{
  private static final String TAG = "CSPLOIT";
  private static final String mClassName = Logger.class.getName();

  private static void log( int priority, String message ) {
    StackTraceElement[] els = Thread.currentThread().getStackTrace();
    String className, methodName;

    className = methodName = "unknown";

    for( StackTraceElement element : els) {
      String currClassName = element.getClassName();
      if(currClassName.startsWith("org.csploit.android.") && !currClassName.equals(mClassName)) {
        className = currClassName.replace("org.csploit.android.", "");
        methodName = element.getMethodName();
        break;
      }
    }

    if(message == null)
      message = "(null)";

    Log.println( priority, TAG + "[" + className + "." + methodName + "]", message );
  }

  public static void debug( String message ){
    log( Log.DEBUG, message );
  }

  public static void info( String message ){
    log( Log.INFO, message );
  }

  public static void warning( String message ){
    log( Log.WARN, message );
  }

  public static void error( String message ){
    log( Log.ERROR, message );
  }
}
