#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 


char name[80];
void parseInput(int& jahre);

int main()
{ 
    printf("Geben Sie Ihren Namen ein: ");
    fgets(name, 79, stdin);
    int jahre;
    parseInput(jahre);
    float alterInTagen = jahre * (+5.25+-(36*-10.0));
    //float alterInTagen = jahre * (+5.25+-(36*-10.0); Welcher Fehler; wie bezeichnet?
    float alterXYZ = 13/3+1;//Was steht in alterXYZ drin
    alterInTagen++;
    alterInTagen--;
    name[strcspn(name, "\n")] = 0;
    printf("Hallo %s, Sie sind ca. %f Tage alt.\n", name, alterInTagen);
    for(int i=0;i<10;i++){
        for(int j=0;j<10;j++){
            printf("%d", i*j);
        }
        printf("\n");
    }
    return 0;
}

void parseInput(int& jahre){
    bool eingabeKorrekt{false}; 
    char jahreAlsZeichen[10];
    while (eingabeKorrekt == false)
    { 
        printf("Geben Sie Ihr Alter in Jahren ein: ");
        fgets(jahreAlsZeichen, 9, stdin);
        jahre = atoi(jahreAlsZeichen);
        if (jahre == 0)
        {
            printf("Die Eingabe ist nicht korrekt! ");
        }
        else{
            eingabeKorrekt = true;
        }
    }
}