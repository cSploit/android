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
import android.util.Log;

import java.util.regex.Matcher;
import java.util.regex.Pattern;

import it.evilsocket.dsploit.core.Shell.OutputReceiver;
import it.evilsocket.dsploit.net.Target;

public class Hydra extends Tool {
    public Hydra(Context context) {
        super("hydra/hydra", context);
    }

    public static abstract class AttemptReceiver implements OutputReceiver {
        private final Pattern ATTEMPT_PATTERN = Pattern.compile("^\\[ATTEMPT\\].+login\\s+\"([^\"]+)\".+pass\\s+\"([^\"]+)\"\\s+-\\s+(\\d+)\\s+of\\s+(\\d+)\\s+\\[child\\s+\\d+]$", Pattern.CASE_INSENSITIVE);
        private final Pattern ERROR_PATTERN = Pattern.compile("^\\[Error\\]\\s+(.+)$");
        private final Pattern FATAL_PATTERN = Pattern.compile("^\\[ERROR\\]\\s+(.+)$");
        private final Pattern ACCOUNT_PATTERN = Pattern.compile("^\\[\\d+\\]\\[[^\\]]+\\]\\s+host:\\s+[\\d\\.]+\\s+login:\\s+([^\\s]+)\\s+password:\\s+(.+)", Pattern.CASE_INSENSITIVE);

        public void onStart(String commandLine) {
            Log.d("HYDRA", "Started '" + commandLine + "'");
        }

        public void onNewLine(String line) {
            Matcher matcher = null;

            line = line.trim();

            if ((matcher = ATTEMPT_PATTERN.matcher(line)) != null && matcher.find()) {
                String login = matcher.group(1),
                        password = matcher.group(2),
                        progress = matcher.group(3),
                        total = matcher.group(4);

                int iprogress,
                        itotal;

                try {
                    iprogress = Integer.parseInt(progress);
                    itotal = Integer.parseInt(total);
                } catch (Exception e) {
                    iprogress = 0;
                    itotal = Integer.MAX_VALUE;
                }

                onNewAttempt(login, password, iprogress, itotal);
            } else if ((matcher = ERROR_PATTERN.matcher(line)) != null && matcher.find())
                onError(matcher.group(1));

            else if ((matcher = FATAL_PATTERN.matcher(line)) != null && matcher.find())
                onFatal(matcher.group(1));

            else if ((matcher = ACCOUNT_PATTERN.matcher(line)) != null && matcher.find())
                onAccountFound(matcher.group(1), matcher.group(2));
        }

        public void onEnd(int exitCode) {
            Log.d("Hydra", "Ended with exit code '" + exitCode + "'");
        }

        public abstract void onNewAttempt(String login, String password, int progress, int total);

        public abstract void onError(String message);

        public abstract void onFatal(String message);

        public abstract void onAccountFound(String login, String password);
    }

    public Thread crack(Target target, int port, String service, String charset, int minlength, int maxlength, String username, String userWordlist, String passWordlist, AttemptReceiver receiver) {
        String command = "-F ";

        if (userWordlist != null)
            command += "-L " + userWordlist;

        else
            command += "-l " + username;

        if (passWordlist != null)
            command += " -P " + passWordlist;

        else
            command += " -x \"" + minlength + ":" + maxlength + ":" + charset + "\" ";

        command += "-s " + port + " -V -t 10 " + target.getCommandLineRepresentation() + " " + service;

        if (service.equalsIgnoreCase("http-head"))
            command += " /";

        return super.async(command, receiver);
    }
}
