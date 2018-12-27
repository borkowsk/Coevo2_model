/* Program symulujacy KOEWOLUCJE */
/* Kazdy osobnik ma swoj bitowy wzorzec odzywiania i bitowy wzorzec */
/* strategi oslony. Jesli ATAKOWANY.OSLONA AND ATAKUJACY.Trofia>0 to  */
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
#include "randoms.h" //WB random generators for "C"

#define HIDE_WB_PTR_IO	0 //MUsi byc IO dla wb_dynarray
#include "wb_ptr.hpp"
//#include "wb_ptrio.h"
#include "bits.h"
#include "datasour.hpp"
#include "simpsour.hpp"
#include "filtsour.hpp"
#include "bifiltso.hpp"
#include "statsour.hpp"
#include "dhistosou.hpp"
#include "fifosour.hpp"
#include "sourmngr.hpp"
#include "logfile.hpp"
#include "layer.hpp"
#include "areamngr.hpp"
#include "mainmngr.hpp"
#include "gadgets.hpp"
#include "textarea.hpp"
#include "graphs.hpp"

#include "klonalinfo.hpp"
#include "intersourc.hpp"//Zrodla specjalne "and" i ...
and_interaction_source	AndDemo("Trofia AND Oslona");
and_exploatation_source ExpDemo("(G AND O)/O * (G AND O)/G");

const char*	   ProgramName="CO-EVOLUTION wersja 2.82b (XII2003) idea i realizacja: Wojciech Borkowski";
const char*    SCREENDUMPNAME="CO-EVOLUTION-2.82b";
const unsigned TAX_OUT=256;
const unsigned dlugosc_logow=25000;

unsigned SWIDTH=750;
unsigned SHEIGHT=550;
unsigned LogRatio=1;				// Co ile krokow zapisywac do logu

char     LogName[128]="coewo2003.out";
char     MappName[128]=""/*coewo2001ini.gif"*/;

unsigned IBOKSWIATA=100; // FAKTYCZNIE UZYWANY BOK SWIATA
unsigned textY=(IBOKSWIATA>TAX_OUT?IBOKSWIATA:TAX_OUT);
int      WSP_KATASTROF=0;//10-100		// Wykladnik rozkladu katastrof - 0 -wylaczone
const double	WYPOSAZENIE_POTOMSTWA=0.1; // jaka czesc sily oddac potomkowi
double EFEKTYWNOSC_AUTOTROFA=0.99;	// jaka czesc swiatla uzywa autotrof
const unsigned	MINIMALNY_WIEK=205;	// Rodzi sie z tym wiekiem. Do smierci ma 255-MINIMALNY_WIEK
const unsigned	NIEPLODNOSC=5;		// Prawdopodobienstwo rozmnazania jest 1/NIEPLODNOSC
const unsigned	PROMIENIOWANIE=160;	// Co ile kopiowanych bitow nastepuje mutacja
//const unsigned BIT_RUCHU=128;		//Wyzerowanie ktorych bitow oslony odpowiada za zdolnosc ruchu					
const unsigned	BIT_RUCHU=1024;		//Poza maske - bez mozliwosci utraty ruchliwosci
unsigned int	PIRACTWO=1;				//Czy eksploatacja piracka czy pasozytnicza
unsigned int	ODWROCONY_KOSZT_OSLONY=0;//1 - czy koszty oslony sa odwrocone
unsigned int	ODWROCONY_KOSZT_ATAKU=0;//1 - czy koszty oslony sa odwrocone
unsigned long	MAX_ITERATIONS=0xffffffff; // najwieksza liczba iteracji
double          MonteCarloMult=10; //Ewentualna modyfikacja dlugosci kroku MonteCarlo


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
    bity_wzoru w;  // i z podzialem na pola
    //void clear(){_full=0;}
};

class agent//:public agent_base
{
public:
    
    wzor			 w; // wzor bitowy taxonu
    unsigned char wiek; // ile razy byl u sterowania, po przerolowaniu - smierc
    unsigned	  sila; // zgromadzona energia
    informacja_klonalna* klon;//Przynaleznosc klonalna i zwiazane z tym informacje
  
    /* Informacje statystyczne dotyczace indywiduow i klon�w */
    static unsigned max;//jaki jest najwiekszy taxon
    static unsigned max_change;//i czy ostatnio max sie zmienil
    static unsigned ile_ind;// ile jest zywych indywiduow
    static unsigned ile_tax;// ile taxonow niezerowych
    static unsigned ile_big_tax;// ile taxonow liczniejszych niz 10
    static unsigned liczniki[/*(size_t)MAXBASE2+1*/];// Liczniki liczebnosci aktualnie istniejacych 'taxonow ekologicznych" wg bit�w wzoru
     
    //Wejscie/wyjscie
    friend
        ostream& operator << (ostream& o,const agent& a);
    
    friend
        istream& operator >> (istream& i,agent& a);
    
    // rejestracja zmiany wartosci taxonu 
    inline static   void tax(base2);  
    
    //Nie wirtualna metoda czyszczaca
    void _clean()
    {
        if(klon!=NULL)
        {
            if(klon->odlicz_indywiduum())//Jesli nieistotny klon to trzeba usunac
                delete klon;
            klon=NULL;
        }
        w.w.geba=0; //Lepiej by bylo gdyby mozna ustawic -1
        w.w.oslona=0;
        wiek=sila=0;
    }
    
    // TO CO MUSI byc zdefiniowane
    ///////////////////////////////////
    agent(const agent& ini):klon(NULL)
    {
        assert(&ini!=NULL);
        
        w._full=ini.w._full;wiek=ini.wiek;sila=ini.sila;
        assert(ini.klon!=NULL);
        klon->dolicz_indywiduum();
    }
    
    agent():klon(NULL)
    {
        _clean();
    }
    
    ~agent()
    {
        _clean();
    }
    
    void clean()
    {
        if(jest_zywy())
            kill();//Uzywa kill zeby wewnetrzne struktury statystyczne sie zgadzaly
    }
    
    void assign_rgb(unsigned char Red,unsigned char Green,unsigned char Blue)
    {assert("agent::assign_rgb(...) not implemented"==NULL);}
    
