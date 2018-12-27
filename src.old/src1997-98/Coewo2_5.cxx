/* Program symulujacy KOEWOLUCJE */
/* Kazdy osobnik ma swoj bitowy wzorzec odzywiania i bitowy wzorzec */
/* strategi oslony. Jesli ATAKOWANY.OSLONA AND ATAKUJACY.GEBA>0 to  */
/* znaczy ze atak zakonczyl sie powodzeniem.			    */
/* Osobniki rozmnazaja sie kosztem wolnego miejsca i zgromadzonej energi */
/* Ruchy wlasne, ofiara ataku, jak i moment rozmnazania wybierane sa */
/* losowo, aby nie zaciemniac modelu dodatkowymi parametrami. 	     */

#include <limits.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <signal.h>
#define USES_RANDG
#include "wb_rand.h"
#include "wb_ptr.hpp"

#include "datasour.hpp"
#include "simpsour.hpp"
#include "filtsour.hpp"
#include "statsour.hpp"
#include "fifosour.hpp"
#include "sourmngr.hpp"
#include "logfile.hpp"

#include "areamngr.hpp"
#include "gadgets.hpp"
#include "textarea.hpp"
#include "graphs.hpp"

const unsigned SWIDTH=750;
const unsigned SHEIGHT=550;
const unsigned TAX_OUT=256;


unsigned IBOKSWIATA=100; // FAKTYCZNIE UZYWANY BOK SWIATA
unsigned textY=(IBOKSWIATA>TAX_OUT?IBOKSWIATA:TAX_OUT);
int      WSP_KATASTROF=10;			// Wykladnik rozkladu katastrof
const double WYPOSAZENIE_POTOMSTWA=0.1; // jaka czesc sily oddac potomkowi
double EFEKTYWNOSC_AUTOTROFA=0.99;	// jaka czesc swiatla uzywa autotrof
const unsigned MINIMALNY_WIEK=155;	// Rodzi sie z tym wiekiem. Do smierci ma 255-MINIMALNY_WIEK
const unsigned NIEPLODNOSC=10;		// Prawdopodobienstwo rozmnazania jest 1/NIEPLODNOSC
const unsigned PROMIENIOWANIE=160;	// Co ile kopiowanych bitow nastepuje mutacja
const unsigned BIT_RUCHU=128;		//Wyzerowanie ktorych bitow oslony odpowiada za zdolnosc ruchu					
unsigned long MAX_ITERATIONS=0xffffffff; // najwieksza liczba iteracji
unsigned LogRatio=1;				// Co ile krokow zapisywac do logu
char     LogName[128]="coewo2_5.log";

/* ROZMIAR TYPU BASE DECYDUJE O MOZLIWEJ KOMPLIKACJI SWIATA */
/* JEST TYLE MOZLIWYCH TAXONOW ILE WZORCOW BITOWYCH W base2 */
typedef unsigned char base;   // musi byc bez znaku
typedef unsigned short base2; // musi miescic 2 zmienne base
const base2 MAXBASE2=(base2)0xffffffffffffffffUL;
const base  MAXBASE =(base)MAXBASE2;
const base  AUTOTROF=MAXBASE;// wzor bitowy autotrofa - swiat go zywi


/* Czesc niezalezna od platformy */
/*********************************/
struct bity_wzoru
{
base geba;	// bitowy wzorzec trybu odzywiania
base oslona;	// bitowy wzozec sposobu ochrony
};

union wzor
{
base2 	_full; // pelny wzor bitowy "taxonu"
bity_wzoru w;  // i z podzilaem na pola
//void clear(){_full=0;}
};

class agent
{
public:
friend class swiat;
wzor			 w; // wzor bitowy taxonu
unsigned char wiek; // ile razy byl u sterowania, po przerolowaniu - smierc
unsigned int  sila; // zgromadzona energia

/* STATIC */
static unsigned max;//jaki jest najwiekszy taxon
static unsigned max_change;//i czy ostatnio max sie zmienil
static unsigned ile_ind;// ile jest zywych indywiduow
static unsigned ile_tax;// ile taxonow niezerowych
static unsigned ile_big_tax;// ile taxonow liczniejszych niz 10
static unsigned liczniki[/*(size_t)MAXBASE2+1*/];// Liczniki liczebnosci taxonow

public:
inline static   void tax(base2);  // rejestracja zmiany wartosci taxonu na ekranie
agent(){w._full=wiek=sila=0;}
void clear(){w._full=wiek=sila=0;}
int  jest_zywy(){ return (sila!=0 && wiek!=0); }
void init(base trof,base osl,unsigned isila); // inicjacja nowego agent
void init(agent& rodzic); // inicjacja nowego jako potomka starego
void kill();		       // smierc agent
void kill(agent& zabojca);// usmiercenie agent przez drugie
void uplyw_czasu();	       // oddzialywanie czasu
static base2 kopioj(base2 r);  // kopiuje genotyp z mozliwa mutacja
};

