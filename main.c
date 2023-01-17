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
#include<ctype.h>

#define SIZE 1024

// programul primeste un singur parametru => un fisier
// - spre a fi criptat
// programul primeste fisierul criptat, respectiv permutarile folosite
// - returneaza fisierul decriptat (+ sterge fisierul cu permutari)

int getNumber(int *permutation, int length);
void generatePermutation(int * permutation, int length);

int main(int argc, char *argv[]){

    if (argc == 2){
        // creez un seed pentru generatorul de elemente ale permutarii
        srand(time(0));

        // programul a primit un singur parametru, un fisier care trebuie criptat
        // -> criptarea fisierului

        // pregatirea datelor pentru criptare
        char *src_path = argv[1]; // fisierul transmis ca parametru
        // deschid fisierul sursa
        int src_fd = open(src_path, O_RDWR, S_IRUSR | S_IWUSR); // file descriptor pentru fisierul sursa
        if(src_fd < 0){
            perror("Open src error");
            return errno;
        }
        // folosesc o structura stat pentru a colecta informatii despre src_path
        struct stat stat_buf;
        if(stat(src_path, &stat_buf) < 0){
            // s-a produs o eroare => terminarea procesului
            perror("Stat failed to load");
            return errno;
        }
        int SRC_SIZE = stat_buf.st_size; // dimensiunea fisierului
        // mapez sursa
        char *src_mem = mmap(NULL, SRC_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, src_fd, 0);

        // creez un fisier de memorie partajata pentru a stoca permutarile folosite
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
        char *key_ptr = mmap(NULL, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, dst_key_fd, 0);

        pid_t pid;
        // citesc datele din fisierul sursa
        int delimitator = 0;
        int word_count = 0;
        int offset = 0;
        for(int i = 0; i <= SRC_SIZE; i++){
            // caracterul este litera, cratima sau apostrof -> face parte dintr-un cuvant
            if(isalpha(src_mem[i]) || src_mem[i] == '-' || src_mem[i] == '\'')
                continue;
            
            // am intalnit un caracter despartitor
            // delimitam cuvantul gasit
            if(isalpha(src_mem[i - 1]) || src_mem[i - 1] == '-' || src_mem[i - 1] == '\'')
            {
                // pozitia cuvantului curent
                char * current_word = src_mem + delimitator; 
                // lungimea cuvantului curent
                int word_length = i - delimitator; 

                char * word = (char *) malloc(sizeof(char) * word_length); //cuvantul curent
                strncpy(word, src_mem + delimitator, word_length);

                // criptarea cuvantului
                pid = fork();

                if(pid < 0){
                    perror("Encriptor fork failed!");
                    return errno;
                }
                if(pid == 0){
                    int *permutation = (int*)malloc(sizeof(int) * word_length);

                    // initializez permutarea
                    for(int index = 0; index < word_length; index++){
                        permutation[index] = index;
                    }
                    generatePermutation(permutation, word_length);
                    
                    for(int index=0; index< word_length; index++)
                        printf("%d ", permutation[index]);
                    printf("\n");

                    // scriu permutarea in fisierul shm_keys
                    int offset_number = 0;
                    for(int index = 0; index < word_length; index++){
                        sprintf(key_ptr + offset + offset_number, "%d ", permutation[index]);
                        offset_number += 2;
                    }
                    sprintf(key_ptr + offset  + offset_number, "\n");

                    // modific cuvantul
                    for(int letter = 0; letter < word_length; letter++)
                        current_word[letter] = word[permutation[letter]];
                    
                    return 0;
                }
                // setam offsetul pentru a scrie urmatoarea permutare in fisier
                offset += 2*word_length + 1;
                // am modificat inca un cuvant
                word_count += 1;
                // cautam urmatorul cuvant, daca exista
            }
            // setam mereu inceputul unui potential nou cuvant
            delimitator = i + 1;
        }
        // asteptam criptarea completa a cuvintelor
        for(int i = 0; i< word_count; i++)
            wait(NULL);
        // crearea fisierului cu key-urile generate la criptare
        //fisierul cu permutarile folosite in codarea cuvintelor, filename_key.out
        char *dst_key = strcat(strtok(src_path, "."), "_key.out");

        int key_fd = open(dst_key, O_RDWR | O_CREAT, S_IRWXU); // file descriptorul fisierului cu cheile generate
        if(key_fd < 0){
            perror("Open destination key file failed");
            return errno;
        }
        // copierea datelor in fisierul generat
        int size = strlen(key_ptr);
        size_t bytes_written = write(key_fd, key_ptr, size);
        if(bytes_written < 0){
            perror("Write failed");
            return errno;
        }
        // ne asiguram ca toate datele din buf au fost transportate corect
        for(size_t index = bytes_written; index < size; index+=bytes_written){

            bytes_written = write(key_fd, key_ptr + index, size - index);
            if(bytes_written < 0){
                perror("Write error");
                return errno;
            }
        }
        if(munmap(src_mem, SRC_SIZE) < 0){
            perror("Munmap error");
            return errno;
        }
        if(shm_unlink(shm_keys) < 0){
            perror("Shm unlink error");
            return errno;
        }
        if(munmap(key_ptr, SIZE) < 0){
            perror("Munmap error");
            return errno;
        }
        return 0;
    }
    else if (argc == 3){
        // decriptarea fisierului
        char *src_path = argv[1]; // fisierul criptat
        char *dst_key = argv[2]; // fisierul cu permutari

        // deschid fisierul sursa
        int src_fd = open(src_path, O_RDWR, S_IRUSR | S_IWUSR); // file descriptor pentru fisierul sursa
        if(src_fd < 0){
            perror("Open src error");
            return errno;
        }
        // folosesc o structura stat pentru a colecta informatii despre src_path
        struct stat stat_buf;
        if(stat(src_path, &stat_buf) < 0){
            // s-a produs o eroare => terminarea procesului
            perror("Stat failed to load");
            return errno;
        }
        int SRC_SIZE = stat_buf.st_size; // dimensiunea fisierului
        // mapez sursa
        char *src_mem = mmap(NULL, SRC_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, src_fd, 0);

        // deschid fisierul cu permutari
        int key_fd = open(dst_key, O_RDWR, S_IRWXU); // file descriptorul fisierului cu cheile generate
        if(key_fd < 0){
            perror("Open key file failed");
            return errno;
        }
        // mapez fisierul de permutari pentru a il putea parcurge
        char *key_ptr = mmap(NULL, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, key_fd, 0);

        pid_t pid;
        // citesc datele din fisierul sursa
        int delimitator = 0;
        int word_count = 0;
        int offset = 0;
        for(int i = 0; i <= SRC_SIZE; i++){
            // caracterul este litera, cratima sau apostrof -> face parte dintr-un cuvant
            if(isalpha(src_mem[i]) || src_mem[i] == '-' || src_mem[i] == '\'')
                continue;
            
            // am intalnit un caracter despartitor
            // delimitam cuvantul gasit
            if(isalpha(src_mem[i - 1]) || src_mem[i - 1] == '-' || src_mem[i - 1] == '\'')
            {
                // pozitia cuvantului curent
                char * current_word = src_mem + delimitator; 
                // lungimea cuvantului curent
                int word_length = i - delimitator; 

                char * word = (char *) malloc(sizeof(char) * word_length); //cuvantul curent
                strncpy(word, src_mem + delimitator, word_length);

                // criptarea cuvantului
                pid = fork();

                if(pid < 0){
                    perror("Encriptor fork failed!");
                    return errno;
                }
                if(pid == 0){
                    int *permutation = (int*)malloc(sizeof(int) * word_length);

                    // extrag permutarea
                    int index = 0;
                    while(key_ptr[offset] != '\n'){
                        permutation[index ++] = key_ptr[offset] - '0';
                        offset ++;

                        if(key_ptr[offset] == '\n')
                            break;
                        offset ++;
                    }

                    for(int e = 0; e < word_length; e++)
                        printf("%d ", permutation[e]);
                    printf("\n");

                    // modific cuvantul
                    for(int letter = 0; letter < word_length; letter++)
                        current_word[permutation[letter]] = word[letter];
                    
                    return 0;
                }
                // setam offsetul pentru a scrie urmatoarea permutare in fisier
                offset += 2*word_length + 1;
                // am modificat inca un cuvant
                word_count += 1;
                // cautam urmatorul cuvant, daca exista
            }
            // setam mereu inceputul unui potential nou cuvant
            delimitator = i + 1;
        }
        for(int i = 0; i < word_count; i++)
            wait(NULL);
            
        // stergem fisierul cu permutarile, nu mai este necesar
        pid = fork();
        if(pid < 0){
            perror("Erase key file - Fork error");
            return errno;
        }
        else if(pid == 0){
            // procesul copil va executa stergerea
            char * rm_argv[] = {"rm", dst_key, NULL}; // argumentele comenzii

            if(execve("/usr/bin/rm", rm_argv, NULL) < 0){
                perror("Erase - Execve error");
                return errno;
            }
            return 0;
        }
        if(munmap(src_mem, SRC_SIZE) < 0){
            perror("Munmap error");
            return errno;
        }
        if(munmap(key_ptr, SIZE) < 0){
            perror("Munmap error");
            return errno;
        }
        return 0;
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
