package org.csploit.android.gui.adapters.mitm;

import android.app.Activity;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.AsyncTask;
import android.support.annotation.StringRes;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.ImageView;
import android.widget.TextView;

import org.csploit.android.R;
import org.csploit.android.core.System;
import org.csploit.android.helpers.GUIHelper;
import org.csploit.android.plugins.mitm.hijacker.Hijacker;
import org.csploit.android.plugins.mitm.hijacker.Session;
import org.json.JSONObject;

import java.io.BufferedInputStream;
import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.net.HttpCookie;
import java.net.URL;
import java.net.URLConnection;
import java.util.List;

public class SessionListAdapter extends ArrayAdapter<Session>
  implements Hijacker.HijackerListener {
  private final Hijacker hijacker;
  private final int mLayoutId;
  private final Activity activity;

  private List<Session> list;

  public class FacebookUserTask extends AsyncTask<Session, Void, Boolean> {
    private Bitmap getUserImage(String uri) {
      Bitmap image = null;
      try {
        URL url = new URL(uri);
        URLConnection conn = url.openConnection();
        conn.connect();

        InputStream input = conn.getInputStream();
        BufferedInputStream reader = new BufferedInputStream(input);

        image = Bitmap.createScaledBitmap(
                BitmapFactory.decodeStream(reader), 48, 48, false);

        reader.close();
        input.close();
      } catch (IOException e) {
        System.errorLogging(e);
      }

      return image;
    }

    private String getUserName(String uri) {
      String username = null;

      try {
        URL url = new URL(uri);
        URLConnection conn = url.openConnection();
        conn.connect();

        InputStream input = conn.getInputStream();
        BufferedReader reader = new BufferedReader(
                new InputStreamReader(input));
        String line, data = "";

        while ((line = reader.readLine()) != null)
          data += line;

        reader.close();
        input.close();

        JSONObject response = new JSONObject(data);

        username = response.getString("name");
      } catch (Exception e) {
        System.errorLogging(e);
      }

      return username;
    }

    @Override
    protected Boolean doInBackground(Session... sessions) {
      Session session = sessions[0];
      HttpCookie user = session.mCookies.get("c_user");

      if (user != null) {
        String fbUserId = user.getValue(), fbGraphUrl = "https://graph.facebook.com/"
                + fbUserId + "/", fbPictureUrl = fbGraphUrl
                + "picture";

        session.mUserName = getUserName(fbGraphUrl);
        session.mPicture = getUserImage(fbPictureUrl);
      }

      return true;
    }

    @Override
    protected void onPostExecute(Boolean result) {
      notifyDataSetChanged();
    }
  }

  public class XdaUserTask extends AsyncTask<Session, Void, Boolean> {
    private Bitmap getUserImage(String uri) {
      Bitmap image = null;
      try {
        URL url = new URL(uri);
        URLConnection conn = url.openConnection();
        conn.connect();

        InputStream input = conn.getInputStream();
        BufferedInputStream reader = new BufferedInputStream(input);

        image = Bitmap.createScaledBitmap(
                BitmapFactory.decodeStream(reader), 48, 48, false);

        reader.close();
        input.close();
      } catch (IOException e) {
        System.errorLogging(e);
      }

      return image;
    }

    @Override
    protected Boolean doInBackground(Session... sessions) {
      Session session = sessions[0];
      HttpCookie userid = session.mCookies.get("bbuserid"), username = session.mCookies
              .get("xda_wikiUserName");

      if (userid != null)
        session.mPicture = getUserImage("http://media.xda-developers.com/customavatars/avatar"
                + userid.getValue() + "_1.gif");

      if (username != null)
        session.mUserName = username.getValue().toLowerCase();

      return true;
    }

    @Override
    protected void onPostExecute(Boolean result) {
      notifyDataSetChanged();
    }
  }

  public class SessionHolder {
    ImageView favicon;
    TextView address;
    TextView domain;
  }

  public SessionListAdapter(Activity activity, int layoutId, Hijacker hijacker) {
    super(activity, layoutId);

    this.activity = activity;
    mLayoutId = layoutId;
    this.hijacker = hijacker;

    list = hijacker.getSessions();

    hijacker.setListener(this);
  }

  @Override
  public int getCount() {
    return list.size();
  }

  @Override
  public View getView(int position, View convertView, ViewGroup parent) {
    View row = convertView;
    SessionHolder holder;
    Session session;

    synchronized (this) {
       session = list.get(position);
    }

    if (row == null) {
      LayoutInflater inflater = (LayoutInflater) activity
              .getSystemService(Context.LAYOUT_INFLATER_SERVICE);
      row = inflater.inflate(mLayoutId, parent, false);

      holder = new SessionHolder();

      holder.favicon = (ImageView) (row != null ? row
              .findViewById(R.id.sessionIcon) : null);
      holder.address = (TextView) (row != null ? row
              .findViewById(R.id.sessionTitle) : null);
      holder.domain = (TextView) (row != null ? row
              .findViewById(R.id.sessionDescription) : null);

      if (row != null)
        row.setTag(holder);

    } else
      holder = (SessionHolder) row.getTag();

    if (!session.mInited) {
      session.mInited = true;

      if (session.mDomain.contains("facebook.")
              && session.mCookies.get("c_user") != null)
        new FacebookUserTask().execute(session);

      else if (session.mDomain.contains("xda-developers.")
              && session.mCookies.get("bbuserid") != null)
        new XdaUserTask().execute(session);
    }

    Bitmap picture;

    if (session.mPicture != null)
      picture = session.mPicture;
    else
      picture = BitmapFactory.decodeResource(activity.getResources(),
              getFaviconFromDomain(session.mDomain));

    if (session.mHTTPS)
      picture = GUIHelper.addLogo(picture, BitmapFactory.decodeResource(
              activity.getResources(), R.drawable.https_session));

    if (holder.favicon != null)
      holder.favicon.setImageBitmap(picture);

    if (session.mUserName != null) {
      if (holder.address != null)
        holder.address.setText(session.mUserName);
    } else if (holder.address != null)
      holder.address.setText(session.mAddress);

    if (holder.domain != null)
      holder.domain.setText(session.mDomain);

    return row;
  }

  private static int getFaviconFromDomain(String domain) {
    if (domain.contains("amazon."))
      return R.drawable.favicon_amazon;

    else if (domain.contains("google."))
      return R.drawable.favicon_google;

    else if (domain.contains("youtube."))
      return R.drawable.favicon_youtube;

    else if (domain.contains("blogger."))
      return R.drawable.favicon_blogger;

    else if (domain.contains("tumblr."))
      return R.drawable.favicon_tumblr;

    else if (domain.contains("facebook."))
      return R.drawable.favicon_facebook;

    else if (domain.contains("twitter."))
      return R.drawable.favicon_twitter;

    else if (domain.contains("xda-developers."))
      return R.drawable.favicon_xda;

    else
      return R.drawable.favicon_generic;
  }

  @Override
  public void onSessionsChanged() {
    synchronized (this) {
      list = hijacker.getSessions();
    }
    notifyDataSetChanged();
  }

  @Override
  public void onStateChanged() { }

  @Override
  public void onError(String message) { }

  @Override
  public void onError(@StringRes int stringId) { }
}