//TAK POWINNO BYC - ALE MSVC NIE KOMPILUJE
//base agent::* geba_ptr=&agent::w.w.geba;
//base agent::* oslona_ptr=&agent::w.w.oslona;

//TAK MSVC KOMPILUJE ALE WPISUJE 0
//base agent::* geba_ptr=&agent::w.wzor::w.bity_wzoru::geba;
//base agent::* oslona_ptr=&agent::w.wzor::w.bity_wzoru::oslona;

//RECZNE WPISYWANIE - ZALEZNE OD "ENDIAN"
#define RECZNIE_WPISZ_PTR
base agent::* geba_ptr=NULL;
base agent::* oslona_ptr=NULL;
//(base agent::*)sizeof(base);


class swiat//Caly swiat symulacji
//-------------------------------
{
size_t DLUG_WIERSZA;	
unsigned klon_auto;
unsigned tax_auto;
wb_dynarray<agent>	ziemia;
unsigned long		licznik;		// licznik krokow symulacji
logfile				log;			// plik z zapisem histori symulacji
sources_menager		Sources;		//Zarzadca seri przekaznikowych
matrix_source<unsigned>              *Liczniki;//Udostepnienie licznikow taxonow
struct_matrix_source<agent,unsigned char >	 *Wiek;
struct_matrix_source<agent,unsigned  int >	 *Sila;
struct_matrix_source<agent,base >			 *Geba;
struct_matrix_source<agent,base >			 *Oslona;
text_area*			OutArea;//Obszar do wypisywania statusu symulacji
public:
//KONSTRUKCJA DESTRUKCJA
swiat(size_t szerokosc,char* logname);
~swiat(){}
//AKCJE
void init();	  // stan startowy symulacji
void krok();	  // kolejny krok symulacji
void kataklizm(); // wygenerowanie katastrofy - reczne lub losowe
//Zapis calosci stara modle
void dump();	  // zrzucenie stanu do logu
//Wspolpraca z menagerem wyswietlania
//---------------------------------------------
void wskazniki(); // aktualizacja nowych wartosci wskaznikow
void tworz_lufciki(area_menager_base&);//Tworzy domyslne lufciki
//METODY POMOCNICZA
void Torus(int& x,int& y);
void ClearPosition(int x,int y);
void ClearLine(int xxp,int yyp,int n);
void Krater(int x,int y,int r); // Robi dziure z obszarze symulacji
agent& Ziemia(size_t Kolumna,size_t Wiersz)
{return ziemia[Wiersz*DLUG_WIERSZA+Kolumna];}
};

