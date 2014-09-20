/*
 * LICENSE
 */

#ifndef DSPLOITD_H
#define DSPLOITD_H

/// path of the socket
#define SOCKET_PATH "dSploit.sock"

/// path to logfile
#define LOG_PATH "dSploitd.log"

#ifndef NDEBUG
/// path to debug log file
#define DEBUG_LOG_PATH "dSploitd.log.debug"
#endif

void stop_daemon(void);

#endif
