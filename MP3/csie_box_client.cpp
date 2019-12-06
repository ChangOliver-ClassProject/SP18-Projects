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
char c_dir_path[256];
char s_dir_path[256];
char buffer[BUF_LEN];
int IN, wd, len = 0;
Instances check_set[MAXLEN];
fd_set set;
fd_set rset;

static int rmFiles(const char *pathname, const struct stat *sbuf, int type, struct FTW *ftwb){ return remove(pathname);}

void SIGhandler(int sig){
	if (sig == SIGINT){
		nftw(c_dir_path, rmFiles, 15, FTW_DEPTH);
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
	fscanf(fp, "%s", c_dir_path);
	fclose(fp);

	//FIFO
	strcpy(cs_fifo_path, sc_fifo_path);
	strcat(sc_fifo_path, "/server_to_client.fifo");
	strcat(cs_fifo_path, "/client_to_server.fifo");
}

void sync_at_connect(){	
	nftw(c_dir_path, rmFiles, 15, FTW_DEPTH);
	mkdir(c_dir_path, 0);

	int reading = -1;
	int writing;
	while (reading < 0){
		reading = open(sc_fifo_path, O_RDONLY);
		writing = open(cs_fifo_path, O_WRONLY);
	}
	chmod(c_dir_path, 00700);

	read(reading, s_dir_path, 256);
	write(writing, c_dir_path, 256);
	close(reading);
	close(writing);	

	char command[1000] = {"rsync -avu --delete "};
	strcat(command, s_dir_path);
	strcat(command, "/ ");
	strcat(command, c_dir_path);
	system(command);
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
	wd = inotify_add_watch(IN, s_dir_path, IN_CREATE | IN_MODIFY | IN_DELETE);
	check_set[len].num = wd;
	strcpy(check_set[len].path, s_dir_path);
	len++;

	FD_ZERO(&rset);
    FD_SET(IN, &rset); 
	while(1){
		set = rset;
	  	if(FD_ISSET(IN, &set)){
	   		sync_in_process(IN);
	  	}
    }

	return 0;
}