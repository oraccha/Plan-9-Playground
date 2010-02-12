#include <u.h>
#include <libc.h>
#include <thread.h>
#include <auth.h>
#include "json.h"

enum {
	BUFSIZ = 64 * 1024,
	INTERVAL = 10 * 60 * 1000, /* 10 min. */
};

#define SEARCH_URL "http://search.twitter.com/search.json"
#define TWITTER_SERVER "search.twitter.com"
#define TWITTER_REALM "Twitter API"

static vlong lastid;
static char *query;

#pragma	varargck	type	"R"	char *
extern int urlencode(Fmt *);

int
fetch(char *file)
{
	int ctlfd, conn, fd, newfd;
	int nr, nw;
	char buf[1024];
	char url[128];

	if(lastid)
		sprint(url, "%s?q=%R&since_id=%lld", SEARCH_URL, query,
		       lastid + 1);
	else
		sprint(url, "%s?q=%R", SEARCH_URL, query);

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
	Json *jv, *jtop;
	vlong id;
	char *text, *created_at, *user;
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

		jtop = parsejson(buf);
		if(jtop == nil){
			fprint(2, "parsejson failed.\n");
			sleep(INTERVAL);
			continue;
		}
		jv = jtop->value[0]; /* FIXME */

		for(i = jv->len - 1; i >= 0; i--){
			id = jint(jlookup(jv->value[i], "id"));
			if(id <= lastid)
				continue;
			text = jstring(jlookup(jv->value[i], "text"));
			created_at = jstring(jlookup(jv->value[i], "created_at"));
			user = jstring(jlookup(jv->value[i], "from_user"));
			print("[%s] %s (%s)\n", user, text, created_at);
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
	if(argc != 2){
		fprint(2, "usage: ticker keyword\n");
		exits("failed");
	}
	query = argv[1];
	fmtinstall('R', urlencode);
	watcher();

	exits(nil);
}
