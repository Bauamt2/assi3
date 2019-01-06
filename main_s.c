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
#include <errno.h>
#include <signal.h>

#define NACHRICHTENLAENGE 100
#define KEY 123458L
#define LOCK -1
#define UNLOCK 1
static struct sembuf semaphore;
static int semid;
int sharedID;
int abbruch = 0;

void intHandler(int nix) {
    abbruch = 1;
    printf("Ok, Server wird sofort beendet!\n");
}
//for each player
struct Player{
    int score;
    char name[10];
};
union semun{
    int val; //Value for SETVALUE
    struct semid_ds  *buf;  //Buffer for IPC_STAT,IPC_SET
    unsigned short  *array; //Array for GETALL,SETALL
    struct seminfo *__buf; //Buffer for IPC_INFO

};
//int *shm;
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
;

    semid = semget(KEY, 0, IPC_PRIVATE);

    if(semid < 0){
        //if semaphore does not exists, create new semaphore


        semid = semget(KEY,1,IPC_CREAT|0666);
        if(semid ==-1){
            printf("Cannot create Semaphore.\n");
            return -1;
        }

        union semun arg;
        arg.val = 1;
        if(semctl(semid,0,SETVAL,arg)==-1){             //erster semaphore wird mit 1 initalisiert
            printf("Oh dear, something went wrong with errno: %d! %s\n", errno, strerror(errno));
            printf("Cannot initialize semaphore with one.\n");
            return -1;
        }
    }

    return 0;
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
    printf("Create shared memory\n");
    //key_t sharedMKey = 42;
    key_t sharedMKey = ftok("main_s.c",'1');
    sharedID = shmget(sharedMKey,10* sizeof(struct Player),IPC_CREAT|0666);
    if(sharedID <0){
        printf("Oh dear, something went wrong with errno: %d! %s\n", errno, strerror(errno));
        printf("Error while getting the shared memory.\n");
        return -1;
    }
    return 0;
}

/**Attaches the shared Memory to our data space;
 * prints an error message, when it occurs an error
 *@return 0 if the attaching was successful
 */
int attachSharedMemory(){
    printf("Attach shared memory\n");
    shm = (struct Player *)shmat(sharedID, NULL, 0);

    /*if (shm ==  (int *)-1) {
        printf("Oh dear, something went wrong with errno: %d! %s\n", errno, strerror(errno));
        printf("Error while attaching shared Memory.\n");
        return -1;
    }
    */

    return 0;
}

/**Creates the scoretable and put it in the shared memory
 *
 */
void create_ScoreTable(){


    semaphoreUsing(LOCK);
    struct Player scoretable [10];
    memcpy(scoretable,shm,sizeof(scoretable));

    for(int i=0; i < 10;i++){


        scoretable[i].score = 0;
        char *name_ptr=scoretable[i].name;
        memset(name_ptr,' ',9);
        scoretable[i].name[9] = '\0';
        printf("Name : %s; Score %i\n",scoretable[i].name,scoretable[i].score);
    }



    memcpy(shm,scoretable,sizeof(scoretable));

    semaphoreUsing(UNLOCK);

}

/**Locks the critical section and gets the score table;after that unlocks the critical section
 * @return pointer to the output for the user
 *
 */
void readScoreTable(){
    semaphoreUsing(LOCK);
    printf("Read Score table\n");
    struct Player scoretable[10];
    memcpy(scoretable,shm,sizeof(scoretable));
    for(int i=0;i<10;i++){
        printf("Name: %s Score:%i\n",scoretable[i].name,scoretable[i].score);
    }


    semaphoreUsing(UNLOCK);

}

/**Converts the given integer to the corresponding char array
 * @param number the int number
 * *@return array_ptr a pointer to the char array
 */
char * convertIntToChar(int number){
    char * array_ptr;
    if(number <10){
        char charArray[2]={number+'0','\0'};
        array_ptr= charArray;
    }
    else if(number< 100){
        char charArray[3];
        charArray[0]=(number/10)+'0';
        charArray[1]=(number%10)+'0';
        charArray[2]='\0';
        array_ptr= charArray;
    }
    else{
        char charArray[4]={'1','0','0','\0'};
        array_ptr= charArray;
    }
    return array_ptr;
}

