#define DEFAULT_PORT 8080
#define isspace(a) (a == ' ' || a == '\n' || a == '\t' || a == '\r')

#ifndef NETWORK_C
int init_listener(int port, int concurrent_connections);
int connect_host_wrapper();
int connect_host(char *host, int port);
#endif
