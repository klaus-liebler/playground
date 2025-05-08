#include <stdio.h>  //macht Funktionen zur Ein- und Ausgabe verfügbar. Hier printf und fgets
#include <stdlib.h> /* atoi */
#include <string.h> //strcspn

bool eingabeKorrekt{false}; //Globale Variable, in der sich der Computer merken kann, ob die Eingabe korrekt war
char name[80];              //Globale Variable, in der sich das System den Namen merkt
char jahreAlsZeichen[10];
int jahre; //Globale Variable für das Alter in Jahren

int main()
{ //hier beginnt das Hauptprogramm
    printf("Geben Sie Ihren Namen ein: ");
    fgets(name, 79, stdin);

    while (eingabeKorrekt == false)
    { //Wiederholung, solange keine korrekte Eingabe
        //Man beachte, dass eingabeKorrekt explizit mit 0 vorinitialisiert wurde
        printf("Geben Sie Ihr Alter in Jahren ein: ");
        fgets(jahreAlsZeichen, 9, stdin); //Einlesen der Zeichen der Jahreszahl
        jahre = atoi(jahreAlsZeichen);
        if (jahre == 0)
        {
            printf("Die Eingabe ist nicht korrekt! ");
        }
        else{
            eingabeKorrekt = true;
        }
    }
    const float alterInTagen = jahre * 365.25f; //Deklaration einer lokalen Variablen, Ausdruck und Zuweisungsanweisung
    //Achten Sie auf den Punkt als Dezimaltrennzeichen!
    name[strcspn(name, "\n")] = 0;                                       //Schneidet den Zeilenumbruch nach dem Namen ab
    printf("Hallo %s, Sie sind ca. %f Tage alt.\n", name, alterInTagen); //Formatierte Ausgabe, mit Zeilenumbruch
    return 0;
} //das ist die schließende Klammer des Hauptprogramms