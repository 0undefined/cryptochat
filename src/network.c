#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <netdb.h>
#include <errno.h>

#define NETWORK_C

#include "network.h"
#include "crypto.h"

#define HTML \
    "HTTP/1.1 200 OK\n" \
    "\n" \
    "<!DOCTYPE html><main>" \
    "We are sorry to announce that this is <u>NOT</u> a http server!" \
    "</main></html>"

int MAX_CONNECTIONS = 256;

pthread_t *handlers = NULL;
pthread_t handler_thread;

unsigned char manager_ready = 0;

extern void add_log_error(char *log_msg, ...);
extern void add_log(char *log_msg, ...);

int is_http_request(char *request) {
    /* Example */
    // GET / HTTP/1.1
    // Host: localhost:8080
    // User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:69.0) Gecko/20100101 Firefox/69.0
    // Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8
    // Accept-Language: en-US,en;q=0.5
    // Accept-Encoding: gzip, deflate
    // DNT: 1
    // Connection: keep-alive
    // Upgrade-Insecure-Requests: 1
    char *a;
    int likelyhood = 0;
    if(strcmp((a = strtok(request, " ")), "GET") == 0) likelyhood++;
    else if(strcmp(a, "HEAD") == 0) likelyhood++;
    else if(strcmp(a, "POST") == 0) likelyhood++;
    else if(strcmp(a, "PUT") == 0) likelyhood++;
    else if(strcmp(a, "DELETE") == 0) likelyhood++;
    else if(strcmp(a, "CONNECT") == 0) likelyhood++;
    else if(strcmp(a, "OPTIONS") == 0) likelyhood++;
    else if(strcmp(a, "TRACE") == 0) likelyhood++;

    a = strtok(NULL, " ");
    if(a == NULL) return likelyhood;
    else if(strcmp(a, "/") == 0) likelyhood++;
    else if(strcmp(a, "*") == 0) likelyhood++;
    else if(a[0] == '/' && a[strlen(a)-1] == '/') likelyhood++;

    a = strtok(NULL, " ");
    if(a == NULL) return likelyhood;
    else if(strcmp(a, "HTTP/1.0") == 0) likelyhood++;
    else if(strcmp(a, "HTTP/1.1") == 0) likelyhood++;

    return likelyhood;
}

int host_name_to_ip(char *hostname) {
    struct hostent *he;
    struct in_addr **addr_list;

    if((he = gethostbyname(hostname)) == NULL) {
        add_log_error("Could not resolve hostname (%d): %s", errno, strerror(errno));
        return -1;
    }

    addr_list = (struct in_addr**) he->h_addr_list;

    if(addr_list[0] != NULL) {
        return inet_addr(inet_ntoa(*addr_list[0]));
    }
    return -1;
}

void *connection_handler(void *in) {
    int sock = *((int*)in),
        read_size;

    char *hostname = (char*)malloc(sizeof(char)*512);
    socklen_t hostlen = sizeof(char) * 512;

    struct sockaddr_in address;
    socklen_t addrlen;

    getpeername(sock, (struct sockaddr*)&address, &addrlen);
    getnameinfo((struct sockaddr*)&address, addrlen,
            hostname, hostlen,
            NULL, 0, 0);

    char *message = "Greetings traveler!",
         client_msg[2048];


    while((read_size = recv(sock, client_msg, 2048, 0)) > 0) {

        if(is_http_request(client_msg) >= 1) {
            write(sock, HTML, strlen(HTML));
            close(sock);
            add_log("A bastard thought this was a http server");
            return NULL;
        }

        write(sock, message, strlen(message));
        //write(sock, client_msg, strlen(client_msg));
        if(hostname != NULL && strlen(hostname) > 0)
            add_log("%s: %s", hostname, client_msg);
        else
            add_log("%s: %s", inet_ntoa(address.sin_addr), client_msg);
    }

    if(read_size == 0) {
        add_log("Client disconnected");
    } else if(read_size == -1) {
        add_log_error("Receive failed (%d): %s", errno, strerror(errno));
    }

    close(sock);
    free(hostname);
    return NULL;
}