void swiat::tworz_lufciki(area_menager_base& Menager)
//Tworzy domyslne lufciki
{
graph* pom;
size_t wys_map=Menager.getheight()/4;
size_t szer_map=size_t(wys_map*2.3);
if(szer_map>Menager.getwidth()-280)
		szer_map=Menager.getwidth()-280;

#ifdef RECZNIE_WPISZ_PTR
{
int *hack1=(int*)&oslona_ptr;
*hack1=sizeof(base);
int *hack2=(int*)&geba_ptr;
*hack2=0;
}
#endif

{
//Zerowa seria w menagerze danych powinna byc pusta - sluzy
//do kontroli wersji danych. Menager moze tworzyc ja sam,
//ale zawsze mozna podmienic
ptr_to_scalar_source<unsigned long>* sca=new ptr_to_scalar_source<unsigned long>(&licznik,"Step#");
if(!sca) goto ERROR;
Sources.replace(0,sca);

//Tylko ze wzgledow demonstracyjnych
array_source<unsigned>* arr=new array_source<unsigned>(MAXBASE2+1,agent::liczniki,"liczniki taksonow - liniowo");
if(!arr) goto ERROR;
Sources.insert(arr);

//SILA
Sila=new struct_matrix_source<agent,unsigned>(DLUG_WIERSZA,DLUG_WIERSZA/2,
								ziemia.get_ptr_val(),
								&agent::sila,"sila");
if(!Sila) goto ERROR;
Sources.insert(Sila);

pom=new carpet_graph(0,0,szer_map,wys_map,//domyslne wspolrzedne
						Sila);//I zrodlo danych
/*
pom=new fast_carpet_graph<struct_matrix_source<unsigned> >
					(0,300,430,530,//domyslne wspolrzedne
						Sila);//I zrodlo danych
*/
pom->setdatacolors(0,255);//Pierwsze 25 kolorow bedzie slabo widoczne
pom->settitle("SILA AGENTOW");
Menager.insert(pom);


//GEBA
Geba=new struct_matrix_source<agent,base>(DLUG_WIERSZA,DLUG_WIERSZA/2,
								ziemia.get_ptr_val(),
								geba_ptr,
								"geba");
if(!Geba) goto ERROR;
Sources.insert(Geba);
Geba->setminmax(0,255);
pom=new carpet_graph(0,wys_map+1,szer_map,2*wys_map+1,//domyslne wspolrzedne
						Geba);//I zrodlo danych

if(!pom) goto ERROR;
pom->setdatacolors(0,255);//Pierwsze 25 kolorow bedzie slabo widoczne
pom->settitle("TROFIA AGENTOW");
Menager.insert(pom);

//OSLONA
Oslona=new struct_matrix_source<agent,base>(DLUG_WIERSZA,DLUG_WIERSZA/2,
								ziemia.get_ptr_val(),
								oslona_ptr,
								"wrazliwosc");
if(!Oslona) goto ERROR;
Sources.insert(Oslona);
Oslona->setminmax(0,255);
pom=new carpet_graph(0,(wys_map+1)*2,szer_map,(wys_map+1)*3,//domyslne wspolrzedne
						Oslona);//I zrodlo danych

if(!pom) goto ERROR;
pom->setdatacolors(0,255);//Pierwsze 25 kolorow bedzie slabo widoczne
pom->settitle("PODATNOSC AGENTOW NA ATAK");
Menager.insert(pom);

//Wiek
Wiek=new struct_matrix_source<agent,unsigned char>(DLUG_WIERSZA,DLUG_WIERSZA/2,
								ziemia.get_ptr_val(),
								&agent::wiek,
								"wiek");
if(!Wiek) goto ERROR;
Sources.insert(Wiek);
pom=new carpet_graph(0,(wys_map+1)*3,szer_map,(wys_map+1)*4,//domyslne wspolrzedne
						Wiek);//I zrodlo danych
pom->setdatacolors(0,255);//Pierwsze 25 kolorow bedzie slabo widoczne
pom->settitle("CZAS ZYCIA AGENTOW");
Menager.insert(pom);

//HISTORIA OSTATNICH 2000 krokow
template_scalar_source_base<unsigned>* staxa=new ptr_to_scalar_source<unsigned>(&tax_auto,"taxony autotroficzne");// ile jest autotroficznych taksonow
if(!staxa) goto ERROR;
int start_index=Sources.insert(staxa);

fifo_source<unsigned>* fpom=new fifo_source<unsigned>(staxa,2000);
if(!fpom) goto ERROR;
Sources.insert(fpom);

template_scalar_source_base<unsigned>* sklona=new ptr_to_scalar_source<unsigned>(&klon_auto,"klony autotroficzne");// ile taxonow liczniejszych niz 10
if(!sklona) goto ERROR;
Sources.insert(sklona);

fpom=new fifo_source<unsigned>(sklona,2000);
if(!fpom) goto ERROR;
Sources.insert(fpom);

template_scalar_source_base<unsigned>* stax=new ptr_to_scalar_source<unsigned>(&agent::ile_big_tax,"taksony");// ile taxonow liczniejszych niz 10
if(!stax) goto ERROR;
Sources.insert(stax);

fpom=new fifo_source<unsigned>(stax,2000);
if(!fpom) goto ERROR;
Sources.insert(fpom);

template_scalar_source_base<unsigned>* sklon=new ptr_to_scalar_source<unsigned>(&agent::ile_tax,"klony");// ile taxonow niezerowych
if(!sklon) goto ERROR;
Sources.insert(sklon);

fpom=new fifo_source<unsigned>(sklon,2000);
if(!fpom) goto ERROR;
Sources.insert(fpom);

template_scalar_source_base<unsigned>* sind=new ptr_to_scalar_source<unsigned>(&agent::ile_ind,"ilosc agentow");// ile taxonow niezerowych
if(!sind) goto ERROR;
Sources.insert(sind);

/*
LT_filter< struct_matrix_source<agent,unsigned> >	TESTOWY1(50,Sila);
generic_EQ_filter									TESTOWY2(50,Sila);
generic_LT_filter									TESTOWY3(50,Sila);
generic_LE_filter									TESTOWY4(50,Sila);
generic_MT_filter									TESTOWY5(50,Sila);
generic_ME_filter									TESTOWY6(50,Sila);
*/

graph* pom1=new sequence_graph(szer_map,401,Menager.getwidth()-1,Menager.getheight()-1,
							   5,Sources.make_series_info(
										start_index+1,start_index+3,
										start_index+5,start_index+7,
													-1
										).get_ptr_val(),
							   1/*Wspolne minimum/maximum*/);
if(!pom1) goto ERROR;
pom1->setframe(128);
pom1->settitle("HISTORIA LICZEBNOSCI");
Menager.insert(pom1);

generic_basic_statistics_source*	GebaStat=new generic_basic_statistics_source(Geba);
if(!GebaStat) goto ERROR;
Sources.insert(GebaStat);

generic_basic_statistics_source*	OslonaStat=new generic_basic_statistics_source(Oslona);
if(!OslonaStat) goto ERROR;
Sources.insert(OslonaStat);

generic_basic_statistics_source*	WiekStat=new generic_basic_statistics_source(Wiek);
if(!WiekStat) goto ERROR;
Sources.insert(WiekStat);

generic_basic_statistics_source*	SilaStat=new generic_basic_statistics_source(Sila);
if(!SilaStat) goto ERROR;
Sources.insert(SilaStat);

//OKNO HISTORI SILY
//dalej zakladamy ze SilaStat nie umieszcza sam podzrodel w menagerze danych
//ale latwo sprawic by to zalozenie nie bylo prawdziwe - wystarczy wywolac
//SilaStat->link_source_menager(Sources);
fifo_source<double>* dpom=new fifo_source<double>(SilaStat->Mean(),2000);//Fifo ze sredniej sily
if(!dpom) goto ERROR;
int MeanSilaFifoIndex=Sources.insert(dpom);
dpom=new fifo_source<double>(SilaStat->Max(),2000);//Fifo ze sredniej sily
if(!dpom) goto ERROR;
int MaxSilaFifoIndex=Sources.insert(dpom);
dpom=new fifo_source<double>(SilaStat->SD(),2000);//Fifo ze sredniej sily
if(!dpom) goto ERROR;
int SdSilaFifoIndex=Sources.insert(dpom);

pom1=new sequence_graph(szer_map+301,300,Menager.getwidth()-1,400,
							    3,Sources.make_series_info(
								MeanSilaFifoIndex,SdSilaFifoIndex,MaxSilaFifoIndex,
													-1
										).get_ptr_val(),
								0/* Z reskalowaniem */);
							   //1/*Wspolne minimum/maximum*/);
if(!pom1) goto ERROR;
pom1->setframe(128);
pom1->settitle("HISTORIA SILY");
Menager.insert(pom1);

//SWITCH HISTORY BUTTON
//{gadget* pom1=new button(szer_map+301,300,Menager.getwidth()-1,400,
//						"PELNA/OSTATNIA HISTORIA");
//if(!pom1) goto ERROR;
////settitlecolor(default_transparent);//Domyslnie jest niewidoczny		
////pom1->settitleback(64/*default_transparent*/);//Ale mozna uwodocznic
//Menager.insert(pom1);
//}

//INFORMACJE
OutArea=new text_area(szer_map,300,szer_map+300,400,"SYMULACJA STARTUJE",
					  default_white,default_black,128,40);
OutArea->settitle("STATYSTYKA:");
if(!OutArea) goto ERROR;
Menager.insert(OutArea);


//TAXONY - OSTANIE OKNO BO MOZE BYC NAJKOSZTOWNIEJSZE W RYSOWANIU

Liczniki=new matrix_source<unsigned>(256,256,
									 agent::liczniki,
									 "liczebnosc taksonow",//NAZWA SERI
									 1//Nie torus
									 );
if(!Liczniki) goto ERROR;
Sources.insert(Liczniki);
{
log_1_plus_F_filter<matrix_source<unsigned> >* fl=new log_1_plus_F_filter<matrix_source<unsigned> >(Liczniki);
if(!fl) goto ERROR;

pom=new fast_carpet_graph<
		log_1_plus_F_filter<matrix_source<unsigned> >
					>
					(szer_map+1,0,Menager.getwidth()-1,299,//domyslne wspolrzedne
						fl,1);//I zrodlo danych
if(!pom) goto ERROR;
}

pom->setbackground(0);
pom->setdatacolors(14,254);//Pierwsze 25 kolorow bedzie slabo widoczne
pom->settextcolors(255,255);
pom->settitle("MAPA ISTNIEJACYCH TAKSONOW");
Menager.insert(pom);

//USTALANIE KOLUMN DO WYDRUKU
log.insert(sca);//Licznik krokow
log.insert(sind);
log.insert(sklon);
log.insert(sklona);
log.insert(stax);
log.insert(staxa);
//log.insert(SilaStat->LenN());
//log.insert(SilaStat->RealN());
log.insert(SilaStat->Mean());
log.insert(SilaStat->SD());
log.insert(SilaStat->Max());
log.insert(WiekStat->Mean());
log.insert(WiekStat->SD());
log.insert(WiekStat->Max());
log.insert(OslonaStat->Mean());
log.insert(OslonaStat->SD());
log.insert(OslonaStat->Max());
log.insert(GebaStat->Mean());
log.insert(GebaStat->SD());
log.insert(GebaStat->Max());

return;
}//KONIEC WLASCIWEGO BLOKU
ERROR://BLOK OBSLUGI BLEDOW
perror("Nie mozna utworzyc obiektow wizualizujacych");
exit(1);																					
}