    //Sprawdzanie czy jest zywy
    bool  jest_zywy(){ return (sila!=0 && wiek!=0); }
    //double zywy(){return jest_zywy();}
    
    bool  is_alive() { return jest_zywy();} //Bo wymaga tego klasa layer i pochodne
    
    int   how_specialised() //Poziom specjalizacji mierzony w bitach
    {
        if(!jest_zywy()) return -1;
        else return bits(w.w.geba)+bits(w.w.oslona);
    }
    
    int   how_specialised_in_defense() //Poziom specjalizacji mierzony w bitach
    {
        if(!jest_zywy()) return -1;
        else return bits(w.w.oslona);
    }

    int   how_specialised_in_feeding() //Poziom specjalizacji mierzony w bitach
    {
        if(!jest_zywy()) return -1;
        else return bits(w.w.geba);
    }

    //Podstawowa inicjacja i kasacja operujaca na licznikach taksonow
    int init(base trof,base osl,unsigned isila); // inicjacja nowego agent
    int kill();		     // smierc agenta - marker czasu dla statystyk
    static base2 duplikuj(base2 r);  // kopiuje genotyp z mozliwa mutacja
    
    //Inicjacja i kasacja w interakcjach
    int init(agent& rodzic); // inicjacja nowego jako potomka starego. Zwraca 0 jesli wadliwy genom
    int kill(agent& zabojca,unsigned& energy_flow);// usmiercenie agenta przez drugiego
    int parasit(agent& zabojca,unsigned& energy_flow);//zubazanie agenta przez drugiego. Zwraca 1 jesli zginal
    
    // oddzialywanie czasu
    int uplyw_czasu();//Zwraca 1 jesli zginal. Marker czasu dla statystyk
    
    //Dla wygody implementacji:
    friend class swiat;  
};

//TAK POWINNO BYC - ALE MSVC NIE KOMPILUJE
//base agent::* geba_ptr=&agent::w.w.geba;
//base agent::* oslona_ptr=&agent::w.w.oslona;

//TAK MSVC KOMPILUJE ALE WPISUJE 0
//base agent::* geba_ptr=&agent::w.wzor::w.bity_wzoru::geba;
//base agent::* oslona_ptr=&agent::w.wzor::w.bity_wzoru::oslona;

//RECZNE WPISYWANIE - ZALEZNE OD "ENDIAN"
#define RECZNIE_WPISZ_PTR 

#ifdef  RECZNIE_WPISZ_PTR 
base agent::* geba_ptr=NULL;
base agent::* oslona_ptr=NULL;
#else
//base agent::* geba_ptr=NULL;
//base agent::* oslona_ptr=NULL;
//(base agent::*)sizeof(base);
#endif


struct inte_stat//Struktura do przechowywania lokalnej informacji statystycznej
//-----------------------------------------------------------------------------
{
    unsigned N;//Ilosc interakcji
    double  sE;//Suma energii przejetej w interakcjach
    double  sP;//Suma udzialow (efektywnosci) interacji

    inte_stat():N(0),sE(0.0),sP(0.0){}
    
    double  MeanEnergyOfInteraction()
    {
        if(N>0)
            return sE/N;
        else
            return -1;
    }
    
    double  MeanEffectOfInteraction()
    {
        if(N>0)
            return sP/N;
        else
            return -1;
    }
};


class swiat//Caly swiat symulacji
//-------------------------------
{
//Parametry jednowartosciowe
/////////////////////////////////
unsigned long		licznik;		// licznik krokow symulacji
size_t				DLUG_WIERSZA;	// Obwod torusa
unsigned klon_auto;					// Liczba klonow autotroficznych
unsigned tax_auto;					// Liczba taksonow autotroficznych
unsigned autotrofy;					// Liczba agentow autrotroficznych

//Warstwy symulacji (sa torusami - bo taka jest wspolna geometria)
////////////////////////////////////////////////////////////////////////
rectangle_unilayer<unsigned char> zdatnosc; //Warstwa definiujaca zdatnosc do zasiedlenia
rectangle_layer_of_agents<agent>    ziemia; //Wla�ciwa warstwa agentow zasiedlajacych
rectangle_layer_of_struct<inte_stat> stats; //Warstwa pamietajaca lokalne statystyki zdazen

//Zarzadzanie zrodlami danych
///////////////////////////////////
sources_menager							Sources;	   //Zarzadca seri przekaznikowych
matrix_source<unsigned>						 *Liczniki;//Udostepnienie licznikow taxonow
struct_matrix_source<agent,unsigned char >	 *Wiek;
struct_matrix_source<agent,unsigned>		 *Sila;
struct_matrix_source<agent,base >			 *Trofia;
struct_matrix_source<agent,base >			 *Oslona;
method_matrix_source<inte_stat,double>       *EDynamism;//Lokalna dynamika przeplywu energii 
method_matrix_source<inte_stat,double>       *PDynamism;//Lokalne efektywnosc eksploatacji
logfile									log;			// plik z zapisem histori symulacji

//Obszar bezposredniego wyswietlania
////////////////////////////////////
text_area*			OutArea;//Obszar do wypisywania statusu symulacji

public:
//KONSTRUKCJA DESTRUKCJA
swiat(size_t szerokosc,
	  char* logname,   //Nazwa pliku do zapisywania histori
	  char* mappname); //Nazwa (bit)mapy inicjujacej "zdatnosc"
~swiat(){}

//AKCJE
void init();	  // stan startowy symulacji
void krok();	  // kolejny krok symulacji
void kataklizm(); // wygenerowanie katastrofy - reczne lub losowe

//Wspolpraca z menagerem wyswietlania
//---------------------------------------------
void wskazniki(); // aktualizacja nowych wartosci wskaznikow
void tworz_lufciki(area_menager_base&);//Tworzy domyslne lufciki

//METODY POMOCNICZA
agent& Ziemia(size_t Kolumna,size_t Wiersz)
	{return ziemia.get(Kolumna,Wiersz);}

unsigned long		get_licznik(){return licznik;}		// licznik krokow symulacji
};