/**Returns the line of the scoretable; including next line (\n)
 *
 * @param line line of the scoretable
 * @return outputline_ptr a pointer to an char array which contains the whole line
 */
char * readScoreTableLine(int line){
    if(line<10 && line>=0){
        semaphoreUsing(LOCK);

        struct Player scoretable[10];
        memcpy(scoretable,shm,sizeof(scoretable));

        //create score array
        char *score_ptr = convertIntToChar(scoretable[line].score);
        int count=0;
        char *count_ptr;
        count_ptr=score_ptr;
        for(int i=0;*count_ptr!='\0';i++){
            count++;
            count_ptr++;
        }
        char score[count+1];

        for(int i=0;*score_ptr !='\0';i++) {
            score[i]=*score_ptr;
            score_ptr++;
        }
        score[count]='\0';


        int length=6+ sizeof(scoretable[line].name)+1+7+sizeof(score); //Name: %s Score: %i\n\0
        char outputline[length];
        printf("recaodres length: %d\n",length);
        char * outputline_ptr;


        //create outputline
       // strcpy(outputline,"Name: ");
        //strcat(outputline,scoretable[line].name);
        //strcat(outputline," Score: ");
        //strcat(outputline,score);
        sprintf(outputline,"Name: %s Score: %s",scoretable[line].name,score);
        outputline_ptr = outputline;
        semaphoreUsing(UNLOCK);
        printf("%s\n",outputline_ptr);
        printf("size pointer:%u\n",strlen(outputline_ptr));
        return outputline_ptr;
    }
}

/**Insert the player to the scoretable if the score is greater than one entry in the scoretable
 * @param points the points the player has
 * @param *name_pointer points to the first character of the playersname
 */
void writeScoreTable(int points, char *name_ptr){
    //schreibe einen neuen wert in die Tabelle;falls möglich
    printf("Write Scoretable\n");
    semaphoreUsing(LOCK);
    struct Player scoretable[10];
    memcpy(scoretable,shm,sizeof(scoretable));


    //create new Player
    struct Player newPlayer;
    newPlayer.score=points;
    int count=0;
    //save name
    while(1) {
        if(*name_ptr == '\0'){
            newPlayer.name[count]='\0';
            break;
        }
        newPlayer.name[count] = *name_ptr;
        name_ptr++;
        count++;
    }


    //prüfe ob der player in die scoretable kommt
    for(int i=0;i<10;i++){
        if(scoretable[i].score <= newPlayer.score){

            for(int j=9;j>i;j--){
                scoretable[j]=scoretable[j-1];
            }
            scoretable[i]=newPlayer;
            break;
        }
    }

    memcpy(shm,scoretable,sizeof(scoretable));
    semaphoreUsing(UNLOCK);
}

/**Checks if the client with the given name is in the scoretable
 *
 * @param name of the client
 * @return 1 if the client is in the scoretable; else 0
 */
int isInScoreTable(char *name,int score){
    semaphoreUsing(LOCK);
    struct Player scoretable[10];
    memcpy(scoretable,shm,sizeof(scoretable));
    for(int i=0;i<10;i++){
        char *playername = scoretable[i].name;
        if(*playername == *name) {
            if(scoretable[i].score == score){
                semaphoreUsing(UNLOCK);
                return 1;
            }
        }
    }
    semaphoreUsing(UNLOCK);
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
    //printf("BEGINNE DELETE :%s\n",postfix);
    int length=1;

    for(int i=0;*(postfix+i) != '\0';i++) {

        length++;
    }

    //array for the new input without whitespace
    char input[length];


    //delete all whitespace
    int count =0;
    char* start;
    start =postfix;

    while(1){

        if(*postfix == ' ' || *postfix == '\0'){
            //create temp array which contains the character before ' '
            int tempLength= postfix -start;
            char temp[tempLength];

            for (int i = 0; i < tempLength; i++) {
                temp[i] = *start + i;
            }

            //is it a operationsymbol?
            if(isOperationsymbol(temp[0])){
                input[count]= temp[0];

            }
                //cast temp array to int
            else {
                int number = atoi(temp);

                input[count] = number + '0';
            }
            count++;
            if(*postfix == '\0'){

                break;
            }

            //point to the new start symbol
            start = postfix + 1;

        }

        postfix++;
    }

    //letztes zeichen

    input[count] = '\0';


    char *input_ptr;
    input_ptr=input;

    return strdup(input_ptr);

}

