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
#include<time.h>
#define SIZE 4096
#define offset 200

// programul primeste un singur parametru => un fisier
// - spre a fi criptat
// programul primeste fisierul criptat, respectiv permutarile folosite
// - returneaza fisierul decriptat (+ sterge fisierul cu permutari)

int getNumber(int *permutation, int length);
void generatePermutation(int * permutation, int length);

int main(int argc, char *argv[]){
    pid_t pid;

    if (argc == 2){
        // programul a primit un singur parametru, un fisier care trebuie criptat
        // criptarea fisierului

        char *src_path = argv[1]; // fisierul transmis ca parametru

        // folosesc o structura stat pentru a colecta informatii despre src_path
        struct stat stat_buf;
        if(stat(src_path, &stat_buf) < 0){
            // s-a produs o eroare => terminarea procesului
            perror("Stat failed to load");
            return errno;
        }

        int SRC_SIZE = stat_buf.st_size; // dimensiunea fisierului

        // fisier de memorie partajata
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
        // mapez sursa
        void *shm_ptr = mmap(0, SRC_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

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

            // creez un fisier de memorie partajata si pentru a stoca permutarile folosite
            // de catre fiecare proces in parte
            char *shm_keys = "keys_file";

            int dst_key_fd = shm_open(shm_keys, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
            if(dst_key_fd < 0){
                perror("Open shm_keys mem error");
                return errno;
            }
            // def dimensiune
            if(ftruncate(dst_key_fd, SIZE) == -1){
                perror("Truncate error");
                return errno;
            }

            // mapez fisierul de permutari pentru a il putea formata
            void *key_ptr = mmap(0, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, dst_key_fd, 0);

            // citesc datele din fisierul sursa

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

            // parcurg bufferul si extrag toate cuvintele
            char * words[1000];
            
            int word_index = 0;
            shm_ptr = strtok(buff, "\n ,.!?;"); // adaugarea mai multor semne de punctuatie  
            while(shm_ptr != NULL){
                words[word_index++] = (char*)shm_ptr;
                shm_ptr = strtok(0, "\n ,.!?;");
            }
            int word_count = word_index;

            // pentru fiecare cuvant
            // generez o permutare si realizez criptarea acestuia
            // salvez permutarea, respectiv cuvantul criptat in shm_keys, respectiv shm_name
            pid_t encriptor_pid;
            for(int index = 0; index < word_count; index ++){
                
                encriptor_pid = fork();

                if(encriptor_pid < 0){
                    perror("Encription - Fork error");
                    return errno;
                }
                else if(encriptor_pid == 0){
                    char * word = words[index]; //cuvantul curent
                    int word_length = strlen(word); // lungimea cuvantului curent
                    
                    // generez o permutare

                    int *permutation = (int*)malloc(sizeof(int) * word_length);
                    // initializez permutarea                 
                    for(int index = 0; index < word_length; index++){
                        permutation[index] = index;
                    }
                    generatePermutation(permutation, word_length);
                    // scriu permutarea in fisierul shm_keys

                    int number = 0;
                    for(int i = 0; i < word_length; i++){
                        sprintf(key_ptr + (offset * index) + number, "%d ", permutation[i]);
                        number = number + 2;
                    }
                    sprintf(key_ptr + (offset * index) + number, "\n");
                    return 0;
                }
            }
            for(int index = 0; index < word_count; index ++)
                wait(NULL);

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
int getNumber(int * permutation, int length){
    // algoritmul fisher-yates
    srand(time(NULL));

    // extrag un numar random de la 0-length
    int index = rand() % length;
    // si aleg numarul din permutare care se gaseste la indexul random extras
    int number = permutation[index];

    // salvez ultimul nr din vector, pe pozitia random aleasa
    permutation[index] = permutation[length - 1];
    // returnez numarul care se afla initial pe pozitia random
    // numar care este sters din array
    // pentru a nu fi ales din nou
    return number;
}
void generatePermutation(int * permutation, int length){
    // generator de permutari in sigma(lungimea cuvantului)

    int word_length = length;

    while(length > 0){
        // cu fiecare iteratie, salvez la adresa ultimului numar din array un numar random
        // si continui sa modific bucata de array ramasa
        
        permutation[length - 1] = getNumber(permutation, length);
        length = length - 1;
    }
    // <=> este realizat un swap intre numarul random ales si ultimul numar din array
}
// // fisierul destinatie, folosind denumirea unica a fisierului sursa, filename.out
            // char *dst_source = strcat(strtok(src_path, "."), ".out");

            // int dst_fd = open(dst_source, O_RDWR | O_CREAT, S_IRWXU);
            // if(dst_fd < 0){
            //     perror("Open destination file failed");
            //     return errno;
            // }

            // fisierul cu permutarile folosite in codarea cuvintelor, filename_key.out
            //char *dst_key = strcat(strtok(src_path, "."), "_key.out");

            //int dst_key_fd = open(dst_key, O_RDWR | O_CREAT, S_IRWXU);
            // if(dst_key_fd < 0){
            //     perror("Open destination key file failed");
            //     return errno;
            // }