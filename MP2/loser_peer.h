#include <limits.h>

/* This header provides example code to handle configurations.
 * The config struct stores whole socket path instead of names.
 * You are free to modifiy it to fit your needs.
 */

struct configuration
{
    int n_peers;
    char host_socket[PATH_MAX];
    char peer_sockets[5][PATH_MAX];
    char host_repo[PATH_MAX];
};

/* Initialize the config struct from path */
int init_config(struct configuration *conf, char *path);

/* Destroy the config struct */
void destroy_config(struct configuration *conf);
void commit(char* argv);