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

int MAX_CONNECTIONS = 256;

pthread_t *handlers = NULL;
pthread_t handler_thread;

unsigned char manager_ready = 0;

extern void add_log_error(char *log_msg, ...);
extern void add_log(char *log_msg, ...);


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

    char *message = "Greetings traveler!",
         client_msg[2048];

    write(sock, message, strlen(message));

    while((read_size = recv(sock, client_msg, 2048, 0)) > 0) {
        // ECHO :turd:
        write(sock, client_msg, strlen(client_msg));
        add_log("recv: %s", client_msg);
    }

    if(read_size == 0) {
        add_log("Client disconnected");
    } else if(read_size == -1) {
        add_log_error("Receive failed (%d): %s", errno, strerror(errno));
    }

    return NULL;
}

void *connection_manager(void *in) {
    int sock = *((int*)in),
        *client_socks = (int*)malloc(sizeof(int) * MAX_CONNECTIONS),
        i = 0;

    unsigned long c;
    struct sockaddr_in client;

    handlers = (pthread_t*)malloc(sizeof(pthread_t) * MAX_CONNECTIONS);

    manager_ready = 0xff;
    // Log something instead
    //add_log("Manager has entered the game!");

    while((client_socks[i] = accept(sock, (struct sockaddr*)&client, (socklen_t*)&c)) != -1) {
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
    //pthread_join();
    //free(handlers);
    return NULL;
}

int init_listener(int port, int concurrent_connections) {
    if(handlers != NULL) {
        add_log_error("Already initialized.");
        return -1;
    }

    int socket_desc;

    struct sockaddr_in server;

    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if(socket_desc == -1) {
        add_log_error("Failed to create socket.");
        return -1;
    }

    server.sin_family       = AF_INET;
    server.sin_addr.s_addr  = INADDR_ANY;
    server.sin_port         = htons(port);

    MAX_CONNECTIONS = concurrent_connections;

    if(bind(socket_desc, (struct sockaddr*)&server,sizeof(server)) < 0) {
        add_log_error("Failed to bind to port %d.", (unsigned)server.sin_port);
        return -1;
    }

    listen(socket_desc, 3);

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
