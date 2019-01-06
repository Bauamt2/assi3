#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <signal.h>

int abbruch = 0;
/*
 * wird benötigt für das Beenden mit strg+c
 */
void intHandler(int nix) {
    abbruch = 1;
    printf("Alles klar! Mit Enter können sie das Spiel nun beenden!\n");
}

/*
 * Fragt nach dem namen und gibt ihn per pname weiter.
 */
void erfrageNamen(char* pname){
    system("clear");
    printf("Willkommen im Sopper Dooper Spiel!\n");
    printf("Verrate mir deinen Namen Spieler!\n");

    scanf("%s",pname);

}
/*
 * Überprüft ob die Aufgabe vom Server erfolgreich empfangen wurde und gültig ist.
 */
int pruefeAufgabe(int erg,int e[]){
    if(erg < 1 || erg > 1000){
        return 0;
    }
    for(int i=0;i<7;i++){
        if(e[i] < 1 || e[i] > 100){
            return 0;
        }
    }
    return 1;
}
/*
 * Diese Funktion zeigt immer wieder den Selben SPielscreen, ist erheblich schöner als alles nur untereinander zu schreiben.
 */
void zeigeAufgaben(int erg, int e[],char name[],char lastcmd[],char lastanswer[]){
    system("clear");
    printf("Willkommen im Sopper Dooper Spiel!\n");
    printf("%s, du hast folgende Zahlen zur Auswahl: ",name);
    for(int i=0;i<7;i++){
        printf("%d ",e[i]);
    }
    printf("\n");
    printf("Das Ergebnis soll möglichst nah an %d liegen!\n",erg);
    printf("Gib deine Rechnung in Postfixnotation ein und vielleicht landest du im Scoreboard!\n");
    printf("Viel Glück!\n\n\n\n");
    printf("Weitere Befehle:\n");
    printf("TOP : zeigt die bisherige Top 10 an in der auch du schon bald sein kannst!\n");
    printf("QUIT : es wäre zwar schade, aber mit diesem Befehl kannst du das Spiel beenden\n\n\n");

    printf("Spielleiter: %s\n",lastanswer);
    printf("Ihre letzte Eingabe: %s\n",lastcmd);
}
/*
 * Diese Funktion funktioneirt genauso wie recv(), jedoch wird hier auf eine Nachricht zwingend gewartet
 * Der Prozess pausiert also bis eine Nachricht empfangen wurde.
 */
void waitRecv(int socket, char* recvbuffer){
    int size=0;
    //printf("warte auf paket\n");
    int wartezeit =0;
    while((size = recv(socket,recvbuffer,100,0)) == -1 || size == 0){
        if(abbruch == 0) {
            usleep(1000);
        }else{
            send(socket,"QUIT",4,0);
            printf("Client wird beendet...\n");
           exit(2);
           return;
        }
        wartezeit++;
        if(wartezeit > 5000){
            abbruch =1;
            send(socket,"QUIT",4,0);
        }
    }
    recvbuffer[size] = '\0';
   // printf("!!wr paket Size: %d\n",size);
    //printf("!!wr paket Inhalt: %s\n",recvbuffer);

    return;
}
/*
 * Diese Funktion wandelt einen Hostnamen in eine ip um(beides als String)
 * SOllte es nicht klappen wird 0 returnt,
 * sollte es erfolgreich sein wird 1 returnt.
 */
