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
#include <sys/shm.h>

#define NACHRICHTENLAENGE 100
#define KEY 123458L
#define LOCK -1
#define UNLOCK 1
static struct sembuf semaphore;
static int semid;
int sharedID;
//for each player
struct Player{
    int score;
    char * name;
};
struct Player *shm;
struct Player *scoretable_ptr;


int erlaubteZahl(int temp,int e[]){

    for(int i=0;i<7;i++){

        if(temp == e[i]){
            return 1;
        }

    }

    return 0;
}

/**Checks if the char is a operation symbol like +,*,-,/
 * @param symbol
 * @return 1 if it is a opertion symbol
 *
 */
int isOperationsymbol(char symbol){
    if(symbol == '+'|| symbol == '-'||symbol=='*' || symbol=='/'){
        return 1;
    }
    return 0;
}

/**Deletes all the whitespace between the numbers and the symbols
 *@param postfix a char array in postfix notation
 *@return a pointer to new char array without whitespace and each number in one index
 *
 */
char* deleteWhitespace(char* postfix){
    //Length of the input
    printf("BEGINNE DELETEW\n");
    int length=0;
    while(*postfix != '\0') {

        length++;
        postfix++;//habe diese Zeile ergänzt, sonst endlosschleife
    }
    //array for the new input without whitespace
    char input[length];
    int count =0;
    //delete all whitespace
    char* start = postfix;
    for(int i=0;*(postfix+i)!='\0';i++){
        if(*postfix+i == ' '){
            //create temp array which contains the character before ' '
            char temp[postfix - start];
            for (int i = 0; i < sizeof(temp); i++) {
                temp[i] = *start + i;
            }
            //is it a operationsymbol?
            if(isOperationsymbol(temp[0])){
                input[count]= *postfix+i;
            }
                //cast temp array to int
            else {
                int number = atoi(temp);
                input[count] = number + '0';
            }
            count++;
            //point to the new start symbol
            start = postfix+i+1;
        }
    }
    printf("FERTIG DELETE: %s\n",input);

    char *input_ptr = input;
    return input_ptr;
}

/**Calculate the result of the given postfix notation
 * @param recvbuffer pointer to the first symbol of the postfix notation
 * @return result
 */
int berechnePostfix(char* recvbuffer){
    int result =0;
    //Links,rechts,mitte
    char *postfix = deleteWhitespace(recvbuffer);

    int firstnumber=0;
    int secondnumber=0;
    int stack[7];
    int top = -1;

    while(*postfix!='\0'){
        if(isOperationsymbol(*postfix)){
            //pull n1 and n2 from the stack
            firstnumber= stack[top--]; //pull
            secondnumber=stack[top--];  //pull

            //which operation?
            //calculate result
            switch(*postfix){
                case '+':
                    result = firstnumber+secondnumber;
                    break;
                case '-':
                    result = firstnumber-secondnumber;
                    break;
                case '*':
                    result = firstnumber*secondnumber;
                    break;
                case '/':
                    result = firstnumber/secondnumber;
                    break;
                default:
                    break;
            }
            stack[++top]= result; //push result on the stack
        }
        else{
            stack[++top] = *postfix -'0'; //push number on the stack
        }
        postfix++;
    }
    result = stack[top];
    return result;
}

/**Calculate the score for the given input
 * @param recvbuffer the input
 * @param correctResult the correct Result
 * @return -1, if the input is syntactical wrong; or 100- |difference between the user result and the correct result|
 *
 */
int getUsersScore(char* recvbuffer,int correctResult){
    int result = berechnePostfix(recvbuffer);
    int difference=correctResult-result;
    if(difference >=0){
        return 100-difference;
    }
    else
        return 100-(difference*-1);
}


int kontrolliereSyntax(char* recvbuffer, int e[]){
    //Returne 1, wenn recvbuffer eine korrekte Postfix notation ist UND
    // wenn jede der nummern nur maximal einmal oder garnicht verwendet wurden UND
    // nur die 4 erlaubten Operationen +-/* verwendet wurden
    //Vielleicht helfen dir meine Methoden deleteWhitespace() und isOperationsymbol() :) Pat ;YO, danke :D
    //JN
    int i = 0;
    int korrekt= 1;
    char temp[4];

    if(recvbuffer[1] == '\0'){
        return 0;
    }
    char* pruefe = deleteWhitespace(recvbuffer);

printf("ich pruefe jetzt: %s\n",pruefe);
while(1) {
    if (isOperationsymbol(pruefe[i])) {
        i++;
    } else if (atoi(pruefe[i]) >= 1 && atoi(pruefe[i]) <= 9) {//prüft ob an der Stelle eine Zahl ist 1-9
        int it = 1;
        temp[0] = pruefe[i];
        while (1) {

            if (atoi(pruefe[i + it]) >= 1 && atoi(pruefe[i + it]) <= 9) {//prüft ob ZAhl 1-9
                temp[it] = pruefe[i + it];
                it++;
                if (it > 3) {
                    return 0;//Zahl ist >4 Stellen lang, Fehler!
                }
            } else {
                temp[it] = '\0';
                break;
            }//ab dieser Stelle beinhaltet temp eine Benutzer Zahl
            //prüfe ob diese Zahl genutzt werden darf mit array e[]


        }
        i = i + it;
        if (!erlaubteZahl(atoi(temp), e)) {
            return 0;//Unerlaubte ZAhl eingegeben
        }

    } else if (pruefe[i] == '\0') {
        return 1;//EIngabe durchgearbeitet ohne Fehler
    } else {
        return 0;//unbekanntes Zeichen vorhanden
    }

}


    return korrekt;
}
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
        semid = semget(KEY,1,IPC_CREAT); //TODO: Bei Fehlern auskommentieren
        if(semid <0){
            printf("Cannot create Semaphore.");
            return -1;
        }
        if(semctl(semid,1,SETVAL,(int) 1)==-1){             //erster semaphore wird mit 1 initalisiert
            printf("Cannot initialize semaphore with one.");
            return -1;
        }
    }
    return 1;
}
/**Try to change the value of the semaphore variable;
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

/**Creates a new shared Memory with the size for the scoretable;
 * Prints an error when the creation failed
 * @return 0 if the creation was successful
 *
 */