void swiat::tworz_lufciki(area_menager_base& Menager)
//Tworzy domyslne lufciki
{
graph* pom;
size_t wys_map=Menager.getheight()/4;
size_t szer_map=size_t(wys_map*2.3);
if((long)szer_map>Menager.getwidth()-280)
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

//ZRODLA SUROWYCH WLASCIWOSCI AGENT�W
Sila=ziemia.make_source("energia",&agent::sila);
if(!Sila) goto ERROR;
Sources.insert(Sila);

Trofia=ziemia.make_source("trofia",geba_ptr);
if(!Trofia) goto ERROR;
Sources.insert(Trofia);
Trofia->setminmax(0,255);

Oslona=ziemia.make_source("wrazliwosc",oslona_ptr);
if(!Oslona) goto ERROR;
Sources.insert(Oslona);
Oslona->setminmax(0,255);

Wiek=ziemia.make_source("wiek",&agent::wiek);
if(!Wiek) goto ERROR;
Wiek->setminmax(0,255);
Sources.insert(Wiek);

EDynamism=stats.make_source("Eksploatacja",&inte_stat::MeanEnergyOfInteraction);
if(!EDynamism) goto ERROR;
Sources.insert(EDynamism);
EDynamism->set_missing(-1);

//ZRODLO METODOWE FILTRUJACE ZYWE
method_matrix_source<agent,bool>* Zycie=ziemia.make_source("martwe/zywe",&agent::jest_zywy);
if(!Zycie) goto ERROR;
Zycie->setminmax(0,1);
int inZycie=Sources.insert(Zycie);

GT_filter<method_matrix_source<agent,bool> >* filtrZycia=//Filtr na "Tylko Zywe"
	new GT_filter<method_matrix_source<agent,bool> >(0.5,Zycie,default_missing<double>(),"Zywe");//
if(!filtrZycia)goto ERROR;
int inFiltrZycia=Sources.insert(filtrZycia);

//SAME ZYWE
if_then_source* GebaZywych=new if_then_source(filtrZycia,Trofia,"TrofiaZ");
if(!GebaZywych) goto ERROR;
Sources.insert(GebaZywych);

if_then_source* OslonaZywych=new if_then_source(filtrZycia,Oslona,"OslonaZ");
if(!OslonaZywych) goto ERROR;
Sources.insert(OslonaZywych);

if_then_source* WiekZywych=new if_then_source(filtrZycia,Wiek,"WiekZ");
if(!WiekZywych) goto ERROR;
Sources.insert(WiekZywych);

if_then_source* SilaZywych=new if_then_source(filtrZycia,Sila,"EnergiaZ");
if(!SilaZywych) goto ERROR;
Sources.insert(SilaZywych);

//Trofia
pom=new carpet_graph(0,wys_map,szer_map-1,2*wys_map-1,//domyslne wspolrzedne
						Trofia);//I zrodlo danych
//						GebaZywych);//I alternatywne zrodlo danych
if(!pom) goto ERROR;
pom->setdatacolors(0,255);//Pierwsze 25 kolorow bedzie slabo widoczne
pom->settitle("TROFIA AGENT�W");
Menager.insert(pom);

//OSLONA
pom=new carpet_graph(0,2*wys_map,szer_map-1,3*wys_map-1,//domyslne wspolrzedne
						Oslona);//I zrodlo danych
//						OslonaZywych);//I alternatywne zrodlo danych

if(!pom) goto ERROR;
pom->setdatacolors(0,255);//Pierwsze 25 kolorow bedzie slabo widoczne
pom->settitle("PODATNO�� AGENT�W NA ATAK");
Menager.insert(pom);

//Energia
pom=new carpet_graph(0,0,szer_map-1,wys_map-1,//domyslne wspolrzedne
//						Sila);//I zrodlo danych
                        SilaZywych);//I alternatywne zrodlo danych
pom->setdatacolors(1,254);//Zeby nie uzywac bialego, ktory jest tlem i calkiem czarnych
pom->settitle("ZASOBY ENERGII AGENT�W");
Menager.insert(pom);

//WIDOK SPECJALIZACJI
method_matrix_source<agent,int>* Specjalizacja=ziemia.make_source("bity ustawione",&agent::how_specialised);
if(!Specjalizacja) goto ERROR;
Specjalizacja->setminmax(1,16);
Specjalizacja->set_missing(-1);
int inSpecjalizacja=Sources.insert(Specjalizacja);
pom=new carpet_graph(0,3*wys_map,szer_map-1,4*wys_map,//domyslne wspolrzedne
						Specjalizacja);//I zrodlo danych
pom->setdatacolors(1,254);//Zeby nie uzywac bialego, ktory jest tlem i calkiem czarnych
pom->settitle("POZIOM SPECJALIZACJI AGENT�W");
Menager.insert(pom);

//HISTORIA OSTATNICH <dlugosc_logow> KROKOW
template_scalar_source_base<unsigned long>* allclons=new ptr_to_fuction_source<unsigned long>(informacja_klonalna::ile_klonow,"klon�w od poczatku");
if(!allclons) goto ERROR;
Sources.insert(allclons);
fifo_source<unsigned long>* flpom=new fifo_source<unsigned long>(allclons,dlugosc_logow);
if(!flpom)goto ERROR;
int inAllclons=Sources.insert(flpom);

template_scalar_source_base<unsigned long>* alltax=new ptr_to_fuction_source<unsigned long>(informacja_klonalna::ile_taksonow,"takson�w od poczatku");
if(!alltax) goto ERROR;
Sources.insert(alltax);
flpom=new fifo_source<unsigned long>(alltax,dlugosc_logow);
if(!flpom)goto ERROR;
int inAlltax=Sources.insert(flpom);

template_scalar_source_base<unsigned>* staxa=new ptr_to_scalar_source<unsigned>(&tax_auto,"taksony autotroficzne");// ile jest autotroficznych taksonow
if(!staxa) goto ERROR;
Sources.insert(staxa);

fifo_source<unsigned>* fpom=new fifo_source<unsigned>(staxa,dlugosc_logow);
if(!fpom) goto ERROR;
int fiftaxa=Sources.insert(fpom);

template_scalar_source_base<unsigned>* sklona=new ptr_to_scalar_source<unsigned>(&klon_auto,"klony autotroficzne");// ile taxonow liczniejszych niz 10
if(!sklona) goto ERROR;
Sources.insert(sklona);

fpom=new fifo_source<unsigned>(sklona,dlugosc_logow);
if(!fpom) goto ERROR;
int fifklona=Sources.insert(fpom);

template_scalar_source_base<unsigned>* stax=new ptr_to_scalar_source<unsigned>(&agent::ile_big_tax,"taksony");// ile taxonow liczniejszych niz 10
if(!stax) goto ERROR;
Sources.insert(stax);

fpom=new fifo_source<unsigned>(stax,dlugosc_logow);
if(!fpom) goto ERROR;
int fiftax=Sources.insert(fpom); 

template_scalar_source_base<unsigned>* sklon=new ptr_to_scalar_source<unsigned>(&agent::ile_tax,"klony");// ile taxonow niezerowych
if(!sklon) goto ERROR;
Sources.insert(sklon);

fpom=new fifo_source<unsigned>(sklon,dlugosc_logow);
if(!fpom) goto ERROR;
int fifklon=Sources.insert(fpom);


//HISTORIA LICZEBNOSCI AGENT�W
template_scalar_source_base<unsigned>* sind=new ptr_to_scalar_source<unsigned>(&agent::ile_ind,"liczba agentow");// ile agentow niezerowych
if(!sind) goto ERROR;
Sources.insert(sind);

fpom=new fifo_source<unsigned>(sind,dlugosc_logow);
if(!fpom) goto ERROR;
int fifagents=Sources.insert(fpom);

template_scalar_source_base<unsigned>* sinda=new ptr_to_scalar_source<unsigned>(&autotrofy,"liczba autotrofow");// ile agentow autotroficznych
if(!sinda) goto ERROR;
Sources.insert(sinda);

fpom=new fifo_source<unsigned>(sinda,dlugosc_logow);
if(!fpom) goto ERROR;
int fifautoagents=Sources.insert(fpom);


//WYSWIETLACZ HISTORI LICZEBNOSCI
graph* pom1=new sequence_graph(Menager.getwidth()-80,199,Menager.getwidth()-1,299,//domyslne wspolrzedne						
							   3,Sources.make_series_info(
										fifagents,fifautoagents,
										-1
										).get_ptr_val(),
							   1/*Wspolne minimum/maximum*/);
if(!pom1) goto ERROR;
pom1->setframe(128);
pom1->settitle("AGENCI - HISTORIA LICZEBNO�CI");
Menager.insert(pom1);

//Historia sily
//szer_map+301,300,Menager.getwidth()-1,400,

//Historia liczby taksonow
pom1=new sequence_graph(szer_map,401,Menager.getwidth()-1,Menager.getheight()-1,
							   5,Sources.make_series_info(
										fiftaxa,fifklona,
										fiftax,fifklon,
													-1
										).get_ptr_val(),
							   1/*Wspolne minimum/maximum*/);
if(!pom1) goto ERROR;
pom1->setframe(128);
pom1->settitle("HISTORIA LICZBY TAKSON�W");
Menager.insert(pom1);


//WIZUALIZACJA FILTRU ZYCIA
pom=new carpet_graph(Menager.getwidth()-80,0,Menager.getwidth()-1,20,//domyslne wspolrzedne
						filtrZycia);//I zrodlo danych
if(!pom) goto ERROR;
pom->setdatacolors(50,254);//Potrzebne tylko dwa kolory naprawde
pom->setframe(32);
pom->settitle("ZYWE");
Menager.insert(pom);

//UDOSTEPNIANIE I WYSWIETLANIE BACKGROUNDU SYMULACJI
rectangle_source_base* Background=zdatnosc.make_source("Mapa zdatnych obszar�w");
if(!Background) goto ERROR;
Sources.insert(Background);
Background->setminmax(0,255);//255 odcieni (szarosci) - czarne niezdatne
pom=new carpet_graph(Menager.getwidth()-80,21,Menager.getwidth()-1,42,//domyslne wspolrzedne						
					 Background);//I zrodlo danych

if(!pom) goto ERROR;
pom->setdatacolors(0,255);//Pierwsze 25 kolorow bedzie slabo widoczne
pom->setframe(64);
pom->settitle("SIEDLISKO");
Menager.insert(pom);

//Demo interakcji AND
pom=new carpet_graph(Menager.getwidth()-80,43,Menager.getwidth()-1,64,//domyslne wspolrzedne
						&AndDemo);//I zrodlo danych
if(!pom) goto ERROR;
//pom->setdatacolors(0,255);//Potrzebne tylko dwa kolory naprawde
pom->setframe(64);
pom->settitle("AND(Trofia,Oslona)");
Menager.insert(pom);

//Demo exploatacji potenclalnej (AND*K)
pom=new carpet_graph(Menager.getwidth()-80,65,Menager.getwidth()-1,86,//domyslne wspolrzedne
						&ExpDemo);//I zrodlo danych
if(!pom) goto ERROR;
//pom->setdatacolors(0,255);//Potrzebne tylko dwa kolory naprawde
pom->setframe(64);
pom->settitle("Eksploatacja potencjalna");
Menager.insert(pom);

//Wiek
pom=new carpet_graph(Menager.getwidth()-80,87,Menager.getwidth()-1,87+21,//domyslne wspolrzedne
						Wiek);//I zrodlo danych
pom->setdatacolors(0,255);//Pierwsze 25 kolorow bedzie slabo widoczne
pom->setframe(32);
pom->settitle("WIEK AGENT�W");
Menager.insert(pom);


//HISTOGRAM SPECJALIZACJI
generic_discrete_histogram_source*  histspecj=new generic_discrete_histogram_source(1,16,Specjalizacja,&Sources);
if(!histspecj) goto ERROR;
Sources.insert(histspecj);
pom=new bars_graph(Menager.getwidth()-80,87+21+1,Menager.getwidth()-1,87+21+1+21,//domyslne wspolrzedne
                    histspecj);
//pom->setdatacolors(0,255);//Pierwsze 25 kolorow bedzie slabo widoczne
pom->setframe(32);
pom->settitle("ROZKLAD SPECJALIZACJI");
Menager.insert(pom);

//Lokalne przeplywy energii
pom=new carpet_graph(Menager.getwidth()-80,87+21+1+21,Menager.getwidth()-1,87+21+1+21+21,
						EDynamism);//I zrodlo danych
pom->setdatacolors(0,255);//Pierwsze 25 kolorow bedzie slabo widoczne
pom->setframe(32);
pom->settitle("LOKALNY PRZEP�YW ENERGII");
Menager.insert(pom);

//LOG???
generic_basic_statistics_source*	OslonaStat=new generic_basic_statistics_source(OslonaZywych);
if(!OslonaStat) goto ERROR;
Sources.insert(OslonaStat);

generic_basic_statistics_source*	WiekStat=new generic_basic_statistics_source(WiekZywych);
if(!WiekStat) goto ERROR;
Sources.insert(WiekStat);

generic_basic_statistics_source*	SilaStat=new generic_basic_statistics_source(SilaZywych);
if(!SilaStat) goto ERROR;
Sources.insert(SilaStat);

generic_basic_statistics_source*	GebaStat=new generic_basic_statistics_source(GebaZywych);
if(!GebaStat) goto ERROR;
Sources.insert(GebaStat);


//OKNO HISTORI SILY
//dalej zakladamy ze SilaStat nie umieszcza sam podzrodel w menagerze danych
//ale latwo sprawic by to zalozenie nie bylo prawdziwe - wystarczy wywolac
//SilaStat->link_source_menager(Sources);
fifo_source<double>* dpom=new fifo_source<double>(SilaStat->Mean(),dlugosc_logow);//Fifo ze sredniej sily
if(!dpom) goto ERROR;
int MeanSilaFifoIndex=Sources.insert(dpom);

dpom=new fifo_source<double>(SilaStat->Max(),dlugosc_logow);//Fifo ze sredniej sily
if(!dpom) goto ERROR;
int MaxSilaFifoIndex=Sources.insert(dpom);

dpom=new fifo_source<double>(SilaStat->SD(),dlugosc_logow);//Fifo ze sredniej sily
if(!dpom) goto ERROR;
int SdSilaFifoIndex=Sources.insert(dpom);

pom1=new sequence_graph(Menager.getwidth()-80,87+21+1+21+21,Menager.getwidth()-1,87+21+1+21+21+21,
							    3,Sources.make_series_info(
								MeanSilaFifoIndex,SdSilaFifoIndex,MaxSilaFifoIndex,
													-1
										).get_ptr_val(),
								0/* Z reskalowaniem */);
							   //1/*Wspolne minimum/maximum*/);
if(!pom1) goto ERROR;
pom1->setframe(128);
pom1->settitle("HISTORIA ENERGII");
Menager.insert(pom1);

//Okno historii liczby taksonow od poczatku
pom1=new sequence_graph(szer_map+301,300,Menager.getwidth()-1,400,
							    2,Sources.make_series_info(
								    inAllclons,inAlltax,
													-1
										).get_ptr_val(),
								0/* Z reskalowaniem */);
							   //1/*Wspolne minimum/maximum*/);
if(!pom1) goto ERROR;
pom1->setframe(32);
pom1->settitle("PRZYROST DRZEWA FILOGENETYCZNEGO");
Menager.insert(pom1);


//INFORMACJE
OutArea=new text_area(szer_map,300,szer_map+300,400,"Information area",
					  default_white,default_black,128,40);
if(!OutArea) goto ERROR;
OutArea->settitle("STATYSTYKA:");
int out_area=Menager.insert(OutArea);


//TAXONY - OSTANIE OKNO BO MOZE BYC NAJKOSZTOWNIEJSZE W RYSOWANIU
Liczniki=new matrix_source<unsigned>("liczebnosc taksonow",
                                     256,256,
									 agent::liczniki,
									 //NAZWA SERI
									 1//Nie torus
									 );
if(!Liczniki) goto ERROR;
Sources.insert(Liczniki);

log_1_plus_F_filter<matrix_source<unsigned> >* fl=new log_1_plus_F_filter<matrix_source<unsigned> >(Liczniki);
if(!fl) goto ERROR;
fl->set_missing(0);

//pom=new fast_carpet_graph< log_1_plus_F_filter<matrix_source<unsigned> > >
pom=new carpet_graph
					(szer_map+1,0,Menager.getwidth()-81,299,//domyslne wspolrzedne
						fl,1);//I zrodlo danych
if(!pom) goto ERROR;

pom->setbackground(0);
pom->setdatacolors(14,254);//Pierwsze 25 kolorow bedzie slabo widoczne
pom->settextcolors(255,255);
pom->settitle("MAPA UZYWANYCH NISZ EKOLOGICZNYCH");
Menager.insert(pom);

//USTALANIE KOLUMN DO WYDRUKU
//log.insert(sca);//Licznik krokow == wersji 
log.insert(sind); //liczba indywiduuw
log.insert(sinda);//liczba autotrofow
//log.insert(SilaStat->RealN());//==liczbai indywiduuw
//log.insert(SilaStat->LenN());//==pojemnoisci siedliska
log.insert(sklon);
log.insert(sklona);
log.insert(stax);
log.insert(staxa);
log.insert(allclons);
log.insert(alltax);
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

Menager.maximize(out_area);

return;
}//KONIEC WLASCIWEGO BLOKU

