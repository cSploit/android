package org.csploit.android.core;

/**
 * the cSploitd client
 */
public class Client {

  static {
    java.lang.System.loadLibrary("cSploitCommon");
    java.lang.System.loadLibrary("cSploitClient");
  }

  native static boolean Login(String username, String pswdHash);

  native static boolean isAuthenticated();

  native static boolean LoadHandlers();

  native static String[] getHandlers();

  native static int StartCommand(String handler, String cmd, String[] env);

  native static boolean Connect(String path);

  native static boolean isConnected();

  native static void Disconnect();

  native static void Shutdown();

  native static void Kill(int id, int signal);

  native static boolean SendTo(int id, byte[] array);

  public native static boolean hadCrashed();
}
