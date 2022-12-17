#include<stdlib.h>
#include<sys/stat.h>
#include<errno.h>
#include<fcntl.h>
#include<stdio.h>

// programul primeste un singur parametru => un fisier
// - spre a fi encriptat, respectiv decriptat
int main(int argc, char *argv[]){

    // realizez citirea fisierului sursa

    // extrag denumirea acestuia din randul argumentelor
    char* src_path = argv[1];

    // folosesc o structura stat pentru a colecta informatii despre src_path
    struct stat stat_buf;
    if(stat(src_path, &stat_buf) < 0){
        // s-a produs o eroare => terminarea procesului
        perror("Stat failed to load");
        return errno;
    }
    
    // deschid fisierul cu permisiunea de read
    int file_descriptor = open(src_path, O_RDONLY);
    if(file_descriptor < 0){
        // eroare la deschidere => terminarea procesului
        perror("Open source file failed");
        return errno;
    }
    
    // buffer in care voi citi datele din fisierul sursa
    char *buff = (char*) malloc(stat_buf.st_size * sizeof(char) + 1);
    if(buff == NULL){
        perror("Malloc failed");
        return errno;
    }
    
    // citirea datelor
    size_t bytes_read = read(file_descriptor, buff, stat_buf.st_size);
    if(bytes_read < 0){
        perror("Read failed");
        return errno;
    }
    for(size_t index = bytes_read; index < stat_buf.st_size; index += bytes_read){
        bytes_read = read(file_descriptor, buff + index, stat_buf.st_size - index);
        if(bytes_read < 0){
            perror("Read failed");
            return errno;
        }
    }
    buff[stat_buf.st_size] = '\0';

    return 0;
}