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
package org.csploit.android.plugins;

import android.content.SharedPreferences;
import android.os.Bundle;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.Spinner;
import android.widget.Toast;
import android.widget.ToggleButton;

import org.csploit.android.R;
import org.csploit.android.core.Plugin;
import org.csploit.android.core.System;
import org.csploit.android.gui.dialogs.ErrorDialog;
import org.csploit.android.net.Endpoint;
import org.csploit.android.net.Target;
import org.csploit.android.net.Target.Type;

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.Socket;

public class PacketForger extends Plugin implements OnClickListener {
	private static final int TCP_PROTOCOL = 0;
	private static final int UDP_PROTOCOL = 1;
	private static final String[] PROTOCOLS = new String[] { "TCP", "UDP" };
	private Spinner mProtocol = null;
	private EditText mPort = null;
	private CheckBox mWaitResponse = null;
	private EditText mData = null;
	private byte[] mBinaryData = null;
	private EditText mResponse = null;
	private ToggleButton mSendButton = null;
	private boolean mRunning = false;
	private Thread mThread = null;
	private Socket mSocket = null;
	private DatagramSocket mUdpSocket = null;

	public PacketForger() {
		super(R.string.packet_forger, R.string.packet_forger_desc,

		new Target.Type[] { Target.Type.ENDPOINT, Target.Type.REMOTE },
				R.layout.plugin_packet_forger, R.drawable.action_forge);
	}

	@Override
	public void onCreate(Bundle savedInstanceState) {
		SharedPreferences themePrefs = getSharedPreferences("THEME", 0);
		Boolean isDark = themePrefs.getBoolean("isDark", false);
		if (isDark)
			setTheme(R.style.DarkTheme);
		else
			setTheme(R.style.AppTheme);
		super.onCreate(savedInstanceState);

		mProtocol = (Spinner) findViewById(R.id.protocolSpinner);
		mPort = (EditText) findViewById(R.id.portText);
		mWaitResponse = (CheckBox) findViewById(R.id.responseCheckBox);
		mData = (EditText) findViewById(R.id.dataText);
		mResponse = (EditText) findViewById(R.id.responseText);
		mSendButton = (ToggleButton) findViewById(R.id.sendButton);
		Button mSendWOL = (Button) findViewById(R.id.sendWOL);

		if (System.getCurrentTarget().getType() != Type.ENDPOINT)
			mSendWOL.setVisibility(View.INVISIBLE);

		mProtocol.setAdapter(new ArrayAdapter<String>(this,
				android.R.layout.simple_spinner_item, PROTOCOLS));

		mSendButton.setOnClickListener(this);
		mSendWOL.setOnClickListener(this);
	}

	@Override
	public void onClick(View v) {
		if (!mRunning) {
			if (v.getId() == R.id.sendButton) {
				mResponse.setText("");

				mRunning = true;

				mThread = new Thread(new Runnable() {
					@Override
					public void run() {
						int protocol = mProtocol.getSelectedItemPosition(), port = -1;
						String data = mData.getText() + "", error = null;
						boolean waitResponse = mWaitResponse.isChecked();

						try {
							port = Integer.parseInt(mPort.getText() + "".trim());
							if (port <= 0 || port > 65535)
								port = -1;
						} catch (Exception e) {
							port = -1;
						}

						if (port == -1)
							error = getString(R.string.invalid_port);

						else if (data.isEmpty())
							error = getString(R.string.request_empty);

						else {
							try {
								if (protocol == TCP_PROTOCOL) {
									mSocket = new Socket(System
											.getCurrentTarget()
											.getCommandLineRepresentation(),
											port);
									OutputStream writer = mSocket
											.getOutputStream();

									writer.write(data.getBytes());
									writer.flush();

									if (waitResponse) {
										BufferedReader reader = new BufferedReader(
												new InputStreamReader(mSocket
														.getInputStream()));
										String response = "", line = null;

										while ((line = reader.readLine()) != null) {
											response += line + "\n";
										}

										final String text = response;
										PacketForger.this
												.runOnUiThread(new Runnable() {
													public void run() {
														mResponse.setText(text);
													}
												});

										reader.close();
									}

									writer.close();
									mSocket.close();
								} else if (protocol == UDP_PROTOCOL) {
									mUdpSocket = new DatagramSocket();
									DatagramPacket packet = null;

									if (mBinaryData != null)
										packet = new DatagramPacket(
												mBinaryData,
												mBinaryData.length, System
														.getCurrentTarget()
														.getAddress(), port);
									else
										packet = new DatagramPacket(data
												.getBytes(), data.length(),
												System.getCurrentTarget()
														.getAddress(), port);

									mUdpSocket.send(packet);

									if (waitResponse) {
										byte[] buffer = new byte[1024];

										DatagramPacket response = new DatagramPacket(
												buffer, buffer.length);

										mUdpSocket.receive(response);

										final String text = new String(buffer);
										PacketForger.this
												.runOnUiThread(new Runnable() {
													public void run() {
														mResponse.setText(text);
													}
												});
									}

									mUdpSocket.close();
								}
							} catch (Exception e) {
								error = e.getMessage();
							}
						}

						mBinaryData = null;

						final String errorMessage = error;
						PacketForger.this.runOnUiThread(new Runnable() {
							public void run() {
								Toast.makeText(PacketForger.this,
										getString(R.string.request_sent),
										Toast.LENGTH_SHORT).show();
								setStoppedState(errorMessage);
							}
						});
					}
				});

				mThread.start();
			} else {
				Endpoint endpoint = System.getCurrentTarget().getEndpoint();

				byte[] mac = endpoint.getHardware();
				int i;

				if (mac != null) {
					mResponse.setText("");
					mProtocol.setSelection(UDP_PROTOCOL);
					mPort.setText("9");

					mBinaryData = new byte[6 + 16 * mac.length];

					for (i = 0; i < 6; i++) {
						mBinaryData[i] = (byte) 0xFF;
					}

					for (i = 6; i < mBinaryData.length; i += mac.length) {
						java.lang.System.arraycopy(mac, 0, mBinaryData, i,
								mac.length);
					}

					String hex = "";

					for (i = 0; i < mBinaryData.length; i++)
						hex += "\\x"
								+ Integer.toHexString(0xFF & mBinaryData[i])
										.toUpperCase();

					mData.setText(hex);

					Toast.makeText(this,
							getString(R.string.customize_wol_port),
							Toast.LENGTH_SHORT).show();
				} else
					Toast.makeText(this,
							getString(R.string.couldnt_send_wol_packet),
							Toast.LENGTH_SHORT).show();
			}
		} else {
			setStoppedState(null);
		}
	}

	private void setStoppedState(String errorMessage) {
		mSendButton.setChecked(false);
		mRunning = false;
		try {
			if (mThread != null && mThread.isAlive()) {
				if (mSocket != null)
					mSocket.close();

				if (mUdpSocket != null)
					mUdpSocket.close();

				mThread.stop();
				mThread = null;
				mRunning = false;
			}
		} catch (Exception e) {

		}

		if (errorMessage != null && !isFinishing())
			new ErrorDialog(getString(R.string.error), errorMessage, this)
					.show();
	}

	@Override
	public void onBackPressed() {
		setStoppedState(null);
		super.onBackPressed();
		overridePendingTransition(R.anim.fadeout, R.anim.fadein);
	}
}
