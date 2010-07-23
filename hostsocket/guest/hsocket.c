#include <u.h>
#include <libc.h>

int vmcall(int b, int c, int d);

int hsocket(int domain, int type, int protocol)
{
	struct {int d; int t; int p;} arg;

	arg.d = domain;
	arg.t = type;
	arg.p = protocol;

	return vmcall(0, (int)&arg, sizeof(arg));
}

int hconnect(int fd, int portno, char *hostname)
{
	struct {int fd; uvlong hn; int len; int pn;} arg;

	arg.fd = fd;
	arg.hn = (uvlong)(uintptr)hostname;
	arg.len = strlen(hostname);
	arg.pn = portno;

	return vmcall(1, (int)&arg, sizeof(arg));
}

int hsend(int fd,void *buf,int len,int flags)
{
    struct {int fd; uvlong buf; int len; int flg;} arg;

    arg.fd = fd;
    arg.buf = (uvlong)(uintptr)buf;
    arg.len = len;
    arg.flg = flags;

    return vmcall(2, (int)&arg, sizeof(arg));
}

int hrecv(int fd,void *buf,int len,int flags)
{
    struct {int fd; uvlong buf; int len; int flg;} arg;

    arg.fd = fd;
    arg.buf = (uvlong)(uintptr)buf;
    arg.len = len;
    arg.flg = flags;

    return vmcall(3, (int)&arg, sizeof(arg));
}

int hclose(int fd)
{
    return vmcall(4, fd, 0);
}
