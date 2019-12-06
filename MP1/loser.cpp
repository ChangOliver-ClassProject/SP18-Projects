#include <stdio.h>
#include <dirent.h>
#include <algorithm>
#include <vector>
#include <string>
#include <string.h>
#include <stdint.h>
#include "md5.h"
using namespace std;

int name_search(char new_name[], char old_name[][256], int l, int r){

	int m = (l + r)/2;
   	do{	
   		int same = strcmp(new_name, old_name[m]);
   		if (same == 0)	return m;
      	else if(same > 0)	l = m + 1;
		else  r = m - 1;  

      	m = (l + r)/2;  
    }while(l <= r);
	
	return -1;
}

int md5_search(char new_name[], char old_name[][17], int l, int r){

	int m = (l + r)/2;
   	do{	
   		int same = strcmp(new_name, old_name[m]);
   		if (same == 0)	return m;
      	else if(same > 0)	l = m + 1;
		else  r = m - 1;  

      	m = (l + r)/2;  
    }while(l <= r);
	
	return -1;
}

void status(int argc, char* argv[]){

	//open loser_record
	char path[256];
	strcpy(path, argv[2]);
	strcat(path, "/.loser_record");
	bool unexist = false;
	FILE *fp;
	fp = fopen(path, "rb");  
	if (fp == NULL) unexist = true;	

	if (!unexist){
		//find start of last commit
		uint32_t commit[7];
		while (fread(&commit, 4, 7, fp)){
			fseek(fp, commit[6]-28, SEEK_CUR);
		}
		fseek(fp, -(long)commit[6]+28, SEEK_CUR);  

		//find start of file list with md5
		uint8_t file_name_size;
		char old_name[commit[1]][256];
		char old_md5[commit[1]][17];
		int sum = commit[2] + commit[3] + 2*commit[4] + commit[5];	
		for (int i = 0; i < sum; ++i){		
			fread(&file_name_size, 1, 1, fp);
			fseek(fp, file_name_size, SEEK_CUR);
		}

		//memo last commit files
		for (int i = 0; i < commit[1]; ++i){
			fread(&file_name_size, 1, 1, fp);	
			fread(old_name[i], file_name_size, 1, fp);
			fread(old_md5[i], 16, 1, fp);
			old_md5[i][16] = '\0';
			old_name[i][file_name_size] = '\0';	
		}
		fclose(fp);

		char new_name[1000][256];
		uint8_t new_MD5[1000][17];

		//open directory & read files
		DIR* dir;
		struct dirent *ent;
		dir = opendir (argv[2]);
		int new_count = 0;
		while ((ent = readdir (dir)) != NULL) {
			if (ent->d_name[0] != '.'){
				strcpy(new_name[new_count], ent->d_name);
	    		new_count++;
			}
		}
		closedir(dir);

		//dictionary order
		qsort(new_name, new_count, 256, (int (*)(const void *, const void *))strcmp); 

		//assign MD5	
		for (int i = 0; i < new_count; ++i){
			char file[256];
			strcpy(file, argv[2]);
			strcat(file, "/");
			strcat(file, new_name[i]);
	  		fp = fopen(file, "r");
	  		
	  		char c[100000];
	  		MD5_CTX ctx;
	  		MD5_Init(&ctx);
	  		while (fgets(c, 99998, fp) != NULL){
	  			MD5_Update(&ctx, c, strlen(c));
	  		}
	  		MD5_Final(new_MD5[i], &ctx);
	  		new_MD5[i][16] = '\0';
	  		fclose(fp);
		}	
		
		char newf[1000][256]; int n = 0;
		char modf[1000][256]; int m = 0;
		char cpyf[2000][256]; int c = 0;
		char delf[1000][256]; int d = 0;
		int isdel[1000] = {0};

		for (int i = 0; i < new_count; ++i){
			int t;
			int j = name_search(new_name[i], old_name, 0, commit[1]);
			if (j != -1){
				isdel[j] = 1;
				if (strcmp((const char*)new_MD5[i], old_md5[j]) != 0){
					strcpy(modf[m], new_name[i]);						
					m++;				
				}
			}
			else if ((t = md5_search((char*)new_MD5[i], old_md5, 0, commit[1])) != -1){
				while (strcmp((const char*)new_MD5[i], old_md5[t-1]) == 0) t--;
				strcpy(cpyf[c], old_name[t]);
				c++;
				strcpy(cpyf[c], new_name[i]);
				c++;			
			}
			else{
				strcpy(newf[n], new_name[i]);
				n++;				
			}
		}

		for (int i = 0; i < commit[1]; ++i){
			if (!isdel[i]){
				strcpy(delf[d], old_name[i]);
				d++;
			}
		}

		printf("[new_file]\n");
		for (int i = 0; i < n; ++i){
			printf("%s\n", newf[i]);
		}	
		printf("[modified]\n");
		for (int i = 0; i < m; ++i){
			printf("%s\n", modf[i]);
		}
		printf("[copied]\n");
		for (int i = 0; i < c; i+=2){
			printf("%s => %s\n", cpyf[i], cpyf[i+1]);
		}
		printf("[deleted]\n");
		for (int i = 0; i < d; ++i){
			printf("%s\n", delf[i]);
		}	
	}
	else {
		char new_name[1000][256];

		//open directory & read files
		DIR* dir;
		struct dirent *ent;
		dir = opendir (argv[2]);
		int new_count = 0;
		while ((ent = readdir (dir)) != NULL) {
			if (ent->d_name[0] != '.'){
				strcpy(new_name[new_count], ent->d_name);
	    		new_count++;
			}
		}
		closedir(dir);

		//dictionary order
		qsort(new_name, new_count, 256, (int (*)(const void *, const void *))strcmp); 

		printf("[new_file]\n");
		for (int i = 0; i < new_count; ++i){
	  		printf("%s\n", new_name[i]);
		}
		printf("[modified]\n");
		printf("[copied]\n");
		printf("[deleted]\n");		
	}
}

