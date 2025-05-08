// Ein Kommentar, der mit zwei Schrägstrichen eingeleitet wird, geht bis zum Zeilenende
/* Ein Kommentar dieser Art kann
sich über mehrere Zeilen
erstrecken oder ... */
a = b /* ... vor dem Zeilenende enden. */ + c;

#include <iostream> /* Ein- und Ausgabebibliothek */
int main()
{ /* Hauptfunktion */
    /*
std::cout << "Hallo, du schöne Welt!" << std::endl; /* Ausgabe */
    * /
}

7 + 8 4 * PI //wobei PI irgendwo als Kontante definiert ist
              a *a +
    2 * a *b + b *b //wobei a und b zuvor als Variablen deklariert wurden
                   4 *
                   (a + b) / potenz(c, 7) //wobei die Funktion potenz existieren muss

                   3 *
                   ((3 + 4 +))2 * 3 +
    4

    x = 7 + 8;    //wobei die Variable x zuvor definiert sein muss
float y = 4 * PI; //hier wird y "an Ort und Stelle" definiert

Zahl = 22;
Zahl += 5; // Zahl = Zahl + 5;
Zahl -= 7; // Zahl = Zahl - 7;
Zahl *= 2; // Zahl = Zahl * 2;
Zahl /= 4; // Zahl = Zahl / 4;
Zahl++;    // Zahl = Zahl + 1;
Zahl--;    // Zahl = Zahl - 1;

gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
gpio_set_level(BLINK_GPIO, s_led_state);

5.0;                   // double
6.7f                   // float
    5.7e10 3.3e-3 'a'  // char
    "Ich bin ein Text" // char const*
    true               //bool
    false              //bool
    double d
{
    5.0
}
char c;
c = 'a';
//usw

static_cast<Zieltyp>(Variable) //bitte nur dieses verwenden
    (Zieltyp) Variable         //Kurzform des vorherigen
    const_cast<Zieltyp>(Variable) dynamic_cast<Zieltyp>(Variable) reinterpret_cast<Zieltyp>(Variable)

        int f(int x)
{
    int ergebnis = x * x + 4 * x + 1;
    return ergebnis
}
//...
int y = f(3)


#include <stdio.h>

int main()
{
    int a = 2;
    int b = 3;
    bool a_g_b = a > b;
    printf("Dass %d größer ist, als %d, ist %d\n", a, b, a_g_b);
    a = 4;
    a_g_b = a > b;
    printf("Dass %d größer ist, als %d, ist %d\n", a, b, a_g_b);
    
    bool kalt = true;
    bool regen = true;
    bool sonnenschein = false;
    printf("Dass es einen Regenbogen gibt, ist %d.\n", regen && sonnenschein);
    printf("Dass es warm ist, ist %d.\n", !kalt);
    printf("Dass die gute Outdoor-Jacke notwendig ist, ist %d.\n", kalt || regen);

    bool regenwetter{false};
    int minuten_bis_vorlesungsbeginn{40};
    float hungerfaktor{0.3};
    bool fahrradfahrt = !regenwetter && (minuten_bis_vorlesungsbeginn < 45 || hungerfaktor > 0.5);
    printf("Als Wahrheitswert für eine Fahrradfahrt meldet ihr Mobilitäts-KI-System: %d", fahrradfahrt);
}



#include <stdio.h>
    using namespace std;
int main()
{
    int a = 2;
    int b = 3;
    bool a_g_b = a > b;
    printf("Dass %d größer ist, als %d, ist %d\n", a, b, a_g_b);
    a = 4;
    a_g_b = a > b;
    printf("Dass %d größer ist, als %d, ist %d\n", a, b, a_g_b);
    bool kalt = true;
    bool regen = true;
    bool sonnenschein = false;
    printf("Dass es einen Regenbogen gibt, ist %d.\n", regen && sonnenschein);
    printf("Dass es warm ist, ist %d.\n", !kalt);
    printf("Dass die gute Outdoor-Jacke notwendig ist, ist %d.\n", kalt || regen);

    bool regenwetter{false};
    int minuten_bis_vorlesungsbeginn{40};
    float hungerfaktor{0.3};
    bool fahrradfahrt = !regenwetter && (minuten_bis_vorlesungsbeginn < 45 || hungerfaktor > 0.5);
    printf("Als Wahrheitswert für eine Fahrradfahrt meldet ihr Mobilitäts-KI-System: %d", fahrradfahrt);

    bool BoolescherAusdruck1 = false;
    bool BoolescherAusdruckN = false;
    if (BoolescherAusdruck1)
    {
        //Anweisungen
    }
    else if (BoolescherAusdruckN)
    {
        //Anweisungen
    }
    else
    {
        //Anweisungen
    }
    int jahre;
    char *jahreAlsZeichen;
    jahre = atoi(jahreAlsZeichen); //Annahme 589
    if (jahre < 16)
        printf("Geh wieder in die Schule!");
    else if (jahre > 67)
        printf("Geh wieder nach Hause, Rentner!");
    else if (jahre > 200)
        printf("Hey, du oller Vampir!");
    else
        printf("Hallo Nutzer");

    int min;
    min = a < b ? a : b;
    // Alternativ ginge:
    if (a < b)
        min = a;
    else
        min = b;
}