/* IMPLEMENTACJA CZESCI NIEZALEZNEJ OD PLATFORMY */
/*************************************************/

//DEFINICJA KONSTRUKTORA
swiat::swiat(size_t szerokosc,char* logname):	
	ziemia((szerokosc/2)*szerokosc), //Prostokat np 20x10
	DLUG_WIERSZA(szerokosc),
	Sources(64),
	OutArea(NULL),
	Liczniki(NULL),
	Wiek(NULL),Sila(NULL),
	Geba(NULL),Oslona(NULL),
	klon_auto(1),tax_auto(1),
	log(64,logname)
{
licznik=0;
}


//METODY POMOCNICZA
inline
void swiat::Torus(int& x,int& y)
{
if(x<0) x=(DLUG_WIERSZA)-x;
if(y<0) y=DLUG_WIERSZA/2-y;
x%=DLUG_WIERSZA;y%=DLUG_WIERSZA/2;
}

inline
void swiat::ClearPosition(int x,int y)
{
Torus(x,y);
if(!Ziemia(x,y).jest_zywy()) return;
Ziemia(x,y).kill();
}

void swiat::init()
{
assert(ziemia);
Ziemia(DLUG_WIERSZA/2,DLUG_WIERSZA/4).init(255,255,255);
log.GetStream()<<
//fprintf(log,"%ux%u\tWYP_POT=%f\tEFEKT=%f\tMAX_WIEK=%d\tPLOD=%f\tRTG=%f\tBUM=0.5^%d\n",
"ROZMIAR="<<DLUG_WIERSZA<<'x'<<DLUG_WIERSZA/2<<'\n'<<
"WYPOSAZENIE_POTOMSTWA="<<WYPOSAZENIE_POTOMSTWA<<'\n'<<
"EFEKTYWNOSC_AUTOTROFA="<<EFEKTYWNOSC_AUTOTROFA<<'\n'<<
"MAX_WIEK="<<(255-MINIMALNY_WIEK)<<'\n'<<
"PLODNOSC="<<1./NIEPLODNOSC<<'\n'<<
"RTG="<<1./PROMIENIOWANIE<<'\n'<<
"PRAWDOPODOBIENSTWO KATASTROF="<<WSP_KATASTROF<<'\n';
}


