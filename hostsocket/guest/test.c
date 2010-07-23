#include <u.h>
#include <libc.h>
#include <stdio.h>
#include "hsocket.h"

void main(int argc, char **argv)
{
	char buf[128];
	int fd;
	int ret;

	if(argc != 3){
		print("usage: 8.out host file\n");
		return;
	}

	fd = hsocket(2 /* AF_INET */,
		     1 /* SOCK_STREAM */, 0);
	if(fd < 0){
		print("err socket %d\n", fd);
		return;
	}
	ret = hconnect(fd, 80, argv[1]);
	if(ret < 0){
		print("err connect %d\n",ret);
		hclose(fd);
		return;
	}

	snprintf(buf, sizof(buf), "GET %s HTTP/1.0\r\n\r\n", argv[2]);
	ret = hsend(fd, buf, strlen(buf), 0);
	if(ret < 0){
		print("err send %d\n", ret);
		hclose(fd);
		return;
	}

	while(1) {
		ret =hrecv(fd, buf, sizeof(buf), 0);
		if(ret < 0){
			print("err recv %d\n",ret);
			break;
		}
		if(ret == 0)
			break;
		write(1, buf, ret);
	}
	hclose(fd);
}