ERROR://BLOK OBSLUGI BLEDOW
perror("Nie mozna utworzyc obiektow wizualizujacych");
exit(1);																					
}

/* IMPLEMENTACJA CZESCI NIEZALEZNEJ OD PLATFORMY */
/*************************************************/

//DEFINICJA KONSTRUKTORA
swiat::swiat(size_t szerokosc,char* logname,char* mappname):	
	zdatnosc(szerokosc,(szerokosc/2),128/*INICJALIZACJA*/),
	ziemia(szerokosc,(szerokosc/2)/*DOMYSLNA INICJALIZACJA KONSTRUKTOREM*/), //Prostokat np 20x10
    stats(szerokosc,(szerokosc/2)),
	DLUG_WIERSZA(szerokosc),
	Sources(100),
	OutArea(NULL),
	Liczniki(NULL),
	licznik(0),
	Wiek(NULL),Sila(NULL),
	Trofia(NULL),Oslona(NULL),
	klon_auto(1),tax_auto(1),autotrofy(1),
	log(100,logname)
{//!!!Nie mozna tu jeszcze polegac na wirtualnych metodach klasy swiat
if(mappname)
    zdatnosc.init_from_bitmap(mappname);
informacja_klonalna::podlacz_marker_czasu(&licznik);
informacja_klonalna::ustaw_maksimum_kasowania(10);
}