/* NAJWAZNIEJSZE FUNKCJE - GLOWNA IDEA SYMULACJI */

void agent::init(base trof,base osl, unsigned isila)
 // inicjacja nowego indywiduum
{
w.w.oslona=osl;
w.w.geba=trof;
sila=isila;
wiek=MINIMALNY_WIEK;
assert(w._full>0);
// Sprawdzenie czy oslona nie jest za dobra i czy inne parametry sa OK
if(w.w.oslona==0 || !jest_zywy())
      {
      clear();
      return;
      }

ile_ind++;
base2 iwfull=w.w.geba*256+w.w.oslona;
liczniki[iwfull]++;
assert(*(liczniki+iwfull));

if(liczniki[iwfull]>max)
		{
		max=liczniki[iwfull];
		max_change=1;
		}

if( liczniki[iwfull]==1 ) // pierwszy przedstawiciel taxonu
		ile_tax++;   // wiec liczba taxonow wzrasta
if( liczniki[iwfull]==11 ) // osiagnal wartosc >10 duzy taxon - rozwojowy
		ile_big_tax++;
}

void agent::kill()
// -----//---- przez swiat
{
assert(w.w.oslona>0);
assert( w._full>0 );
ile_ind--;
base2 wfull=w.w.geba*256+w.w.oslona;
liczniki[wfull]--;
if( liczniki[wfull]==0 )	//ostatni przedstawiciel tego taxonu
	ile_tax--;
if( liczniki[wfull]==10 ) // osiagnal wartosc <=10 maly taxon - nie rozwojowy
	ile_big_tax--;
assert(w._full>0 );
w._full=sila=wiek=0;
}