void *connection_manager(void *in) {
    int sock = *((int*)in),
        *client_socks = (int*)malloc(sizeof(int) * MAX_CONNECTIONS),
        i = 0;

    unsigned long c;
    struct sockaddr_in client;

    handlers = (pthread_t*)malloc(sizeof(pthread_t) * MAX_CONNECTIONS);
    for(int j = 0; j < MAX_CONNECTIONS; j++) handlers[i] = 0;
    add_log("thread: %p", handlers[i]);

    manager_ready = 0xff;
    // Log something instead
    add_log("Connection manager is ready");

    while(manager_ready != 0) {
        client_socks[i] = accept(sock, (struct sockaddr*)&client, (socklen_t*)&c);

        if(client_socks[i] == -1) {
            if(errno == EAGAIN || errno == EWOULDBLOCK) { sleep(1); continue; }
            else break;
        }
        if(manager_ready == 0) break;

        add_log("Accepted connection");

        if(pthread_create(&handlers[i], NULL, connection_handler, &(client_socks[i])) < 0) {
            add_log_error("Failed to create thread (existing threads: %d)", i);
        }
        i++;
        if(i == MAX_CONNECTIONS) break; // and die
    }
    if(client_socks[i] == -1) {
        add_log_error("Error (%d): %s", errno, strerror(errno));
    }
    // Cleanup
    for(int j = 0; j < MAX_CONNECTIONS; j++) if(handlers[j] != 0) pthread_join(handlers[j], NULL);

    free(client_socks);
    free(handlers);
    return NULL;
}

void cleanup() {
    manager_ready = 0x00;
    pthread_join(handler_thread, NULL);
    return;
}

int init_listener(int port, int concurrent_connections) {
    if(handlers != NULL) {
        add_log_error("Already initialized.");
        return -1;
    }

    int socket_desc;

    struct sockaddr_in server;

    socket_desc = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if(socket_desc == -1) {
        add_log_error("Failed to create socket.");
        return -1;
    }

    server.sin_family       = AF_INET;
    server.sin_addr.s_addr  = INADDR_ANY;
    server.sin_port         = htons(port);

    MAX_CONNECTIONS = concurrent_connections;

    if(bind(socket_desc, (struct sockaddr*)&server,sizeof(server)) < 0) {
        add_log_error("Failed to bind to port %d, (%d): %s", (unsigned)server.sin_port, errno, strerror(errno));
        return -1;
    }

    listen(socket_desc, MAX_CONNECTIONS);

    pthread_create(&handler_thread, NULL, connection_manager, &socket_desc);

    while(!manager_ready) {sleep(1);};

    return socket_desc;
}

int connect_host(char *host, int port) {
    char msg[2048];

    int sock;
    struct sockaddr_in server;

    char reply[2048];

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock == -1) {
        add_log_error("Failed to create socket (%d):%s", errno, strerror(errno));
        return -1;
    }

    server.sin_family       = AF_INET;
    server.sin_addr.s_addr  = inet_addr(host);
    server.sin_port         = htons(port);

    if(server.sin_addr.s_addr == INADDR_NONE) {
        server.sin_addr.s_addr = host_name_to_ip(host);
    }

    if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0) {
        add_log_error("Failed to connect to server (%d): %s", errno, strerror(errno));
        return -1;
    }

    while(1)
    {
        add_log("Send message: ");
        fgets(msg, 2048, stdin);

        //Send some data
        if(send(sock , msg, strlen(msg) , 0) < 0) {
            add_log_error("Failed to send message.");
            return -1;
        }

        //Receive a reply from the server
        if( recv(sock, reply, 2048, 0) < 0)
        {
            add_log_error("Receive failed (%d): %s", errno, strerror(errno));
            break;
        }

        add_log("recv reply: %s", reply);
    }

    close(sock);
    return 0;
}

int connect_host_wrapper() {
    int max_address_len = 1024;
    char *target = (char*)malloc(max_address_len * sizeof(char));

    add_log("connect: ");

    if(fgets(target, max_address_len, stdin) == NULL) {
        add_log_error("Failed to read from stdin.");
        exit(EXIT_FAILURE);
    }
    // remove newline
    target[strlen(target) - 1] = '\0';
    char *host = strtok(target, ":");
    int port = DEFAULT_PORT;
    {
        char *tmp = strtok(NULL, ":");
        if(tmp != NULL) port = atoi(tmp);
    }
    if(strtok(NULL, ":") != NULL) {
        add_log_error("Invalid target.");
        exit(EXIT_FAILURE);
    }
    for(unsigned i = 0; i < strlen(host) - 1; i++) {
        if(isspace(host[i])) {
            add_log_error("Target cannot contain spaces. %d . %x", i, host[i]);
            exit(EXIT_FAILURE);
        }
    }
    return connect_host(host, port);
}

//int send_msg(char *host, char *msg) {
//    //
//}
