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

import org.csploit.android.core.*;
import org.csploit.android.core.System;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.IOException;
import java.security.InvalidKeyException;

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
  private JSONArray mReleases = null;
  private JSONObject mBranch = null;
  private JSONObject mLastCommit = null;
  private JSONObject mLastRelease = null;

  private String mTagFilter = null;

  private static GitHubParser msfRepo = new GitHubParser("cSploit", "android.MSF");
  private static GitHubParser cSploitRepo = new GitHubParser("cSploit", "android");
  private static GitHubParser coreRepo = new GitHubParser("cSploit", "android.native");
  private static GitHubParser rubyRepo = new GitHubParser("cSploit", "android.native.ruby");

  public synchronized static GitHubParser getMsfRepo() {
    String customUsername = System.getSettings().getString("MSF_GITHUB_USERNAME", "cSploit");
    String customProject = System.getSettings().getString("MSF_GITHUB_PROJECT", "android.MSF");

    if(!customUsername.equals(msfRepo.username) || !customProject.equals(msfRepo.project)) {
      msfRepo = new GitHubParser(customUsername, customProject);
    }

    msfRepo.mTagFilter = ".*csploit.*";

    return msfRepo;
  }

  public static GitHubParser getcSploitRepo() {
    return cSploitRepo;
  }

  public static GitHubParser getCoreRepo() {
    return coreRepo;
  }

  public static GitHubParser getRubyRepo() {
    return rubyRepo;
  }

  public static void resetAll() {
    cSploitRepo.reset();
    coreRepo.reset();
    rubyRepo.reset();
    msfRepo.reset();
  }

  public GitHubParser(String username, String project) {
    this.username = username;
    this.project = project;
  }

  private void fetchReleases() throws IOException, JSONException {
    JSONArray releases;
    JSONObject release;
    boolean found;

    releases = new JSONArray(
            new String(RemoteReader.fetch(
                    String.format(RELEASES_URL, username, project)
            ))
    );

    mReleases = new JSONArray();
    found = false;

    for(int i=0;i<releases.length();i++) {
      release = releases.getJSONObject(i);

      if(release.getBoolean("draft") || release.getBoolean("prerelease"))
        continue;

      if(mTagFilter != null) {
        String tag = release.getString("tag_name");

        if(!tag.matches(mTagFilter))
          continue;
      }

      if(!found) {
        mLastRelease = release;
        found = true;
      }
      mReleases.put(release);
    }
  }

  private void fetchBranches() throws IOException, JSONException {
    mBranches = new JSONArray(
            new String(RemoteReader.fetch(
                    String.format(BRANCHES_URL, username, project)
            ))
    );
  }

  public synchronized String[] getReleasesTags() throws JSONException, IOException {
    if(mReleases==null)
      fetchReleases();

    String[] result = new String[mReleases.length()];

    for(int i=0; i < mReleases.length(); i++) {
      result[i] = (mReleases.getJSONObject(i)).getString("tag_name");
    }

    return result;
  }

  public synchronized String getReleaseBody(int index) throws JSONException, IOException, IndexOutOfBoundsException {
    if(mReleases==null)
      fetchReleases();

    return mReleases.getJSONObject(index).getString("body");
  }

  public synchronized String getReleaseBody(String tag_name) throws JSONException, IOException {
    JSONObject release;
    String current;
    if(mReleases==null)
      fetchReleases();

    for(int i=0;i<mReleases.length();i++) {
      release = mReleases.getJSONObject(i);
      current = release.getString("tag_name");
      if(current.equals(tag_name) || current.equals("v" + tag_name))
        return release.getString("body");
    }


    throw new JSONException(String.format("release '%s' not found", tag_name));
  }

  public synchronized String getLastReleaseVersion() throws JSONException, IOException {
    if(mLastRelease==null)
      fetchReleases();

    if(mLastRelease==null)
      return null;

    String name = mLastRelease.getString("tag_name");

    return name.startsWith("v") ? name.substring(1) : name;
  }

  public synchronized String getLastReleaseAssetUrl() throws JSONException, IOException {
    if(mLastRelease==null)
      fetchReleases();

    if(mLastRelease==null)
      return null;

    JSONArray assets = mLastRelease.getJSONArray("assets");

    if(assets.length() == 0) {
      return null;
    }

    return assets.getJSONObject(0).getString("browser_download_url");
  }

  /**
   * if the last release of this repo has an assets that match {@code assetFilter}
   * this function will return it's downloadable URL
   *
   * @param assetFilter asset must have this inside it's name
   * @return the found asset URL
   */
  public synchronized String getLastReleaseAssetUrl(String assetFilter) throws JSONException, IOException {
    if(mLastRelease==null)
      fetchReleases();

    if(mLastRelease==null)
      return null;

    JSONArray assets = mLastRelease.getJSONArray("assets");

    for(int i=0; i<assets.length(); i++) {
      JSONObject item = assets.getJSONObject(i);

      if(item.getString("name").contains(assetFilter)) {
        return item.getString("browser_download_url");
      }
    }

    return null;
  }

  public synchronized String[] getBranches() throws JSONException, IOException {
    if(mBranches==null)
      fetchBranches();

    String[] result = new String[mBranches.length()];

    for(int i = 0; i < mBranches.length(); i++) {
      result[i] = (mBranches.getJSONObject(i).getString("name"));
    }

    return result;
  }

  public synchronized void setBranch(String branch) throws InvalidKeyException, JSONException, IOException {
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

  public synchronized String getBranch() throws JSONException {
    if(mBranch == null) {
      Logger.debug("no branch has been selected yet");
      return null;
    }
    return mBranch.getString("name");
  }

  public synchronized String getLastCommitSha() throws JSONException, IllegalStateException {
    if(mLastCommit == null)
      throw new IllegalStateException("no branch has been selected yet");
    return mLastCommit.getString("sha");
  }

  public synchronized String getZipballUrl() throws JSONException, IllegalStateException {
    if(mBranch == null)
      throw new IllegalStateException("no branch has been selected yet");
    return String.format(ZIPBALL_URL, username, project, mBranch.getString("name"));
  }

  public synchronized String getZipballName() throws JSONException, IllegalStateException {
    if(mBranch == null)
      throw new IllegalStateException("no branch has been selected yet");
    return String.format("%s.zip", mBranch.getString("name"));
  }

  public synchronized String getZipballRoot() throws JSONException, IllegalStateException {
    if(mBranch == null)
      throw new IllegalStateException("no branch has been selected yet");
    return String.format("%s-%s/", project, mBranch.getString("name"));
  }

  public synchronized void reset() {
    mLastCommit = null;
    mBranch = null;
    mBranches = null;
    mLastRelease = null;
  }

}
