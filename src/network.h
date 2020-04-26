#define DEFAULT_PORT 8080

#ifndef NETWORK_C
int init_listener(int port, int concurrent_connections);
int connect_host_wrapper();
int connect_host(char *host, int port);
void cleanup();
#endif