bool BoolescherAusdruck{false};
while (BoolescherAusdruck)
{
    //Anweisungsblock
}

int zahl = 1;
printf("Ich kann bis 20 zählen\n");
while (zahl <= 20)
{
    printf("%d\n", zahl);
    zahl++;
}
printf("Weiter kann ich noch nicht\n");

int zahl = 1;
printf("Ich kann bis 100 zählen\n");
while (zahl <= 100)
{
    printf("%d\n", zahl);
    zahl++;
    if (zahl > 50)
    {
        printf("Nö, heute will ich nur bis 50\n");
        break;
    }
}
printf("Weiter kann ich noch nicht\n");
int zahl = 1;
printf("Ich kann bis 50 zählen\n");
while (zahl <= 50)
{
    if (zahl % 10 == 0)
    {
        printf("Hoho, jede zehnte überspriiiinge ich!\n");
        zahl++;
        continue;
    }
    printf("%d\n", zahl);
    zahl++;
}
printf("Weiter kann ich noch nicht\n");

for (Initialisierung; Bedingung; Anweisung)
{
    //Anweisungsblock
}

for (int i{0}; i < 10; i++)
{
    printf("%d\n", i);
}

printf("Das Gauss-Problem\n");
int summe = 0;
for (int i{0}; i <= 100; i++)
{
    summe += i;
}
printf("Die Summer der Zahlen von 1 bis 100 ist %d\n", summe);

#include <stdio.h>
#include <iostream>
using namespace std;
int main()
{
    int auswahl;
    cout << "Wählen Sie Ihre Lieblingsleckerei:\n"
            "1 - Käsesahnetorte\n"
            "2 - Streuselkuchen\n"
            "3 - Windbeutel\n";
    cin >> auswahl;
    switch (auswahl)
    {
    case 1:
        cout << "Sie mögen also Käsesahnetorte!";
        break;
    case 2:
        cout << "Streuselkuchen ist ja auch lecker...";
        break;
    case 3:
        cout << "Windbeutel sind so flüchtig wie ihr Name, das können Sie sicher bestätigen?";
        break;
    default:
        cout << "Wollen Sie wirklich behaupten, dass Ihnen nichts davon zusagt?";
    }
    return 0;
}

int f(int); // Funktionsdeklaration
int main()
{
    int a = f(3); // Funktionsaufruf
    printf("%d", a);
}
int f(int x)
{ // Funktionsdefinition
    return x * x;
}

int g(int a, float b);
//..
int foo = g(4, 5.0);
//..
int g(int a, float b)
{
    return 42;
}
//CallByValue
void f(int x)
{
    x = 3 * x;
    printf("%d\n", x);
}
int main()
{
    int a = 7;
    f(a); // Ausgabe: 21
    //printf("%d\n",x); // Fehler! x ist hier nicht definiert
    printf("%d\n", a); // a hat immer noch den Wert 7
}

//CallByReference
void f(int &x)
{
    x = 3 * x;
    printf("%d\n", x);
}
int main()
{
    int a = 7;
    f(a); // Ausgabe: 21
    //printf("%d\n",x); // Fehler! x ist hier nicht definiert
    printf("%d\n", a); // a hat auch den Wert 21
}

//Default-Parameter
int summe(int a, int b, int c = 0, int d = 0); // Deklaration
int main()
{
    int x = summe(2, 3, 4, 5); //x == 14
    x = summe(2, 3, 4);        //x == 9, es wird d=0 gesetzt
    x = summe(2, 3);           //x == 5, es wird c=0, d=0 gesetzt
}
int summe(int a, int b, int c, int d)
{ // Definition
    return a + b + c + d;
}