int create_sharedMemory(){
    key_t sharedMKey = 42;
    sharedID = shmget(sharedMKey,10* sizeof(struct Player),IPC_CREAT|0666);
    if(sharedID <0){
        printf("Error while getting the shared memory.");
        return 1;
    }
    return 0;
}

/**Attaches the shared Memory to our data space;
 * prints an error message, when it occurs an error
 *@return 0 if the attaching was successful
 */
int attachSharedMemory(){
    shm = shmat(sharedID,NULL,0);
    if(shm == (char*) -1){
        printf("Error while attaching shared Memory.");
        return 1;
    }
    return 0;
}

/**Creates the scoretable and put it in the shared memory
 *
 */
void create_ScoreTable(){
    scoretable_ptr = shm;
    struct Player scoretable [10];
    for(int i=0; i <sizeof(scoretable);i++){
        scoretable[i].score =0;
        scoretable[i].name = " ";
    }
    *scoretable_ptr=scoretable[0];
}

/**Locks the critical section and gets the score table;after that unlocks the critical section
 * @return pointer to the output for the user
 *
 */
char* readScoreTable(){
    semaphoreUsing(LOCK);

    scoretable_ptr = shm;
    char scoreTable[1024];
    char *output=scoreTable[0];
    //Title
    strcat(output,"Name\tScore");
    //one row in the table
    for(int i=0;i<10;i++){
        struct Player p = *(scoretable_ptr+i);
        strcat(output,p.name);
        strcat(output,'\t');
        strcat(output,p.score);
    }

    semaphoreUsing(UNLOCK);
    return output;
}

/**
 * @param points the points the player has
 * @param *name_pointer points to the first character of the playersname
 */
void writeScoreTable(int points, char *name_pointer){
    //schreibe einen neuen wert in die Tabelle;falls möglich
    semaphoreUsing(LOCK);
    scoretable_ptr = shm;
    //for(int i=0;) TODO: Ausgeklammert wegen Syntaxfehler
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

    //Create semaphore and shared memory

    if(create_semaphore()==-1){
        exit(1);
    }

    if(create_sharedMemory()==1){
        exit(1);
    }

    if(attachSharedMemory()==1){
        exit(1);
    }

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
    char spielername[100];


    printf("Verbindung gestartet von: %s\n",inet_ntoa(dest.sin_addr));//Zeige Ip des Clients an

    sprintf(sendbuffer,"%d",erg);//diese beiden Zeilen senden erg
    send(consocket,sendbuffer,strlen(sendbuffer),0);

    for(int i=0;i<7;i++){//DIese Schleife sendet alle werte von e[]
        usleep(1000);
        sprintf(sendbuffer,"%d",e[i]);//diese beiden Zeilen senden erg
        send(consocket,sendbuffer,strlen(sendbuffer),0);
    }

    waitRecv(consocket,recvbuffer);//Spielername empfangen und abgespeichert
    strcpy(spielername,recvbuffer);


    while(1==1){//Schleife in der Nachrichten verarbeitet werden
        waitRecv(consocket,recvbuffer);//Empfängt Nachricht vom Client
        printf("Nachricht vom Client: %s\n",recvbuffer);//printet diese aus


        if(strncmp(recvbuffer,"QUIT",4)== 0){//erkenne disconnect vom Client
            break;

        }else if(strncmp(recvbuffer,"TOP",3)== 0){//erkenne TOP vom Client
            //TODO: SENDE ALLE 10 EINTRÄGE IN DER RICHTIGEN REIHENFOLGE
            // 1. NAME             PUNKTZAHL
            printf("SPIELER VERLANGT TOP 10\n");

            char *scoreTable = readScoreTable(); //get ScoreTable as correct output; saved in *scoreTable
            //TODO: JN Tabelle übergeben und ausgeben(hoffe das klappt)



        }else if(kontrolliereSyntax(recvbuffer,e)){
            printf("1p\n");
            int spielererg = getUsersScore(recvbuffer,erg);
            printf("2p\n");
            //TODO: gebe spielererg und spielername weiter an die Highscoretabelle
            //antworte ob er dmait in die top10 gekommen ist oder nicht
            sprintf(sendbuffer,"Gültige Postfix erkannt: %n Du bist damit ja/nein in die top10 gekommen!",spielererg);//bereitet den Antworttext vor
            send(consocket,sendbuffer,strlen(sendbuffer),0);//sendet diesen
                //TODO: PAT int aktualisiereScoreboard(pielername,score)
        }else{
            printf("3p\n");
            sprintf(sendbuffer,"Keine gültige Nachricht: %s: %s\n",spielername,recvbuffer);//bereitet den Echo Antworttext vor
            send(consocket,sendbuffer,strlen(sendbuffer),0);//sendet diesen
        }


    }













    printf("Verbindung geschlossen von: %s\n",inet_ntoa(dest.sin_addr));
    close(consocket);//schließe verbindung
    close(mysocket);//schließe verbindung

//delete semaphore and shared memory after a program crash
//ipcrm -M;
//ipcrm -s;



    return 0;
}

