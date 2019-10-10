//Raul Pulido 6/10/19
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <utility>
#include <vector>
#include <string>
#include "disk.c"
using namespace std;
int make_fs(char *disk_name);
int mount_fs(char *disk_name);
int umount_fs(char *disk_name);
int fs_open(char *name);
int fs_close(int fildes);
int fs_create(char *name);
int fs_delete(char *name);
int fs_read(int fildes, void *buf, size_t nbyte);
int fs_write(int fildes, void *buf, size_t nbyte);
int fs_get_filesize(int fildes);
int fs_lseek(int fildes, off_t offset);
int fs_truncate(int fildes, off_t length);
struct FD{
    int num;
    int offset;
    string file;
};
vector<FD> fds;
vector<string> DFile;
vector<int> DSize;
vector<int> DStart;
vector<int> Fat;
vector<bool> empty;
int make_fs(char *disk_name){
    if(make_disk(disk_name) < 0){return -1;}
    if(open_disk(disk_name) < 0){return -1;}
    char temp[4096] = {'R','E','A','D','Y','\0'};
    if(block_write(0, temp) < 0){return -1;}
    memset(temp, 0, sizeof(temp));
    char section[5] = {'5','0','0','0',' '};
    for(int j = 0; j < 128; j++){memcpy(temp + (j*5), section, 5);}
    for(int i = 2; i <= 33; i++){if(block_write(i, temp) < 0){return -1;}}
    memset(temp, 0, sizeof(temp));
    char section2[30] = {'z','z','z','z','z','z','z','z','z','z','z','z','z','z','z',' ','1','6','7','7','7','2','1','7',' ','5','0','0','0',' '};
    for(int j = 0; j < 64; j++){memcpy(temp + (j*30), section2, 30);}
    if(block_write(1, temp) < 0){return -1;}
    if(close_disk() < 0){return -1;}
    return 0;
}
int mount_fs(char *disk_name){
    char temp[4096];
    if(open_disk(disk_name) < 0){return -1;}
    if(block_read(0, temp) < 0){return -1;}
    string Signature = "READY";
    if(Signature != (string)temp){return -1;}
    memset(temp, 0, sizeof(temp));
    if(block_read(1, temp) < 0){return -1;}
    int i = 0;
    char *token = strtok(temp, " ");
    while(token != NULL){
        if(i == 0){
            DFile.push_back((string)token);
            token = strtok(NULL, " ");
            i++;
        }
        if(i == 1){
            DSize.push_back((int) strtol(token, (char **)NULL, 10));
            token = strtok(NULL, " ");
            i++;
        }
        if(i == 2){
            DStart.push_back((int) strtol(token, (char **)NULL, 10)); 
            token = strtok(NULL, " ");
            i = 0;
        }
    }
    for(int i = 2; i <= 33; i++){
        memset(temp, 0, sizeof(temp));
        char *token1 = strtok(temp, " ");
        if(block_read(i, temp) < 0){return -1;}
        token1 = strtok(temp, " ");
        while(token1 != NULL){
            Fat.push_back((int) strtol(token1, (char **)NULL, 10));
            empty.push_back(true);  
            token1 = strtok(NULL, " "); 
        }
    }
    return 0;
}
int umount_fs(char *disk_name){
    char temp[4096];
    if(block_read(0, temp) < 0){return -1;}
    string Signature = "READY";
    if(Signature != (string)temp){return -1;}
    fds.clear();
    empty.clear();
    memset(temp, 0, sizeof(temp));
    for(int i = 0; i < 64; i++){
        string name = DFile[i];
        int size = DSize[i];
        int start = DStart[i];
        memcpy(temp + strlen(temp), (name + ' ').c_str(), name.length() + 1);
        memcpy(temp + strlen(temp), (to_string(size) + ' ').c_str(), to_string(size).length() + 1);
        memcpy(temp + strlen(temp), (to_string(start) + ' ').c_str(), to_string(start).length() + 1);
    }
    if(block_write(1, temp) < 0){return -1;}
    memset(temp, 0, sizeof(temp));
    for(int i = 2; i <= 33; i++ ){
        for(int j = 0; j < 128; j++){
            int block = Fat[128*(i-2) + j];
            memcpy(temp + strlen(temp), (to_string(block) + ' ').c_str(), to_string(block).length() + 1);
        }
        if(block_write(i, temp) < 0){return -1;}
        memset(temp, 0, sizeof(temp));
    }
    if(close_disk() < 0){return -1;}
    return 0;
}
int fs_create(char *name){
    if(strlen(name) > 15 || strlen(name) < 1){return -1;}
    for(int i = 0; i < 64; i++){
        if(DFile[i] == (string)name){return -1;}
    }
    for(int i = 0; i < 64; i++){
        if(DFile[i] == "zzzzzzzzzzzzzzz"){
            DFile[i] = name;
            DSize[i] = 0;
            DStart[i] = 5000; 
            return 0;
        }
    }
    return -1;
}
int fs_open(char *name){
    if(strlen(name) > 15 || strlen(name) < 1){return -1;}
    if(fds.size() == 32){return -1;}
    for(int i = 0; i < 64; i++){
        if(DFile[i] == (string)name){
            FD temp; 
            temp.num = fds.size() + 1;
            temp.offset = 0;
            temp.file = name;
            fds.push_back(temp);
            return temp.num;
        }
    }
    return -1;
}
int fs_close(int fildes){
    for(int i = 0; i < fds.size(); i++){
        if(fds[i].num == fildes){
            fds.erase(fds.begin() + i);
            return 0;
        }
    }
    return -1;
}
int fs_delete(char *name){
    char clear[4096];
    memset(clear, 0, sizeof(clear));
    if(strlen(name) > 15 || strlen(name) < 1){return -1;}
    for(int i = 0; i < 64; i++){
        if(DFile[i] == (string)name){
            for(int j = 0; j < fds.size(); j++){if(fds[j].file == (string)name){return -1;}}
            int START = DStart[i];
            if(START == 5000){
                DStart[i] = 5000;
                DFile[i] = "zzzzzzzzzzzzzzz";
                DSize[i] = 0;
                return 0;
            }
            while(START != 5000){
                int n = START;
                if(block_write(START + 4096, clear) < 0){return -1;}
                START = Fat[START];
                Fat[n] = 5000;
                empty[n] = true;
            }
            DStart[i] = 5000;
            DFile[i] = "zzzzzzzzzzzzzzz";
            DSize[i] = 0;
            return 0;
        }
    }
    return -1;
}
int fs_read(int fildes, void *buf, size_t nbyte){ 
    char temp[4097];
    bool found = false;
    for(int i = 0; i < fds.size(); i++){
        if(fds[i].num == fildes){
            for(int j = 0; j < 64; j++){
                if(DFile[j] == fds[i].file){
                    if(fds[i].offset > DSize[j]){return -1;};
                    int START = DStart[j];
                    int StartBlock = fds[i].offset / 4096;
                    int StartOff = fds[i].offset % 4096;
                    int EndBlock = 0;
                    int EndOff = 0;
                    int total = 0;
                    if((fds[i].offset + nbyte) < DSize[j]){
                        EndBlock =  (fds[i].offset + nbyte) / (4096);
                        EndOff = (fds[i].offset + nbyte) % 4096;
                        if(EndOff == 0){
                            EndOff = 4096;
                        }
                    }
                    else{
                        EndBlock =  DSize[j] / (4096);
                        EndOff = DSize[j] % 4096;
                        if(EndOff == 0){
                            EndOff = 4096;
                        }
                    }
                    if(EndOff == 4096){EndBlock--;}
                    for(int k = 0; k < StartBlock; k++){START = Fat[START];}
                    if(StartBlock == EndBlock){
                        if(block_read(START + 4096, temp) < 0){return -1;}
                        memcpy((char*)buf, temp + StartOff, EndOff - StartOff);
                        fds[i].offset = fds[i].offset + (EndOff - StartOff);
                        return (EndOff - StartOff);
                    }
                    for(int l = StartBlock; l <= EndBlock; l++){
                        if(l == StartBlock){
                            if(block_read(START + 4096, temp) < 0){return -1;}
                            memcpy((char*) buf, temp + StartOff, 4096 - StartOff);
                            total = 4096 - StartOff;
                        }
                        else if(l == EndBlock){
                            if(block_read(START + 4096, temp) < 0){return -1;}
                            memcpy((char*) buf + total, temp, EndOff);
                            total += EndOff;
                            fds[i].offset =  fds[i].offset + total;
                            return total;
                        }
                        else{
                            if(block_read(START + 4096, temp) < 0){return -1;}
                            memcpy((char*)buf + total, temp, 4096);
                            total += 4096;
                        }
                        START = Fat[START];
                        memset(temp, 0, sizeof(temp));
                    }
                }
            }
        }
    }
    return -1;
}
int fs_write(int fildes, void *buf, size_t nbyte){
    char temp[4097];
    bool found = false;
    for(int i = 0; i < fds.size(); i++){
        if(fds[i].num == fildes){
            for(int j = 0; j < 64; j++){
                if(DFile[j] == fds[i].file){
                    if(fds[i].offset > DSize[j]){return -1;}
                    if(nbyte == 0){return 0;}
                    int Blocks = 0;
                    int START = DStart[j];
                    int END = 5000;
                    while(START != 5000){
                        Blocks++;
                        END = START;
                        START = Fat[START];
                    }
                    if((Blocks * 4096) < (fds[i].offset + nbyte)){
                        int need = (((fds[i].offset + nbyte) - (Blocks *4096)) / 4096);
                        if(((fds[i].offset + nbyte) % 4096) > 0){need++;}
                        for(int k = 0; k < need; k++){
                            for(int l = 0; l < 4096; l++){
                                if(empty[l] == true){
                                    if(DStart[j] == 5000){
                                        DStart[j] = l;
                                        empty[l] = false;
                                        END = l;
                                    }
                                    else{
                                        Fat[END] = l;
                                        empty[l] = false;
                                        END = l;
                                    }
                                    Blocks++;
                                    break;
                                }
                            }
                        }
                    }
                    START = DStart[j];
                    int StartBlock = fds[i].offset / 4096;
                    int StartOff = fds[i].offset % 4096;
                    int EndBlock = 0;
                    int EndOff = 0;
                    int total = 0;
                    bool full = false;
                    if((fds[i].offset + nbyte) < (Blocks * 4096)){
                        EndBlock =  (fds[i].offset + nbyte) / (4096);
                        EndOff = (fds[i].offset + nbyte) % 4096;
                        if(EndOff == 0){    
                            EndOff = 4096;
                        }
                    }
                    else{
                        EndOff = 4096;
                        EndBlock =  (Blocks * 4096) / (4096);
                    }
                
                    if(EndOff == 4096){EndBlock--;}
                    for(int k = 0; k < StartBlock; k++){START = Fat[START];}
                    if(StartBlock == EndBlock){
                        if(block_read(START + 4096, temp) < 0){return -1;}
                        memcpy(temp + StartOff,(char*) buf, EndOff - StartOff);
                        if(block_write(START + 4096, temp) < 0){return -1;}
                        if(DSize[j] < (fds[i].offset + nbyte)){DSize[j] = DSize[j] + (EndOff - StartOff);}
                        fds[i].offset = fds[i].offset + (EndOff - StartOff);
                        memset(temp, 0, sizeof(temp));
                        return (EndOff - StartOff);
                    }
                    for(int l = StartBlock; l <= EndBlock; l++){
                        if(l == StartBlock){
                            if(block_read(START + 4096, temp) < 0){return -1;}
                            memcpy(temp + StartOff,(char*)buf, 4096 - StartOff);
                            if(block_write(START + 4096, temp) < 0){return -1;}
                            total = 4096 - StartOff;
                        }
                        else if(l == EndBlock){
                            if(block_read(START + 4096, temp) < 0){return -1;}
                            if(full){
                                memcpy(temp,(char*)buf + total, 4096);
                                total += 4096;
                                if(block_write(START + 4096, temp) < 0){return -1;}
                                fds[i].offset =  fds[i].offset + total;
                                if(DSize[j] < (fds[i].offset + nbyte)){DSize[j] = DSize[j] + total;}
                            }
                            else{
                                memcpy(temp,(char*)buf + total, EndOff);
                                total += EndOff;
                                if(block_write(START + 4096, temp) < 0){return -1;}
                                fds[i].offset =  fds[i].offset + total;
                                if(DSize[j] < (fds[i].offset + nbyte)){DSize[j] = DSize[j] + total;}
                            }
                            memset(temp, 0, sizeof(temp));
                            return total;
                        }
                        else{
                            memcpy(temp,(char*) buf + total, 4096);
                            if(block_write(START + 4096, temp) < 0){return -1;}
                            total += 4096;
                        }
                        START = Fat[START];
                        memset(temp, 0, sizeof(temp));
                    }
                }
            }
        }
    }
    return -1;
}
int fs_get_filesize(int fildes){
    for(int i = 0; i < fds.size(); i++){
        if(fds[i].num == fildes){
            for(int j = 0; j < 64; j++){
                if(DFile[j] == fds[i].file){return DSize[j];}
            }
        }
    }
    return -1;
}
int fs_lseek(int fildes, off_t offset){
    if(offset < 0){ return -1;}
    for(int i = 0; i < fds.size(); i++){
        if(fds[i].num == fildes){
            for(int j = 0; j < 64; j++){
                if(DFile[j] == fds[i].file){
                    if(offset > fs_get_filesize(fildes)){return -1;}
                    else{
                        fds[i].offset = offset;
                        return 0;
                    }
                }
            }
        }
    }
    return -1;
}
int fs_truncate(int fildes, off_t length){
    char clear[4096];
    char temp[4096];
    memset(clear, 0, sizeof(clear));
    for(int i = 0; i < fds.size(); i++){
        if(fds[i].num == fildes){
            for(int j = 0; j < 64; j++){
                if(DFile[j] == fds[i].file){
                    if(length > DSize[j]){return -1;}
                    int EndBlock =  length / 4096;
                    int EndOff = length % 4096;
                    int START = DStart[j];
                    if(START == 5000 || length == 0){return 0;}
                    for(int k = 0; k < EndBlock; k++){START = Fat[START];}
                    if(block_read(START + 4096, temp) < 0){return -1;}
                    if(EndOff != 0){memset(temp + EndOff, 0, sizeof(temp) - EndOff);}
                    else{memset(temp, 0, 4096);}
                    if(block_write(START + 4096, temp) < 0){return -1;}
                    int n = START;
                    START = Fat[START];
                    if(EndOff == 0){
                        if(EndBlock == DStart[j]){DStart[j] = 5000;}
                        Fat[n] = 5000;
                        empty[n] = true;
                    }
                    while(START != 5000){
                        n = START;
                        if(block_write(START + 4096, clear) < 0){return -1;}
                        START = Fat[START];
                        Fat[n] = 5000;
                        empty[n] = true;
                    }
                    fds[i].offset = length;
                    DSize[j] = length;
                    return 0;
                }
            }
        }
    }
    return -1;    
}