void commit(int argc, char* argv[]){

    //open loser_record
	char path[256];
	strcpy(path, argv[2]);
	strcat(path, "/.loser_record");
	bool unexist = false;
	FILE *fp;
	fp = fopen(path, "rb");  
	if (fp == NULL) unexist = true;

	if (!unexist){
        //find start of last commit
        uint32_t commit[7];
        while (fread(&commit, 4, 7, fp)){
              fseek(fp, commit[6]-28, SEEK_CUR);
        }
        fseek(fp, -(long)commit[6]+28, SEEK_CUR);

        //find start of file list with md5
        uint8_t file_name_size;
        char old_name[commit[1]][256];
        char old_md5[commit[1]][17];
        uint32_t sum = commit[2] + commit[3] + 2*commit[4] + commit[5];  
        for (int i = 0; i < sum; ++i){            
              fread(&file_name_size, 1, 1, fp);
              fseek(fp, file_name_size, SEEK_CUR);
        }

        //memo last commit files
        for (int i = 0; i < commit[1]; ++i){
              fread(&file_name_size, 1, 1, fp);         
              fread(old_name[i], file_name_size, 1, fp);
              fread(old_md5[i], 16, 1, fp);
              old_md5[i][16] = '\0';
              old_name[i][file_name_size] = '\0';     
        }
        fclose(fp);


        char new_name[1000][256];
        uint8_t new_MD5[1000][17];

        //open directory & read files
        DIR* dir;
        struct dirent *ent;
        dir = opendir (argv[2]);
        int new_count = 0;
        while ((ent = readdir (dir)) != NULL) {
			if (ent->d_name[0] != '.'){
				strcpy(new_name[new_count], ent->d_name);
	    		new_count++;
			}
		}
        closedir(dir);  

        //dictionary order
		qsort(new_name, new_count, 256, (int (*)(const void *, const void *))strcmp); 

        //assign MD5
        uint32_t commitsize = 28 + 16*new_count; 
        for (int i = 0; i < new_count; ++i){
            char file[256];
			strcpy(file, argv[2]);
			strcat(file, "/");
			strcat(file, new_name[i]);
	  		fp = fopen(file, "r");
              
            char c[100000];
            MD5_CTX ctx;
	  		MD5_Init(&ctx);
	  		while (fgets(c, 99998, fp) != NULL){
	  			MD5_Update(&ctx, c, strlen(c));
	  		}
	  		MD5_Final(new_MD5[i], &ctx);
	  		new_MD5[i][16] = '\0';
            fclose(fp);
        }

        char newf[1000][256]; uint32_t n = 0;
        char modf[1000][256]; uint32_t m = 0;
        char cpyf[2000][256]; uint32_t c = 0;
        char delf[1000][256]; uint32_t d = 0;
 		int isdel[1000] = {0};

		for (int i = 0; i < new_count; ++i){
			int t;
			int j = name_search(new_name[i], old_name, 0, commit[1]);
			if (j != -1){
				isdel[j] = 1;
				if (strcmp((const char*)new_MD5[i], old_md5[j]) != 0){
					strcpy(modf[m], new_name[i]);
					m++;				
				}
			}
			else if ((t = md5_search((char*)new_MD5[i], old_md5, 0, commit[1])) != -1){
				while (strcmp((const char*)new_MD5[i], old_md5[t-1]) == 0) t--;
				strcpy(cpyf[c], old_name[t]);
				c++;
				strcpy(cpyf[c], new_name[i]);
				c++;			
			}
			else{
				strcpy(newf[n], new_name[i]);
				n++;				
			}
		}

		for (int i = 0; i < commit[1]; ++i){
			if (!isdel[i]){
				strcpy(delf[d], old_name[i]);
				d++;
			}
		}        

        if (!n && !m && !c && !d) return;

        fp = fopen(path, "ab");
        uint32_t buf[7];
        buf[0] = commit[0]+1;
        buf[1] = new_count;
        buf[2] = n; buf[3] = m; buf[4] = c/2; buf[5] = d; buf[6] = commitsize;
        fwrite(buf, 4, 7, fp);
        for (int i = 0; i < n; ++i){
        	uint8_t size = strlen(newf[i]);
        	commitsize += size + 1;
        	fwrite(&size, 1, 1, fp);
        	fwrite(&newf[i], size, 1, fp);
        }
        for (int i = 0; i < m; ++i){
        	uint8_t size = strlen(modf[i]);
        	commitsize += size + 1;
        	fwrite(&size, 1, 1, fp);
        	fwrite(&modf[i], size, 1, fp);
        }
        for (int i = 0; i < c; i+=2){
        	uint8_t size = strlen(cpyf[i]);
        	commitsize += size + 1;
        	fwrite(&size, 1, 1, fp);
        	fwrite(&cpyf[i], size, 1, fp);
        	size = strlen(cpyf[i+1]);
        	commitsize += size + 1;
        	fwrite(&size, 1, 1, fp);
        	fwrite(&cpyf[i+1], size, 1, fp);
        }
        for (int i = 0; i < d; ++i){
        	uint8_t size = strlen(delf[i]);
        	commitsize += size + 1;
        	fwrite(&size, 1, 1, fp);
        	fwrite(&delf[i], size, 1, fp);
        }
        for (int i = 0; i < new_count; ++i){
        	uint8_t size = strlen(new_name[i]);
        	fwrite(&size, 1, 1, fp);
        	commitsize += size + 1;
        	fwrite(new_name[i], size, 1, fp);
        	fwrite(new_MD5[i], 16, 1, fp);                 
        }
        fclose(fp);
        fp = fopen(path, "rb+");
        fseek(fp, -(long)commitsize+24, SEEK_END);
        fwrite(&commitsize, 4, 1, fp);
        fclose(fp);                             
  	}
  	else{
       	char new_name[1000][256];
        uint8_t new_MD5[1000][17];

        //open directory & read files
        DIR* dir;
        struct dirent *ent;
        dir = opendir (argv[2]);
        int new_count = 0;
        while ((ent = readdir (dir)) != NULL) {
			if (ent->d_name[0] != '.'){
				strcpy(new_name[new_count], ent->d_name);
	    		new_count++;
			}
		}
        closedir(dir);  

        //dictionary order
		qsort(new_name, new_count, 256, (int (*)(const void *, const void *))strcmp); 

        //assign MD5
        uint32_t commitsize = 28 + 16*new_count; 
        for (int i = 0; i < new_count; ++i){
			char file[256];
			strcpy(file, argv[2]);
			strcat(file, "/");
			strcat(file, new_name[i]);
	  		fp = fopen(file, "r");
              
            char c[100000];
	  		MD5_CTX ctx;
	  		MD5_Init(&ctx);
	  		while (fgets(c, 99998, fp) != NULL){
	  			MD5_Update(&ctx, c, strlen(c));
	  		}
	  		MD5_Final(new_MD5[i], &ctx);
	  		new_MD5[i][16] = '\0';
	  		fclose(fp);              
        }  		

        fp = fopen(path, "ab");

        uint32_t buf[7];
        buf[0] = 1; buf[1] = new_count; buf[2] = new_count;
        buf[3] = buf[4] = buf[5] = 0; buf[6] = commitsize;
        fwrite(buf, 4, 7, fp);
        for (int i = 0; i < new_count; ++i){
        	uint8_t size = strlen(new_name[i]);
        	commitsize += size + 1;
        	fwrite(&size, 1, 1, fp);
        	fwrite(new_name[i], size, 1, fp);
        }
        for (int i = 0; i < new_count; ++i){
        	uint8_t size = strlen(new_name[i]);
        	commitsize += size + 1;
        	fwrite(&size, 1, 1, fp);
        	fwrite(new_name[i], size, 1, fp);
        	fwrite(new_MD5[i], 16, 1, fp);
        } 
        fclose(fp);
        fp = fopen(path, "rb+");
        fseek(fp, -(long)commitsize+24, SEEK_END);
        fwrite(&commitsize, 4, 1, fp);
        fclose(fp);                    
  	}
}

