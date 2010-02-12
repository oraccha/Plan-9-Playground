/*
 * URL encode verb function.
 */
#include <u.h>
#include <libc.h>

int whitelist[] = {
	/* reserved characters */
	[36] '$',
	[38] '&',
	[43] '+',
	[44] ',',
	[47] '/',
	[58] ':',
	[59] ';',
	[61] '=',
	[63] '?',
	[64] '@',
};

int
urlencode(Fmt *fmt)
{
	char *p, *str;
	int c, e;
	int cnt;

	cnt = 0;
	str = p = smprint("%s", va_arg(fmt->args, char *));
	while(c = *p++){
		/* [0-9A-Za-z] */
		if (('0' <= c && c <= '9') ||
		     ('A' <= c && c <= 'Z') ||
		     ('a' <= c && c <= 'z')) {
			cnt += fmtprint(fmt, "%c", c);
			continue;
		}
		e = whitelist[c];
		if(e){
			cnt += fmtprint(fmt, "%c", c);
			continue;
		}
		cnt += fmtprint(fmt, "%%%02x", c);
	}
	free(str);
	return cnt;
}

/*
void
main(int argc, char **argv)
{
	fmtinstall('X', urlencode);
	print("%X\n", argv[1]);
	exits(nil);
}
*/