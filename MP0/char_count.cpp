#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

int is_regular_file(const char *path){
    struct stat path_stat;
    stat(path, &path_stat);
    return S_ISREG(path_stat.st_mode);
}

void print_ans(char *argv[], long long count[], int argvlen){
	long long sum = 0;
	for (int i = 0; i < argvlen; ++i){
		sum += count[argv[1][i]];
	}
	printf("%lld\n", sum);
}

void char_count(FILE *fp, char *argv[]){
	char *c = NULL;
	size_t len = 0;
	long long count[256] = {0};
	int clength;
	int argvlen = strlen(argv[1]);
	while ((clength = getline(&c, &len, fp)) >= 0){
		for (int i = 0; i < clength; ++i){
			count[c[i]]++;
		}
		print_ans(argv, count, argvlen); 
		memset(count, 0, sizeof(long long)*256);
	}	
}

int main(int argc, char* argv[]){

	if (argc == 2){
		char_count(stdin, argv);	
	}
	else if (argc == 3){

		FILE *fp = fopen(argv[2], "r");

		if (fp == NULL || !is_regular_file(argv[2])){
			fprintf(stderr, "error\n"); 
			return 0;
		}
		char_count(fp, argv);
		fclose(fp);
	}
	return 0;
}