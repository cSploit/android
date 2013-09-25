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
package it.evilsocket.dsploit.plugins;

import android.os.Bundle;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.ToggleButton;

import it.evilsocket.dsploit.R;
import it.evilsocket.dsploit.core.Plugin;
import it.evilsocket.dsploit.core.System;
import it.evilsocket.dsploit.net.Network;
import it.evilsocket.dsploit.net.Target;
import it.evilsocket.dsploit.net.Target.Port;
import it.evilsocket.dsploit.tools.NMap.InspectionReceiver;

public class Inspector extends Plugin {

    private ToggleButton mStartButton = null;
    private ProgressBar mActivity = null;
    private TextView mDeviceType = null;
    private TextView mDeviceOS = null;
    private TextView mDeviceServices = null;
    private boolean mRunning = false;
    private Receiver mReceiver = null;

    public Inspector() {
        super(
                "Inspector",
                "Perform target operating system and services deep detection (slower than port scanner, but more accurate).",
                new Target.Type[]{Target.Type.ENDPOINT, Target.Type.REMOTE},
                R.layout.plugin_inspector,
                R.drawable.action_inspect
        );
    }

    private void setStoppedState() {
        System.getNMap().kill();
        mActivity.setVisibility(View.INVISIBLE);
        mRunning = false;
        mStartButton.setChecked(false);
    }

    private void setStartedState() {
        mActivity.setVisibility(View.VISIBLE);
        mRunning = true;

        System.getNMap().inpsect(System.getCurrentTarget(), mReceiver).start();
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        mStartButton = (ToggleButton) findViewById(R.id.inspectToggleButton);
        mActivity = (ProgressBar) findViewById(R.id.inspectActivity);
        TextView mDeviceName = (TextView) findViewById(R.id.deviceName);
        mDeviceType = (TextView) findViewById(R.id.deviceType);
        mDeviceOS = (TextView) findViewById(R.id.deviceOS);
        mDeviceServices = (TextView) findViewById(R.id.deviceServices);

        mDeviceName.setText(System.getCurrentTarget().toString());

        if (System.getCurrentTarget().getDeviceType() != null)
            mDeviceType.setText(System.getCurrentTarget().getDeviceType());

        if (System.getCurrentTarget().getDeviceOS() != null)
            mDeviceOS.setText(System.getCurrentTarget().getDeviceOS());

        if (System.getCurrentTarget().hasOpenPortsWithService()) {
            mDeviceServices.setText("");

            for (Port port : System.getCurrentTarget().getOpenPorts()) {
                if (port.service != null && !port.service.isEmpty()) {
                    mDeviceServices.append(port.number + " ( " + port.protocol.toString().toLowerCase() + " ) : " + port.service + "\n");
                }
            }
        }

        mStartButton.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                if (mRunning) {
                    setStoppedState();
                } else {
                    setStartedState();
                }
            }
        }
        );

        mReceiver = new Receiver();
    }

    @Override
    public void onBackPressed() {
        setStoppedState();
        super.onBackPressed();
    }

    private class Receiver extends InspectionReceiver {
        @Override
        public void onServiceFound(final int port, final String protocol, final String service) {
            final boolean hasServiceDescription = !service.trim().isEmpty();

            Inspector.this.runOnUiThread(new Runnable() {
                @SuppressWarnings("ConstantConditions")
                @Override
                public void run() {
                    if (mDeviceServices.getText().equals("unknown"))
                        mDeviceServices.setText("");

                    if (hasServiceDescription)
                        mDeviceServices.append(port + " ( " + protocol + " ) : " + service + "\n");
                    else
                        mDeviceServices.append(port + " ( " + protocol + " )\n");
                }
            });

            if (hasServiceDescription)
                System.addOpenPort(port, Network.Protocol.fromString(protocol), service);
            else
                System.addOpenPort(port, Network.Protocol.fromString(protocol));
        }

        @Override
        public void onOpenPortFound(int port, String protocol) {
            System.addOpenPort(port, Network.Protocol.fromString(protocol));
        }

        @Override
        public void onOsFound(final String os) {
            System.getCurrentTarget().setDeviceOS(os);

            Inspector.this.runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    mDeviceOS.setText(os);
                }
            });
        }

        @Override
        public void onGuessOsFound(final String os) {
            System.getCurrentTarget().setDeviceOS(os);

            Inspector.this.runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    mDeviceOS.setText(os);
                }
            });
        }

        @Override
        public void onDeviceFound(final String device) {
            System.getCurrentTarget().setDeviceType(device);

            Inspector.this.runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    mDeviceType.setText(device);
                }
            });
        }

        @Override
        public void onServiceInfoFound(final String info) {
            System.getCurrentTarget().setDeviceOS(info);

            Inspector.this.runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    mDeviceOS.setText(info);
                }
            });
        }

        @Override
        public void onEnd(int code) {
            Inspector.this.runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    if (mRunning)
                        setStoppedState();
                }
            });
        }
    }
}