void agent::init(agent& rodzic)
 // inicjacja nowego jako potomka starego
{
w._full=kopioj(rodzic.w._full);
unsigned uposazenie=unsigned(rodzic.sila*WYPOSAZENIE_POTOMSTWA);
unsigned cena=w.w.geba + (base)(~w.w.oslona) + uposazenie; // Oslona 0 jest najdrozsza
if( rodzic.sila<=cena )  // Nie ma sily na potomka
	{ w._full=0; return; }
rodzic.sila-=cena; 	 // Placi za wyprodukowanie i wyposazenie
assert(rodzic.sila!=0);
init(w.w.geba,w.w.oslona,uposazenie);   // prawdziwa inicjacja
}

void agent::kill(agent& zabojca)
// usmiercenie indywiduum przez drugie
{
if(zabojca.sila==0) return; //niezdolny zabijac
assert(w.w.oslona>0);
assert(w._full>0);
/* Zabojca dostaje pewna czesc sily */
/* proporcjonalna do tego ile bitow oslony pasuje do jego geby */
if(w.w.oslona!=0)
   if(zabojca.w.w.geba!=0)
       zabojca.sila+=unsigned(sila *
    double(w.w.oslona & zabojca.w.w.geba)/(zabojca.w.w.geba)*
    double(w.w.oslona & zabojca.w.w.geba)/(w.w.oslona)
    );
assert(zabojca.sila!=0);
assert(w._full>0);
/* Potem ofiara ginie */
kill();
}

void agent::uplyw_czasu()
// prawo czasu - wszystko sie starzeje
{
assert(w.w.oslona>0);
assert(w._full>0);
wiek++;	       // Normalne starzenie sie
if(w.w.geba==AUTOTROF) //JEST  A U T O T R O F E M
	{
    sila+=unsigned(100*EFEKTYWNOSC_AUTOTROFA-1);// bez zutu gnu sie czepia a tak ma byc!
	assert(sila!=0);
	}
	else
	sila--;// Metaboliczne zurzycie energii
assert(w._full>0);
}

base2 agent::kopioj(base2 r)
// kopiuje genotyp z mozliwa mutacja
{
base2 mask=RANDOM(PROMIENIOWANIE);
if(mask<=16) // Prowizorka - nieprzenosne jesli base >16bitowe
	{
	mask=0x1<<mask;
	r^=mask;
	}
return r;
}

struct vector2
{ signed char x,y; };

void swiat::krok()
{
static vector2 kierunki[]={{1,1},{-1,-1},{1,-1},{-1,1},
			   {0,1},{ 1, 0},{0,-1},{-1,0} };

long ile= long(DLUG_WIERSZA)*long(DLUG_WIERSZA/2); // ile na krok MonteCarlo
licznik++;//Licznik krokow

// rob krok MonteCarlo
for(long i=0;i<ile;i++) 
	{
	int x=RANDOM(DLUG_WIERSZA);
	int y=RANDOM(DLUG_WIERSZA/2);
	Torus(x,y);
	if(!Ziemia(x,y).jest_zywy())  //jest martwy
		continue;		          //obrob nastepnego
	Ziemia(x,y).uplyw_czasu();    //Koszty czasu zycia
	if(!Ziemia(x,y).jest_zywy())  //nie moze dalej zyc
		{
		Ziemia(x,y).kill();		  //Usun
		continue;                 //i idz obrob nastepnego
		}

	//Losowanie sasiedniego miejsca
	unsigned a=RANDOM(8);	
	assert(a<8);
	int x1=(x+kierunki[a].x);
	int y1=(y+kierunki[a].y);
	Torus(x1,y1);

	
	if(!Ziemia(x1,y1).jest_zywy()) //Jesli wolne miejsce
	   {//------------------------------------------------
	   if(RANDOM(NIEPLODNOSC)==0)  // rozmnazanie
		{
		Ziemia(x1,y1).init(Ziemia(x,y));
		}
		else							//lub przemieszczenie jesli ma zdolnosc ruchu
		if(!(Ziemia(x,y).w.w.oslona&BIT_RUCHU))//...czyli BIT_RUCHU jest wyzerowany
			{
			Ziemia(x1,y1)=Ziemia(x,y);
			Ziemia(x,y).clear();
			}
	   }
	   else  // Jesli naprawde jest zywy 
		//-----------------------------------
	   if( Ziemia(x,y).w.w.geba!=AUTOTROF &&					  //atakujacy nie jest autotrofem
	      (Ziemia(x1,y1).w.w.oslona & Ziemia(x,y).w.w.geba) != 0 )//...i moze uszknac sasiada 
		   {
		   Ziemia(x1,y1).kill(Ziemia(x,y));	//Zabicie sasiada
		   }
	}


//Koniec kroku monte-carlo po agentach
//------------------------------------------
kataklizm(); // jeden, chocby bardzo maly na krok monte-carlo

{//Uaktualnienie klonow
klon_auto=0;
tax_auto=0;
for(unsigned i=0xffff;i>0xffff-0x100;i--)//Ile niezerowych klonow autotroficznych
	{
	if(agent::liczniki[i]>0)
		klon_auto++;
	if(agent::liczniki[i]>10)
		tax_auto++;
	}
}

if(Liczniki)
	{
	Liczniki->setminmax(0,agent::max);//Oszczedza liczenia max
	}

Sources.new_data_version(1,1);//Oznajmia seriom ze dane sie uaktualnily

}

