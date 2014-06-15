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

package it.evilsocket.dsploit.net;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.net.HttpURLConnection;
import java.net.URL;
import java.security.InvalidKeyException;

import it.evilsocket.dsploit.core.Logger;

/**
 * This class parses JSON from api.github.com
 * @see <a href="https://developer.github.com/v3/repos/commits/">github documentation</a>
 */
public class GitHubParser {
  private static final String BRANCHES_URL = "https://api.github.com/repos/%s/%s/branches";

  public String mUsername;
  public String mProject;

  private JSONArray mBranches = null;
  private JSONObject mBranch = null;
  private JSONObject mLastCommit = null;

  public GitHubParser(String username, String project) {
    mUsername = username;
    mProject = project;
  }

  private void fetchBranches() throws IOException, JSONException {
    HttpURLConnection connection;
    HttpURLConnection.setFollowRedirects(true);
    URL url = new URL(String.format(BRANCHES_URL, mUsername, mProject));
    connection = (HttpURLConnection) url.openConnection();
    connection.connect();
    int ret = connection.getResponseCode();

    if (ret != 200)
      throw new IOException(String.format("unable to retrieve branches from github: '%s' => %d",
              String.format(BRANCHES_URL, mUsername, mProject),
              ret));

    StringBuilder sb = new StringBuilder();
    BufferedReader reader = new BufferedReader(new InputStreamReader(connection.getInputStream()));
    String line;

    while((line = reader.readLine())!=null)
      sb.append(line);

    mBranches = new JSONArray(sb.toString());
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

  public void reset() {
    mLastCommit = null;
    mBranch = null;
    mBranches = null;
  }

}
