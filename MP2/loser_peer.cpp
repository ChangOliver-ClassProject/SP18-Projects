#include <sys/socket.h>
#include <sys/un.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


#include "loser_peer.h"
#include "md5.h"


int init_config(struct configuration *conf, char *path)
{
    int fd = open(path, O_RDONLY);
    if (fd < 0)
        return -1;

    FILE* fp = fdopen(fd, "r");
    if (fp == NULL)
        return -1;

    char line[PATH_MAX];
    fgets(line, PATH_MAX, fp);
    line[strlen(line)-1] = '\0';

    char name[PATH_MAX];
    strcpy(name, "/tmp/mp2-");
    strcat(name, line + 7);
    strcat(name, "-listen.sock");
    strcpy(conf->host_socket, name);

    fgets(line, PATH_MAX, fp);
    line[strlen(line)-1] = '\0';
    conf->n_peers = 0;
    char *tok;
    tok = strtok(line + 8, " ");
    while (tok != NULL){
        strcpy(name, "/tmp/mp2-");
        strcat(name, tok);
        strcat(name, "-resol-connect.sock");
        strcpy(conf->peer_sockets[conf->n_peers], name);
        conf->n_peers++;       
        tok = strtok(NULL, " ");
    }

    fgets(line, PATH_MAX, fp);
    line[strlen(line)-1] = '\0';
    strcpy(conf->host_repo, line + 7);

    close(fd);
    return 0;
}

void destroy_config(struct configuration *conf)
{
    /* TODO do something like free() */
    chdir("/tmp");
    remove(conf->host_socket);
    for (int i = 0; i < conf->n_peers; ++i){
       remove(conf->peer_sockets[i]); 
    }
}

void create_socket(int* sock, struct sockaddr_un* sock_addr, char name[])
{
    /* Create host UNIX socket */
    *sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (*sock < 0){
        perror("socket");
        exit(EXIT_FAILURE);
    }        
    memset(sock_addr, 0, sizeof(struct sockaddr_un));

    /* Bind socket to a name. */
    sock_addr->sun_family = AF_UNIX;
    strncpy(sock_addr->sun_path, name, sizeof(sock_addr->sun_path) - 1);
}

int main(int argc, char *argv[])
{
    /* Make sure argv has a config path */
    assert(argc == 2);
    int ret;

    /* Load config file */
    struct configuration config;
    ret = init_config(&config, argv[1]);

    if (ret < 0){
        perror("init_config");
        exit(EXIT_FAILURE);
    }

    assert(config.host_socket != NULL);
    assert(config.peer_sockets != NULL);
    assert(config.n_peers >= 0);
    //commit(config.host_repo);

    struct sockaddr_un listen_sock_addr;
    struct sockaddr_un connect1_sock_addr;
    struct sockaddr_un connect2_sock_addr;
    struct sockaddr_un connect3_sock_addr;
    struct sockaddr_un connect4_sock_addr;
    int listen_socket, connect1_socket, connect2_socket, connect3_socket, connect4_socket;

    create_socket(&listen_socket, &listen_sock_addr, config.host_socket);
    create_socket(&connect1_socket, &connect1_sock_addr, config.peer_sockets[0]);
    create_socket(&connect2_socket, &connect2_sock_addr, config.peer_sockets[1]);
    create_socket(&connect3_socket, &connect3_sock_addr, config.peer_sockets[2]);
    create_socket(&connect4_socket, &connect4_sock_addr, config.peer_sockets[3]);

    /* Prepare for accepting connections */
    ret = bind(listen_socket, (const struct sockaddr *) &listen_sock_addr, sizeof(struct sockaddr_un));
    if (ret == -1){
        perror("bind");
        exit(EXIT_FAILURE);
    }    
    ret = listen(listen_socket, 20);
    if (ret == -1){
        perror("listen");
        exit(EXIT_FAILURE);
    }
    ret = connect(connect1_socket,(const struct sockaddr *) &connect1_sock_addr, sizeof(struct sockaddr_un));
    ret = connect(connect2_socket,(const struct sockaddr *) &connect2_sock_addr, sizeof(struct sockaddr_un));
    ret = connect(connect3_socket,(const struct sockaddr *) &connect3_sock_addr, sizeof(struct sockaddr_un));
    ret = connect(connect4_socket,(const struct sockaddr *) &connect4_sock_addr, sizeof(struct sockaddr_un));
    if (ret == -1){
        perror("connect");
        exit(EXIT_FAILURE);
    }

    /* Enter the serving loop.
     * It calls select() to check if any file descriptor is ready.
     * You may look up the manpage select(2) for details.
     */

    int max_fd = sysconf(_SC_OPEN_MAX);

    fd_set read_set;
    fd_set write_set;

    FD_ZERO(&read_set);
    FD_ZERO(&write_set);

    FD_SET(STDIN_FILENO, &read_set);       /* check for user input */
    FD_SET(listen_socket, &read_set);  /* check for new peer connections */

    while (1)
    {
        struct timeval tv = {1, 0};
        fd_set working_read_set, working_write_set;

        memcpy(&working_read_set, &read_set, sizeof(working_read_set));
        memcpy(&working_write_set, &write_set, sizeof(working_write_set));

        ret = select(max_fd, &working_read_set, &working_write_set, NULL, &tv);

        if (ret < 0){            /* We assume it doesn't happen. */
            perror("select");
            exit(EXIT_FAILURE);
        }

        if (ret == 0)            /* No fd is ready */
            continue;

        if (FD_ISSET(STDIN_FILENO, &working_read_set))
        {
            /* TODO Handle user commands */
            char command[20];
            scanf("%s", command);

            if (strcmp(command, "list") == 0){

            }
            else if (strcmp(command, "history") == 0){
            	
            }
            else if (strcmp(command, "cp") == 0){

            }
            else if (strcmp(command, "mv") == 0){

            }
            else if (strcmp(command, "rm") == 0){

            }
            else if (strcmp(command, "exit") == 0){
                puts("bye");
                break;
            }
        }

        if (FD_ISSET(listen_socket, &working_read_set))
        {
            int peer_socket = accept(listen_socket, NULL, NULL);
            if (peer_socket < 0)
                exit(EXIT_FAILURE);
            printf("peer_socket: %d\n", peer_socket);

            /* TODO Store the peer fd */
        }
    }

    /* finalize */
    destroy_config(&config);
    close(listen_socket);
    close(connect1_socket);
    close(connect2_socket);
    close(connect3_socket);
    close(connect4_socket);
    return 0;
}
