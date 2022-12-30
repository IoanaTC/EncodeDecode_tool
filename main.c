#include<stdlib.h>
#include<stdio.h>
#include<sys/stat.h>
#include<errno.h>
#include<fcntl.h>
#include<string.h>
#include<stdio.h>
#include<unistd.h>
#include<sys/mman.h>
#include<sys/wait.h>


// programul primeste un singur parametru => un fisier
// - spre a fi criptat
// programul primeste fisierul criptat, respectiv permutarile folosite
// - returneaza fisierul decriptat (+ sterge fisierul cu permutari)

int main(int argc, char *argv[]){
    pid_t pid;

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

        // creez un fisier de memorie partajata
        char *shm_name = "sharedmem_file";
        int shm_fd = shm_open(shm_name, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
        if(shm_fd < 0){
            perror("Open shm mem error");
            return errno;
        }
        // def dimensiune
        if(ftruncate(shm_fd, SRC_SIZE) == -1){
            perror("Truncate error");
            return errno;
        }

        // copiez datele din fiserul sursa in fiserul de memorie partajata
        pid = fork();
        if(pid < 0){
            perror("Copy data - Fork error");
            return errno;
        }
        else if(pid == 0){
            // procesul copil va executa copiere
            char * cp_argv[] = {"cp", src_path, "/dev/shm/sharedmem_file", NULL}; // argumentele comenzii

            if(execve("/usr/bin/cp", cp_argv, NULL) < 0){
                perror("Copy Execve error");
                return errno;
            }
            return 0;
        }
        else{
            // procesul parinte realizeaza modificarile
            wait(NULL);

            // buffer in care voi citi datele din fisierul sursa
            char *buff = (char*) malloc(SRC_SIZE * sizeof(char) + 1);
            if(buff == NULL){
                perror("Malloc failed");
                return errno;
            }
            
            // citirea datelor
            size_t bytes_read = read(shm_fd, buff, SRC_SIZE);
            if(bytes_read < 0){
                perror("Read failed");
                return errno;
            }
            // ma asigur ca datele din fisier au fost corect citite
            for(size_t index = bytes_read; index < SRC_SIZE; index += bytes_read){
                bytes_read = read(shm_fd, buff + index, SRC_SIZE - index);
                if(bytes_read < 0){
                    perror("Read failed");
                    return errno;
                }
            }
            buff[SRC_SIZE] = '\0';

            // creez fisierul destinatie, folosind denumirea unica a fisierului sursa
            char *dst_source = strcat(strtok(src_path, "."), ".out");
            // creez fisierul cu permutarile folosite in codarea cuvintelor
            char *dst_key = strcat(strtok(src_path, "."), "_key.out");

            // deschid aceste fisiere
            int dst_descriptor = open(dst_source, O_RDWR | O_CREAT, S_IRWXU);
            if(dst_descriptor < 0){
                perror("Open destination file failed");
                return errno;
            }
            int dst_key_descriptor = open(dst_key, O_RDWR | O_CREAT, S_IRWXU);
            if(dst_key_descriptor < 0){
                perror("Open destination key file failed");
                return errno;
            }

            // mapez sursa
            void *shm_ptr = mmap(0, SRC_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

            // parcurg fisierul sursa
            // iar pentru fiecare cuvant, un proces va genera o permutare si va codifica cuvantul in cauza
            // cuvantul codificat va fi stocat in fisierul cu extensia .out, iar permutarea in fisierul _key.out
            shm_ptr = strtok(buff, "\n ,.!?;");
            while(shm_ptr != NULL){

                pid = fork();
                if(pid < 0){
                    perror("Encription - Fork error");
                    return errno;
                }
                else if(pid == 0){
                    char * word = (char *)malloc(strlen(shm_ptr) * sizeof(char));
                    word = shm_ptr;

                    // generez o permutare
                     

                    return 0;
                }
            }
            return 0;
        }
    }
    else if (argc == 3){
        // decriptarea fisierului
    }
    else{
        // apel invalid
        printf("Nr de argumente invalid\n");

        return -1;
    }
    return 0;
}