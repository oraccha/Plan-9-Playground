#include <u.h>
#include <libc.h>
#include <thread.h>
#include "json.h"

enum {
	BUFSIZ = 64 * 1024,
	INTERVAL = 10 * 60 * 1000, /* 10 min. */
};

#define HOME_TIMELINE_URL "http://api.twitter.com/1/statuses/home_timeline.json"

static vlong lastid;

int
fetch(char *file)
{
	int ctlfd, conn, fd, newfd;
	int nr, nw;
	char buf[1024];
	char url[128];

	if(lastid)
		sprint(url, "%s?since_id=%lld", HOME_TIMELINE_URL, lastid + 1);
	else
		sprint(url, "%s", HOME_TIMELINE_URL);

	ctlfd = open("/mnt/web/clone", ORDWR);
	if(ctlfd < 0)
		sysfatal("open /mnt/web/clone: %r");
	nr = read(ctlfd, buf, sizeof(buf));
	if(nr < 0)
		sysfatal("read: %r");
	if(nr == 0)
		sysfatal("read clone failed");
	buf[nr] = 0;
	conn = atoi(buf);

	if(fprint(ctlfd, "url %s", url) <= 0)
		sysfatal("write ctl failed 'url %s': %r", url);

	snprint(buf, sizeof(buf), "/mnt/web/%d/body", conn);
	fd = open(buf, OREAD);
	if(fd < 0)
		sysfatal("open %s: %r", buf);

	sprint(file, "/tmp/tweet.%d", conn);
	newfd = create(file, OWRITE, 0666);
	if(newfd < 0)
		sysfatal("create %s: %r", file);

	while((nr = read(fd, buf, sizeof(buf))) > 0){
		nw = write(newfd, buf, nr);
		if(nw < 0)
			sysfatal("write: %r");
	}
	if(nr < 0)
		sysfatal("read: %r");

	close(newfd);
	close(fd);

	return 0;
}

void
watcher(void)
{
	Json *jv;
	vlong id;
	char *text, *created_at, *screen_name;
	char *buf;
	int fd, n, i;
	char fname[64];

	buf = malloc(BUFSIZ);
	if(buf == nil)
		sysfatal("malloc: %r");

	for(;;){
		fetch(fname);
		fd = open(fname, OREAD);
		n = readn(fd, buf, BUFSIZ);
		if(n <= 0){
			fprint(2, "read failed.\n");
			sleep(INTERVAL);
			continue;
		}
		/*print("(read %d bytes in %s)\n", n, fname);*/
		buf[n] = 0;

		jv = parsejson(buf);
		if(jv == nil){
			fprint(2, "parsejson failed.\n");
			sleep(INTERVAL);
			continue;
		}

		for(i = jv->len - 1; i >= 0; i--){
			id = jint(jlookup(jv->value[i], "id"));
			if(id <= lastid)
				continue;
			text = jstring(jlookup(jv->value[i], "text"));
			created_at = jstring(jlookup(jv->value[i], "created_at"));
			screen_name = jstring(jwalk(jv->value[i], "user/screen_name"));
			print("[%s] %s (%s)\n", screen_name, text, created_at);
			lastid = id;
		}

		jclose(jv);
		close(fd);
		sleep(INTERVAL);
	}
}

void
main(int argc, char **argv)
{
	watcher();

	exits(nil);
}