int hostnameToIp(char hostname[],char ip[]){
    struct hostent* host;
    struct in_addr** addr;

    if((host = gethostbyname(hostname)) == NULL){
        return 0;
    }

    addr = (struct in_addr**) host->h_addr_list;

    for(int i=0;addr[i] != NULL;i++){
        strcpy(ip, inet_ntoa(*addr[i]));
        return 1;
    }
    return 0;
}
int main(int argc, char **argv) {// -h hostname -p port
    signal(SIGINT, intHandler);
    printf("HALLO ICH BIN DER CLIENT\n");
int opt;
char  hostname[30] = "";
int port=0;

    while((opt = getopt(argc,argv, "h:p:")) != -1){
        switch (opt){
            case 'p':
                port = atoi(optarg);
                break;
            case 'h':
                strcpy(hostname,optarg);
                break;
        }
    }
    if(port < 1024 || port > 49151){
        printf("Parameter p fehlerhaft! Der Wert muss zwischen 1024 und 49151 liegen!\n");
        return -1;
    }
    if(hostname == ""){
        printf("Parameter h fehlerhaft!\n");
        return -1;
    }


//Client kann nun connecten mit folgenden Werten:
    printf("Port: %d\n",port);
    printf("Hostname: %s\n",hostname);
    char ip[17];

    if(inet_addr(hostname) != -1){
        printf("IP WURDE ANGEBEN!\n");
        strcpy(ip,hostname);
    }else if( hostnameToIp(hostname,ip) == 0){//IP Herausfinden
       printf("Fehler beim auflösen des Hostnames!\n");
       return -1;
   }
    printf("Ip: %s\n",ip);

//BEGINN VERBINDUNG
char recvbuffer[101];
int len, mysocket;
struct sockaddr_in dest;

mysocket = socket(AF_INET, SOCK_STREAM, 0);

memset(&dest, 0, sizeof(dest));
dest.sin_family = AF_INET;
dest.sin_addr.s_addr = inet_addr(ip);
dest.sin_port = htons(port);

connect(mysocket, (struct sockaddr*)&dest, sizeof(struct sockaddr_in));
printf("ich bin verbunden!\n");
//Ab hier ist der Client mit dem Server verbunden
int erg=0;
int e[7];

//1. Aufgaben bekommen, also erg und e befüllen!

waitRecv(mysocket,recvbuffer);//in diesen 2 zeilen wurde erg empfangen
erg = atoi(recvbuffer);


for(int i=0;i<7;i++){//hier werden alle werte von e empfangen
    waitRecv(mysocket,recvbuffer);
    e[i]= atoi(recvbuffer);
    printf("e[%d]= %d\n",i,e[i]);
}
if(pruefeAufgabe(erg,e) == 0){
    printf("Fehler bei den Aufgabenparameter, Programm muss beendet werden\n");
    return -1;
}

char name[100];
char *pname = &name[0];
erfrageNamen(pname);
//SENDE NAME
send(mysocket,pname,strlen(pname),0);

zeigeAufgaben(erg,e,pname,"","");



    char* input[100];
    char * lastcmd[100];
    char * lastanswer[100];

while(1==1 && abbruch == 0){//user eingabeschleife

    fgets(&input,100,stdin);

    if(strncmp(input,"QUIT",4)== 0){
        send(mysocket,"QUIT",strlen("QUIT"),0);
        break;

    }else if(strncmp(input,"TOP",3)== 0){
        strcpy(lastcmd,input);
        send(mysocket,input,strlen(input),0);//Sende befehl an server

        //TODO: EMPFANGE TOP 10
        //ZEIGE TOP 10 AN
        //GEHE AUF BEFHEL ZURÜCK ZUM NORMALEN SCREEN
        for(int i=0;i<10;i++){
            waitRecv(mysocket,recvbuffer);
            printf("%s\n",recvbuffer);
        }
        fgets(&input,40,stdin);
    }else{
        strcpy(lastcmd,input);
        send(mysocket,input,strlen(input),0);//Sende befehl an server
        waitRecv(mysocket,recvbuffer);//antwort vom server
        strcpy(lastanswer,recvbuffer);
    }


    zeigeAufgaben(erg,e,pname,&lastcmd[0],&lastanswer[0]);
}


    send(mysocket,"QUIT",4,0);
    close(mysocket);//schließe Verbindung
    printf("Client wird beendet...\n");
   return 2;










//ENDE VERBINDUNG



    return 0;
}