/**Calculate the result of the given postfix notation
 * @param recvbuffer pointer to the first symbol of the postfix notation
 * @return result
 */
int berechnePostfix(char* recvbuffer){

    int result =0;
    //Links,rechts,mitte
    char *postfix;
    char *free_ptr;

    postfix=deleteWhitespace(recvbuffer);
    free_ptr = postfix;



    int firstnumber=0;
    int secondnumber=0;
    int stack[7]={0}; //initalize all elements to zero

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
    free(free_ptr);

    return result;

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
        }
        else if (pruefe[i]-'0' >= 1 && pruefe[i]-'0' <= 100) {//prüft ob an der Stelle eine Zahl ist 1-100
            int it = 1;
            temp[0] = pruefe[i];
            while (1) {

              if (pruefe[i + it]-'0' >= 1 && pruefe[i + it]-'0' <= 9) {//prüft ob ZAhl 1-9
                    temp[it] = pruefe[i + it];
                    it++;
                    if (it > 3) {
                        free(pruefe);
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
                free(pruefe);
                return 0;//Unerlaubte ZAhl eingegeben
            }

        } else if (pruefe[i] == '\0') {
            free(pruefe);
            return 1;//EIngabe durchgearbeitet ohne Fehler
        } else {
            free(pruefe);
            return 0;//unbekanntes Zeichen vorhanden
        }

    }


    return korrekt;
}

