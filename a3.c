#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdbool.h>

#define REQ_PIPE "REQ_PIPE_38268"
#define RESP_PIPE "RESP_PIPE_38268"

int main(int argc, char **argv){

	int fd_resp = -1, fd_req = -1;
	
	//create resp pipe
    if(mkfifo(RESP_PIPE, 0644) != 0) {
        printf("ERROR\nCannot create the response pipe\n");
        return 1;
    }

    //open req pipe
    fd_req = open(REQ_PIPE, O_RDONLY);
    if(fd_req == -1) {
        printf("ERROR\ncannot open the request pipe\n");
        return 1;
    }

    //open resp pipe
    fd_resp = open(RESP_PIPE, O_WRONLY);
    if(fd_resp == -1) {
        printf("ERROR\ncannot open the response pipe\n");
        return 1;
    }

    unsigned int connect_length = strlen("CONNECT");
    if(write(fd_resp, &connect_length, 1) != -1){
        if(write(fd_resp, "CONNECT", connect_length) != -1){
            printf("SUCCESS\n");
        }
    }

    int shmFd = -1, fdFile = -1;
    char *data = NULL, *fileData = NULL;
    char *fileName = NULL;

    unsigned int length = 0, offset = 0, value = 0, file_length = 0, file_name_length = 0, shmSize = 0;
    unsigned int rf_offset = 0, no_bytes = 0;
    unsigned int rs_offset = 0, rs_no_bytes = 0, section = 0;
    unsigned int new_offset = 0;
    unsigned int logical_offset = 0, ls_no_bytes = 0;
    char *req_string = NULL;
    unsigned int ping_nr = 38268;

    unsigned int sect_size = 0, offset_for_logical = 0;

    unsigned int success_length = strlen("SUCCESS"), error_length = strlen("ERROR");


    while(1){
    	read(fd_req, &length, 1);
    	req_string = (char *)malloc((length+1) * sizeof(char));
    	read(fd_req, req_string, length);
        req_string[length] = '\0';

       
        if(strcmp(req_string, "EXIT") == 0){        ///////////////////////////EXIT
            
            close(fd_resp);
            close(fd_req);

            munmap(data, shmSize);     //create 
            data = NULL;
            shm_unlink("/Jg2nDbw3");

            munmap(fileData, file_length);  //map
            fileData = NULL;
            close(fdFile);

            free(fileName);
            free(req_string);

            unlink(RESP_PIPE);
            break;
        }

        if(strcmp(req_string, "PING") == 0){        ///////////////////////////////////////////////////PING
           write(fd_resp, &length, 1);
           write(fd_resp, "PING", length);
           write(fd_resp, &length, 1);
           write(fd_resp, "PONG", length);
           write(fd_resp, &ping_nr, sizeof(unsigned int));
        }


        if(strcmp(req_string, "CREATE_SHM") == 0){                  ///////////////////////////////////CREATE_SHM

            read(fd_req, &shmSize, sizeof(unsigned int));
            shmFd = shm_open("/Jg2nDbw3", O_CREAT | O_RDWR, 0664);
            if(shmFd < 0) {
                write(fd_resp, &length, 1);
                write(fd_resp, "CREATE_SHM", length);
                write(fd_resp, &error_length, 1);
                write(fd_resp, "ERROR", error_length);
            }
            else{
                ftruncate(shmFd, shmSize);
                data = (char *)mmap(NULL, shmSize, PROT_READ | PROT_WRITE, MAP_SHARED, shmFd, 0); 
                if(data == (void*)-1){
                    write(fd_resp, &length, 1);
                    write(fd_resp, "CREATE_SHM", length);
                    write(fd_resp, &error_length, 1);
                    write(fd_resp, "ERROR", error_length);
                }
                else{
                    write(fd_resp, &length, 1);
                    write(fd_resp, "CREATE_SHM", length);
                    write(fd_resp, &success_length, 1);
                    write(fd_resp, "SUCCESS", success_length);
                }
            }
        }


        if(strcmp(req_string, "WRITE_TO_SHM") == 0){      ///////////////////////////////////////WRITE_TO_SHM

            read(fd_req, &offset, sizeof(unsigned int));
            read(fd_req, &value, sizeof(unsigned int));
            
            if(offset < 0 || offset > 4226547){   //-4 sa incapa value
                write(fd_resp, &length, 1);
                write(fd_resp, "WRITE_TO_SHM", length);
                write(fd_resp, &error_length, 1);
                write(fd_resp, "ERROR", error_length);
            }
            else{
                memcpy(&data[offset], &value, sizeof(unsigned int));
                write(fd_resp, &length, 1);
                write(fd_resp, "WRITE_TO_SHM", length);
                write(fd_resp, &success_length, 1);
                write(fd_resp, "SUCCESS", success_length);
            }
        }


        if(strcmp(req_string, "MAP_FILE") == 0){            ////////////////////////////////////////////MAP_FILE

            read(fd_req, &file_name_length, 1);
            fileName = (char *)malloc((file_name_length+1) * sizeof(char));
            read(fd_req, fileName, file_name_length);
            fileName[file_name_length] = '\0';

            fdFile = open(fileName, O_RDONLY);
            if(fdFile == -1){
                write(fd_resp, &length, 1);
                write(fd_resp, "MAP_FILE", length);
                write(fd_resp, &error_length, 1);
                write(fd_resp, "ERROR", error_length);
            }else{
                file_length = lseek(fdFile, 0, SEEK_END);
                lseek(fdFile, 0, SEEK_SET);

                fileData = (char*)mmap(NULL, file_length, PROT_READ, MAP_SHARED, fdFile, 0);
                if(fileData == (void*)-1) {
                    write(fd_resp, &length, 1);
                    write(fd_resp, "MAP_FILE", length);
                    write(fd_resp, &error_length, 1);
                    write(fd_resp, "ERROR", error_length);
                }
                else{
                    write(fd_resp, &length, 1);
                    write(fd_resp, "MAP_FILE", length);
                    write(fd_resp, &success_length, 1);
                    write(fd_resp, "SUCCESS", success_length);
                }
                close(fdFile);
            }  
        }


        if(strcmp(req_string, "READ_FROM_FILE_OFFSET") == 0){   ///////////////////////////////////READ_FROM_FILE_OFFSET
            read(fd_req, &rf_offset, sizeof(unsigned int));
            read(fd_req, &no_bytes, sizeof(unsigned int));
            
            if(rf_offset + no_bytes > file_length || shmFd < 0 || fileData == NULL){
                write(fd_resp, &length, 1);
                write(fd_resp, "READ_FROM_FILE_OFFSET", length);
                write(fd_resp, &error_length, 1);
                write(fd_resp, "ERROR", error_length);
            }
            else{
                int crtPos = 0;
                for(int i = rf_offset; i < rf_offset + no_bytes; i++){
                    data[crtPos] = fileData[i];
                    crtPos++;
                }
                write(fd_resp, &length, 1);
                write(fd_resp, "READ_FROM_FILE_OFFSET", length);
                write(fd_resp, &success_length, 1);
                write(fd_resp, "SUCCESS", success_length);
            }
        }
        

        if(strcmp(req_string, "READ_FROM_FILE_SECTION") == 0){      //////////////////////////////READ_FROM_FILE_SECTION
            read(fd_req, &section, sizeof(unsigned int));
            read(fd_req, &rs_offset, sizeof(unsigned int));
            read(fd_req, &rs_no_bytes, sizeof(unsigned int));

            if(section < 1 || section > fileData[8]){
                write(fd_resp, &length, 1);
                write(fd_resp, "READ_FROM_FILE_SECTION", length);
                write(fd_resp, &error_length, 1);
                write(fd_resp, "ERROR", error_length);
            }  
            else{
                memcpy(&new_offset, &fileData[8+27*(section-1)+20], 4);
                int sectPos = 0;
                for(int i = new_offset + rs_offset; i < new_offset + rs_offset + rs_no_bytes; i++){
                    data[sectPos] = fileData[i];
                    sectPos++;
                }
                write(fd_resp, &length, 1);
                write(fd_resp, "READ_FROM_FILE_SECTION", length);
                write(fd_resp, &success_length, 1);
                write(fd_resp, "SUCCESS", success_length);
            }          
           
        }


        if(strcmp(req_string, "READ_FROM_LOGICAL_SPACE_OFFSET") == 0){  ///////////////////READ_FROM_LOGICAL_SPACE_OFFSET
            read(fd_req, &logical_offset, sizeof(unsigned int));
            read(fd_req, &ls_no_bytes, sizeof(unsigned int));

            int actual_sect_no = 1, actual_block = 0, size_sum = 0;
            bool performed = false;

            while(actual_sect_no <= fileData[8]){
                memcpy(&sect_size, &fileData[8+27*(actual_sect_no-1)+24], 4);
                size_sum = actual_block*2048 + sect_size;
                if(logical_offset < size_sum){
                    offset_for_logical = logical_offset - actual_block*2048;
                    performed = true;
                    break;
                }
                actual_sect_no++;
                actual_block += 1 + sect_size/2048;
                if(sect_size % 2048 == 0)
                    actual_block--;
            }

            unsigned int logical_new_offset = 0;
            memcpy(&logical_new_offset, &fileData[8+27*(actual_sect_no-1)+20], 4);

            int logicPos = 0;
            for(int i = logical_new_offset + offset_for_logical; i < logical_new_offset + offset_for_logical + ls_no_bytes; i++){
                data[logicPos] = fileData[i];
                logicPos++;
            }
            if(performed){
                write(fd_resp, &length, 1);
                write(fd_resp, "READ_FROM_LOGICAL_SPACE_OFFSET", length);
                write(fd_resp, &success_length, 1);
                write(fd_resp, "SUCCESS", success_length);
            }
            else{
                write(fd_resp, &length, 1);
                write(fd_resp, "READ_FROM_LOGICAL_SPACE_OFFSET", length);
                write(fd_resp, &error_length, 1);
                write(fd_resp, "ERROR", error_length);
            }
        }

      
    }
	
	return 0;
}