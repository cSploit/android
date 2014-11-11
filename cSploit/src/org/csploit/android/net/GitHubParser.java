/*
 * This file is part of the dSploit.
 *
 * Copyleft of Simone Margaritelli aka evilsocket <evilsocket@gmail.com>
 *             Massimo Dragano aka tux_mind <massimo.dragano@gmail.com>
 *
 * dSploit is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * dSploit is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with dSploit.  If not, see <http://www.gnu.org/licenses/>.
 */

package org.csploit.android.net;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.net.HttpURLConnection;
import java.net.URL;
import java.security.InvalidKeyException;

import org.csploit.android.core.Logger;

/**
 * This class parses JSON from api.github.com
 * @see <a href="https://developer.github.com/v3/">github documentation</a>
 */
public class GitHubParser {
  private static final String BRANCHES_URL = "https://api.github.com/repos/%s/%s/branches";
  private static final String RELEASES_URL = "https://api.github.com/repos/%s/%s/releases";
  private static final String ZIPBALL_URL  = "https://github.com/%s/%s/archive/%s.zip";

  public final String username;
  public final String project;

  private JSONArray mBranches = null;
  private JSONObject mBranch = null;
  private JSONObject mLastCommit = null;
  private JSONObject mLastRelease = null;

  public GitHubParser(String username, String project) {
    this.username = username;
    this.project = project;
  }

  private String fetchRemoteData(String _url) throws IOException {
    HttpURLConnection connection;
    HttpURLConnection.setFollowRedirects(true);
    URL url = new URL(_url);

    connection = (HttpURLConnection) url.openConnection();

    try {
      connection.connect();
      int ret = connection.getResponseCode();

      if (ret != 200)
        throw new IOException(String.format("unable to fetch remote data: '%s' => %d",
                _url, ret));

      StringBuilder sb = new StringBuilder();
      BufferedReader reader = new BufferedReader(new InputStreamReader(connection.getInputStream()));
      String line;

      while ((line = reader.readLine()) != null)
        sb.append(line);

      return sb.toString();
    } finally {
      connection.disconnect();
    }
  }

  private void fetchLastRelease() throws IOException, JSONException {
    JSONArray releases;
    JSONObject release;

    releases = new JSONArray(
            fetchRemoteData(
                    String.format(RELEASES_URL, username, project)
            )
    );

    for(int i=0;i<releases.length();i++) {
      release = releases.getJSONObject(i);

      if(!release.getBoolean("draft") && !release.getBoolean("prerelease")) {
        mLastRelease = release;
        break;
      }
    }
  }

  private void fetchBranches() throws IOException, JSONException {
    mBranches = new JSONArray(
            fetchRemoteData(
                    String.format(BRANCHES_URL, username, project)
            )
    );
  }

  public String getLastReleaseVersion() throws JSONException, IOException {
    if(mLastRelease==null)
      fetchLastRelease();

    if(mLastRelease==null)
      return null;

    return mLastRelease.getString("tag_name").substring(1);
  }

  public String getLastReleaseAssetUrl() throws JSONException, IOException {
    if(mLastRelease==null)
      fetchLastRelease();

    if(mLastRelease==null)
      return null;

    JSONArray assets = mLastRelease.getJSONArray("assets");

    if(assets.length() != 1) {
      return null;
    }

    return assets.getJSONObject(0).getString("browser_download_url");
  }

  public String[] getBranches() throws JSONException, IOException {
    if(mBranches==null)
      fetchBranches();

    String[] result = new String[mBranches.length()];

    for(int i = 0; i < mBranches.length(); i++) {
      result[i] = (mBranches.getJSONObject(i).getString("name"));
    }

    return result;
  }

  public void setBranch(String branch) throws InvalidKeyException, JSONException, IOException {
    if(mBranches==null)
      fetchBranches();

    for (int i = 0; i < mBranches.length(); i++) {
      if((mBranches.getJSONObject(i)).getString("name").equals(branch)) {
        mBranch = (mBranches.getJSONObject(i));
        mLastCommit = mBranch.getJSONObject("commit");
        return;
      }
    }
    throw new InvalidKeyException("branch '" + branch + "' not found");
  }

  public String getBranch() throws JSONException {
    if(mBranch == null) {
      Logger.debug("no branch has been selected yet");
      return null;
    }
    return mBranch.getString("name");
  }

  public String getLastCommitSha() throws JSONException, IllegalStateException {
    if(mLastCommit == null)
      throw new IllegalStateException("no branch has been selected yet");
    return mLastCommit.getString("sha");
  }

  public String getZipballUrl() throws JSONException, IllegalStateException {
    if(mBranch == null)
      throw new IllegalStateException("no branch has been selected yet");
    return String.format(ZIPBALL_URL, username, project, mBranch.getString("name"));
  }

  public String getZipballName() throws JSONException, IllegalStateException {
    if(mBranch == null)
      throw new IllegalStateException("no branch has been selected yet");
    return String.format("%s.zip", mBranch.getString("name"));
  }

  public String getZipballRoot() throws JSONException, IllegalStateException {
    if(mBranch == null)
      throw new IllegalStateException("no branch has been selected yet");
    return String.format("%s-%s/", project, mBranch.getString("name"));
  }

  public void reset() {
    mLastCommit = null;
    mBranch = null;
    mBranches = null;
    mLastRelease = null;
  }

}
