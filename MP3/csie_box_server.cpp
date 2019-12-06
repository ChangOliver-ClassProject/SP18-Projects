#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <ftw.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/inotify.h>

#define EVENT_SIZE  (sizeof(struct inotify_event))
#define BUF_LEN     (1024 * (EVENT_SIZE + 16))
#define MAXLEN 		30

typedef struct Instances{
	int num;
	char path[1000];
}Instances;

char sc_fifo_path[256];
char cs_fifo_path[256];
char s_dir_path[256];
char c_dir_path[256];
char buffer[BUF_LEN];
int IN, wd, len = 0;
Instances check_set[MAXLEN];
fd_set set;
fd_set rset;

void SIGhandler(int sig){
	if (sig == SIGINT){
		int ret = remove(sc_fifo_path);
		if (ret < 0) perror("remove");
		ret = remove(cs_fifo_path);
		if (ret < 0) perror("remove");
		inotify_rm_watch(IN, wd);
		close(IN);
		exit(0);		
	}
}

void config(int argc, char* argv[]){
	FILE* fp = fopen(argv[1], "r");
	if (fp == NULL)
		exit(EXIT_FAILURE);

	//input pathname
	fseek(fp, 12, SEEK_SET);
	fscanf(fp, "%s", sc_fifo_path);
	fseek(fp, 12, SEEK_CUR);
	fscanf(fp, "%s", s_dir_path);
	fclose(fp);

	//FIFO
	strcpy(cs_fifo_path, sc_fifo_path);
	strcat(sc_fifo_path, "/server_to_client.fifo");
	strcat(cs_fifo_path, "/client_to_server.fifo");
	mkfifo(sc_fifo_path, 00777);
	mkfifo(cs_fifo_path, 00777);
}

void sync_at_connect(){
	int writing = open(sc_fifo_path, O_WRONLY);
	int reading = open(cs_fifo_path, O_RDONLY);
	write(writing, s_dir_path, 256);
	read(reading, c_dir_path, 256);
	close(reading);
	close(writing);
	sleep(2);
}

void sync_in_process(int IN)
{
	int i = 0;
	int length = read(IN, buffer, BUF_LEN);

	while (i < length) {
	    struct inotify_event *event = (struct inotify_event *) &buffer[i];
	    if (event->len){
	    	if (event->mask & IN_CREATE){
	        	if (event->mask & IN_ISDIR){
	        		strcpy(check_set[len].path, s_dir_path);
	        		strcat(check_set[len].path, "/");
	        		strcat(check_set[len].path, event->name);	        		
	        		check_set[len].num = inotify_add_watch(IN, check_set[len].path, IN_CREATE | IN_MODIFY | IN_DELETE);
	        		len++;
	          		printf( "The directory %s was created.\n", event->name );       
	        	}
	        	else{
	          		printf( "The file %s was created.\n", event->name );
	        	}
	      	}
	      	else if (event->mask & IN_DELETE){
	        	if (event->mask & IN_ISDIR){
	          		printf( "The directory %s was deleted.\n", event->name );       
	        	}
	        	else{
	          		printf( "The file %s was deleted.\n", event->name );
	        	}
	      	}
	      	else if (event->mask & IN_MODIFY){
	        	if (event->mask & IN_ISDIR){
	          		printf( "The directory %s was modified.\n", event->name );
	        	}
	        	else{
	          		printf( "The file %s was modified.\n", event->name );
	        	}
	      	}
	    }
	    i += EVENT_SIZE + event->len;
	}
}

int main(int argc, char* argv[]){
	
	signal(SIGINT, SIGhandler);

	config(argc, argv);
	sync_at_connect();

	IN = inotify_init();
	wd = inotify_add_watch(IN, c_dir_path, IN_CREATE | IN_MODIFY | IN_DELETE);
	check_set[len].num = wd;
	strcpy(check_set[len].path, s_dir_path);
	len++;

	FD_ZERO(&rset);
    FD_SET(IN, &rset);
  	while(1){
  		sync_at_connect();
      	set = rset;
      	if(FD_ISSET(IN, &set)){
       		sync_in_process(IN);
     	}  	
 	}
	return 0;
}