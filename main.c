#include<stdlib.h>
#include<sys/stat.h>
#include<errno.h>
#include<fcntl.h>
#include<string.h>
#include<stdio.h>
#include<unistd.h>

// programul primeste un singur parametru => un fisier
// - spre a fi criptat
// programul primeste fisierul criptat, respectiv permutarile folosite
// - returneaza fisierul decriptat (+ sterge fisierul cu permutari)

int main(int argc, char *argv[]){

    if (argc == 2){
        // encriptarea fisierului

        char *src_path = argv[1]; // fisierul transmis ca parametru

        // folosesc o structura stat pentru a colecta informatii despre src_path
        struct stat stat_buf;
        if(stat(src_path, &stat_buf) < 0){
            // s-a produs o eroare => terminarea procesului
            perror("Stat failed to load");
            return errno;
        }

        int SRC_SIZE = stat_buf.st_size; // dimensiunea fisierului

        // mapez sursa
        void *src_ptr = mmap(0, SRC_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, )
        
    }
    else if (argc == 3){
        // descriptarea fisierului
    }
    else{
        // apel invalid
        printf("Nr de argumente invalid\n");

        return -1;
    }

    // // realizez citirea fisierului sursa

    // // extrag denumirea acestuia din randul argumentelor
    // char* src_path = argv[1];

    // // folosesc o structura stat pentru a colecta informatii despre src_path
    // struct stat stat_buf;
    // if(stat(src_path, &stat_buf) < 0){
    //     // s-a produs o eroare => terminarea procesului
    //     perror("Stat failed to load");
    //     return errno;
    // }
    
    // // deschid fisierul cu permisiunea de read
    // int file_descriptor = open(src_path, O_RDONLY);
    // if(file_descriptor < 0){
    //     // eroare la deschidere => terminarea procesului
    //     perror("Open source file failed");
    //     return errno;
    // }
    
    // // buffer in care voi citi datele din fisierul sursa
    // char *buff = (char*) malloc(stat_buf.st_size * sizeof(char) + 1);
    // if(buff == NULL){
    //     perror("Malloc failed");
    //     return errno;
    // }
    
    // // citirea datelor
    // size_t bytes_read = read(file_descriptor, buff, stat_buf.st_size);
    // if(bytes_read < 0){
    //     perror("Read failed");
    //     return errno;
    // }
    // // ma asigur ca datele din fisier au fost corect citite
    // for(size_t index = bytes_read; index < stat_buf.st_size; index += bytes_read){
    //     bytes_read = read(file_descriptor, buff + index, stat_buf.st_size - index);
    //     if(bytes_read < 0){
    //         perror("Read failed");
    //         return errno;
    //     }
    // }
    // buff[stat_buf.st_size] = '\0';
    

    // // cream fisierul destinatie, folosind denumirea unica a fisierului sursa
    // char *dst_source = strcat(strtok(file, "."), ".out");
    
    // // deschid fisierul destinatie
    // int dst_descriptor = open(dst_source, O_RDWR | O_CREAT, S_IRWXU);
    // if(dst_descriptor < 0){
    //     perror("Open destination file failed");
    //     return errno;
    // }

    // // verificam daca fisierul sursa trebuie encriptat sau decriptat
    // char *words[], *permutations[];

    // char* aux_parser = strtok(buff, " \n");
    // while(aux_parser != NULL){
        
    //     if(atoi(aux_parser)){
    //         permutations
    //     }
    // }
    return 0;
}