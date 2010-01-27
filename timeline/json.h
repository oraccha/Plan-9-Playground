/*
 * JSON parser
 * It is diverted from smugfs written by Russ Cox.
 */
typedef struct Json Json;

enum
{
	Jstring,
	Jnumber,
	Jobject,
	Jarray,
	Jtrue,
	Jfalse,
	Jnull
};

struct Json
{
	int ref;
	int type;
	char *string;
	double number;
	char **name;
	Json **value;
	int len;
};

void	jclose(Json*);
Json*	jincref(Json*);
vlong	jint(Json*);
Json*	jlookup(Json*, char*);
double	jnumber(Json*);
int	jsonfmt(Fmt*);
int	jstrcmp(Json*, char*);
char*	jstring(Json*);
Json*	jwalk(Json*, char*);
Json*	parsejson(char*);
void		printjval(Json*);