void swiat::init()
{
Ziemia(DLUG_WIERSZA/2,DLUG_WIERSZA/4).init(255,255,255);
log.GetStream()<<"KO-EWOLUCJA\n"
//fprintf(log,"%ux%u\tWYP_POT=%f\tEFEKT=%f\tMAX_WIEK=%d\tPLOD=%f\tRTG=%f\tBUM=0.5^%d\n",
"ROZMIAR="<<DLUG_WIERSZA<<'x'<<DLUG_WIERSZA/2<<'\n'<<
"WYPOSAZENIE_POTOMSTWA="<<WYPOSAZENIE_POTOMSTWA<<'\n'<<
"EFEKTYWNOSC_AUTOTROFA="<<EFEKTYWNOSC_AUTOTROFA<<'\n'<<
"MAX_WIEK="<<(255-MINIMALNY_WIEK)<<'\n'<<
"PLODNOSC="<<1./NIEPLODNOSC<<'\n'<<
"RTG="<<1./PROMIENIOWANIE<<'\n'<<
"PRAWDOPODOBIENSTWO KATASTROF="<<WSP_KATASTROF<<(WSP_KATASTROF<=0?"(disabled)":"")<<'\n'<<
"EKSPLOATACJA="<<(PIRACTWO?"DRAPIEZNICZA":"PASOZYTNICZA")<<'\n';
}

