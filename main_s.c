#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
//
void efuellen(int e[],char* estring){
    int i=0;
while(estring != '\0' && i < 7){
    if(*estring != ' '){
        e[i] = *estring;
        printf("schreibe an i: %d wert: %d\n",i,*estring);
        i++;
    }
    estring++;

}
}

int main(int argc, char **argv) {// -e 1 2 3 4 5 6 7 -n 199(ergebnis) -t 23000 (port)
    int e[7];
    char* estring = NULL;
    int erg = NULL;
    int port = NULL;
    int opt = 0;
while((opt = getopt(argc,argv, "e:n:t:")) != -1){
    switch (opt){
        case 't':
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
    efuellen(e, estring);//TODO IST MÜLL MOMENTAN
for(int i=0;i<7;i++){
    printf("e: %d\n",e[i]);
}



if(erg != NULL && port != NULL && estring != NULL ){//Alle Argumente wurden angegeben
    printf("Kann jetzt starten\n");
}else{
    printf("Fehler in der Eingabe!");
    return -1;
}

    return 0;
}

