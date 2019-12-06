#include <stdio.h>
#include <dirent.h>
#include <algorithm>
#include <vector>
#include <string>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include "md5.h"
using namespace std;

int search(char new_name[], char old_name[][256], int l, int r){

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

void retrieve_last_commit(FILE* fp, uint32_t commit[7], char old_name[][256], char old_md5[][256]){
    
    //find start of last commit
    while (fread(&commit, 4, 7, fp)){
          fseek(fp, commit[6]-28, SEEK_CUR);
    }
    fseek(fp, -(long)commit[6]+28, SEEK_CUR);

    //find start of file list with md5
    uint8_t file_name_size;
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
}

void construct_current_commit(uint32_t commit[], char new_name[], char new_MD5[], char old_name[][256], char old_md5[][256], char newf[], char modf[], char cpyf[][256], char delf[], int isdel, uint32_t *n, uint32_t *m, uint32_t *c, uint32_t *d){
    int t;
    int j = search(new_name, old_name, 0, commit[1]);
    if (j != -1){
        isdel = 1;
        if (strcmp((const char*)new_MD5, old_md5[j]) != 0){
            strcpy(modf, new_name);
            (*m)++;                
        }
    }
    else if ((t = search((char*)new_MD5, old_md5, 0, commit[1])) != -1){
        while (strcmp((const char*)new_MD5, old_md5[t-1]) == 0) t--;
        strcpy(cpyf[0], old_name[t]);
        (*c)++;
        strcpy(cpyf[1], new_name);
        (*c)++;            
    }
    else{
        strcpy(newf, new_name);
        (*n)++;                
    }

    for (int i = 0; i < commit[1]; ++i){
        if (!isdel){
            strcpy(delf, old_name[i]);
            (*d)++;
        }
    }     
}

void write_commit(FILE* fp, uint32_t *commitsize, uint32_t commit, int new_count, char new_name[][256], uint8_t new_MD5[][256], char newf[][256], char modf[][256], char cpyf[][256], char delf[][256], uint32_t n, uint32_t m, uint32_t c, uint32_t d){
    
    uint32_t buf[7];
    buf[0] = commit+1;
    buf[1] = new_count;
    buf[2] = n; buf[3] = m; buf[4] = c/2; buf[5] = d; buf[6] = *commitsize;
    fwrite(buf, 4, 7, fp);
    for (int i = 0; i < n; ++i){
        uint8_t size = strlen(newf[i]);
        *commitsize += size + 1;
        fwrite(&size, 1, 1, fp);
        fwrite(&newf[i], size, 1, fp);
    }
    for (int i = 0; i < m; ++i){
        uint8_t size = strlen(modf[i]);
        *commitsize += size + 1;
        fwrite(&size, 1, 1, fp);
        fwrite(&modf[i], size, 1, fp);
    }
    for (int i = 0; i < c; i+=2){
        uint8_t size = strlen(cpyf[i]);
        *commitsize += size + 1;
        fwrite(&size, 1, 1, fp);
        fwrite(&cpyf[i], size, 1, fp);
        size = strlen(cpyf[i+1]);
        *commitsize += size + 1;
        fwrite(&size, 1, 1, fp);
        fwrite(&cpyf[i+1], size, 1, fp);
    }
    for (int i = 0; i < d; ++i){
        uint8_t size = strlen(delf[i]);
        *commitsize += size + 1;
        fwrite(&size, 1, 1, fp);
        fwrite(&delf[i], size, 1, fp);
    }
    for (int i = 0; i < new_count; ++i){
        uint8_t size = strlen(new_name[i]);
        fwrite(&size, 1, 1, fp);
        *commitsize += size + 1;
        fwrite(new_name[i], size, 1, fp);
        fwrite(new_MD5[i], 16, 1, fp);                 
    }
}

void commit(char* argv, char name[], char MD5[]){

    //open loser_record
	char record_path[256];
	strcpy(record_path, argv);
	strcat(record_path, "/.loser_record");
	FILE *record_fp;
	record_fp = fopen(record_path, "rb");  

    char content_path[256];
    strcpy(content_path, argv);
    strcat(content_path, "/.repo_content");
    FILE* repo_fp = fopen(content_path, "a");
    fclose(repo_fp);

	if (record_fp != NULL){

        uint32_t commit[7];
        char old_name[1000][256];
        char old_md5[1000][256];
        retrieve_last_commit(record_fp, commit, old_name, old_md5);
        fclose(record_fp);

        char new_name[256];
        char new_MD5[256];
        strcpy(new_name, name);
        strcpy(new_MD5, MD5);

        char newf[256]; uint32_t n = 0;
        char modf[256]; uint32_t m = 0;
        char cpyf[2][256]; uint32_t c = 0;
        char delf[256]; uint32_t d = 0;
 		int isdel = 0;

        construct_current_commit(commit, new_name, new_MD5, old_name, old_md5, newf, modf, cpyf, delf, isdel, &n, &m, &c, &d);
        if (!n && !m && !c && !d) return;

        uint32_t commitsize = 28 + 16;
        record_fp = fopen(record_path, "ab");
        write_commit(record_fp, &commitsize, commit[0], new_count, new_name, new_MD5, newf, modf, cpyf, delf, n, m, c, d);
        fclose(record_fp);
 
        record_fp = fopen(record_path, "rb+");
        fseek(record_fp, -(long)commitsize+24, SEEK_END);
        fwrite(&commitsize, 4, 1, record_fp);
        fclose(record_fp);

        repo_fp = fopen(content_path, "w");
        for (int i = 0; i < n; ++i){
        	fprintf(repo_fp, "%s %s\n", newf[i], );
        }
  	}
  	else{
       	char new_name[1000][256];
        uint8_t new_MD5[1000][17];

        //open directory & read files
        DIR* dir;
        struct dirent *ent;
        dir = opendir (argv);
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
			strcpy(file, argv);
			strcat(file, "/");
			strcat(file, new_name[i]);
	  		FILE *fp = fopen(file, "r");
              
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

        record_fp = fopen(record_path, "ab");

        uint32_t buf[7];        
        buf[0] = 1; buf[1] = new_count; buf[2] = new_count;
        buf[3] = buf[4] = buf[5] = 0; buf[6] = commitsize;
        fwrite(buf, 4, 7, record_fp);

        for (int i = 0; i < new_count; ++i){
        	uint8_t size = strlen(new_name[i]);
        	commitsize += size + 1;
        	fwrite(&size, 1, 1,record_fp);
        	fwrite(new_name[i], size, 1, record_fp);
        }
        for (int i = 0; i < new_count; ++i){
        	uint8_t size = strlen(new_name[i]);
        	commitsize += size + 1;
        	fwrite(&size, 1, 1, record_fp);
        	fwrite(new_name[i], size, 1, record_fp);
        	fwrite(new_MD5[i], 16, 1, record_fp);
        } 
        fclose(record_fp);

        record_fp = fopen(record_path, "rb+");
        fseek(record_fp, -(long)commitsize+24, SEEK_END);
        fwrite(&commitsize, 4, 1, record_fp);
        fclose(record_fp);

        repo_fp = fopen(content_path, "w");
        for (int i = 0; i < new_count; ++i){
            fprintf(repo_fp, "%s %s\n", new_name[i], new_MD5[i]);
        }
  	}
}