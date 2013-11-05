#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <limits.h> // for PASS_MAX

#if	!(_BSD_SOURCE || \
				(_XOPEN_SOURCE >= 500 || _XOPEN_SOURCE && _XOPEN_SOURCE_EXTENDED) && \
				!(_POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600))
char * getpass(const char *prompt)
{
    struct termios oflags, nflags;
    static char password[PASS_MAX];

    /* disabling echo */
    tcgetattr(fileno(stdin), &oflags);
    nflags = oflags;
    nflags.c_lflag &= ~ECHO;
    nflags.c_lflag |= ECHONL;

    if (tcsetattr(fileno(stdin), TCSANOW, &nflags) != 0) {
        perror("tcsetattr");
        return NULL;
    }

    printf("%s",prompt);
    fgets(password, PASS_MAX, stdin);
    password[strlen(password) - 1] = 0;

    /* restore terminal */
    if (tcsetattr(fileno(stdin), TCSANOW, &oflags) != 0)
        perror("tcsetattr");

    return password;
}
#endif