ostream& operator << (ostream& o,const agent& a)
{
	o<<a.w.w.geba<<' '<<a.w.w.oslona<<' '<<a.wiek<<' '<<a.sila<<'\t';
	return o;
}

istream& operator >> (istream& i,agent& a)
{
	i>>a.w.w.geba;
	i>>a.w.w.oslona;
	i>>a.wiek;
	i>>a.sila;
	return i;
}

/* NAJWAZNIEJSZE FUNKCJE - GLOWNA IDEA SYMULACJI */

int agent::init(base trof,base osl, unsigned isila)
// inicjacja kazdego nowego indywiduum
{
w.w.oslona=osl;
w.w.geba=trof;
sila=isila;
wiek=MINIMALNY_WIEK;
assert(w._full>0);

// Sprawdzenie czy oslona nie jest za dobra i czy inne parametry sa OK
if(w.w.oslona==0 || !jest_zywy())
      {
      klon->dolicz_indywiduum(); //Bo inaczej nigdy by slad nie zostal - a moze czasem powinien
      _clean();
      return 0;//Nieprawidlowe geny
      }

//AGENT OK - REJESTRACJA 
////////////////////////////
//Wzrasta liczba indywiduuw
if(klon==NULL)
{
    static int tylko_raz=0;
    assert(tylko_raz==0);//Upewnienie sie ze to sie zdaza tylko raz na synulacje
    tylko_raz++;

    klon=new informacja_klonalna(NULL);//Klon bez "rodzica"   
}

//Operacje na licznikach klonow i taksonow
klon->dolicz_indywiduum();//Klonalny licznik indywidu�w
ile_ind++;                //Globalny licznik indywidu�w

base2 iwfull=w.w.geba*256+w.w.oslona;
liczniki[iwfull]++;                                            assert(*(liczniki+iwfull));//Tablica dla aktualnych "nisz ekologicznych"

if(liczniki[iwfull]>max)
		{
		max=liczniki[iwfull];
		max_change=1;
		}

if( liczniki[iwfull]==1 ) // pierwszy przedstawiciel taxonu
		ile_tax++;   // wiec liczba taxonow wzrasta

if( liczniki[iwfull]==11 ) // osiagnal wartosc >10 duzy taxon - rozwojowy
		ile_big_tax++;

return 1;
}

int agent::kill()
// Kazde zabicie agenta
{
assert( w._full>0 );

//DEREJESTRACJA AGENTA
ile_ind--;
base2 wfull=w.w.geba*256+w.w.oslona;//Zeby bylo niezalezne od "endian"
liczniki[wfull]--;
if( liczniki[wfull]==0 )	//ostatni przedstawiciel tego taxonu
	ile_tax--;
if( liczniki[wfull]==10 ) // osiagnal wartosc <=10 maly taxon - nie rozwojowy
	ile_big_tax--;
assert(w._full>0 );

//FIZYCZNA KASACJA
_clean();
return 1;
}

int  agent::init(agent& rodzic)
 // inicjacja nowego jako potomka starego
{
    w._full=duplikuj(rodzic.w._full);//Tworzy nowy genotyp
    unsigned uposazenie=unsigned(rodzic.sila*WYPOSAZENIE_POTOMSTWA);//Kalkuluje uposazenie
    unsigned cena=  (ODWROCONY_KOSZT_ATAKU?(base)(~w.w.geba):w.w.geba) + 
                    (ODWROCONY_KOSZT_OSLONY?(base)(~w.w.oslona):w.w.oslona) + 
                    uposazenie; //I caly koszt potomka
    
    if( rodzic.sila<=cena )  // Rodzic nie ma sily na potomka
    { 
        _clean(); 
        return 0; //Niepowiodlo sie
    }
    else
    {
        rodzic.sila-=cena; 	 // Placi za wyprodukowanie i wyposazenie
        assert(rodzic.sila!=0);
        
        if(w._full!=rodzic.w._full)//Zaszla jakas mutacja - powstal nowy klon       
        {
            klon=new informacja_klonalna(rodzic.klon);//Nigdy nie jest zwalniane w przebiegu programu       
        }
        else
        {
            klon=rodzic.klon;
        }

        return init(w.w.geba,w.w.oslona,uposazenie);   // prawdziwa inicjacja i wywolanie "klon->dolicz_indywiduum()"
    }
}