double poison(int n); // generuje rozklad para-poison w zakresie 0..1 o n stopniach

void  swiat::kataklizm(void)
{
double power;
int x,y;
if(WSP_KATASTROF<0)
   return; //spontaniczne katastrofy wylaczone
x=RANDOM(DLUG_WIERSZA);
y=RANDOM(DLUG_WIERSZA/2);
power=poison(WSP_KATASTROF);
assert(power>=0 && power<=1);
Krater(x,y,int(power*DLUG_WIERSZA/2));
}



//IMPLEMENTGACJA WIZUALIZACJI Z UZYCIEM MENAGERA
//--------------------------------------------------

void  swiat::wskazniki(void)
{
//* LOSOWANIE SKAMIENIALOSCI 
unsigned x=RANDOM(DLUG_WIERSZA);
unsigned y=RANDOM(DLUG_WIERSZA/2);

if(licznik%LogRatio==0)
		log.try_writing();
//
wb_dynarray<char> bufor(1024);//ze sporym zapsem
sprintf(bufor.get_ptr_val(),"%lu KROK SYMULACJI\nILOSC AGENTOW:%lu\n"
							"ILOSC TAKSONOW:%lu "
							"(%lu AUTOTROFICZNYCH)\n"
							"ILOSC KLONOW:%lu "	
							"(%lu AUTOTROFICZNYCH)\n",
	//agent::nazwy[agent::plot_mode],
	(unsigned long)licznik,
	(unsigned long)agent::ile_ind,
	(unsigned long)agent::ile_big_tax,
	(unsigned long)tax_auto,
	(unsigned long)agent::ile_tax,
	(unsigned long)klon_auto);


if(OutArea)
	{
	OutArea->clean();//Stare linie usuwamy - czyste okno
	OutArea->add_text(bufor.get_ptr_val());
	}

}

/*  OGOLNA FUNKCJA MAIN */
/************************/
int parse_options(const int argc,const char* argv[]);
int main(const int argc,const char* argv[])
{
printf("CO-EWOLUCJA: program symulujacy kooewolucje wielu gatunkow\n");
printf("POLECENIA: 'g': GEBA 'o':OSLONA 's':SILA 'w':WIEK 'q':QUIT\n");
printf("LICZBA MOZLIWYCH KLONOW=%lu MAXINT=%d\n",(unsigned long)MAXBASE2,INT_MAX);

if(sizeof(base)*2!=sizeof(base2))
	{
	fprintf(stderr,"Niewlasciwe rozmiary dla typow bazowych:2*%u!=%u\n",
		sizeof(base),sizeof(base2));
	exit(1);
	}

if(!parse_options(argc,argv))
        exit(1);

swiat& tenSwiat=*new swiat(IBOKSWIATA,LogName);
if(&tenSwiat==NULL)
    {
    fprintf(stderr,"Brak pamieci!\n");
    exit(1);
    }


RANDOMIZE(); 
main_area_menager Lufciki(24,SWIDTH,SHEIGHT,28);
if(!Lufciki.start("CO-EVOLUTION version 2.5",argc,argv))
		{
		 printf("%s\n","Can't initialize graphics");
		 exit(1);
		}
Lufciki.process_input();//Pierwsze zdazenia

tenSwiat.init();
tenSwiat.tworz_lufciki(Lufciki);

//zamiast Lufciki.run_input_loop();
while(1)
	{
	tenSwiat.wskazniki();//Aktualizacja informacji
	Lufciki._replot();//Odrysowanie
	Lufciki.flush();
	Lufciki.process_input();//Obsluga zdarzen zewnetrznych
	if(!Lufciki.should_continue())
				break;//Koniec zabawy
	tenSwiat.krok();  //Kolejny krok symulacji
	}

delete &tenSwiat;//Dealokacja swiata wraz ze wszystkimi skladowymi
printf("Do widzenia!!!\n");
return 0;
}

