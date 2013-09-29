package it.evilsocket.dsploit.core;

import android.util.Log;

public class Logger
{
  private static final String TAG = "DSPLOIT";

  private static void log( int priority, String message ) {
    StackTraceElement[] els = Thread.currentThread().getStackTrace();
    StackTraceElement caller = null;

    for( int i = 0; i < els.length; i++ ){
      // search for the last stack frame in our namespace
      if( els[i].getClassName().startsWith("it.evilsocket.dsploit") ){
        caller = els[i];
      }
    }

    String Tag = TAG + "[" + caller.getClassName().replace( "it.evilsocket.dsploit.", "" ) + "." + caller.getMethodName() + "]";

    Log.println( priority, Tag, message );
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