void log(int argc, char* argv[]){

	//open loser_record
	string path = argv[3];
	path += "/.loser_record";
	FILE *fp;
	fp = fopen(path.c_str(), "rb");  
	if (fp == NULL) return;
	
	//get commit info
	uint32_t commit[1000][7];
	uint8_t file_name_size;
	char file_name[256];
	int k;
	for (k = 0; fread(&commit[k], 4, 7, fp); ++k)  
		fseek(fp, commit[k][6]-28, SEEK_CUR);		
	k--;
	
	//n commit to be print
	int n = (atoi(argv[2])-1 > k)? k+1:atoi(argv[2]);  
	
	//outputting
	for (int j = 0; j < n; ++j){
		if (j)
			fseek(fp, -(long)commit[k-j+1][6]-(long)commit[k-j][6]+28, SEEK_CUR);
		else
			fseek(fp, -(long)commit[k][6]+28, SEEK_CUR);
		
		printf("# commit %u\n", commit[k-j][0]);
		//printf("num_of_file:   %d\n", commit[k-j][1]);
		//printf("num_of_add:    %d\n", commit[k-j][2]);
		//printf("num_of_mod:    %d\n", commit[k-j][3]);
		//printf("num_of_cpy:    %d\n", commit[k-j][4]);
		//printf("num_of_del:    %d\n", commit[k-j][5]);	
		//printf("commit_size:   %d\n", commit[k-j][6]);

		printf("[new_file]\n");
		for (int i = 0; i < commit[k-j][2]; ++i){		
			fread(&file_name_size, 1, 1, fp);		
			fread(&file_name, file_name_size, 1, fp);
			file_name[file_name_size] = '\0';
			printf("%s\n", file_name);
		}
		printf("[modified]\n");
		for (int i = 0; i < commit[k-j][3]; ++i){
			fread(&file_name_size, 1, 1, fp);
			fread(&file_name, file_name_size, 1, fp);
			file_name[file_name_size] = '\0';
			printf("%s\n", file_name);
		}
		printf("[copied]\n");
		for (int i = 0; i < commit[k-j][4]; ++i){
			fread(&file_name_size, 1, 1, fp);
			fread(&file_name, file_name_size, 1, fp);
			file_name[file_name_size] = '\0';
			printf("%s => ", file_name);
			fread(&file_name_size, 1, 1, fp);				
			fread(&file_name, file_name_size, 1, fp);
			file_name[file_name_size] = '\0';
			printf("%s\n", file_name);
		}
		printf("[deleted]\n");
		for (int i = 0; i < commit[k-j][5]; ++i){
			fread(&file_name_size, 1, 1, fp);
			fread(&file_name, file_name_size, 1, fp);
			file_name[file_name_size] = '\0';
			printf("%s\n", file_name);
		}
		printf("(MD5)\n");
		for (int i = 0; i < commit[k-j][1]; ++i){
			fread(&file_name_size, 1, 1, fp);
			fread(file_name, file_name_size, 1, fp);
			file_name[file_name_size] = '\0';
			uint8_t md5[17];
			fread(md5, 16, 1, fp);
			printf("%s ", file_name);
			for (int s = 0; s < 16; ++s){
				printf("%02hhx", md5[s]);
			}
			printf("\n");
		}
		printf("%s", (j == n-1)? "":"\n");		
	}
	fclose(fp);
}

int main(int argc, char* argv[]){

	if (!strcmp(argv[1], "status"))
		status(argc, argv);
	else if (!strcmp(argv[1], "commit"))
		commit(argc, argv);
	else if (!strcmp(argv[1], "log"))
		log(argc, argv);
	return 0;
}