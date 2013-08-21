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
package it.evilsocket.dsploit.tools;

import android.content.Context;

import it.evilsocket.dsploit.core.Shell.OutputReceiver;
import it.evilsocket.dsploit.core.System;
import it.evilsocket.dsploit.net.Target;
import it.evilsocket.dsploit.net.Target.Type;

public class ArpSpoof extends Tool {
    private static final String TAG = "ARPSPOOF";

    public ArpSpoof(Context context) {
        super("arpspoof/arpspoof", context);
    }

    public Thread spoof(Target target, OutputReceiver receiver) {
        String commandLine = "";

        try {
            if (target.getType() == Type.NETWORK)
                commandLine = "-i " + System.getNetwork().getInterface().getDisplayName() + " " + System.getGatewayAddress();

            else
                commandLine = "-i " + System.getNetwork().getInterface().getDisplayName() + " -t " + target.getCommandLineRepresentation() + " " + System.getGatewayAddress();
        } catch (Exception e) {
            System.errorLogging(TAG, e);
        }

        return super.asyncStatic(commandLine, receiver);
    }

    public boolean kill() {
        // arpspoof needs SIGINT ( ctrl+c ) to restore arp table.
        return super.kill("SIGINT");
    }
}