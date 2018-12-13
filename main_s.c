#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/sem.h>

#define NACHRICHTENLAENGE 100
#define KEY 123458L
#define LOCK -1
#define UNLOCK 1
static struct sembuf semaphore;
static int semid;
//

/*
 * Diese Funktion funktioneirt genauso wie recv(), jedoch wird hier auf eine Nachricht zwingend gewartet
 * Der Prozess pausiert also bis eine Nachricht empfangen wurde.
 */
void waitRecv(int socket, char* recvbuffer){
    int size=0;
    printf("warte auf paket\n");

    while((size = recv(socket,recvbuffer,100,0)) == -1 || size == 0){
        usleep(1000);
    }
    recvbuffer[size] = '\0';
    printf("!!wr paket Size: %d\n",size);
    printf("!!wr paket Inhalt: %s\n",recvbuffer);

    return;
}
int efuellen(int e[],char estring[]){
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
/**Tries to get the semaphore with the KEY;
 * if the semaphore doesnt exists, a new one is created;
 * The semaphoreid is saved in semid.
 *
 * @return -1, when there are errors while creating the semaphore
 *
 */
int create_semaphore(){

    //try to get existing semaphore
    semid = semget(KEY, 0, IPC_PRIVATE);
    if(semid < 0){
        //if semaphore does not exists, create new semaphore
        //semid = semget(KEY,1,IPC_CREAT| IPC_EXCL|PERM); //TODO: OUTCOMMENTED damits compiliert
        if(semid <0){
            printf("Cannot create Semaphore.");
            return -1;
        }
        if(semctl(semid,0,SETVAL,(int) 1)==-1){
            printf("Cannot initialize semaphore with one.");
            return -1;
        }
    }
    return 1;
}


/**Try to chnage the value of the semaphore variable;
 * if there is an error, it would be print out and exit with 1
 * @param operation; 1 for locking the critical section
 * and -1 to unlock the critical section
 * @return 1, when there are no errors
 *
 */
int semaphoreUsing(int operation){
    semaphore.sem_op = operation;
    semaphore.sem_flg = SEM_UNDO;
    if(semop(semid, &semaphore, 1)== -1){
        //Fehler abfangen?
        perror("semop");
        exit(1);
    }
    return 1;
}


void getScoreTable(int operation){
}

/**
 * @param *name_pointer points to the first character of the playersname
 * ;is NULL if the user want to see the whole table
 */
void readScoreTable(char *name_pointer){
    semaphoreUsing(LOCK);
    //show all results or only the position of one player?
    //TODO Tabelle
    semaphoreUsing(UNLOCK);
}

/**
 * @param points the points the player has
 * @param *name_pointer points to the first character of the playersname
 */
void writeScoreTable(int points, char *name_pointer){
    //schreibe einen neuen wert in die Tabelle;falls möglich
    semaphoreUsing(LOCK);
    semaphoreUsing(UNLOCK);
}

int main(int argc, char **argv) {// -e 1 2 3 4 5 6 7 -n 199(ergebnis) -t 23000 (port)
    int e[7];
    char* anfangestring;
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
            if(efuellen(e, optarg) < 1){
                printf("Fehler beim Parameter e!\n");
                return -1;
            }
            break;
    }
}

printf("Port: %d\n",port);
printf("Ergebnis: %d\n",erg);

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


    int consocket = accept(mysocket, (struct sockaddr *) &dest, &socksize);//Ab hier besteht eine Clientverbindung
    char recvbuffer[101];//Für die empfangene Nachricht vom Client
    char sendbuffer[101];

    printf("Verbindung gestartet von: %s\n",inet_ntoa(dest.sin_addr));//Zeige Ip des Clients an

    while(1==1){
        waitRecv(consocket,recvbuffer);
        printf("Nachricht vom Client: %s\n",recvbuffer);
    }













    printf("Verbindung geschlossen von: %s\n",inet_ntoa(dest.sin_addr));
    close(consocket);//schließe verbindung
    close(mysocket);//schließe verbindung

//delete semaphore and shared memory after a program crash
//ipcrm -M;
//ipcrm -s;



    return 0;
}

