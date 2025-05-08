#include <stdio.h>
#include <stdlib.h>
constexpr int N{500};
void ausgabe(int* pointerAufSieb, int siebLaenge);
int main()
{
     int sieb[N];
    
     for (int i=1;i<N;i++){
         sieb[i]=i; //Sieb füllen
     }
     int zahl=2;
     while(zahl<N/2)
     {
         for(int i=2*zahl;i<N;i+=zahl){
            sieb[i]=0; //Alle Vielfachen von Zahl ausstreichen (aber noch nicht die Zahl selbst)
         }
         zahl++; //gehe jetzt mindestens zur nächsten Zahl
         while(sieb[zahl]==0){//wenn diese nächste Zahl bereits gestrichen ist...
            zahl++;//...dann müssen auch ihre Vielfachen nicht gestrichen werden und man kann sich weitere Zahlen anschauen
         }   
     }
     ausgabe(sieb, N);
     return 0;
}

void ausgabe(int* pointerAufSieb, int siebLaenge){
    for(int i=0;i<siebLaenge;i++){
         if(pointerAufSieb[i]!=0){
             printf("%d\n",i);
         }
     }   
}