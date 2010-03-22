#include <u.h>
#include <libc.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>
#include "usb.h"

enum
{
	YurexCSP = 0x000103,

	Stack = 32 * 1024,
};

enum
{
	YCMD_NONE=0xf0,
	YCMD_EOF=0x0d,
	YCMD_ACK=0x21,
	YCMD_MODE=0x41,
	YCMD_VALUE=0x43,
	YCMD_READ=0x52,
	YCMD_WRITE=0x53,
	YCMD_PADDING=0xff,
};

enum
{
	Qbbu,
	Qmbbups,
};

struct Yurex
{
	Dev	*ep;
	int	initialized;
	int	issueing_cmd;
	int	accepted_cmd;
	ulong	curval;
	ulong	oldval;
	vlong	oldnsec;
} yc;

void 
yurexread(Req *r)
{
	int qid = (int)r->fid->qid.path;
	char data[32];
	vlong tnsec;

	switch(qid){
	case Qbbu:
		seprint(data, data+sizeof(data), "%ld\n", yc.curval);
		readstr(r, data);
		break;
	case Qmbbups:
		tnsec = nsec();
		if(tnsec == yc.oldnsec) tnsec++;
		seprint(data, data+sizeof(data), "%lld\n", (yc.curval-yc.oldval)*1000*10000/((tnsec-yc.oldnsec)/100000));
		readstr(r, data);
		yc.oldval = yc.curval;
		yc.oldnsec = tnsec;
	}

	respond(r, nil);
}

void 
yurexwrite(Req *r)
{
	respond(r, nil);
}

void 
yurexopen(Req *r)
{
	respond(r, nil);
}

void 
yurexcreate(Req *r)
{
	respond(r, nil);
}

void 
destroyfile(File *)
{
}

Srv fs = {
	.open = yurexopen,
	.read = yurexread,
	.write = yurexwrite,
	.create = yurexcreate,
};

static void
yurexwork(void *)
{
	char buf[8];
	ulong val;
	int yfd;
	int c;

	yfd = yc.ep->dfd;
	yc.issueing_cmd = YCMD_NONE;
	yc.accepted_cmd = YCMD_NONE;

	print("YUREX: maxpkt = %d dfd = %d\n", yc.ep->maxpkt, yfd);

	//set mode
	memset(buf, YCMD_PADDING, sizeof(buf));
	buf[0] = YCMD_MODE;
	buf[1] = 0;
	buf[2] = YCMD_EOF;
	write(yfd, buf, sizeof(buf));
	sleep(500);

	//read value request
	memset(buf, YCMD_PADDING, sizeof(buf));
	buf[0] = YCMD_READ;
	buf[1] = YCMD_EOF;
	yc.issueing_cmd = YCMD_READ;
	yc.accepted_cmd = YCMD_NONE;
	write(yfd, buf, sizeof(buf));
	sleep(500);

	for(;;){
		if(yc.ep == nil) sysfatal(nil);
		memset(buf, YCMD_PADDING, sizeof(buf));
		c = read(yfd, buf, sizeof(buf));
		if(c <= 0) sysfatal("%r");
		switch(buf[0]){
		case YCMD_ACK:
			if(buf[1] == yc.issueing_cmd){
				fprint(2, "YUREX DEBUG: ack cmd 0x%.2x\n", buf[1]);
				yc.accepted_cmd = buf[1];
			}else{
				fprint(2, "YUREX DEBUG: cmd-ack mismatch: recvd:0x%.2x expect 0x%.02x\n",
					buf[1], yc.issueing_cmd);
				yc.issueing_cmd = YCMD_NONE;
				yc.accepted_cmd = YCMD_NONE;
			}
			break;
		case YCMD_READ:
		case YCMD_VALUE:
			val = (buf[2]<<24) + (buf[3]<<16) + (buf[4]<<8) + buf[5];
			if(yc.initialized == 0){
				yc.oldval = val;
				yc.oldnsec = nsec();
				yc.initialized = 1;
			}
			yc.curval = val;
			//fprint(2, "YUREX DEBUG: BBU %ld\n", val);
			break;
		default:
			fprint(2, "YUREX DEBUG: unknown message: 0x%.2x\n", buf[0]);
		}
	}
}

void 
threadmain(int argc, char **argv)
{
	Dev *yd;
	char *devdir = nil;
	Ep *ep;
	int csps[] = {YurexCSP, 0};
	int i;

	if(finddevs(matchdevcsp, csps, &devdir, 1) < 1){
		fprint(2, "No yurex device\n");
		threadexitsall("yurex not found");
	}
	yd = opendev(devdir);
	if(yd == nil)
		sysfatal("opendev: %r");
	if(configdev(yd)<0)
		sysfatal("configdev: %r");

	for(i = 0; i < nelem(yd->usb->ep); i++){
		if((ep = yd->usb->ep[i]) == nil)
			break;
		if(ep->type == Eintr && ep->dir == Ein)
			if(ep->iface->csp == YurexCSP){
				yc.ep = openep(yd, ep->id);
				if(yc.ep == nil)
					sysfatal("YUREX: %s: openep %d: %r\n", yd->dir, ep->id);
				if(opendevdata(yc.ep, OREAD) < 0){
					fprint(2, "YUERX: %s: opendevdata: %r\n",  yc.ep->dir);
					closedev(yc.ep);
					yc.ep = nil;
					break;
				}

				fs.tree = alloctree(nil, nil, DMDIR|0777, destroyfile);
				createfile(fs.tree->root, "bbu", nil, 0444, nil);
				createfile(fs.tree->root, "mbbups", nil, 0444, nil);
				threadpostmountsrv(&fs, "yurex", nil, MREPL|MCREATE);
				proccreate(yurexwork, nil, Stack);
			}
	}

	threadexits(nil);
}
