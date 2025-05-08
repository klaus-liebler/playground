#include <stdio.h>
#include <stdlib.h>

char buf[80];
int zahl{0};
int min{0};
int zaehler{0};

int main()
{
    printf("Wo fängt die Zahlenreihe an?: ");
    fgets(buf, 79, stdin);
    min = atoi(buf);
    printf ("Wo hört die Zahlenreihe auf ? : ");
    fgets(buf, 79, stdin);
    int max=atoi(buf);
    
    while(true){
        zahl = min + (max - min) / 2;
        printf(" Ist Ihre Zahl (k)leiner, (g)rößer oder = %d ? (x zum Abbruch) ", zahl);
        fgets(buf, 79, stdin);
        char eingabe = buf[0];
        if(eingabe == 'k'){
            max=zahl;
            zaehler++;
        }
        else if(eingabe=='g'){
            min=zahl;
            zaehler++;
        }
        else if(eingabe=='='){
            zaehler++;
            printf("Ich bin einfach der beste Computer der ganzen Welt, oder?!?!?\n");
            break;
        }
        else if(eingabe=='x'){
            printf("Tschüss\n");
            return 0;
        }
        else{
            printf("Sie haben ein unzulässiges Zeichen eingegeben!\n");
            continue;
        }
        if(min+1>=max){
            printf("Da flunkert aber jemand...");
            break;
        }
    }
    printf("Die von Ihnen ausdeachte Zahl ist %d und ich habe dafür nur %d Versuche benötigt!", zahl, zaehler);
}