int summe(int a, int b, int c, int d)
{
    return a + b + c + d;
}
int summe(int a, int b, int c)
{
    return a + b + c;
}
int summe(int a, int b)
{
    return a + b;
}
int main()
{
    // ...
}

int summe(int a, int b){return a + b;}
int produkt(int a, int b){return a * b;}
int diff(int a, int b){return a - b;}
int main()
{
    int x = summe(produkt(2, 3), diff(4, 5)); //x == 14
    printf("%d\n", x);
}

//Aufruf von Funktionen innerhalb von Funktionen
void wiegeabZutaten() { printf("wiegeabZutaten\n"); }
void ruehreTeig() { printf("rühreTeig\n"); }
void stelleEinBackofen() { printf("stelleEinBackofen\n"); }
void schiebeReinBrot() { printf("schiebeReinBrot\n"); }
void warte() { printf("warte\n"); }
void holeRausBrot() { printf("holeRausBrot\n"); }

void backeBrot()
{
    stelleEinBackofen();
    schiebeReinBrot();
    warte();
    holeRausBrot();
}
void brotzubereitung()
{
    wiegeabZutaten();
    ruehreTeig();
    backeBrot();
}
int main()
{
    brotzubereitung();
}


int globaleVariable{0};
int counter()
{
    static int localStaticCounter{0}; //lokal statisch wird beim ersten Aufruf initialisiert
    globaleVariable++;
    localStaticCounter++;
    return localStaticCounter;
}

int main()
{
    int locStatCnt = counter();
    printf("%d %d\n", locStatCnt, globaleVariable);
    locStatCnt = counter();
    printf("%d %d\n", locStatCnt, globaleVariable);
}

//Pointer-Basics
int main()
{
    int Zahl;        // eine int-Variable
    int *pZahl1;     // eine Zeigervariable, zeigt auf einen int
    int *pZahl2;     // ein weiterer "Zeiger auf int"
    Zahl = 10;       // Zuweisung eines Wertes an eine int-Variable
    pZahl1 = &Zahl;  // Adressoperator ’&’ liefert die Adresse einer Variable
    pZahl2 = pZahl1; // pZahl und pWert zeigen jetzt auf dieselbe Variable
    *pZahl1 += 5;    //Wir ändern die Variable über Dereferenzierung des einen Pointers
    *pZahl2 += 8;    //...und dann auch noch mal über den anderen Pointer
    printf("%d %d %d\n", Zahl, pZahl1, *pZahl1);
}

int Wert1;                        // eine int-Variable
int Wert2;                        // noch eine int-Variable
int const *p1Wert = &Wert1;       // Zeiger auf konstanten int
int *const p2Wert = &Wert1;       // konstanter Zeiger auf int
int const *const p3Wert = &Wert1; // konstanter Zeiger auf konstanten int

int main()
{
    int a = 1; // a wird 1
    printf("a=%d\n", a);
    a++; // a wird 2
    printf("a=%d\n", a);

    int *p = &a; // Adresse von a an p zuweisen
    printf("p=%d\n", p);
    printf("Größe von int =%d\n", sizeof(int));

    p++;
    printf("p=%d\n", p);
}

int main()
{
    int feldOhneExpliziteInitialisierung[10];     // Anlegen ohne Initialisierung
    int feld[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}; // Mit Initialisierung (automatisch 10 Elemente)
    feld[3] = 40;
    int summe = feld[4]+feld[5];
    printf("Summe = %d\n", summe);
    int pointersumme = *(feld+4)+*(feld+5);
    printf("Pointersumme = %d\n", pointersumme);
    for (int i = 0; i < 10; i++)
    {
        printf("Wert an der %d. Stelle = %d\n", i, feld[i]);
    }
}

//Verschiedene Speicherorte von Objekten
#include <stdio.h>
class Bruch{
public:
    Bruch (int z, int n):zaehler_ (z), nenner_ (n){
    }
    double CalcDoubleValue(){
        return static_cast<double>(zaehler_)/static_cast<double>(nenner_);
    }
private: 
    int zaehler_;
    int nenner_;
};
static Bruch staticBruch{6,10};
Bruch* bruchOnHeap; //=NULL

int main ()
{
    printf("%f\n", staticBruch.CalcDoubleValue());
    Bruch bruchOnStack{7, 10};
    printf("%f\n", bruchOnStack.CalcDoubleValue());
    bruchOnHeap=new Bruch{8,10};
    printf("%f\n", bruchOnHeap->CalcDoubleValue());
    return 0;
} 