int agent::kill(agent& zabojca,unsigned& energy_flow)
// usmiercenie indywiduum przez drugie
{
if(zabojca.sila==0) 
           return 0; //Juz niezdolny zabijac
                                        assert(w.w.oslona>0 && w._full>0);
int ret=parasit(zabojca,energy_flow);//Zubazanie. 1 gdy zabil. Zwraca ilosc przejetej energii
if(ret==1) return ret;
	else   return kill();// Potem ofiara i tak ginie. Nadmiar sily idzie w kosmos
}

int agent::parasit(agent& zabojca,unsigned& energy_flow)
//zubazanie agenta przez drugiego. Jesli calkowite to ofiara ginie
{
/* Zabojca dostaje pewna czesc sily */
/* proporcjonalna do tego ile bitow oslony pasuje do jego geby */
assert(w.w.oslona!=0);
assert(zabojca.w.w.geba!=0);
		
energy_flow=0;
unsigned pom=unsigned(sila *
			double(w.w.oslona & zabojca.w.w.geba)/(zabojca.w.w.geba)*
			double(w.w.oslona & zabojca.w.w.geba)/(w.w.oslona)
			);

//assert(pom>0);//To powinno byc wykluczone wczesniej, ale dla malej sily moze sie zniweczyc przez rachunek calkowitoliczbowy
assert(sila>=pom);//Nie mozna wziac wiecej niz ma

zabojca.sila+=pom;
sila-=pom;	
energy_flow=pom;

assert(zabojca.sila!=0);//I zabojca nie powinien "umrzec z przejedzenia" (przekret licznika)

if(sila==0) //Czy zjedzony w calosci
	return kill();
	else
	return 0;
}

int agent::uplyw_czasu()
// prawo czasu - wszystko sie starzeje i traci energie
{
assert(sila>0);
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

if(sila==0 || wiek==0) //Czy CALKIEM brak sily, lub zycie sie "przekrecilo"
	return kill();
	else
	return 0;
}

base2 agent::duplikuj(base2 r)
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

stats.Reinitialise();//Czyszczenie lokalnej statystyki
long ile= long( sqr(double(DLUG_WIERSZA))/2.0*MonteCarloMult ); // ile na krok MonteCarlo
licznik++;//Licznik krokow

// rob krok MonteCarlo
for(long i=0;i<ile;i++) 
	{
	int x=RANDOM(DLUG_WIERSZA);
	int y=RANDOM(DLUG_WIERSZA/2);

	if(zdatnosc.get(x,y)==0)//jest pusto
		continue;			//obrob nastepnego

	if(!Ziemia(x,y).jest_zywy() )//jest martwy
		continue;		         //obrob nastepnego

	if(Ziemia(x,y).uplyw_czasu()==1)    //Koszty czasu zycia
		continue;				 //Zginal ze starosci
	
	//Losowanie sasiedniego miejsca
	unsigned	a=0,licznik=0;	
	int			x1,y1;
	do{
	a=RANDOM(8);
	if((licznik++)==8) goto JESZCZE_RAZ;//Wyskok kontynuujacy petle zewnetrzna
	assert(a<8);
	x1=(x+kierunki[a].x);
	y1=(y+kierunki[a].y);
	}while(zdatnosc.get(x1,y1)==0);//Dopuki nie wylosujemy zdatnego pola
	
    if(!Ziemia(x1,y1).jest_zywy()) //Jesli wylosowane pole jest wolne 
	   {//------------------------------------------------------------
        if(RANDOM(NIEPLODNOSC)==0)  //Pronuje sie rozmnazac
        {
            Ziemia(x1,y1).init(Ziemia(x,y));
        }
        else					   //lub przemieszczenie jesli ma zdolnosc ruchu
        {
            if(!(Ziemia(x,y).w.w.oslona&BIT_RUCHU))//...czyli BIT_RUCHU jest wyzerowany
            {
                ziemia.swap(x1,y1,x,y);			
            }
        }
	   }
	   else // Jesli na wylosowanym polu jest zywy - potencjalna ofiara
           //-------------------------------------------------------------------------
       {
           if( Ziemia(x,y).w.w.geba!=AUTOTROF &&					  //atakujacy nie jest autotrofem
               (Ziemia(x1,y1).w.w.oslona & Ziemia(x,y).w.w.geba) != 0 )//...i moze uszknac sasiada 
           {
               unsigned energy=0;
               if(PIRACTWO)
                   Ziemia(x1,y1).kill(Ziemia(x,y),energy);	//Zabicie sasiada
               else
                   Ziemia(x1,y1).parasit(Ziemia(x,y),energy);	//Eksploatacja sasiada
               stats.get(x,y).sE+=energy;
               stats.get(x,y).N++;
           }
       }
	continue;	//NASTEPNY NAWROT PETLI
	JESZCZE_RAZ://AWARYJNE WYJSCIE
		   ;
	}


//Koniec kroku monte-carlo po agentach
//------------------------------------------
kataklizm(); // jeden, chocby bardzo maly na krok monte-carlo

