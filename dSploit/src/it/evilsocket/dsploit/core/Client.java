package it.evilsocket.dsploit.core;

/**
 * the dSploitd client
 */
public class Client {

  static {
    java.lang.System.loadLibrary("dSploitCommon");
    java.lang.System.loadLibrary("dSploitClient");
  }

  native static boolean Login(String username, String pswdHash);

  native static boolean isAuthenticated();

  native static boolean LoadHandlers();

  native static String[] getHandlers();

  native static int StartCommand(String handler, String cmd);

  native static boolean Connect(String path);

  native static boolean isConnected();

  native static void Disconnect();

  native static void Shutdown();

  native static void Kill(int id, int signal);

  native static boolean SendTo(int id, byte[] array);

  static int StartCommand(String cmd) {
    return StartCommand("blind", cmd);
  }
}
