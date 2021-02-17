#include <stdio.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <linux/un.h>
#include <sys/poll.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include <signal.h>

int do_gc = 0;

void sigalarm(int dummy) {
	do_gc = 1;
}

struct linfo {
	int type;
	time_t ok_stamp;
};

#define MAX_CONNS	1024
struct pollfd poll_array[MAX_CONNS];
int poll_used = 0;
struct linfo poll_infos[MAX_CONNS];

struct sockaddr_un clnt;
socklen_t clntlen;

void do_unix_gc(void) {
	int fd;
	int i;
		
	fd = socket(PF_UNIX, SOCK_DGRAM, 0);
	if (fd == -1)
		return;
	for (i = 0; i < poll_used; ) {
		if (poll_infos[i].type == 2) {
			struct sockaddr_un peer;
			socklen_t pname;
			int err;
		
			pname = sizeof(peer);
			err = getpeername(poll_array[i].fd, (struct sockaddr*)&peer, &pname);
			if (!err) {
				if (!bind(fd, (struct sockaddr*)&peer, pname)) {
					peer.sun_family = AF_UNSPEC;
					bind(fd, (struct sockaddr*)&peer, pname);
					close(poll_array[i].fd);
					printf("Garbage-collected entry %d\n", i);
					poll_used--;
					if (i < poll_used) {
						poll_array[i] = poll_array[poll_used];
						poll_infos[i] = poll_infos[poll_used];
					}
					continue;
				}
			}
		}
		i++;
	}
	close(fd);
}