{//Uaktualnienie informacji o frakcjach autotrofow
klon_auto=0;
tax_auto=0;
autotrofy=0;
for(unsigned i=0xffff;i>0xffff-0x100;i--)//Ile niezerowych klonow autotroficznych
	{
	unsigned pom=agent::liczniki[i];
	autotrofy+=pom;
	if(pom>0)
		klon_auto++;
	if(pom>10)
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
if(WSP_KATASTROF<=0)
   return; //spontaniczne katastrofy wylaczone
x=RANDOM(DLUG_WIERSZA);
y=RANDOM(DLUG_WIERSZA/2);
power=poison(WSP_KATASTROF);
assert(power>=0 && power<=1);
ziemia.clean_circle(x,y,int(power*DLUG_WIERSZA/2));
}



//IMPLEMENTACJA WIZUALIZACJI Z UZYCIEM MENAGERA
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
sprintf(bufor.get_ptr_val(),"%lu KROK SYMULACJI\n"
							"LICZBA AGENTOW:%lu "
							"(%lu AUTOTROFICZNYCH)\n"
							"LICZBA TAKSONOW:%lu "
							"(%lu AUTOTROFICZNYCH)\n"
							"LICZBA KLONOW:%lu "	
							"(%lu AUTOTROFICZNYCH)\n"
                            "ILE KLON�W OD POCZATKU:%lu "
                            "(%lu ISTOTNYCH)\n",
	//agent::nazwy[agent::plot_mode],
	(unsigned long)licznik,
	(unsigned long)agent::ile_ind,
	(unsigned long)autotrofy,
	(unsigned long)agent::ile_big_tax,
	(unsigned long)tax_auto,
	(unsigned long)agent::ile_tax,
	(unsigned long)klon_auto,
    (unsigned long)informacja_klonalna::ile_klonow(),
    (unsigned long)informacja_klonalna::ile_taksonow()
    );


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
//printf("POLECENIA: 'g': Trofia 'o':OSLONA 's':SILA 'w':WIEK 'q':QUIT\n");
printf("LICZBA MOZLIWYCH KLONOW=%lu MAXINT=%d\n",(unsigned long)MAXBASE2,INT_MAX);

if(sizeof(base)*2!=sizeof(base2))
	{
	fprintf(stderr,"Niewlasciwe rozmiary dla typow bazowych:2*%u!=%u\n",
		sizeof(base),sizeof(base2));
	exit(1);
	}

if(!parse_options(argc,argv))
        exit(1);

swiat& tenSwiat=*new swiat(IBOKSWIATA,LogName,MappName);
if(&tenSwiat==NULL)
    {
    fprintf(stderr,"Brak pamieci!\n");
    exit(1);
    }


RANDOMIZE(); 
main_area_menager Lufciki(24,SWIDTH,SHEIGHT,28);

if(!Lufciki.start(ProgramName,argc,argv,1/*Buffered*/))
		{
		 printf("%s\n","Can't initialize graphics");
		 exit(1);
		}

//Utworzenie sensownej nazwy pliku(-�w) do zrzutow ekranu
{
wb_pchar buf(strlen(SCREENDUMPNAME)+20);
buf.prn("%s_%ld",SCREENDUMPNAME,time(NULL));
Lufciki.set_dump_name(buf.get());
}


//Lufciki.process_input();//Pierwsze zdazenia

tenSwiat.init();
tenSwiat.tworz_lufciki(Lufciki);

//zamiast Lufciki.run_input_loop();
while(tenSwiat.get_licznik()<MAX_ITERATIONS)
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

void koszty()
{
    ofstream tabkoszt("tabkoszt.out");
    for(int i=0;i<=255;i++)
    {
        base mask=i;
        unsigned cost=(base)(~mask);
        tabkoszt<<unsigned(mask)<<'\t'<<unsigned(cost)/*<<hex<<'\t'<<unsigned(cost)<<dec*/<<endl;
    }
}

int parse_options(const int argc,const char* argv[])
{
char* pom;
//koszty();
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
    printf("Liczba iteracji ustawiona na %lu\n",MAX_ITERATIONS);
    }
    else
    if((pom=strstr(argv[i],"MAX="))!=NULL) //Nie NULL czyli jest
    {
    MAX_ITERATIONS=atol(pom+4);
    if(MAX_ITERATIONS<=0)
		{
		fprintf(stderr,"Liczba iteracji nie moze byc <=0\n");
		return 0;
        }
    printf("Liczba iteracji ustawiona na %lu\n",MAX_ITERATIONS);
    }
    else
    if((pom=strstr(argv[i],"LOGC="))!=NULL) //Nie NULL czyli jest
    {
    LogRatio=atol(pom+5);
    if(LogRatio<=0)
		{
		fprintf(stderr,"Czestosc zapisu nie moze byc <=0\n");
		return 0;
        }
    }
    else
	if((pom=strstr(argv[i],"PIRA="))!=NULL) //Nie NULL czyli jest
    {
    PIRACTWO=(pom[5]=='Y');
    }
    else
    if((pom=strstr(argv[i],"LOGF="))!=NULL) //Nie NULL czyli jest
    {
    strcpy(LogName,pom+5);
    }
	else
    if((pom=strstr(argv[i],"MAPP="))!=NULL) //Nie NULL czyli jest
    {
    strcpy(MappName,pom+5);
    }
    else
    if((pom=strstr(argv[i],"WIDTHWIN="))!=NULL) //Nie NULL czyli jest
	{
	SWIDTH=atol(pom+9);
    if(SWIDTH<50)
		{
		cerr<<"Bad WIDTHWIN = "<<SWIDTH<<" (must be >50)"<<endl;
		return 0;
		}
	}
	else
	if((pom=strstr(argv[i],"HEIGHTWIN="))!=NULL) //Nie NULL czyli jest
	{
	SHEIGHT=atol(pom+10);
    if(SHEIGHT<50)
		{
		cerr<<"Bad HEIGHTWIN = "<<SHEIGHT<<" (must be >50)"<<endl;
		return 0;
		}
	}
	else /* Ostatecznie wychodzi ze nie ma takiej opcji */
	{
	fprintf(stderr,"Bledna opcja %s\n",argv[i]);
	fprintf(stderr,"MOZLIWE TO:\n");
    fprintf(stderr,"BOK=NN - bok obszaru symulacji\n"
					"AUTO=0.XX -efektywnosc autotrofa\n"
					"PIRA=0/1 - gospodarka pasozytnicza versus drapieznicza\n"
				   "MAX=NNNN - najwieksza mozliwa liczba krokow symulacji\n"
				   "BUM=NN - wykladnik czestosci katastrof\n"
				   "LOGC=N - czestosc zapisow do logu\n"
				   "LOGF=name.log - nazwa pliku z logiem\n"
				   "MAPP=init.gif - nazwa pliku z mapa zasobow\n"
                   "WIDTHWIN,HEIGHTWIN - rozmiary obszaru roboczego okna\n");
	return 0;
	}
    }
return 1;
}

