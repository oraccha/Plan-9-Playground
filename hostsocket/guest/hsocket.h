int hsocket(int domain, int type, int protocol);
int hconnect(int fd, int portno, char *hostname);
int hsend(int fd,void *buf,int len,int flags);
int hrecv(int fd,void *buf,int len,int flags);
int hclose(int fd);
