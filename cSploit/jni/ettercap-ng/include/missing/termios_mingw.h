#if !defined(FAKE_TERMIOS_FOR_WINDOWS_H) && defined(_WIN32)
#define FAKE_TERMIOS_FOR_WINDOWS_H

#define ECHO     0x01
#define VTIME    0x00
#define ICANON   0x01
#define TCSANOW  0x02
#define NCCS     12

struct termios {
       unsigned  c_lflag;
       unsigned  c_cc [NCCS];
     };

#define tcgetattr(x,y)     ((void)0)
#define tcsetattr(x,y,z)   ((void)0)

#endif
