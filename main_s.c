#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

#define NACHRICHTENLAENGE 100
#define KEY 1234
static struct sembuf semaphore;
static int semid;
//
int efuellen(int e[],char* estring){
    char* temp[7];
    int warn =0;

    for(int i = 0;i<7;i++) {
        while (*estring == ' ') {
            estring++;
        }
        temp[i] = estring;
        while (*estring != ' ') {
            estring++;
        }
    }//Hiernach zeigt temp auf den Beginn jedes Zahlenstrings

    for(int i = 0;i<7;i++){
        e[i] = atoi(temp[i]);
        if(e[i] < 1 || e[i] > 100){
            printf("Parameter e fehlerhaft! Es müssen 7 Zahlen zwischen 1 und 100 durch Leerzeichen getrennt angegeben werden!\n");
            return -1;

        }
    }

return 1;
}
int create_semaphore(){

    //try to get existing semaphore
    semid = semget(KEY, 0, IPC_PRIVATE);
    if(semid < 0){
        //if semaphore does not exists, create new semaphore
        semid = semget(KEY,1,IPC_CREAT| IPC_EXCL|PERM);
        if(semid <0){
            printf("Cannot create Semaphore.");
            return -1;
        }
        if(semctl(semid,0,SETVAL,(int) 1)==-1){
            printf("Cannot initialize semaphore with one.")
            return -1;
        }
    }
}

int semaphoreUsing(int operation){
    semaphore.sem_op = operation;
    semaphore.sem_flg = SEM_UNDO;
    if(semop(semid, &semaphore, 1)== -1){
        //Fehler abfangen
    }
}
void getScoreTable(){
    //semaphore anfragen
   semop();//wert des Semaphores auf 0 setzten;später nutzen, um semaphore wieder auf 1 zu setzten
    //schreiben oder lesen?
}

void readScoreTable(){
    //show all results or only the position of one player?
}
void writeScoreTable(int points, char *name_pointer){
    //schreibe einen neuen wert in die Tabelle;falls möglich
}

int main(int argc, char **argv) {// -e 1 2 3 4 5 6 7 -n 199(ergebnis) -t 23000 (port)
    int e[7];
    char* estring = NULL;
    int erg = 0;
    uint16_t port = 0;
    int opt = 0;
while((opt = getopt(argc,argv, "e:n:p:")) != -1){
    switch (opt){
        case 'p':
            port = atoi(optarg);
            break;
        case 'n':
            erg = atoi(optarg);
            break;
        case 'e':
            estring = optarg;
            break;
    }
}

printf("Port: %d\n",port);
printf("Ergebnis: %d\n",erg);
printf("Aufgabe: %s\n",estring);
if(efuellen(e, estring) < 1){
    return -1;
}
if(erg < 1 || erg > 1000){
    printf("Parameter n fehlerhaft! Der Wert muss zwischen 1 und 1000 liegen!\n");
    return -1;
}
if(port < 1024 || port > 49151){
    printf("Parameter p fehlerhaft! Der Wert muss zwischen 1024 und 49151 liegen!\n");
    return -1;
}
for(int i=0;i<7;i++){
    printf("e: %d\n",e[i]);
}

    printf("Parameter OK\n");
//Jetzt muss der server auf Verbindungen warten

char *nachricht = "Willkommen client!";

struct sockaddr_in dest;
struct sockaddr_in serv;
int mysocket;
socklen_t socksize = sizeof(struct sockaddr_in);

memset(&serv, 0 ,sizeof(serv));
serv.sin_family = AF_INET;
serv.sin_addr.s_addr = htonl(INADDR_ANY);
serv.sin_port = htons(port);

mysocket = socket(AF_INET, SOCK_STREAM,0);

bind(mysocket, (struct sockaddr *)&serv, sizeof(struct sockaddr));

int parent =1;
int ende = 0;
while(parent == 1 && ende == 0) {
    listen(mysocket, 10);
    usleep(10000);
   // printf("JUMP!");
    if(fork() == 0){
        //child
        parent = 0;
    }
    //TODO: abbruchbedingung durch eingabe
}

if(parent == 1){
    printf("Server wird beendet!\n");
    return 0;
}


    int consocket = accept(mysocket, (struct sockaddr *) &dest, &socksize);
while(consocket){
    printf("Verbindung von: %s\n",inet_ntoa(dest.sin_addr));
    send(consocket, nachricht, strlen(nachricht),0);
    close(consocket);
    consocket = accept(mysocket, (struct sockaddr *)&dest, &socksize);
}

close(mysocket);





    return 0;
}