double poison(int n)
// generuje rozklad para-poison w zakresie 0..1 o n stopniach
{
double pom=1;
for(int i=1;i<=n;i++)
	pom*= DRAND() ; // Mnozenie przez wartosc od 0..1
assert(pom>=0 && pom<=1);
return pom;
}


/* STATIC ALLOCATION */
unsigned agent::max=0;//jaki jest najwiekszy taxon
unsigned agent::max_change=0;//i czy ostatnio max sie zmienil
unsigned agent::ile_ind=0;// ile jest zywych indywiduow
unsigned agent::ile_tax=0;// ile taxonow niezerowych
unsigned agent::ile_big_tax=0;// ile taxonow liczniejszych niz 10
const /*unsigned long*/size_t TAXNUMBER=(unsigned long)MAXBASE2+1;
unsigned agent::liczniki[ /*TAXNUMBER*/MAXBASE2+1 ];// Liczniki liczebnosci taxonow
								//Musi miec mozliwosc posiadania 0xffff+1 elementow

void swiat::ClearLine(int xxp,int yyp,int n)
{
for(int i=xxp;i<xxp+n;i++)
	ClearPosition(i,yyp);
}

#include "krater2.imp"

int parse_options(const int argc,const char* argv[])
{
char* pom;
for(int i=1;i<argc;i++)
    {
    if( *argv[i]=='-' ) /* Opcja X lub symshella */
		continue;
    if((pom=strstr(argv[i],"BOK="))!=NULL) //Nie NULL czyli jest
	{
	IBOKSWIATA=atol(pom+4);
    if(IBOKSWIATA<3 || IBOKSWIATA>=SWIDTH)
		{
		fprintf(stderr,"Niewlasciwy BOK ==  %u\n",IBOKSWIATA);
		return 0;
		}
	}
    else
    if((pom=strstr(argv[i],"BUM="))!=NULL) //Nie NULL czyli jest
    {
    WSP_KATASTROF=atol(pom+4);
    if(WSP_KATASTROF<=0)
	{
	fprintf(stderr,"Ujemny wykladnik katastrof. Katastrofy wylaczone\n");
	}
    }
    else
    if((pom=strstr(argv[i],"AUTO="))!=NULL) //Nie NULL czyli jest
    {
    EFEKTYWNOSC_AUTOTROFA=atof(pom+5);
    if(EFEKTYWNOSC_AUTOTROFA<=0)
		{
        fprintf(stderr,"Efektywnosc autotrofa musi byc w zakresie 0..0.99\n");
		return 0;
        }
    printf("Ilosc iteracji ustawiona na %lu\n",MAX_ITERATIONS);
    }
    else
    if((pom=strstr(argv[i],"MAX="))!=NULL) //Nie NULL czyli jest
    {
    MAX_ITERATIONS=atol(pom+4);
    if(MAX_ITERATIONS<=0)
		{
	fprintf(stderr,"Ilosc iteracji nie moze byc <=0\n");
		return 0;
        }
    printf("Ilosc iteracji ustawiona na %lu\n",MAX_ITERATIONS);
    }
    else
    if((pom=strstr(argv[i],"LOG="))!=NULL) //Nie NULL czyli jest
    {
    LogRatio=atol(pom+4);
    if(LogRatio<=0)
		{
	fprintf(stderr,"Czestosc zapisu nie moze byc <=0\n");
		return 0;
        }
    }
    else
    if((pom=strstr(argv[i],"PLIK="))!=NULL) //Nie NULL czyli jest
    {
    strcpy(LogName,pom+5);
    }
	else /* Ostatecznie wychodzi ze nie ma takiej opcji */
	{
	fprintf(stderr,"Bledna opcja %s\n",argv[i]);
	fprintf(stderr,"MOZLIWE TO:\n");
    fprintf(stderr,"BOK=NN BUM=NN LOG=N \n");
	return 0;
	}
    }
return 1;
}