char* checkInput(char *recvbuffer,int e[]){
    printf("Check input\n");
    char errormessage[100];
    char *error_ptr;
    error_ptr=errormessage;
    char *start;
    start=recvbuffer;
    int whitespaceCount =0;
    int endLoop=0;

    while(1){
        if(*recvbuffer==' ' || *recvbuffer == '\0'){
            if(*recvbuffer == '\0'){
                if(whitespaceCount ==0){
                    break;
                }
                endLoop=1;
            }

            int tempLength= recvbuffer -start;
            char temp[tempLength];

            for (int i = 0; i < tempLength; i++) {
                temp[i] = *start + i;
            }


            int foundOperation =0;
            //check the syntax
            for(int i=0;i<tempLength;i++){
                //too much whitespace
                if(isOperationsymbol(temp[i]) && i=0){
                    if(tempLength >1){
                        sprintf(errormessage,"Use withespace between operationsymbol and next symbol. Symbol is after: %c",temp[i]);
                        return error_ptr;
                    }
                    foundOperation =1;
                }
                if(temp[i]==' '){

                    sprintf(errormessage,"Too much whitespace is used.");
                    return error_ptr;
                }
                //operationsymbol at the wrong position
                else if(i!=0 && isOperationsymbol(temp[i])){

                    sprintf(errormessage,"Operationsymbol is at a wrong position. Insert whitespace around it. Symbol is: %c",temp[i]);
                    return error_ptr;
                }
                //wrong symbol
                else if(atoi(&temp[i])==0 && isOperationsymbol(temp[i])==0){

                    sprintf(errormessage,"You use a wrong symbol. Symbol is: %c",temp[i]);
                    return error_ptr;
                }

            }


            if(foundOperation ==0){
                int found =0;
                int number=atoi(temp);
                if(number >0 && number <=100){

                    for(int i=0;i<7;i++){

                        if(e[i] == number){
                            found =1;
                            break;
                        }
                    }

                }
                if(found ==0){
                    sprintf(errormessage,"You used a wrong number. Number is: %i",number);
                    return error_ptr;
                }
            }

            start = recvbuffer+1;

            whitespaceCount++;

        }
        recvbuffer++;
        if(endLoop){
           break;
       }
    }
    if(whitespaceCount ==0){
        sprintf(errormessage,"You used no whitespace.");
    }
    else{
        sprintf(errormessage,"Success");
    }
    return error_ptr;
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
    else {
        return 100 + difference ;
    }
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
    signal(SIGINT, intHandler);

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
    printf("p1\n");
    if(create_semaphore()==-1){
        exit(1);
    }
    printf("p2\n");
    if(create_sharedMemory()==-1){
        exit(1);
    }
    printf("p3\n");
    if(attachSharedMemory()==-1){
        exit(1);
    }



    //create the scoretable
    create_ScoreTable();


    //berechnePostfix(test);

    //getUsersScore(test,10);
    char name[2]={'t','\0'};
    char *name_ptr=name;

    char nametwo[2]={'s','\0'};
    char *name_ptrtwo=nametwo;

    writeScoreTable(10,name);
    writeScoreTable(5,name);
    writeScoreTable(8,name);
    writeScoreTable(88,name);
    writeScoreTable(100,name);
    writeScoreTable(76,name);
    writeScoreTable(46,name);
    writeScoreTable(0,name);
    writeScoreTable(37,name);

    char postfixinput[10]={'2','3','2',' ','4',' ','-','\0'}; //zu große zahl
    char postfixinput2[10]={'2','3',' ',' ','4',' ','-','\0'}; //zu viele leerzeichen

    char postfixinput3[10]={'2','3',' ','+','4',' ','-','\0'}; //+4
    char postfixinput4[10]={'2','3',' ','#',' ','-','\0'};//#
    char postfixinput5[10]={'2','3','2','4','-','\0'};//keine leerzeichen

    char *postfixinput_ptr=postfixinput;
    char *postfixinput_ptr2=postfixinput2;

    char *postfixinput_ptr3=postfixinput3;
    char *postfixinput_ptr4=postfixinput4;

    char *postfixinput_ptr5=postfixinput5;
    int d[4]={4,2,5,23};
    printf("%s\n",checkInput(postfixinput_ptr,d));
    printf("%s\n",checkInput(postfixinput_ptr2,d));
    printf("%s\n",checkInput(postfixinput_ptr3,d));
    printf("%s\n",checkInput(postfixinput_ptr4,d));
    printf("%s\n",checkInput(postfixinput_ptr5,d));




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
while(parent == 1 && abbruch == 0) {
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
    close(mysocket);//schließe verbindung

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


    while(1==1 && abbruch == 0){//Schleife in der Nachrichten verarbeitet werden
        waitRecv(consocket,recvbuffer);//Empfängt Nachricht vom Client
        printf("Nachricht vom Client: %s\n",recvbuffer);//printet diese aus


        if(strncmp(recvbuffer,"QUIT",4)== 0){//erkenne disconnect vom Client
            break;

        }else if(strncmp(recvbuffer,"TOP",3)== 0){//erkenne TOP vom Client

            printf("SPIELER VERLANGT TOP 10\n");
            //send all lines of the scoretable in the correct order to the client
           for(int i=0;i<10;i++){

               send(consocket,readScoreTableLine(i),strlen(readScoreTableLine(i)),0); //send the line to the client
               printf("size: %d\n",strlen(readScoreTableLine(i)));
               usleep(5000);
           }
            //TODO: JN Tabelle übergeben und ausgeben(hoffe das klappt)



        }else if(kontrolliereSyntax(recvbuffer,e)){
            printf("syntax gültig\n");
            int spielererg = getUsersScore(recvbuffer,erg);

            writeScoreTable(spielererg,spielername);
            if(isInScoreTable(spielername,spielererg)) {
                //client is in the scoretable

                sprintf(sendbuffer, "Gültige Postfix erkannt: %i Du bist damit in die top10 gekommen!",
                        spielererg);//bereitet den Antworttext vor
                send(consocket, sendbuffer, strlen(sendbuffer), 0);
            }
            else{
                //client is outside the scoretable
                sprintf(sendbuffer, "Gültige Postfix erkannt: %i Du bist damit nicht in die top10 gekommen!",
                        spielererg);
                send(consocket, sendbuffer, strlen(sendbuffer), 0);

            }
        }else{
            printf("syntax ungültig\n");
            sprintf(sendbuffer,"Keine gültige Nachricht: %s: %s\n",spielername,recvbuffer);//bereitet den Echo Antworttext vor
            send(consocket,sendbuffer,strlen(sendbuffer),0);//sendet diesen
        }


    }













    printf("Verbindung geschlossen von: %s\n",inet_ntoa(dest.sin_addr));
    close(consocket);//schließe verbindung
    close(mysocket);//schließe verbindung

    return 0;
}

