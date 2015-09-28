package org.csploit.android.net.datasource;

import org.csploit.android.net.RemoteReader;
import org.csploit.android.net.Target;

import java.util.concurrent.Future;

/**
 * Searcher manager
 *
 * it handle multiple searchers
 */
public class Search {
  private static final Rapid7 RAPID7 = new Rapid7();
  private static final ExploitDb EXPLOIT_DB = new ExploitDb();

  public interface Receiver<T> extends RemoteReader.EndReceiver {
    void onItemFound(T item);
    /* TODO make (target|exploit|reference) extends Observable class.
     * so we can drop this method and UI get updated when we call notify() on updated objects.
     */
    void onFoundItemChanged(T item);
  }

  public static Future searchExploitForServices(Target target, Receiver<Target.Exploit> receiver) {
    RemoteReader.Job job = null;

    for(Target.Port p : target.getOpenPorts()) {
      String service = p.getService();

      if(service == null)
        continue;

      if(job == null) {
        job = new RemoteReader.Job(receiver);
      }

      String pref = org.csploit.android.core.System.getSettings().getString("SEARCH_EXDB", "BOTH");

      if(pref.equals("EXDB")) {
        EXPLOIT_DB.beginSearch(job, service, p, receiver);
      } else if(pref.equals("MSF")) {
        RAPID7.beginSearch(job, service, p, receiver);
      } else {
        RAPID7.beginSearch(job, service, p, receiver);
        EXPLOIT_DB.beginSearch(job, service, p, receiver);
      }
    }

    return job;
  }
}
