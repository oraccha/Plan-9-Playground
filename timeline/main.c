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
	int pid, p[2], f;
	int nr, nw;
	char buf[1024];
	char cmd[64];

	if(lastid)
		sprint(cmd, "hget '%s?since_id=%lld'", HOME_TIMELINE_URL, lastid + 1);
	else
		sprint(cmd, "hget %s", HOME_TIMELINE_URL);

	if(pipe(p) < 0)
		sysfatal("pipe: %r");

	switch(pid = fork()){
	case -1:
		sysfatal("fork: %r");
	case 0:
		close(p[0]);
		dup(p[1], 1);
		execl("/bin/rc", "rc", "-c", cmd, nil);
	default:
		close(p[1]);

		sprint(file, "/tmp/tweet.%d", pid);
		f = create(file, OWRITE, 0666);
		if(f < 0)
			sysfatal("open: %r");

		for(;;){
			nr = read(p[0], buf, sizeof(buf));
			if(nr < 0)
				sysfatal("read: %r");
			if(nr == 0)
				break;

			nw = write(f, buf, nr);
			if(nw < 0)
				sysfatal("write: %r");
		}
		close(f);
		close(p[0]);
	}

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
		if(n < 0){
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