int unix_got(int item) {
	unsigned char buffer[2000];
	struct msghdr msg;
	unsigned char ctrl[2000];
	struct iovec io[1];
	int ret;
				
	msg.msg_name = &clnt;
	msg.msg_namelen = sizeof(clnt);
	msg.msg_control = ctrl;
	msg.msg_controllen = sizeof(ctrl);
	msg.msg_flags = 0;
	msg.msg_iovlen = 1;
	msg.msg_iov = io;
	io[0].iov_base = buffer;
	io[0].iov_len = sizeof(buffer);
				
	ret = recvmsg(poll_array[item].fd, &msg, MSG_DONTWAIT);
	if (ret < 0) {
		perror("UNIX false alarm");
	} else if (ret == 0) {
		perror("UNIX zero data");
	} else {
		struct cmsghdr* cmsg;
		int sv[2];
		int err;
		struct ucred * cred = NULL;
					
		clntlen = msg.msg_namelen;
		printf("%d bytes received from UNIX...\n", ret);
		printf("%d bytes cmsg data\n", msg.msg_controllen);
		printf("%d bytes name data\n", msg.msg_namelen);
		printf("%08X flags\n", msg.msg_flags);
					
		for (cmsg = CMSG_FIRSTHDR(&msg); cmsg; cmsg = CMSG_NXTHDR(&msg, cmsg)) {
			printf("cmsg len/level/type: %d/%d/%d\n",
				cmsg->cmsg_len, cmsg->cmsg_level, cmsg->cmsg_type);
			if (cmsg->cmsg_level == SOL_SOCKET) {
				switch (cmsg->cmsg_type) {
					case SCM_RIGHTS:	/* someone sent filedescriptor to us... 
								 * we could run out of fds, so close it...
								 */
								close(*(int*)CMSG_DATA(cmsg));
								break;
					case SCM_CREDENTIALS:	cred = (struct ucred*)CMSG_DATA(cmsg);
								break;
				}
			}
		}

		if (cred) {
			printf("PID=%u, UID=%u, GID=%u\n", cred->pid, cred->uid, cred->gid);
		}
#if 0		
		if (!cred) {
			err = EACCES;
			goto sendreply;
		}
		
		
		
sendreply:;
		{
			struct msghdr replymsg;
			struct iovec iov[5];
			
			replymsg.msg_name = msg.msg_name;
			replymsg.msg_namelen = msg.msg_namelen;
			replymsg.msg_flags = 0;
			replymsg.msg_iovlen = 1;
			replymsg.msg_iov = iov;
			replymsg.msg_control = NULL;
			replymsg.msg_controllen = 0;
			if (err) {
				int32_t err2;
			
				err2 = htonl(err);
				iov[0].iov_base = &err2;
				iov[0].iov_len = sizeof(err2);
			} else {
			}
			ret = sendmsg(poll_array[item].fd, &replymsg, MSG_DONTWAIT);
#endif			
					
		err = socketpair(PF_UNIX, SOCK_DGRAM, 0, sv);
		if (err)
			perror("socketpair(PF_UNIX, SOCK_DGRAM, 0, &sv)");
		else {
			union {
				struct cmsghdr cmsg;
				unsigned char rsvd[2000];
			} scmout;
			struct msghdr replymsg;
			
			scmout.cmsg.cmsg_level = SOL_SOCKET;
			scmout.cmsg.cmsg_type = SCM_RIGHTS;
			scmout.cmsg.cmsg_len = CMSG_LEN(sizeof(int));
			*(int*)(CMSG_DATA(&scmout.cmsg)) = sv[1];
			
			buffer[0] = 'Y';
			io[0].iov_base = buffer;
			io[0].iov_len = 1;
			replymsg.msg_namelen = msg.msg_namelen;
			replymsg.msg_name = msg.msg_name;
			replymsg.msg_control = &scmout;
			replymsg.msg_controllen = scmout.cmsg.cmsg_len;
			replymsg.msg_flags = 0;
			replymsg.msg_iovlen = 1;
			replymsg.msg_iov = io;
			
			ret = sendmsg(poll_array[item].fd, &replymsg, MSG_DONTWAIT);
			close(sv[1]);
			if (ret < 0)
				close(sv[0]);
			else {
				poll_array[poll_used].fd = sv[0];
				poll_array[poll_used].events = POLLIN | POLLPRI | POLLERR | POLLHUP | POLLNVAL;
				poll_infos[poll_used].type = 2;
				poll_used++;
			}
		}
	}
	return -1;
}

int inet_got(int item) {
	struct sockaddr_in unaddr;
	socklen_t alen;
	unsigned char buffer[2000];
	int ret;
	int rval = -1;

	alen = sizeof(unaddr);
	ret = recvfrom(poll_array[item].fd, buffer, sizeof(buffer), MSG_DONTWAIT, (struct sockaddr*)&unaddr, &alen);
	if (ret < 0) {
		perror("INET false alarm");
	} else if (ret == 0) {
		perror("INET zero data");
	} else {
		printf("%d bytes received from network...\n", ret);
		ret = send(poll_array[2].fd, buffer, ret, MSG_DONTWAIT);
		if (ret < 0) {
			switch (errno) {
				case ECONNREFUSED:
				case ENOTCONN:
					rval = 2;
					break;
				default:
					perror("PAIR send");
			}
		}
	}
	return -1;
}

int pair_got(int item) {
	int ret;
	unsigned char buffer[2000];
	
	printf("Got message on socketpair %d: %08X\n", item, poll_array[item].revents);
	ret = recv(poll_array[item].fd, buffer, sizeof(buffer), MSG_DONTWAIT);
	if (ret < 0) {
		perror("PAIR false alarm");
	} else if (ret == 0) {
		perror("PAIR zero data");
	} else {
		ret = send(poll_array[1].fd, buffer, ret, 0);
		if (ret < 0)
			perror("INET send");
	}
	return -1;
}

int main(int argc, char* argv[]) {
	{
		int fd_unix;
		static int val = 1;
		struct sockaddr_un unaddr;
		
		fd_unix = socket(PF_UNIX, SOCK_DGRAM, 0);
		if (fd_unix == -1) {
			perror("socket(PF_UNIX, SOCK_DGRAM, 0)");
			return 1;
		}
		if (setsockopt(fd_unix, SOL_SOCKET, SO_PASSCRED, &val, sizeof(val)) == -1) {
			perror("setsockopt(unix, SOL_SOCKET, SO_PASSCRED, &1, sizeof(int))");
			return 1;
		}
		unaddr.sun_family = AF_UNIX;
		memcpy(unaddr.sun_path, "\000ncpfs", 6);
		if (bind(fd_unix, (struct sockaddr*)&unaddr, sizeof(short) + 6) == -1) {
			perror("bind(unix, @ncpfs, 8)");
			return 1;
		}
		poll_array[poll_used].fd = fd_unix;
		poll_array[poll_used].events = POLLIN;
		poll_infos[poll_used].type = 0;
		poll_used++;
	}
	{
		int fd_inet;
		struct sockaddr_in inetaddr;
		int err;
		
		fd_inet = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (fd_inet == -1) {
			perror("socket(PF_INET, SOCK_DGRAM, 0)");
			return 1;
		}
		inetaddr.sin_family = AF_INET;
		inetaddr.sin_port = htons(0);
		inetaddr.sin_addr.s_addr = htonl(INADDR_ANY);
		err = bind(fd_inet, (struct sockaddr*)&inetaddr, sizeof(inetaddr));
		if (err) {
			perror("bind(inet, &INADDR_ANY, x)");
			return 1;
		}
		inetaddr.sin_port = htons(524);
		inetaddr.sin_addr.s_addr = htonl(0xC0A81A05);
		err = connect(fd_inet, (struct sockaddr*)&inetaddr, sizeof(inetaddr));
		if (err) {
			perror("connect(inet, &192.168.26.5, x)");
			return 1;
		}
		poll_array[poll_used].fd = fd_inet;
		poll_array[poll_used].events = POLLIN;
		poll_infos[poll_used].type = 1;
		poll_used++;
	}
	{
		struct itimerval val;
		struct sigaction sigact;
		
		sigact.sa_handler = sigalarm;
		sigact.sa_flags = 0 /* | SA_RESTART */;
		sigemptyset(&sigact.sa_mask);
		sigaction(SIGALRM, &sigact, NULL);
		
		val.it_interval.tv_sec = 60;
		val.it_interval.tv_usec = 0;
		val.it_value.tv_sec = 60;
		val.it_value.tv_usec = 0;
		if (setitimer(ITIMER_REAL, &val, NULL)) {
			perror("setitimer(ITIMER_REAL,60sec)");
		}
	}
	
	while (1) {
		int ret;

		ret = poll(poll_array, poll_used, -1);
		printf("poll returned %d\n", ret);
		if (ret < 0) {
			perror("poll");
		} else if (ret > 0) {
			int cnt;
			
			for (cnt = 0; cnt < poll_used; ) {
				int del = -1;
				
				if (poll_array[cnt].revents & poll_array[cnt].events) {
					switch (poll_infos[cnt].type) {
						case 0:	del = unix_got(cnt); break;
						case 1: del = inet_got(cnt); break;
						case 2: del = pair_got(cnt); break;
					}
				}
				if (del == -1) {
					cnt++;
				} else {
					close(poll_array[del].fd);
					if (del < cnt) {
						poll_array[del] = poll_array[cnt];
						poll_infos[del] = poll_infos[cnt];
						del = cnt;
					}
					poll_used--;
					if (del < poll_used) {
						poll_array[del] = poll_array[poll_used];
						poll_infos[del] = poll_infos[poll_used];
					}
					if (del != cnt)
						cnt++;
				}
			}
		}
		if (do_gc) {
			do_gc = 0;
			printf("Doing garbage collection\n");
			do_unix_gc();
		}
	}
}



