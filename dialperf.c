#include <u.h>
#include <libc.h>

int sink;
int len = 4096;
int iter = 1024;
char *port = "10000";
char *combuf;

double
wtime(void)
{
	vlong tv;
	static vlong tv0;

	if(tv0 == 0){
		tv = nsec();
		tv0 = tv;
		return 0.0;
	}

	tv = nsec();
	return ((double)(tv - tv0) / 1e+6);
}

void
getaddr(char *dir, char *addr, int len, int local)
{
	char buf[64];
	int fd, n;

	snprint(buf, sizeof(buf), "%s/%s", dir, local ? "local" : "remote");
	fd = open(buf, OREAD);
	if(fd < 0){
		snprint(addr, sizeof(addr), "unknown");
		return;
	}
	n = read(fd, addr, len-1);
	close(fd);
	if(n <= 0){
		snprint(addr, sizeof(addr), "unknown");
		return;
	}
	if(n > 0)
		n--;
	addr[n] = 0;
}

void
localaddr(char *dir, char *laddr, int len)
{
	getaddr(dir, laddr, len, 1);
}

void
remoteaddr(char *dir, char *laddr, int len)
{
	getaddr(dir, laddr, len, 0);
}

void
dosink(int fd)
{
	double start, end;
	int i;
	long c, n;

	start = wtime();
	for(i = 0; i < iter; i++){
		c = 0;
		while(c != len){
			n = read(fd, combuf + c, len - c);
			if(n < 0){
				perror("dosink");
				exits("read");
			}
			c += n;
		}
	}
	end = wtime();

	print("%f sec %d bytes %f bps\n", end - start, len * iter, (double)(len * iter) / (end - start));
	return;
}

void
doburst(int fd)
{
	double start, end;
	int i;
	long c, n;

	start = wtime();
	for(i = 0; i < iter; i++){
		c = 0;
		while(c != len){
			n = write(fd, combuf + c, len - c);
			if(n < 0){
				perror("doburst");
				exits("write");
			}
			c += n;
		}
	}
	end = wtime();

	print("%f sec %d bytes %f bps\n", end - start, len * iter, (double)(len * iter) / (end - start));
	return;
}

void
main(int argc, char *argv[])
{
	char laddr[64], raddr[64];

	argc--;
	argv++;

	while(argc > 0){
		if(strncmp(argv[0], "-sink", 5) == 0){
			sink = 1;
			argc--;
			argv++;
		}else if(strncmp(argv[0], "-port", 5) == 0){
			port = strdup(argv[1]);
			argc -= 2;
			argv += 2;
		}else if(strncmp(argv[0], "-len", 4) == 0){
			len = atoi(argv[1]);
			argc -= 2;
			argv += 2;
		}else if(strncmp(argv[0], "-iter", 5) == 0){
			iter = atoi(argv[1]);
			argc -= 2;
			argv += 2;
		}else
			break;
	}

	combuf = malloc(len);
	if(combuf == 0)
		exits("malloc");
	memset(combuf, 0, len);

	if(sink){
		/* RECEIVER SIDE */
		int acfd, lcfd, dfd;
		char addr[40], adir[40], ldir[40];

		sprint(addr, "tcp!*!%s", port);
		acfd = announce(addr, adir);
		if(acfd < 0)
			exits("announce");

		for(;;){
			lcfd = listen(adir, ldir);
			if(lcfd < 0)
				exits("listen");

			switch(fork()){
			case -1:
				perror("fork");
				close(lcfd);
				break;
			case 0:
				dfd = accept(lcfd, ldir);
				if(dfd < 0)
					exits("accept");
				localaddr(ldir, laddr, sizeof(laddr));
				remoteaddr(ldir, raddr, sizeof(raddr));	
				print("local %s connected with %s\n", laddr, raddr);
				dosink(dfd);
				exits(0);
			default:
				close(lcfd);
				break;
			}
		}
	}else{
		/* SENDER SIDE */
		char ldir[40];
		int fd;

		if(argc != 1)
			exits("no sink host.");

		fd = dial(netmkaddr(argv[0], 0, port), 0, ldir, 0);
		localaddr(ldir, laddr, sizeof(laddr));
		remoteaddr(ldir, raddr, sizeof(raddr));
		print("local %s connected with %s\n", laddr, raddr);
		doburst(fd);
	}
}
