/* Program symulujący KOEWOLUCJĘ (Wersja z roku 2013 - prawdopodobnie reaktywowana na nową wersję BDS)*/
/* Każdy osobnik ma swój bitowy wzorzec odżywiania i bitowy wzorzec */
/* strategii osłony. Jeśli ATAKOWANY.OSLONA AND ATAKUJACY.Trofia>0 to  */
/* znaczy, że atak zakończył się powodzeniem.			    */
/* Osobniki rozmnażają się kosztem wolnego miejsca i zgromadzonej energii */
/* Ruchy własne, ofiara ataku, jak i moment rozmnażania wybierane są */
/* losowo, aby nie zaciemniać modelu dodatkowymi parametrami. 	     */

//MODYFIKACJE DLA UMOŻLIWIENIA KOMPILACJI 2022.07!

//W razie czego:  __sbh_heap_check albo podobne, jeśli jest

#ifdef NDEBUG
const char*	   ProgramName="CO-EVOLUTION wer. 3.61b (c)1994-2013: Wojciech Borkowski [idea i realizacja]";
#else
const char*	   ProgramName="CO-EVOLUTION wer. 3.61b DEBUG (c)1994-2023 Wojciech Borkowski";
#endif

#pragma warn -aus //??? i jaki kompilator?


int My_Rand_seed=1; //Jak 0 to RANDOMIZE jak inny to SRAND(My_Rand_seed)

namespace wbrtm { /// To jest zadeklarowane, ale trzeba gdzieś zdefiniować z wartością.
    unsigned _lingo_selector = 1; ///< lang_selector=1;   - Jaki język
}

//WERSJA 3.61
//1. Możliwość puszczania z tym samym ziarnem losowym

//WERSJA 3.6
//1. Angielskie nazewnictwo okien i oraz tłumaczenie innych elementów
//2. Zmienne w czasie nasłonecznienie
//3. Angielskie nazwy parametrów i komunikaty

//Wersja 3.5
//Uporządkowanie liczników kroków i przejście na kroki Monte-Carlo tam, gdzie się dało...
//Zostały licznik kroków oraz osobno liczone 'wersje' danych.
//Wizualizacja 'on run' serii czasowych daje zawsze równe odstępy punktów, niezależnie
//od aktualnej multiplikacji MC. Jednak w pliku log jest seria z właściwymi wartościami MC

//Wersja 3 - nowe możliwości
//1. Różne dane do kolorowania drzewa filogenetycznego wybierane z menu
//2. Rozbudowa listy parametrów 'wyprowadzonych' na zewnątrz programu i usunięcie blokady na 'światy' większe od ekranu
//3. Dodanie miernika czasu procesora w głównej pętli i wyprowadzenie wraz z krokiem na konsole
//4. 25.04.08 Usunięcie błędu w bibliotecznym menadżerze lufcików wywalającego program przy robieniu TILE
//5. 11.09.08 Wypisywanie do pliku kompletnej filogenezy
//6. Wersja 3.1 - poprawki techniczne i kompilowalność pod MSVC 2008
//7. Uelastycznienie wizualizacji sieci ekologicznej i jej zapisywania
//8. Umożliwienie ciągłych zrzutów zawartości okna graficznego i sieci ekologicznej w dwu formatach
//9. Regulacja częstość zrzutów. NIEDOKOŃCZONA, bo POWODUJE BŁĘDNĄ NUMERACJĘ PLIKÓW I KROKÓW

//Istotne nowości w wersji 2.5
//1. Odwrotność prawdopodobieństwa mutacji wyprowadzona jako parametr 'RTG'.
//Mutacje w okolicy 1/16000 dla obszaru 400x200 doprowadzają do wyparcia heterotrofów 70000:100
//i to przy różnych wartościach globalności oddziaływań.
//Przy poziomie 1/1600 jest zachowana równowaga podobna do tej z typowego poziomu 1/160
//2. Wprowadzenie parametru wewnętrznego `ZAMIANY' umożliwiającego elastyczne przepływanie obok siebie obojętnych osobników
//Ma ono zapobiegać formowaniu się frontów, czyli blokowaniu rozprzestrzeniania się skutecznych taksonów przez 'gęstość zaludnienia'
//3. Wprowadzenie maksymalnego dystansu skoku pozwala zbadać zależność od 'globalności ekologii'
//4. Dodano też wyświetlanie wieku taksonów
//5. Dodano moduł 'kladystyka' oparty o dane z "klonalinfo" i służący preparowaniu prymitywnego (na razie) drzewa filogenetycznego
//   kolorowanego specjalizacją
//6. Nieco uelastyczniono sposób ustalania, co jest klonem a co taksonem. Symbol 'treshold' jest teraz zmienną.
//7. Dodano moduł ekologiczny rejestrujący i analizujący siec troficzna
//8. Uzupełniono interface o możliwości własnych opcji użytkownika, np. wyłączanie obliczania sieci ekologicznej i kladystyki
//9. Zmieniono na szary tło niektórych grafów
//15.02.2006
//  Rekompilacja z poprawioną biblioteką wspierająca
//  ...

#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <csignal>
#include <cassert>

#include <time.h>
//#include <process.h>

//#include "INCLUDE/platform.hpp"

#define USES_RANDG
#include "random.h" //WB random generators for C-lang  (MACROS!)

#define HIDE_WB_PTR_IO	(0) //Musi być IO dla wb_dynarray
#include "wb_ptr.hpp"
#include "wb_ptrio.h"
#include "wb_cpucl.hpp"

#include "datasour.hpp"
#include "simpsour.hpp"
#include "filtsour.hpp"
#include "bifiltso.hpp"
#include "statsour.hpp"
#include "dhistosou.hpp"
#include "fifosour.hpp"
#include "funcsour.hpp"
#include "sourmngr.hpp"
#include "logfile.hpp"

#include "gadgets.hpp"
#include "textarea.hpp"
#include "graphs.hpp"
#include "areamngr.hpp"
#include "mainmngr.hpp"

#include "layer.hpp"

#include "sshutils.hpp"
#include "sshmenuf.h"

#include "lingo.hpp"

#ifdef _MSC_VER		  //MSVC on Windows only!
#include <process.h>  //_getpid
#include <direct.h>   //_getcwd
#define MyGetPid (::_getpid)
#else
#include <unistd.h>   //getpid() ?
#define MyGetPid (::getpid)
#define _getcwd  (::getcwd)
#endif

#include "ResSrc/coewo3rc.h"

#ifdef __BORLANDC__
#ifndef _getpid
#define _getpid()  (getpid())
#endif
#endif

class swiat; //Zapowiedz głównej klasy symulacji

// Manager lufcików z uzupełnioną obsługa i/o (z menu)
class my_area_menager:public main_area_menager
{
	bool trace_fillog_tree;
	bool trace_trophi_netw;
	swiat*   MojSwiat;
	virtual int _pre_process_input(int input_char); //Przed obsługą domyślną. Zwraca 1, jeśli obsłużył.
public:

	my_area_menager(size_t size, //Konstruktor dający zarządcę o określonym rozmiarze listy
				int width,int height,
				unsigned ibkg=default_color):main_area_menager(size,width,height,ibkg),
				trace_fillog_tree(true),
				trace_trophi_netw(true),
				MojSwiat(NULL)
	{}
	int start(const char* wintitle,int argc=0,const char** argv=NULL,int double_buffering=-1); //Zwraca 1, jeśli OK.
	void connect(swiat* sw) {MojSwiat=sw;}
	bool trace_filogenetic_tree_enabled() { return trace_fillog_tree;}
	bool trace_trophic_network_enabled() { return trace_trophi_netw;}
};

#include "intersourc.hpp" //Źródła specjalne AND i ...

// Źródła specjalne do celów demonstracyjnych
// ... tzn ładne dywaniki oddziaływań
and_interaction_source	AndDemo(lang("Trofia AND Oslona","Trophy AND Defence"));       
and_exploatation_source ExpDemo(lang("(T AND O)/O * (T AND O)/T","(T AND D)/D * (T AND D)/T"));

#include "klonalinfo.hpp"
#include "troficinfo.h"
#include "kladystyka.hpp"
#include "ekologia.hpp"
#include "co_agent.hpp"

//BARDZO WAŻNE WSKAŹNIKI DO MASEK CZYLI GENÓW BĘDĄCYCH SKŁADOWYMI STRUKTURY
//base agent::* geba_ptr=&agent::w.w.geba;      //TAK POWINNO BYC - ALE NIE KOMPILUJE
//base agent::* oslona_ptr=&agent::w.w.oslona;

#define RECZNIE_WPISZ_PTR //RĘCZNE WPISYWANIE - ZALEŻNE OD 'ENDIAN' I WIDZIMISIĘ TWÓRCÓW KOMPILATORA

#ifdef  RECZNIE_WPISZ_PTR
base agent::* geba_ptr=NULL; //Tu w zasadzie tylko deklaracja...
base agent::* oslona_ptr=NULL; //Przypisanie jest dalej, też zabezpieczone przez "ifdef RECZNIE_WPISZ_PTR"
#else
base agent::* geba_ptr=&(agent::w.w.geba); //TAK NIE DZIAŁA
base agent::* oslona_ptr=&(agent::w.w.oslona);
#endif

//TAK MSVC KOMPILUJE ALE WPISUJE 0
//base agent::* geba_ptr=&agent::w.wzor::w.bity_wzoru::geba;
//base agent::* oslona_ptr=&agent::w.wzor::w.bity_wzoru::oslona;

//Ważne domyślne ustawienia
char     SCREENDUMPNAME[256]="CO-EVO-3a";
char     LogName[256]="coewo3.0a.out";
char     MappName[256]=""; //coewo2008ini.gif

//Często zmieniane parametry kompilacji i wykonania samego programu
unsigned SWIDTH=1200; //750; //960; //1200; //; //1200; Też zadziała
unsigned SHEIGHT=900; //550; //750; //750//; //1000;

//const unsigned TAX_OUT=256; //PO CO TO?  zamiast REJESTROWANY_ROZMIAR_TAKSONU?
//PODSTAWOWE PARAMETRY MODELU
unsigned IBOKSWIATA=300;                // FAKTYCZNIE UŻYWANY BOK ŚWIATA
unsigned MINIMALNY_ROZMIAR_TAKSONU=100;  //Arbitralny rozmiar klonu, wystarczający, żeby go uznać za zarejestrowany takson
unsigned REJESTROWANY_ROZMIAR_TAKSONU=MINIMALNY_ROZMIAR_TAKSONU; //Domyślna wartość kasowania w wizualizacji drzewa
unsigned REJESTROWANY_ROZMIAR_WEZLA=0; //MINIMALNY_ROZMIAR_TAKSONU-1; //Domyślna wartość kasowania w wizualizacji sieci eko

unsigned	PROMIENIOWANIE=BITS_PER_GENOM*500;	// Co ile kopiowanych bitów następuje mutacja
bool		UZYWAJ_PELNE_PROM=true;				// Czy używamy pełnego promieniowania, czy obniżonego? NIE UŻYWANE???

int    WSP_KATASTROF=0; //10-100		   // Wykładnik rozkładu katastrof. 0 - wyłączone.
int    DZIELNIK_1_OKRESU_MILANKOVICA=50;   // Przez ile dzielić kroki M C, żeby uzyskać coś do zmian oświetlenia
double POWER_COS_NASLONECZNIENIA=0;        // "Potęga cyklu słonecznego". 0 - wyłączone; 1 - zwykły cosinus; 2 - tropikalny 3 - pory roku.

double		EFEKTYWNOSC_AUTOTROFA=0.99;	// jaka część światła używa autotrof.
unsigned	NIEPLODNOSC=5;		        // Prawdopodobieństwo rozmnażania jest 1/NIEPLODNOSC.
double		WYPOSAZENIE_POTOMSTWA=0.05; // Jaką część siły oddać potomkowi. UWAGA! 0.1 to ZA DUŻO!!! {???}

unsigned int	MINIMALNY_WIEK=205;	      // Rodzi się z tym wiekiem. Do śmierci ma 255-MINIMALNY_WIEK
unsigned int	ODWROCONY_KOSZT_OSLONY=0; // 1 - czy koszty osłony są odwrócone
unsigned int	ODWROCONY_KOSZT_ATAKU=0;  // 1 - czy koszty ataku są odwrócone

unsigned int	PIRACTWO=1;	  //Czy eksploatacja "piracka", czy pasożytnicza.
unsigned int	DYS_RUCHU=1;  //Dystans ruchu w jednym kierunku.
//unsigned BIT_RUCHU=128;	  //Wyzerowanie których bitów osłony odpowiada za zdolność ruchu
unsigned	BIT_RUCHU=1024;   //1		//Gdy poza maskę to bez możliwości utraty ruchliwości
unsigned	ZAMIANY=1; //0//255;        //Czy może przeciskać się na zajęte pola (0 na nie lub inna liczba na tak)

// Zewnętrzne zarządzanie symulacją i zbieraniem danych
unsigned long	MAX_ITERATIONS=0xffffffff; // największa liczba iteracji
unsigned dlugosc_logow=100000;       //Długość wewnętrznych logów podręcznych (do wizualizacji)
unsigned LogRatio=1;				 // Co ile kroków zapisywać do logu.
int      MonteCarloMultiplic=10;     //Ewentualna modyfikacja multiplikacji długości kroku MonteCarlo
                                     //... ale dlaczego było double ??????

bool DumpScreenContinously=false;
bool DumpNETContinously=false;
bool DumpVNAContinously=false;

//unsigned        textY=(IBOKSWIATA>TAX_OUT?IBOKSWIATA:TAX_OUT);         ???


/*  Część niezależna od platformy  */
/* ******************************* */

// Struktura do przechowywania lokalnej informacji statystycznej
// TODO??? Gdzie to jest używane?
struct inte_stat
{
	unsigned N; //Ilość interakcji
	double  sE; //Suma energii przejętej w interakcjach
	double  sP; //Suma udziałów (efektywności) interakcji

    //KONSTRUKTOR
    inte_stat():N(0),sE(0.0),sP(0.0){}

    //OPERATOR wyjścia
    //Bardzo prymitywne, bez kontroli błędów!!!
	friend ostream& operator<< (ostream& o, const inte_stat& inst)
	{
		o<<inst.N<<'\t'<<inst.sE<<'\t'<<inst.sP<<'\t';
		return o;
	}

    //OPERATOR wejścia
    //Bardzo prymitywne, bez kontroli błędów!!!
	friend istream& operator >> (istream& in, inte_stat& inst)
	{
		in>>inst.N;
		in>>inst.sE;
		in>>inst.sP;
		return in;
	}

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


// Cały świat symulacji
//-------------------------------
class swiat
{
// Parametry jednowartościowe:
//*////////////////////////////
size_t			DLUG_WIERSZA;	 // Obwód torusa
double 			globalneSwiatlo; // Bazowa globalna ilość światła (stała w modelu)
double 			AktualneSwiatlo; // Aktualna ilość światła (może się zmieniać w czasie)

// Statystyki i liczniki
//*///////////////////////////////
unsigned long	licznik_krokow_w; // licznik wywołań procedury kroku symulacji
unsigned long	monte_carlo_licz; //licznik realnie wykonanych kroków monte-carlo

unsigned 		klon_auto;		// Licznik klonów autotroficznych
unsigned 		tax_auto;		// Licznik taksonów autotroficznych
unsigned 		autotrofy;		// Licznik agentów autotroficznych

klad   filogeneza;              //Informacje statystyczne z drzewa filogenetycznego
ekologia trophNet;              //Informacje statystyczne dla sieci ekologicznej

//  Warstwy symulacji (są torusami, bo taka jest wspólna geometria)
//*//////////////////////////////////////////////////////////////////////
rectangle_unilayer<unsigned char> zdatnosc; //Warstwa definiująca zdatność do zasiedlenia
rectangle_layer_of_agents<agent>    ziemia; //Właściwa warstwa agentów zasiedlających
rectangle_layer_of_struct<inte_stat> stats; //Warstwa pamiętająca lokalne statystyki zdarzeń

// Zarządzanie źródlami danych
// /////////////////////////////////
sources_menager								 Sources;  //Zarządca serii danych (przekaźnikowych)
matrix_source<unsigned>						 *Liczniki; //Udostępnienie liczników taksonów
struct_matrix_source<agent,unsigned char >	 *Wiek;
struct_matrix_source<agent,unsigned>		 *Sila;
struct_matrix_source<agent,base >			 *Trofia;
struct_matrix_source<agent,base >			 *Oslona;
method_matrix_source<inte_stat,double>		 *EDynamism; //Lokalna dynamika przepływu energii
method_matrix_source<inte_stat,double>		 *PDynamism; //Lokalne efektywności eksploatacji
logfile										 log;	// plik z zapisem historii symulacji

// Obszary wyświetlania
//*//////////////////////////////////
my_area_menager&	MyAreaMenager;
net_graph*			FilogeneticTree;	//Drzewo filogenetyczne
net_graph*			TrophicNet;		//Sieć zależności pokarmowych
text_area*			OutArea; //Obszar do wypisywania statusu symulacji

public:
wb_pchar dumpmarker; //Uchwyt do nazwy eksperymentu

// KONSTRUKCJA DESTRUKCJA
//*//////////////////////
swiat(size_t szerokosc,
	  char* logname,   //Nazwa pliku do zapisywania historii
	  char* mappname,
	  my_area_menager& AreaMenager
	  ); //Nazwa (bit)mapy inicjującej 'zdatności' pól
~swiat(){}

//AKCJE
void inicjuj();	  // stan startowy symulacji
void krok();	  // kolejny krok symulacji
void kataklizm(); // wygenerowanie katastrofy (ręczne lub losowe)

// Współpraca z menadżerem wyświetlania:
//*//////////////////////////////////////////
void wskazniki(); // aktualizacja nowych wartości wskaźników
void tworz_lufciki(); //Tworzy domyślne lufciki

// Dostęp do głównych danych:
//*//////////////////////////
agent& Ziemia(size_t Kolumna,size_t Wiersz) {return ziemia.get(Kolumna,Wiersz);} //Dostęp do podłoża
float  Swiatlo(size_t Kolumna,size_t Wiersz) {return globalneSwiatlo;} //Dostęp do światła (tu globalnego)

// INNE METODY POMOCNICZE
//*/////////////////////////////////////////

// Do wizualizacji:
//*//////////////////
void ZmienKoloryDrzewaFil(klad::Kolorowanie co);
void UstalDetalicznoscSieci(int Ile,bool Relative=true);

// Do statystyk:
//*///////////////
unsigned long	daj_licznik_krokow(){return licznik_krokow_w;}		// licznik kroków symulacji
unsigned long   daj_kroki_monte_carlo(){return monte_carlo_licz;}   //suma realnie wykonanych kroków MC
void print_params(ostream& out);

// Do dodatkowych logów:
//*//////////////////////
void ZapiszFilogeneze(ostream& out,unsigned min_time=0,unsigned max_time=0,unsigned size_tres=0);
void ZapiszEkologieNET(ostream& out,unsigned size_tres=0,double weight_tres=0);
void ZapiszEkologieVNA(ostream& out,unsigned size_tres=0,double weight_tres=0);
};

//Tworzy domyślne lufciki
void swiat::tworz_lufciki()
{
if(0)   //SPECJALNY BLOK OBSŁUGI BŁĘDÓW
{
DO_OBSLUGI_BLEDOW: ;

	perror(lang( "Nie można utworzyć obiektów wizualizujących!",
                    "I'm not able to make visualisation stuff!"));

	//Tu tylko w wypadku błędu
	exit(1);
}

graph* pom;
size_t wys_map=MyAreaMenager.getheight()/4;
size_t szer_map=size_t(wys_map*2.3);
if((long)szer_map>MyAreaMenager.getwidth()-280)
		szer_map=MyAreaMenager.getwidth()-280;

#ifdef RECZNIE_WPISZ_PTR
{
int *hack1=(int*)&oslona_ptr;
int *hack2=(int*)&geba_ptr;

#ifdef __MSVC__
???? *hack2=0; //sizeof(base)?
*hack1=1; //?
#elif __GNUC__
*hack2=0; //sizeof(base)?
*hack1=1; //?
#else //BDS2006
*hack2=1; //sizeof(base)?
*hack1=2;
#endif

}
#endif

{
//Zerowa seria w menadżerze danych powinna byc pusta.
//Służy ona do kontroli wersji danych. Menadżer może tworzyć ją sam, ale zawsze można też podmienić.
ptr_to_scalar_source<unsigned long>* sca=new ptr_to_scalar_source<unsigned long>(&monte_carlo_licz,lang("KrokMC","MCStep"));
if(!sca) goto DO_OBSLUGI_BLEDOW;
data_source_base* first=Sources.get(0);
int index=0;
if(first!=NULL)
	Sources.replace(first->name(),sca);
	else
	index=Sources.insert(sca);        assert(index==0);

// Tylko ze względów demonstracyjnych
array_source<unsigned>* arr=new array_source<unsigned>(MAXBASE2+1,agent::liczniki,lang("liczniki taksonów - liniowo","taxon counters"));
if(!arr) goto DO_OBSLUGI_BLEDOW;
Sources.insert(arr);

// ŹRÓDŁA SUROWYCH ATRYBUTÓW AGENTÓW
Sila=ziemia.make_source(lang("energia","energy"),&agent::sila);
if(!Sila) goto DO_OBSLUGI_BLEDOW;
Sources.insert(Sila);

Trofia=ziemia.make_source(lang("trofia","trophy"),geba_ptr);
if(!Trofia) goto DO_OBSLUGI_BLEDOW;
Sources.insert(Trofia);
Trofia->setminmax(0,255);

Oslona=ziemia.make_source(lang("obrona","defence"),oslona_ptr);
if(!Oslona) goto DO_OBSLUGI_BLEDOW;
Sources.insert(Oslona);
Oslona->setminmax(0,255);

Wiek=ziemia.make_source(lang("wiek","age"),&agent::wiek);
if(!Wiek) goto DO_OBSLUGI_BLEDOW;
Wiek->setminmax(0,255);
Sources.insert(Wiek);

EDynamism=stats.make_source(lang("Eksploatacja","ExploItation"),&inte_stat::MeanEnergyOfInteraction);
if(!EDynamism) goto DO_OBSLUGI_BLEDOW;
Sources.insert(EDynamism);
EDynamism->set_missing(-1);

// ŹRÓDŁO METODOWE CZYTAJĄCE WIEK TAKSONU, DO KTÓREGO NALEŻY AGENT
method_matrix_source<agent,unsigned long>* WiekFilog=ziemia.make_source(lang("WiekFilogenetyczny","PhylogeneticAge"),&agent::how_old_taxon);
if(!WiekFilog) goto DO_OBSLUGI_BLEDOW;
//WiekFilog->setminmax(0,1);
WiekFilog->set_missing(0xffffffff);
int inWiekFilog=Sources.insert(WiekFilog);


// ŹRÓDŁO METODOWE FILTRUJĄCE ŻYWYCH
method_matrix_source<agent,bool>* Zycie=ziemia.make_source(lang("martwe-żywe","alive or not"),&agent::jest_zywy);
if(!Zycie) goto DO_OBSLUGI_BLEDOW;
Zycie->setminmax(0,1);
int inZycie=Sources.insert(Zycie);

GT_filter<method_matrix_source<agent,bool> >* filtrZycia=//Filtr na "Tylko żywe"
	new GT_filter<method_matrix_source<agent,bool> >(0.5,Zycie,default_missing<double>(),lang("Żywe","Alive")); //
if(!filtrZycia)goto DO_OBSLUGI_BLEDOW;
int inFiltrZycia=Sources.insert(filtrZycia);

//SAME ŻYWE INDYWIDUA
if_then_source* GebaZywych=new if_then_source(filtrZycia,Trofia,lang("TrofiaZ","TrophyA")); // Ale ma kłopot z missing_val i przez to zle liczy min i max TODO
if(!GebaZywych) goto DO_OBSLUGI_BLEDOW;
Sources.insert(GebaZywych);

if_then_source* OslonaZywych=new if_then_source(filtrZycia,Oslona,lang("ObronaZ","DefenceA")); //Ale ma kłopot z missing_val i przez to zle liczy min i max TODO
if(!OslonaZywych) goto DO_OBSLUGI_BLEDOW;
//OslonaZywych->if_then_source::
Sources.insert(OslonaZywych);

if_then_source* WiekZywych=new if_then_source(filtrZycia,Wiek,lang("WiekZ","AgeA"));
if(!WiekZywych) goto DO_OBSLUGI_BLEDOW;
Sources.insert(WiekZywych);

if_then_source* WiekFilogZ=new if_then_source(filtrZycia,WiekFilog,lang("WiekFiloZ","PhylAgeA"));
if(!WiekFilogZ) goto DO_OBSLUGI_BLEDOW;
Sources.insert(WiekFilogZ);

if_then_source* SilaZywych=new if_then_source(filtrZycia,Sila,lang("EnergiaZ","EnergyA"));
if(!SilaZywych) goto DO_OBSLUGI_BLEDOW;
Sources.insert(SilaZywych);

// Trofia
pom=new carpet_graph(0,wys_map,szer_map-1,2*wys_map-1, //domyślne współrzędne
						//TrofiaZ);  //I źródło danych
						GebaZywych); //I alternatywne źródło danych
if(!pom) goto DO_OBSLUGI_BLEDOW;
//pom->setdatacolors(0,255); //Pierwsze 25 kolorów będzie słabo widoczne
pom->settitle(lang("PREFERENCJE TROFICZNE (maski ataku)","TROPHIC PREFERENCES (masks of attack)"));
pom->settitlecolo(255,default_transparent);
pom->setbackground(default_half_gray);
MyAreaMenager.insert(pom);

// OSŁONA
pom=new carpet_graph(0,2*wys_map,szer_map-1,3*wys_map-1, //domyślne współrzędne
//						Oslona); //I źródło danych
						OslonaZywych); //I alternatywne źródło danych

if(!pom) goto DO_OBSLUGI_BLEDOW;
//pom->setdatacolors(0,255); //Pierwsze 25 kolorów będzie słabo widoczne
pom->settitle(lang("PODATNOŚĆ AGENTÓW NA ATAK (maski obrony)","SENSITIVITY TO THE ATTACK (mask of defence)"));
pom->settitlecolo(255,default_transparent);
pom->setbackground(default_half_gray);
MyAreaMenager.insert(pom);

//Energia
pom=new carpet_graph(MyAreaMenager.getwidth()-80,87+21+1+21+21,MyAreaMenager.getwidth()-1,87+21+1+21+21+21,
//						Sila); //I źródło danych
                        SilaZywych); //I alternatywne źródło danych
pom->setdatacolors(1,254); //Żeby nie używać białego, który jest tłem, a także całkiem czarnych.
pom->setbackground(default_half_gray);
pom->settitle(lang("ZASOBY ENERGII AGENTÓW","ENERGY RESOURCES OF AGENTS"));
pom->settitlecolo(255,default_transparent);
MyAreaMenager.insert(pom);

//ŹRÓDŁA O SPECJALIZACJI
method_matrix_source<agent,int>* Specjalizacja=ziemia.make_source(lang("bity ustawione","how much specialised"),
                                                                  &agent::how_specialised);
if(!Specjalizacja) goto DO_OBSLUGI_BLEDOW;
Specjalizacja->setminmax(1,16);
Specjalizacja->set_missing(-1);
int inSpecjalizacja=Sources.insert(Specjalizacja);

pom=new carpet_graph(0,3*wys_map,szer_map-1,4*wys_map, //domyślne współrzędne
						Specjalizacja); //I źródło danych
//pom->setdatacolors(1,254); //Żeby nie używać białego, który jest tłem, a także całkiem czarnych.
pom->setbackground(default_half_gray);
pom->settitlecolo(255,default_transparent);
pom->settitle(lang("POZIOM SPECJALIZACJI AGENTÓW","LEVEL OF SPECIALISATION"));
MyAreaMenager.insert(pom);

method_matrix_source<agent,int>* SpecjalizacjaA=ziemia.make_source(lang("bity autotrofów","spec. bits of autotrophs"),
                                                                   &agent::how_specialised_autotrof);
if(!SpecjalizacjaA) goto DO_OBSLUGI_BLEDOW;
SpecjalizacjaA->setminmax(1,16);
SpecjalizacjaA->set_missing(-1);
int inSpecjalizacjaA=Sources.insert(SpecjalizacjaA);

method_matrix_source<agent,int>* SpecjalizacjaH=ziemia.make_source(lang("bity heterotrofów","spec. bits of heterotrophs"),
                                                                   &agent::how_specialised_heterotrof);
if(!SpecjalizacjaH) goto DO_OBSLUGI_BLEDOW;
SpecjalizacjaH->setminmax(1,16);
SpecjalizacjaH->set_missing(-1);
int inSpecjalizacjaH=Sources.insert(SpecjalizacjaH);

//HISTORIA OSTATNICH <dlugosc_logow> KROKOW
//Parametry 'abiotyczne'
//AktualneSwiatlo????
template_scalar_source_base<double>* currLight=new ptr_to_scalar_source<double>(&AktualneSwiatlo,lang("Nasłonecznienie","Insolation"));
if(!currLight) goto DO_OBSLUGI_BLEDOW;
Sources.insert(currLight);
fifo_source<double>* curlpom=new fifo_source<double>(currLight,dlugosc_logow);
if(!curlpom)goto DO_OBSLUGI_BLEDOW;
int inCurrLight=Sources.insert(curlpom);

// Bogactwo klonów i gatunków
template_scalar_source_base<unsigned long>* allclons=
					new ptr_to_fuction_source<unsigned long>(informacja_klonalna::ile_klonow,lang("klonów od początku","clones from the beginning"));
if(!allclons) goto DO_OBSLUGI_BLEDOW;
Sources.insert(allclons);
fifo_source<unsigned long>* flpom=new fifo_source<unsigned long>(allclons,dlugosc_logow);
if(!flpom)goto DO_OBSLUGI_BLEDOW;
int inAllclons=Sources.insert(flpom);

template_scalar_source_base<unsigned long>* alltax=
					new ptr_to_fuction_source<unsigned long>(informacja_klonalna::ile_taksonow,lang("eko-gatunków od początku","species from the beginning"));
if(!alltax) goto DO_OBSLUGI_BLEDOW;
Sources.insert(alltax);
flpom=new fifo_source<unsigned long>(alltax,dlugosc_logow);
if(!flpom)goto DO_OBSLUGI_BLEDOW;
int inAlltax=Sources.insert(flpom);

// ile jest autotroficznych taksonów (liczniejszych niż treshold)
template_scalar_source_base<unsigned>* staxa=new ptr_to_scalar_source<unsigned>(&tax_auto,lang("eko-gatunki autotroficzne","autotrophic species"));
if(!staxa) goto DO_OBSLUGI_BLEDOW;
Sources.insert(staxa);

fifo_source<unsigned>* fpom=new fifo_source<unsigned>(staxa,dlugosc_logow);
if(!fpom) goto DO_OBSLUGI_BLEDOW;
int fiftaxa=Sources.insert(fpom);

// ile klonów autotroficznych
template_scalar_source_base<unsigned>* sklona=new ptr_to_scalar_source<unsigned>(&klon_auto,lang("klony autotroficzne","autotrophic clones"));
if(!sklona) goto DO_OBSLUGI_BLEDOW;
Sources.insert(sklona);

fpom=new fifo_source<unsigned>(sklona,dlugosc_logow);
if(!fpom) goto DO_OBSLUGI_BLEDOW;
int fifklona=Sources.insert(fpom);

// ile taksonów liczniejszych niż treshold
template_scalar_source_base<unsigned>* stax=new ptr_to_scalar_source<unsigned>(&agent::ile_big_tax,lang("eko-gatunki","species"));
if(!stax) goto DO_OBSLUGI_BLEDOW;
Sources.insert(stax);

fpom=new fifo_source<unsigned>(stax,dlugosc_logow);
if(!fpom) goto DO_OBSLUGI_BLEDOW;
int fiftax=Sources.insert(fpom);

// ile w ogóle istniejących klonów
template_scalar_source_base<unsigned>* sklon=new ptr_to_scalar_source<unsigned>(&agent::ile_tax,lang("klony","clones"));
if(!sklon) goto DO_OBSLUGI_BLEDOW;
Sources.insert(sklon);

fpom=new fifo_source<unsigned>(sklon,dlugosc_logow);
if(!fpom) goto DO_OBSLUGI_BLEDOW;
int fifklon=Sources.insert(fpom);

// RÓŻNE INNE STATYSTYKI
generic_basic_statistics_source*	OslonaStat=new generic_basic_statistics_source(OslonaZywych);
if(!OslonaStat) goto DO_OBSLUGI_BLEDOW;
Sources.insert(OslonaStat);

generic_basic_statistics_source*	WiekStat=new generic_basic_statistics_source(WiekZywych);
if(!WiekStat) goto DO_OBSLUGI_BLEDOW;
Sources.insert(WiekStat);

generic_basic_statistics_source*	SilaStat=new generic_basic_statistics_source(SilaZywych);
if(!SilaStat) goto DO_OBSLUGI_BLEDOW;
Sources.insert(SilaStat);

generic_basic_statistics_source*	GebaStat=new generic_basic_statistics_source(GebaZywych);
if(!GebaStat) goto DO_OBSLUGI_BLEDOW;
Sources.insert(GebaStat);

// HISTORIA LICZEBNOŚCI AGENTÓW

// ile agentów niezerowych
template_scalar_source_base<unsigned>* sind=new ptr_to_scalar_source<unsigned>(&agent::ile_ind,lang("liczba agentow","num. of agents"));
if(!sind) goto DO_OBSLUGI_BLEDOW;
Sources.insert(sind);

fpom=new fifo_source<unsigned>(sind,dlugosc_logow);
if(!fpom) goto DO_OBSLUGI_BLEDOW;
int fifagents=Sources.insert(fpom);

// ile agentów autotroficznych
template_scalar_source_base<unsigned>* sinda=new ptr_to_scalar_source<unsigned>(&autotrofy,lang("liczba autotrofów ","num. of autotrophs"));
if(!sinda) goto DO_OBSLUGI_BLEDOW;
Sources.insert(sinda);

fpom=new fifo_source<unsigned>(sinda,dlugosc_logow);
if(!fpom) goto DO_OBSLUGI_BLEDOW;
int fifautoagents=Sources.insert(fpom);


// WYŚWIETLACZ HISTORII LICZEBNOŚCI
graph* pom1=new sequence_graph(
                               //szer_map,300,szer_map+300,400, //z wycięciem na male kwadratowe okienko
                               szer_map,300,MyAreaMenager.getwidth()-1,400, //równolegle do okna liczby taksonów
							   3,Sources.make_series_info(
										inCurrLight,fifagents,fifautoagents,
										-1
										).get_ptr_val(),
							  // 1/*Wspólne minimum/maximum*/);
                                 0/*reskalowanie*/);
if(!pom1) goto DO_OBSLUGI_BLEDOW;
pom1->setframe(128);
pom1->settitle(lang("AGENCI - HISTORIA LICZEBNOŚCI","History of the number of agents"));
//pom1->setdatacolors()
MyAreaMenager.insert(pom1);

// Historia siły
//szer_map+301,300,MyAreaMenager.getwidth()-1,400,

// Historia liczby taksonów
pom1=new sequence_graph(szer_map,401,MyAreaMenager.getwidth()-1,MyAreaMenager.getheight()-1,
							   6,Sources.make_series_info(
										inCurrLight,
										fiftaxa,fifklona,
										fiftax,fifklon,
													-1
										).get_ptr_val(),
							   1/*Wspolne minimum/maximum*/);
if(!pom1) goto DO_OBSLUGI_BLEDOW;
pom1->setframe(128);
pom1->settitle(lang("HISTORIA LICZBY TAKSONÓW","History of the number of taxa"));
MyAreaMenager.insert(pom1);


// WIZUALIZACJA FILTRU ŻYCIA
pom=new carpet_graph(MyAreaMenager.getwidth()-80,0,MyAreaMenager.getwidth()-1,20, //domyślne współrzędne
						filtrZycia); //I źródło danych
if(!pom) goto DO_OBSLUGI_BLEDOW;
pom->setdatacolors(50,254); //Potrzebne tylko dwa kolory tak naprawdę
pom->setframe(32);
pom->setbackground(default_half_gray);
pom->settitlecolo(255,default_transparent);
pom->settitle(lang("ZYWI AGENCI","ALIVE AGENTS"));
MyAreaMenager.insert(pom);

//UDOSTĘPNIANIE I WYŚWIETLANIE BACKGROUNDU SYMULACJI
rectangle_source_base* Background=zdatnosc.make_source(lang("przydatność terenu","suitability of areas"));
if(!Background) goto DO_OBSLUGI_BLEDOW;
Sources.insert(Background);
Background->setminmax(0,255); //255 odcieni (szarości) - czarne niezdatne
pom=new carpet_graph(MyAreaMenager.getwidth()-80,21,MyAreaMenager.getwidth()-1,42, //domyślne współrzędne
					 Background); //I źródło danych

if(!pom) goto DO_OBSLUGI_BLEDOW;
pom->setdatacolors(0,255); //Pierwsze 25 kolorów będzie słabo widoczne
pom->setframe(64);
pom->setbackground(default_half_gray);
pom->settitlecolo(255,default_transparent);
pom->settitle(lang("Mapa zdatnych obszarów","Map of useful/unusable areas"));
MyAreaMenager.insert(pom);

//Demo interakcji AND
pom=new carpet_graph(MyAreaMenager.getwidth()-80,43,MyAreaMenager.getwidth()-1,64, //domyślne współrzędne
						&AndDemo); //I źródło danych
if(!pom) goto DO_OBSLUGI_BLEDOW;
//pom->setdatacolors(0,255); //Potrzebne tylko dwa kolory naprawdę
pom->setframe(64);
pom->settitle(lang("AND(Trofia,Oslona)","AND(Trophy,Defense)"));
MyAreaMenager.insert(pom);

//Demo exploatacji potenclalnej (AND*K)
pom=new carpet_graph(MyAreaMenager.getwidth()-80,65,MyAreaMenager.getwidth()-1,86, //domyślne współrzędne
						&ExpDemo); //I źródło danych
if(!pom) goto DO_OBSLUGI_BLEDOW;
//pom->setdatacolors(0,255); //Potrzebne tylko dwa kolory naprawdę
pom->setframe(64);
pom->settitle(lang("Eksploatacja potencjalna","exploitation potential"));
MyAreaMenager.insert(pom);

//Wiek
pom=new carpet_graph(MyAreaMenager.getwidth()-80,87,MyAreaMenager.getwidth()-1,87+21, //domyślne współrzędne
						Wiek); //I źródło danych
pom->setdatacolors(0,255); //Pierwsze 25 kolorów będzie słabowidoczne
pom->setframe(32);
pom->setbackground(default_half_gray);
pom->settitlecolo(255,default_transparent);
pom->settitle(lang("WIEK AGENTÓW","AGE OF AGENTS"));
MyAreaMenager.insert(pom);

// OKNO HISTORII SIŁY
//dalej zakładamy, że SilaStat nie umieszcza sama pod-zrodel w MyAreaMenagerze danych,
//ale łatwo sprawić, by to założenie nie było prawdziwe (wystarczy wywolać)
//SilaStat->link_source_MyAreaMenager(Sources);
fifo_source<double>* dpom=new fifo_source<double>(SilaStat->Mean(),dlugosc_logow); //Fifo dla średniej siły
if(!dpom) goto DO_OBSLUGI_BLEDOW;
int MeanSilaFifoIndex=Sources.insert(dpom);

dpom=new fifo_source<double>(SilaStat->Max(),dlugosc_logow);
if(!dpom) goto DO_OBSLUGI_BLEDOW;
int MaxSilaFifoIndex=Sources.insert(dpom);

dpom=new fifo_source<double>(SilaStat->SD(),dlugosc_logow);
if(!dpom) goto DO_OBSLUGI_BLEDOW;
int SdSilaFifoIndex=Sources.insert(dpom);

pom1=new sequence_graph(//szer_map,300,szer_map+300,400,
                        MyAreaMenager.getwidth()-80,87+21+1,MyAreaMenager.getwidth()-1,87+21+21, //domyślne współrzędne
								4,Sources.make_series_info(
								inCurrLight,MeanSilaFifoIndex,SdSilaFifoIndex,MaxSilaFifoIndex,
													-1
										).get_ptr_val(),
								0/* Z reskalowaniem */);
							   //1/*Wspolne minimum/maximum*/);
if(!pom1) goto DO_OBSLUGI_BLEDOW;
pom1->setframe(128);
pom1->settitle(lang("HISTORIA ENERGII AGENTÓW","HISTORY OF AGENTS ENERGY"));
MyAreaMenager.insert(pom1);

// Lokalne przepływy energii
pom=new carpet_graph(MyAreaMenager.getwidth()-80,87+21+1+21,MyAreaMenager.getwidth()-1,87+21+1+21+21,
						EDynamism); //I źródło danych
pom->setdatacolors(0,255); //Pierwsze 25 kolorów będzie słabo widoczne
//pom->setframe(32);
pom->setbackground(default_half_gray);
pom->settitlecolo(255,default_transparent);
pom->settitle(lang("LOKALNY PRZEPŁYW ENERGII","LOCAL ENERGY FLOW"));
MyAreaMenager.insert(pom);

// Dywanowa mapa używanych nisz ekologicznych
// ///////////////////////////////////////////////////
Liczniki=new matrix_source<unsigned>(lang("liczebności taksonów","abundance of taxa"),
                                     256,256,
									 agent::liczniki,
									 //NAZWA SERI
									 1//Nie torus
									 );
if(!Liczniki) goto DO_OBSLUGI_BLEDOW;
Sources.insert(Liczniki);

log_1_plus_F_filter<matrix_source<unsigned> >* fl=new log_1_plus_F_filter<matrix_source<unsigned> >(Liczniki);
if(!fl) goto DO_OBSLUGI_BLEDOW;
//fl->set_missing(0);

//pom=new fast_carpet_graph< log_1_plus_F_filter<matrix_source<unsigned> > >
pom=new carpet_graph(MyAreaMenager.getwidth()-80,87+21+1+21+21+21,MyAreaMenager.getwidth()-1,87+21+1+21+21+21+21, //domyślne współrzędne
                     //MyAreaMenager.getwidth()-80,87+21+1+21+21,MyAreaMenager.getwidth()-1,87+21+1+21+21+21,
						fl,1); //I źródło danych
if(!pom) goto DO_OBSLUGI_BLEDOW;

//pom->setbackground(0);
pom->setdatacolors(0,255);
pom->settitlecolo(255,default_transparent);
pom->setbackground(default_dark_gray);
pom->settitle(lang("MAPA UŻYWANYCH NISZ EKOLOGICZNYCH","USAGE OF ECOLOGICAL NICHES"));
MyAreaMenager.insert(pom);

//  Wiek filogenetyczny klonów
// //////////////////////////////////////////////////
pom1=new carpet_graph(0,0,szer_map-1,wys_map-1, //domyślne współrzędne
					  //MyAreaMenager.getwidth()-80,87+21+1+21+21,MyAreaMenager.getwidth()-1,87+21+1+21+21+21,
						WiekFilogZ); //I źródło danych
pom1->setdatacolors(0,255); //Pierwsze 25 kolorów będzie słabo widoczne
pom1->setbackground(default_half_gray);
pom1->settitlecolo(255,default_transparent);
pom1->settitle(lang("FILOGENETYCZNY WIEK AGENTÓW","PHYLOGENETIC AGE OF AGENTS"));
MyAreaMenager.insert(pom1);

// Okno historii liczby taksonów od początku
// ////////////////////////////////////////////////
pom1=new sequence_graph(//szer_map+301,300,MyAreaMenager.getwidth()-1,400, //domyślne współrzędne
						//MyAreaMenager.getwidth()-80,199,MyAreaMenager.getwidth()-1,299, //nowe domyślne współrzędne
						MyAreaMenager.getwidth()-80,87+21+1+21+21+21+21,MyAreaMenager.getwidth()-1,87+21+1+21+21+21+21+21, //domyślne współrzędne
								2,Sources.make_series_info(
									inAllclons,inAlltax,
													-1
										).get_ptr_val(),
								0/* Z reskalowaniem */);
							   //1/*Wspólne minimum/maximum*/);
if(!pom1) goto DO_OBSLUGI_BLEDOW;
pom1->setframe(32);
pom1->settitle(lang("PRZYROST DRZEWA FILOGENETYCZNEGO","GROWTH OF PHYLOGENETIC TREE"));
MyAreaMenager.insert(pom1);

// HISTOGRAM SPECJALIZACJI AUTOTROFÓW
// //////////////////////////////////////////////////////////////
/*	discrete_histogram_source(
		int    LowestClass,     // Najniższa klasa.
		size_t HowManyClass,    // Ile klas od niej.
		DATA_SOURCE* ini=NULL,  // Klasa źródłowa.
								// Jeśli nie pokrywa się z minX-maxX, to faktycznie liczony jest wycinek.
		const char* format="DISCR.DISTRIBUTION(%s[%d..%d])",
		sources_menager_base* MyMenager=NULL,
		size_t table_size=11	//BEZ ZAPASU
		):
		*/
generic_discrete_histogram_source*  histspecjA=
		new generic_discrete_histogram_source(
				1,16,
				SpecjalizacjaA,
				"histogram(%s[%d..%d])", //format
				&Sources,
				17//table_size BEZ ZAPASU?*/
			);
if(!histspecjA) goto DO_OBSLUGI_BLEDOW;
Sources.insert(histspecjA);
pom=new bars_graph(//szer_map+1,130,szer_map+119,130+84, //domyślne współrzędne
					MyAreaMenager.getwidth()-80,87+21+1+21+21+21+21+21,MyAreaMenager.getwidth()-1,87+21+1+21+21+21+21+21+21, //domyślne współrzędne
				   histspecjA);
pom->setdatacolors(200,250); //max kolor to kolor ostatniego słupka
pom->setframe(232);
pom->settitle(lang("SPECJALIZACJA AUTOTROFÓW","SPECIALISATION OF AUTOTROPHS"));
MyAreaMenager.insert(pom);

// HISTOGRAM SPECJALIZACJI HETEROTROFÓW
// ///////////////////////////////////////////////////////////////
generic_discrete_histogram_source*  histspecjH=
		new generic_discrete_histogram_source(
				1,16,
				SpecjalizacjaH,
				"histogram(%s[%d..%d])", //format
				&Sources,
				17 //table_size BEZ ZAPASU?
			);
if(!histspecjH) goto DO_OBSLUGI_BLEDOW;
Sources.insert(histspecjH);
pom=new bars_graph(//szer_map+1,130+85,szer_map+119,299, //domyślne współrzędne
				   MyAreaMenager.getwidth()-80,87+21+21+1+21+21+21+21+21,MyAreaMenager.getwidth()-1,87+21+1+21+21+21+21+21+21+21, //domyślne współrzędne
					histspecjH);
pom->setdatacolors(0,64); //max kolor to kolor ostatniego słupka
pom->setframe(32);
pom->settitle(lang("SPECJALIZACJA HETEROTROFÓW","SPECIALISATION OF HETEROTROPHS"));
MyAreaMenager.insert(pom);

// FILOGENEZA TAKSONÓW -  MOŻE BYĆ KOSZTOWNE W RYSOWANIU
// ////////////////////////////////////////////////////////////////////////
linear_source_base* Tm=filogeneza.NodeTime();
Sources.insert(Tm);
linear_source_base* Sp=filogeneza.NodeSpread();
Sources.insert(Sp);
linear_source_base* Ls=filogeneza.LineStarts();
Sources.insert(Ls);
linear_source_base* Le=filogeneza.LineEnds();
Sources.insert(Le);
linear_source_base* Lw=filogeneza.LineWeights();
Sources.insert(Lw);
//Drzewo filogenetyczne
pom=FilogeneticTree=new net_graph(szer_map+170,130,MyAreaMenager.getwidth()-81,299, //domyślne współrzędne
				  Tm,0,
				  Sp,0,
                  Ls,0,
				  Le,0,
				  Lw,0, //Kolory linii jako punktów. Powoduje narysowanie skali barwnej, ale działa tylko dlatego, że domyślna figura jest NULL!
                  NULL,0, //Rozmiary punktów faktycznie nie są potrzebne.
                  NULL,0, //I rozmiary grotów też nie.
				  Lw,0
				  );
pom->setframe(128);
pom->setbackground(default_light_gray);
wb_pchar buf(1024);
buf.prn(lang("FILOGENEZA - taksony powyżej %d agentów",
			 "PHYLOGENETIC TREE - taxa larger than %d individuals"),REJESTROWANY_ROZMIAR_TAKSONU-1);
pom->settitle(buf.get());
MyAreaMenager.insert(pom);

// Sieciowa mapa zależności ekologicznych
// //////////////////////////////////////////////////////////////
linear_source_base* X=trophNet.NodeX();
Sources.insert(X);
linear_source_base* Y=trophNet.NodeY();
Sources.insert(Y);
linear_source_base* W=trophNet.NodeWeight();
Sources.insert(W);
linear_source_base* Prey=trophNet.ConnPrey();
Sources.insert(Prey);
linear_source_base* Pred=trophNet.ConnPred();
Sources.insert(Pred);
linear_source_base* CW=trophNet.ConnWeight();
Sources.insert(CW);
data_source_base*  Dummy=new function_source< constans< 5 > >(1000000,0,1000000,"const=5",0,10);
Sources.insert(Dummy/*,1*/);
//Sieć zalezności pokarmowych
pom=TrophicNet=new net_graph(szer_map+1,130,szer_map+169,299,
					  X,0,
					  Y,0,
					  Prey,0,
					  Pred,0,

					  W,0,
					  W,0,
					  Dummy,0,
					  CW,0,
					  new circle_point,1
					  );
pom->setframe(128);
//pom->setdatacolors(0,128);
pom->setbackground(default_light_gray);
pom->settitle(lang("SIEC TROFICZNA","TROPHIC WEB"));
MyAreaMenager.insert(pom);

//szer_map+1,130,szer_map+119,130+84
//szer_map+1,130+85,szer_map+119,299

// INFORMACJE STATYSTYCZNE I STATUSOWE
// ///////////////////////////////////////////////////
OutArea=new text_area(
                      szer_map+1,0,MyAreaMenager.getwidth()-81,87+21+21, //domyślne współrzędne
					  lang("Informacje","Information_area"),
					  default_white,default_black,128,40);
if(!OutArea) goto DO_OBSLUGI_BLEDOW;
OutArea->settitle("STATUS:");
int out_area=MyAreaMenager.insert(OutArea);


// USTALANIE KOLUMN DO WYDRUKU
log.insert(sca); //Licznik kroków == wersji
log.insert(sind); //liczba indywiduów
log.insert(sinda); //liczba autotrofów
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
log.insert(OslonaStat->Min());
log.insert(OslonaStat->Max());
log.insert(GebaStat->Min());
log.insert(GebaStat->Max());
log.insert(histspecjA->WhichMain());
log.insert(histspecjA->MainClass());
log.insert(histspecjH->WhichMain());
log.insert(histspecjH->MainClass());
for(int ka=0;ka<BITS_PER_GENOM;ka++)
    log.insert(histspecjA->Class(ka));
for(int kh=0;kh<BITS_PER_GENOM;kh++)
    log.insert(histspecjH->Class(kh));


// Ostatnie przygotowania do wyświetlania
MyAreaMenager.maximize(out_area);
return;
}

}

/*
unsigned MINIMALNY_ROZMIAR_TAKSONU=100;  //Arbitralny rozmiar klonu, żeby go uznać za zarejestrowany takson
unsigned REJESTROWANY_ROZMIAR_TAKSONU=MINIMALNY_ROZMIAR_TAKSONU; //Domyślna wartość  kasowania w wizualizacji drzewa
enum Kolorowanie {K_SPECJALIZACJA=1,K_TROFIA=2,K_OBRONA=3}      */
void swiat::ZmienKoloryDrzewaFil(klad::Kolorowanie co)
{
	filogeneza.ChangeLineWeightsSource(co);
}

void swiat::UstalDetalicznoscSieci(int Ile,bool Relative)
{
	this->trophNet.set_tax_size_tres(Ile); //Relative jest ignorowane. Na razie?
}

void swiat::ZapiszFilogeneze(ostream& out,unsigned min_time,unsigned max_time,unsigned size_tres)
{
	//Tu być może trzeba cos dodać, ale na razie tak - bez kontroli
	//...
	//Informacje nagłówkowe
	out<<lang("ISTOTNIEJSZE LINIE FILOGENETYCZNE","IMPORTANT PHYLOGENETIC LINES");
	if(size_tres>0)
		out<<" larger than "<<size_tres<<" agents,";
	out<<" from "<<min_time<<"-th step";
	if(max_time>min_time)
		out<<" to "<<max_time<<" step";
		else
		{
			max_time=0;
			out<<" to global \"end of life\" in "<<this->daj_kroki_monte_carlo()<<"-th step";
		}
	out<<endl<<lang("PARAMETRY MODELU:","MODEL PARAMETERS:")<<endl;
	print_params(out);
	out<<endl;
	filogeneza.ZapiszTxt(out,min_time,max_time,size_tres);
}

void swiat::ZapiszEkologieNET(ostream& out,unsigned size_tres,double weight_tres)
{/*    Nie wiem czy to bezpieczne...
	if(trophNet.tax_size_tres()!=size_tres && trophNet.connection_tres()!=weight_tres)
	{
		this->trophNet.aktualizuj_liste_wezlow();
	}*/
	this->trophNet.ZapiszWFormacieNET(out,size_tres,weight_tres);
}

void swiat::ZapiszEkologieVNA(ostream& out,unsigned size_tres,double weight_tres)
{
	this->trophNet.ZapiszWFormacieVNA(out,size_tres,weight_tres);
}
/*
void swiat::ZapiszEkologie(ostream& out,unsigned size_tres=0);
*/

/*  IMPLEMENTACJA CZĘŚCI NIEZALEŻNEJ OD PLATFORMY  */
/* *********************************************** */

// DEFINICJA KONSTRUKTORA
swiat::swiat(size_t szerokosc,char* logname,char* mappname,my_area_menager& AreaMenager):
	MyAreaMenager(AreaMenager),
	Sources(255),
	zdatnosc(szerokosc,(szerokosc/2),128/*INICJALIZACJA*/),
	ziemia(szerokosc,(szerokosc/2)/* DOMYŚLNA INICJALIZACJA KONSTRUKTOREM*/), //Prostokąt np 20x10
	stats(szerokosc,(szerokosc/2)),
	DLUG_WIERSZA(szerokosc),
	OutArea(NULL),		    //Obszar statusu
	FilogeneticTree(NULL),	//Drzewo filogenetyczne
	TrophicNet(NULL),		//Sieć zależności pokarmowych
	Liczniki(NULL),
	licznik_krokow_w(0),
	monte_carlo_licz(0),
	Wiek(NULL),Sila(NULL),
	Trofia(NULL),Oslona(NULL),
	klon_auto(1),tax_auto(1),autotrofy(1),
	log(100,logname),
    filogeneza(NULL),
	trophNet(NULL,REJESTROWANY_ROZMIAR_WEZLA),
	globalneSwiatlo(1)
{ // UWAGA!!! Nie można tu jeszcze polegać na wirtualnych metodach klasy 'swiat'
if(mappname && mappname[0]!='\0') {
    cerr<<"Reading from bitmap still not ported! "<<__FILE__<<':'<<__LINE__<<endl;
    //zdatnosc.init_from_bitmap(mappname);
}
informacja_klonalna::podlacz_marker_czasu(&monte_carlo_licz);
informacja_klonalna::ustaw_maksimum_kasowania(REJESTROWANY_ROZMIAR_TAKSONU-1);
}

void swiat::inicjuj()
{
Ziemia(DLUG_WIERSZA/2,DLUG_WIERSZA/4).init(255,255,255);
filogeneza=Ziemia(DLUG_WIERSZA/2,DLUG_WIERSZA/4).klon;
trophNet=Ziemia(DLUG_WIERSZA/2,DLUG_WIERSZA/4).klon;

dumpmarker.alloc(128);
dumpmarker.prn("%lu",time(NULL));
log.GetStream()<<lang("KO-EWOLUCJA [","CO-EVOLUTION [")<<dumpmarker.get()<<"]\n";
//fprintf(log,"%ux%u\tWYP_POT=%f\tEFEKT=%f\tMAX_WIEK=%d\tPLOD=%f\tRTG=%f\tBUM=0.5^%d\n",
print_params(log.GetStream());
}

void swiat::print_params(ostream& out)
{
out<<
lang("ROZMIAR_SWIATA=\t",
	 "WORLD_SIZE=\t")<<DLUG_WIERSZA<<'x'<<DLUG_WIERSZA/2<<'\n'<<
lang("MAPA=\t",
	 "THE_MAPP=\t")<<MappName<<'\n'<<
lang("UPOSAZENIE_POTOMSTWA=\t",
	 "DOWRY_OF_OFFSPRING=\t")<<WYPOSAZENIE_POTOMSTWA<<'\n'<<
lang("EFEKTYWNOSC_AUTOTROFA=\t",
	 "AUTOTROPHS_EFFECTIVENESS=\t")<<EFEKTYWNOSC_AUTOTROFA<<'\n'<<
lang("MAX_WIEK=\t",
	 "MAX_AGE=\t")<<(255-MINIMALNY_WIEK)<<'\n'<<
lang("PLODNOSC=\t",
	 "FERTILITY=\t")<<1./NIEPLODNOSC<<'\n'<<
	 "RTG=\t"<<1./PROMIENIOWANIE<<'\n'<<
lang("PRAWDOPODOBIENSTWO_KATASTROF=\t",
	 "LIKELIHOOD_OF_DISASTERS=\t")<<WSP_KATASTROF<<(WSP_KATASTROF<=0?"(disabled)":"")<<'\n'<<
lang("EKSPLOATACJA DRAPIEŻNICZA=\t",
	 "PREDATORY EXPLOITATION=\t")<<(PIRACTWO?lang("TAK","YES"):lang("NIE","NO"))<<'\n'<<
lang("DYSTANS_RUCHU=\t",
	 "MAX_MOVE=\t")<<DYS_RUCHU<<'\n'<<      //Dystans ruchu w jednym kierunku
lang("BIT_STABILIZACJI_RUCHU=\t",
	 "DONT_MOVE_BIT=\t")<<BIT_RUCHU<<'\n'<<		//Gdy poza maską to bez możliwości utraty ruchliwości
lang("ELASTYCZNE_MIJANIE=\t",
	 "FLEXIBLE_MOVING=\t")<<(ZAMIANY?lang("TAK","YES"):lang("NIE","NO"))<<'\n'
	 <<        //Czy może przeciskać się na zajęte pola
lang("KROKI M C w jednym DUZYM KROKU=\t",
	 "MONTECARLO STEPS per BIG STEP=\t")<<MonteCarloMultiplic<<endl;
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

/* NAJWAŻNIEJSZE FUNKCJE - GŁÓWNA IDEA SYMULACJI */

// inicjacja każdego nowego indywiduum
int agent::init(base trof,base osl, unsigned isila)
{
w.w.oslona=osl;
w.w.geba=trof;
sila=isila;
wiek=MINIMALNY_WIEK;
assert(w._full>0);

// Sprawdzenie, czy osłona nie jest za dobra i czy inne parametry są OK
if(w.w.oslona==0 || !(sila!=0 && wiek!=0) )
      {
      klon->dolicz_indywiduum(); //Bo inaczej nigdy by ślad nie został (a może czasem powinien)
      _clean();
      return 0; //Nieprawidłowe geny
      }

// AGENT jest OK, zatem REJESTRACJA
// ////////////////////////////////////////////
// Wzrasta liczba indywiduów
if(klon==NULL)
{
    //Zapewnienie, że to się zdarza tylko raz na symulacje
    static int tylko_raz=0;                                                                        assert(tylko_raz==0);
    tylko_raz++;

    klon=new informacja_klonalna(NULL,w); //Klon bez 'rodzica'
}

//Operacje na licznikach klonów i taksonów
klon->dolicz_indywiduum(); //Klonalny licznik indywiduów
ile_ind++;                //Globalny licznik indywiduów

//Tablica dla aktualnych 'nisz ekologicznych'
base2 iwfull=w.w.geba*256+w.w.oslona;
liczniki[iwfull]++;                                                                          assert(*(liczniki+iwfull));

if(liczniki[iwfull]>max)
		{
		max=liczniki[iwfull];
		max_change=1;
		}

if( liczniki[iwfull]==1 ) // pierwszy przedstawiciel taksonu
		ile_tax++;       // wiec liczba klonów wzrasta

if( liczniki[iwfull]==informacja_klonalna::tresh_taksonow()+1 ) // osiągnął w górę wartość, czyli duży takson, rozwojowy!
		ile_big_tax++;

return 1;
}

// Każde zabicie agenta
int agent::kill()
{                                                                                                   assert( w._full>0 );
// DE-REJESTRACJA AGENTA
ile_ind--;
base2 wfull=w.w.geba*256+w.w.oslona; //Żeby było niezależne od 'endian'
liczniki[wfull]--;
if( liczniki[wfull]==0 )	//ostatni przedstawiciel tego taksonu
	ile_tax--;
if( liczniki[wfull]==informacja_klonalna::tresh_taksonow()) // osiagnął w dół wartość  == tresh_kasowania --> mały taxon
	ile_big_tax--;
                                                                                                     assert(w._full>0 );
// FIZYCZNA KASACJA
_clean();
return 1;
}

// inicjacja nowego jako potomka starego
int  agent::init(agent& rodzic)
{
    w._full=duplikuj(rodzic.w._full); //Tworzy nowy genotyp
    unsigned uposazenie=unsigned(rodzic.sila*WYPOSAZENIE_POTOMSTWA); //Kalkuluje uposażenie
    unsigned cena=  (ODWROCONY_KOSZT_ATAKU?(base)(~w.w.geba):w.w.geba) +
                    (ODWROCONY_KOSZT_OSLONY?(base)(~w.w.oslona):w.w.oslona) +
                    uposazenie; //I cały koszt potomka

    if( rodzic.sila<=cena )  // Rodzic nie ma sił na potomka
    {
        _clean();
        return 0; //Nie powiodło się
    }
    else
    {
        rodzic.sila-=cena; 	 // Płaci za wyprodukowanie i wyposażenie
        assert(rodzic.sila!=0);

        if(w._full!=rodzic.w._full) //Zaszła jakaś mutacja, czyli powstał nowy klon
        {
            klon=new informacja_klonalna(rodzic.klon,w); //Historia filogenezy.
                                                               // Efemeryczne i bezpotomne klony mogą byc czasem usuwane
        }
        else
        {
            klon=rodzic.klon;
        }

		return init(w.w.geba,w.w.oslona,uposazenie); // prawdziwa inicjacja i wywołanie klon->dolicz_indywiduum()
    }
}

// uśmiercenie indywiduum przez inne.
int agent::kill(agent& zabojca,unsigned& energy_flow)
{
if(zabojca.sila==0)
           return 0; //Juz niezdolny zabijac
                                        assert(w.w.oslona>0 && w._full>0);

int ret=parasit(zabojca,energy_flow); //Zubażanie. 1 - gdy zabił. Parametr 'energy_flow' daje ilość przejętej energii (?)

if(ret==1) return ret;
	else   return kill(); // Potem ofiara i tak ginie. Niewykorzystana siła/biomasa idzie w kosmos.
}

// Zubażanie agenta przez drugiego. Jeśli całkowite to ofiara ginie.
int agent::parasit(agent& zabojca,unsigned& energy_flow)
{
/* Pasożyt/zabójca dostaje pewna cześć siły  */
/* proporcjonalna do tego ile bitów osłony pasuje do jego maski ataku ("gęby") */
                                                                                                    assert(w.w.oslona!=0);
                                                                                                    assert(zabojca.w.w.geba!=0);
energy_flow=0; // Pole do celów statystycznych
//Nie można wziąć więcej niż ma ofiara
unsigned pom=unsigned(sila *
			double(w.w.oslona & zabojca.w.w.geba)/(zabojca.w.w.geba)*
			double(w.w.oslona & zabojca.w.w.geba)/(w.w.oslona)
			);
                                                                                                      assert(sila>=pom);
//To powinno byc wykluczone wcześniej, ale dla małej siły może się zniweczyć przez rachunek calkowitoliczbowy (???)
                                                                                                      //assert(pom>0);
zabojca.sila+=pom;
sila-=pom;
energy_flow=pom;

//Jednak zabójca nie powinien 'umrzeć z przejedzenia' (przekręt licznika?).
                                                                                                assert(zabojca.sila!=0);
if(sila==0) //Czy zjedzony w całości
	return kill();
	else
	return 0;
}

// Prawo czasu: wszystko się starzeje i traci energie
int agent::uplyw_czasu(float IloscSwiatla)
{
                                                                                                    assert(sila>0);
                                                                                                    assert(w.w.oslona>0);
                                                                                                    assert(w._full>0);
wiek++;	       // Normalne starzenie się

if(w.w.geba==AUTOTROF) //JEST  A U T O T R O F E M
	{
	sila+=unsigned(IloscSwiatla*EFEKTYWNOSC_AUTOTROFA-1); // bez rzutu GNU się czepia, więc tak ma być! TODO LATER
	                                                                                                   assert(sila!=0);
	}
	else
	sila--; // Metaboliczne zużycie energii

if(sila==0 || wiek==0) //Czy CAŁKIEM brak siły, lub życie się 'przekręciło' (nomen omen)
	return kill();
	else
	return 0;
}

// kopiuje genotyp z możliwą mutacją
base2 agent::duplikuj(base2 r)
{
base2 mask=RANDOM(PROMIENIOWANIE);
if(mask<=16) // Prowizorka - nieprzenośne, jeśli 'base' > 16bitowe
	{
	mask=0x1<<mask;
	r^=mask;
	}
return r;
}

struct vector2int
{
    vector2int(int ix,int iy):x(ix),y(iy){}
    int x,y;
};

void swiat::krok()
{
stats.Reinitialise(); //Czyszczenie lokalnej statystyki

if(WSP_KATASTROF!=0)
	kataklizm(); // najwyżej jeden na krok symulacji, choćby bardzo, bardzo mały


long ile= long( (double(DLUG_WIERSZA*DLUG_WIERSZA))/2.0*MonteCarloMultiplic ); // ile na krok MonteCarlo. /2.0 bo to prostokąt a nie kwadrat
licznik_krokow_w++; //Kolejny krok symulacji
monte_carlo_licz+=MonteCarloMultiplic; //Licznik kroków MonteCarlo (bezwzględna jednostka czasu)

//AktualneSwiatlo=cos(monte_carlo_licz/double(DZIELNIK_1_OKRESU_MILANKOVICA))+1.3;
if(POWER_COS_NASLONECZNIENIA>0)
	AktualneSwiatlo=pow(cos(monte_carlo_licz/double(DZIELNIK_1_OKRESU_MILANKOVICA)),POWER_COS_NASLONECZNIENIA)+1.3;
	else
	AktualneSwiatlo=1;
AktualneSwiatlo*=globalneSwiatlo;
AktualneSwiatlo*=100;

// rob krok(i) MonteCarlo
for(long i=0;i<ile;i++)
	{
	int x=RANDOM(DLUG_WIERSZA);
	int y=RANDOM(DLUG_WIERSZA/2);

	if(zdatnosc.get(x,y)==0)//jest pusto
		continue;			//obrób następnego

	if(!Ziemia(x,y).jest_zywy() )//jest martwy
		continue;		         //obrób następnego

	if(Ziemia(x,y).uplyw_czasu(AktualneSwiatlo)==1)    //Koszty czasu życia
		continue;				 //Zginał ze starości

	//Losowanie sąsiedniego miejsca
	unsigned	a=0,licznik=0;
	int			x1,y1;
	do{ //Losowanie kierunku ruchu - ??? można by trochę przyśpieszyć, ale chyba tylko trochę...
	ZNOWU:
		vector2int dxy{DYS_RUCHU - RANDOM(DYS_RUCHU * 2 + 1),
                       DYS_RUCHU - RANDOM(DYS_RUCHU * 2 + 1) };
		if(dxy.x==0 && dxy.y==0) goto ZNOWU; //Musi być jakieś przesuniecie
		if((licznik++)==8) goto JESZCZE_RAZ; //Wyskok kontynuujący pętlę zewnętrzną,
											//gdy nie można trafić w cos zdatnego
		x1=(x+dxy.x);
		y1=(y+dxy.y);
	}while(zdatnosc.get(x1,y1)==0); //Aż nie wylosujemy zdatnego pola

	if(!Ziemia(x1,y1).jest_zywy()) //Jeśli wylosowane pole jest wolne
	   {//------------------------------------------------------------
		if(RANDOM(NIEPLODNOSC)==0)  //Proponuje się rozmnażać
		{
			Ziemia(x1,y1).init(Ziemia(x,y));
		}
		else					   //lub przemieszczenie, jeśli ma zdolności ruchu
		{
			if(!(Ziemia(x,y).w.w.oslona&BIT_RUCHU))//...czyli BIT_RUCHU jest wyzerowany
			{
				ziemia.swap(x1,y1,x,y);
			}
		}
	   }
	   else // Jeśli na wylosowanym polu jest żywy - potencjalna ofiara
		   //-------------------------------------------------------------------------
	   {
		   if( Ziemia(x,y).w.w.geba!=AUTOTROF &&					  //atakujący nie jest autotrofem
			   (Ziemia(x1,y1).w.w.oslona & Ziemia(x,y).w.w.geba) != 0 )//...i może uszknać sąsiada
		   {
			   //wzor v=Ziemia(x,y).w; //Debug only
			   //wzor w=Ziemia(x1,y1).w;

			   unsigned long kogo=Ziemia(x1,y1).klon->identyfikator(); //Trzeba zapamiętać, bo obiekt x1-y1 jak i jego klon może zostać zniszczony
			   unsigned energy=0;   //Na energie podana przez procedury zjadania

			   if(PIRACTWO)
				   Ziemia(x1,y1).kill(Ziemia(x,y),energy);	//Zabicie sąsiada
			   else
				   Ziemia(x1,y1).parasit(Ziemia(x,y),energy);	//Eksploatacja sąsiada

			   if(MyAreaMenager.trace_trophic_network_enabled())
				   Ziemia(x,y).klon->dolicz_zezarcie(kogo,energy);

			   stats.get(x,y).sE+=energy;
			   stats.get(x,y).N++;
		   }
		   else //Jeśli nie może go uszczknąć to może się z nim zamienić na miejsca
		   if(ZAMIANY && !(Ziemia(x1,y1).w.w.oslona&BIT_RUCHU ))// ... o ile nie jest on 'przyrośnięty'
		   {
			  ziemia.swap(x1,y1,x,y);
		   }
	   }
	continue;	 //NASTĘPNY NAWROT PĘTLI
	JESZCZE_RAZ: //AWARYJNE WYJŚCIE
		   ;
	}


	//Koniec kroków monte-carlo po agentach
	//------------------------------------------  NIE W TYM MIEJSCU!?!?!?


	//  AKCJE DO ZROBIENIA PO KROKU SYMULACJI
	// //////////////////////////////////////////////////////////////////////////

	// Uaktualnienie informacji o frakcjach autotrofów
	klon_auto=0;
	tax_auto=0;
	autotrofy=0;
	for(unsigned i=0xffff;i>0xffff-0x100;i--) //Ile niezerowych klonów autotroficznych
	{
		unsigned pom=agent::liczniki[i];
		autotrofy+=pom;
		if(pom>0)
			klon_auto++;
		if(pom>informacja_klonalna::tresh_taksonow())
			tax_auto++;
	}

	if(Liczniki)
		Liczniki->setminmax(0,agent::max); //Oszczędza liczenia max

	Sources.new_data_version(1,1); //Oznajmia seriom, że dane się uaktualniły

	//Aktualizacja danych filogenezy i sieci troficznej
	if(MyAreaMenager.trace_trophic_network_enabled())
		trophNet.aktualizuj_liste_wezlow();
	else
		trophNet.zapomnij_liste(true);

	if(MyAreaMenager.trace_filogenetic_tree_enabled())
		filogeneza.aktualizuj_liste_zstepnych();
	else
		filogeneza.zapomnij_liste(true);
}

// Generuje rozkład para-Poisson w zakresie 0..1 o 'n' stopniach
// Nie wiem czy to dobra nazwa, pomijając że z błędem ort. TODO
// Patrz.: https://pl.wikipedia.org/wiki/Rozk%C5%82ad_Poissona
double poison(int n);

// Wygenerowanie katastrofy.
void  swiat::kataklizm(void)
{
double power;
int x,y,r;
if(WSP_KATASTROF<=0)
   return; //spontaniczne katastrofy wyłączone
x=RANDOM(DLUG_WIERSZA);
y=RANDOM(DLUG_WIERSZA/2);
power=poison(WSP_KATASTROF);						assert(power>=0 && power<=1);
r=power*DLUG_WIERSZA/2;
if(r>1)
	{
	cerr<<endl<<lang("Katastrofa w punkcie ","Disaster occurred at ")<<"("<<x<<","<<y<<") r="<<r<<endl;
	ziemia.clean_circle(x,y,r);
	}
}



// IMPLEMENTACJA WIZUALIZACJI Z UŻYCIEM MENADŻERA
//--------------------------------------------------

void  swiat::wskazniki(void)
{
//* LOSOWANIE SKAMIENIAŁOŚCI
unsigned x=RANDOM(DLUG_WIERSZA);
unsigned y=RANDOM(DLUG_WIERSZA/2);

if(this->monte_carlo_licz%LogRatio==0)      //Czy tu?????????????????????
		log.try_writing();

char bufor[2048]; // Bufor jest co prawda ze sporym zapasem
bufor[2047]='A';  // STRAŻNIK. Może zostać albo 'A', albo NULL, ale jak coś innego to przepełniony.

sprintf(bufor,lang("%ld KROK MONTE CARLO [%lu - symulacji]   PID=%lu\n"
							"LICZBA AGENTÓW:%lu "
							"(%lu AUTOTROFICZNYCH)\n"
							"LICZBA EKO-GATUNKÓW:%lu "
							"(%lu AUTOTROFICZNYCH)\n"
							"LICZBA KLONOW:%lu "
							"(%lu AUTOTROFICZNYCH)\n"
							"ILE KLONÓW OD POCZĄTKU:%lu "
							"(%lu ISTOTNYCH)",
				   "%ld MONTE CARLO STEP [%lu of simulation] PID=%lu\n"
							"NUMBER OF AGENTS:%lu "
							"(%lu AUTOTROPHIC)\n"
							"NUMBER OF EKO-SPECIES:%lu "
							"(%lu AUTOTROPHIC)\n"
							"NUMBER OF CLONES:%lu "
							"(%lu AUTOTROPHIC)\n"
							"NUMBER OF CLONES FROM THE BEG.:%lu "
							"(%lu IMPORTANT)"
				   )
							,
	//agent::nazwy[agent::plot_mode],
	(unsigned long)this->daj_kroki_monte_carlo(),
	(unsigned long)this->daj_licznik_krokow(),
	(unsigned long)MyGetPid(),
	(unsigned long)agent::ile_ind,
	(unsigned long)autotrofy,
	(unsigned long)agent::ile_big_tax,
	(unsigned long)tax_auto,
	(unsigned long)agent::ile_tax,
	(unsigned long)klon_auto,
    (unsigned long)informacja_klonalna::ile_klonow(),
    (unsigned long)informacja_klonalna::ile_taksonow()
    );
																          assert(bufor[2047]=='A' || bufor[2047]=='\0');

if(OutArea)
	{
	OutArea->clean(); //Stare linie usuwamy i mamy czyste okno
	OutArea->add_text(bufor);
	}

}

/**  OGÓLNA FUNKCJA MAIN   */
/* *********************** */
int parse_options(const int argc,const char* argv[]);

int main(const int argc,const char* argv[])
{
cout<<lang( "CO-EWOLUCJA: program symulujący kooewolucję wielu gatunków (do 65000)\n",
			"CO-EVOlution: this program simulate co-evolution of many species (up to 65000)\n");
cout<<ProgramName;
cout<<( "\nCompilation: " __DATE__ " " __TIME__  )<<endl;

if((sizeof(base)*2)!=sizeof(base2))
	{
	cerr<<"Niewłaściwe rozmiary dla typów bazowych: "<< 2*sizeof(base) << " != " << sizeof(base2)  <<endl;
	exit(1);
	}

printf(lang("\nROZMIAR GENOMU: %uB\nLICZBA MOŻLIWYCH KLONÓW=%lu\nMAXINT=%d\n",
			"\nSIZE OF GENOM: %uB\nPOSSIBLE CLONES=%lu\nMAXINT=%d\n"),sizeof(bity_wzoru),(unsigned long)MAXBASE2,INT_MAX);

if(My_Rand_seed==0)
	{RANDOMIZE();}
	else
	{SRAND(My_Rand_seed);}

if(!parse_options(argc,argv))
		exit(1);

my_area_menager Lufciki(255,SWIDTH,SHEIGHT,28); //255 możliwych lufcików
swiat& tenSwiat=*new swiat(IBOKSWIATA,LogName,MappName,Lufciki);
if(&tenSwiat==NULL)
{
	cerr<<lang("Brak pamięci!\n","Not enough memory!\n")<<endl;
	exit(1);
}
Lufciki.connect(&tenSwiat);

if(!Lufciki.start(ProgramName,argc,argv,1/*Buffered*/))
{
	cerr<<lang("Nie mogę wystartować grafiki","Can't initialize graphics")<<endl;
    exit(1);
}

tenSwiat.inicjuj();
tenSwiat.tworz_lufciki();

// Utworzenie sensownej nazwy pliku(-ów) do zrzutów ekranu
{
wb_pchar buf(strlen(SCREENDUMPNAME)+20);
buf.prn("%s_%s",SCREENDUMPNAME,tenSwiat.dumpmarker.get());
Lufciki.set_dump_name(buf.get());
printf(lang("\nPID procesu: %u Marker zrzutów ekranu: %lu",
                   "\nProcess PID: %u Dump files marker: %lu"),MyGetPid(),tenSwiat.dumpmarker.get());
}

// zamiast Lufciki.run_input_loop();
cerr<<endl;

while(tenSwiat.daj_kroki_monte_carlo()<MAX_ITERATIONS) //Do założonego koÃ±ca
	{
	wb_cpu_clock fulltimer;
	tenSwiat.wskazniki(); //Aktualizacja informacji
	Lufciki._replot(); //Odrysowanie
	Lufciki.flush();
	Lufciki.reset_dump_number(tenSwiat.daj_kroki_monte_carlo()		);

    //  Obsługa zdarzeń zewnętrznych
    // ///////////////////////////////
	Lufciki.process_input();

	if(!Lufciki.should_continue())
				break; //Albo koniec zabawy wymuszony "klikiem"

	wb_cpu_clock steptimer;
	tenSwiat.krok();  //Kolejny krok symulacji
	double czas_kroku=steptimer;

	 // Ciągłe zrzuty plików, jeśli jest potrzeba.
	 if(DumpScreenContinously)  //Obrazek ekranu
	 {
		Lufciki.dump_screen();
	 }
	 if(DumpNETContinously)	 //stan sieci troficznej w formacie NET
	 {
		wb_pchar bufor(1024);
		bufor.prn("%s_%07dMC.net",Lufciki.get_dump_name(),tenSwiat.daj_kroki_monte_carlo());
		ofstream netstateout(bufor.get());
		tenSwiat.ZapiszEkologieNET(netstateout); //Bez ograniczeń, tzn. z domyślnymi
		netstateout.close();
	 }
	 if(DumpVNAContinously)	 //stan sieci troficznej w formacie VNA
	 {
		wb_pchar bufor(1024);
		bufor.prn("%s_%07dMC.vna",Lufciki.get_dump_name(),tenSwiat.daj_kroki_monte_carlo());
		ofstream netstateout(bufor.get());
		tenSwiat.ZapiszEkologieVNA(netstateout); //Bez ograniczeń, tzn. z domyślnymi
		netstateout.close();
	 }

	//OSTATECZNIE POMIAR CZASU OBROTU PĘTLI I NOWA PĘTLA
		cout<<'\r'<<'\t'<<tenSwiat.daj_kroki_monte_carlo()<<"MC "<<tenSwiat.daj_licznik_krokow()
			<<lang(" krok. Czas:\t","-th step. Time:\t")<<double(fulltimer)
			<<lang("s. w tym symul.:\t","s. but simulation:\t")<<double(czas_kroku)<<" s.  ";
	}

//   Ostateczne zrzuty plików do analizy
// //////////////////////////////////////////

//Pełna filogeneza
ofstream filogout("lastfilog.out");
tenSwiat.ZapiszFilogeneze(filogout); //Bez ograniczeń, tzn. z domyślnymi
filogout.close();

//Ostatni stan sieci troficznej
ofstream netstateout("lasttrophnet.net");
tenSwiat.ZapiszEkologieNET(netstateout); //Bez ograniczeń, tzn. z domyślnymi
netstateout.close();
netstateout.open("lasttrophnet.vna");
tenSwiat.ZapiszEkologieVNA(netstateout); //Bez ograniczeń, tzn. z domyślnymi
netstateout.close();

//De-alokacja świata wraz ze wszystkimi składowymi
cerr<<endl;
delete &tenSwiat;
printf("%s",lang("Bye, bye!","Do widzenia!!!\n"));
return 0;
}

// Generuje bardzo skośny rozkład w zakresie 0..1 o n "stopniach" skośności.
// Nazwa prawdopodobnie myląca i z błędem ort. TODO!
double poison(int n)
{
double pom=1;
for(int i=1;i<=n;i++)
	pom*= DRAND() ; // Mnożenie przez wartość od 0..1
														assert(pom>=0 && pom<=1);
return pom;
}


/* STATIC STATISTICS */
unsigned agent::max=0; // Jaki jest największy takson
unsigned agent::max_change=0; // Czy ostatnio max się zmienił?
unsigned agent::ile_ind=0; // ile jest żywych indywiduów
unsigned agent::ile_tax=0; // ile taksonów niezerowych
unsigned agent::ile_big_tax=0; // ile taksonów liczniejszych niż informacja_klonalna::tresh_taksonow()
const /*unsigned long*/size_t TAXNUMBER=(unsigned long)MAXBASE2+1;
unsigned agent::liczniki[ /*TAXNUMBER*/MAXBASE2+1 ]; // Liczniki liczebności taksonów
								//Musi mieć możliwość posiadania 0xffff+1 elementów

void koszty()
{
	ofstream tabkoszt(lang("table_of_costs.out","tabela_kosztow.out"));
    for(int i=0;i<=255;i++)
    {
		base mask=i;
        unsigned cost=(base)(~mask);
        tabkoszt<<unsigned(mask)<<'\t'<<unsigned(cost)/*<<hex<<'\t'<<unsigned(cost)<<dec*/<<endl;
    }
}



// Reimplementacja start() żeby obsługiwać wyspecjalizowane menu
// Zwraca 1 jeśli   ok
int my_area_menager::start(const char* wintitle,int argc,const char** argv,int double_buffering)
{
	int ret=main_area_menager::start(wintitle,argc,argv,double_buffering);
	if(ret==1)
	{
		ssh_menu_handle menu=ssh_main_menu();
		ssh_menu_mark_item(menu,trace_fillog_tree,ID_VIEWOPT_TRACETREE);
		ssh_menu_mark_item(menu,trace_trophi_netw,ID_VIEWOPT_TRACETROPHICNET);
		ssh_menu_mark_item(menu,1,ID_VIEWOPT_TREESPEC);
		ssh_menu_mark_item(menu,!UZYWAJ_PELNE_PROM,ID_LOWMUTATIONS);
		ssh_realize_menu(menu);
	}
	return ret;
}

//Przed-obsługa domyślna. Zwraca 1, jeśli zdołał obsłużyć.
int my_area_menager::_pre_process_input(int input_char)
{
										                                                        assert(MojSwiat!=NULL);
static bool firs_change_viewfreq=true;	// Do ostrzeżenia przy pierwszej
								        // zmianie częstotliwości wizualizacji
	switch(input_char){
	case ID_VIEWOPT_DUMPCO:     // Stałe zrzucanie grafiki
		{
			DumpScreenContinously=!DumpScreenContinously;
			ssh_menu_handle menu=ssh_main_menu();
			ssh_menu_mark_item(menu,DumpScreenContinously,ID_VIEWOPT_DUMPCO);
			ssh_realize_menu(menu);
		}
		return 1;
	case ID_VIEWOPT_DMPCNET:	//Stałe zrzucanie sieci troficznej
		{
			DumpNETContinously=!DumpNETContinously;
			ssh_menu_handle menu=ssh_main_menu();
			ssh_menu_mark_item(menu,DumpNETContinously,ID_VIEWOPT_DMPCNET);
			ssh_realize_menu(menu);
		}
		return 1;
	case ID_VIEWOPT_LESSOFT:
		{
			switch(MonteCarloMultiplic)
			{
			case 0:MonteCarloMultiplic=1;break;
			case 1:MonteCarloMultiplic=2;break;
			case 2:MonteCarloMultiplic=10;break;
			case 10:MonteCarloMultiplic=100;break;
			default:
			MonteCarloMultiplic*=2;break;
			}
			cerr<<endl
                <<lang("Częstość wizualizacji zmieniona na ","Visualisation frequency changed into ")<<MonteCarloMultiplic<<endl;
			if(firs_change_viewfreq)
				{cerr<<lang("WIZUALIZACJA SERII MOŻE BYC NIESPÓJNA!!!","TIME SERIES VISUALISATION MAY BE INCONSISTENT!!!")<<endl;
				firs_change_viewfreq=false;}
		}
		return 1;
	case ID_VIEWOPT_MOREOFT:
		{
			switch(MonteCarloMultiplic)
			{
			case 0:MonteCarloMultiplic=1;break;
			case 1:MonteCarloMultiplic=1;break;
			case 2:MonteCarloMultiplic=1;break;
			case 10:MonteCarloMultiplic=2;break;
			case 100:MonteCarloMultiplic=10;break;
			default:
			MonteCarloMultiplic/=2;break;
			}
			cerr<<endl
                <<lang("Częstość wizualizacji zmieniona na ","Visualisation frequency changed into ")<<MonteCarloMultiplic<<endl;
			if(firs_change_viewfreq)
				{cerr<<lang("WIZUALIZACJA SERII MOŻE BYC NIESPÓJNA!!!","TIME SERIES VISUALISATION MAY BE INCONSISTENT!!!")<<endl;
				firs_change_viewfreq=false;}
		}
		return 1;
	case ID_VIEWOPT_DMPCVNA:      	//Stałe zrzucanie sieci troficznej w formacie VNA
		{
			DumpVNAContinously=!DumpVNAContinously;
			ssh_menu_handle menu=ssh_main_menu();
			ssh_menu_mark_item(menu,DumpVNAContinously,ID_VIEWOPT_DMPCVNA);
			ssh_realize_menu(menu);
		}
		return 1;
	case ID_VIEWOPT_TRACETREE:
		{
			trace_fillog_tree=!trace_fillog_tree;
			ssh_menu_handle menu=ssh_main_menu();
			ssh_menu_mark_item(menu,trace_fillog_tree,ID_VIEWOPT_TRACETREE);
			ssh_realize_menu(menu);
			this->replot();
		}
		return 1;
	case ID_VIEWOPT_TREESPEC:
		{
		 //enum Kolorowanie {K_SPECJALIZACJA=1,K_TROFIA=2,K_OBRONA=3}      */
		 MojSwiat->ZmienKoloryDrzewaFil(klad::K_SPECJALIZACJA);
		 ssh_menu_handle menu=ssh_main_menu();
		 ssh_menu_mark_item(menu,1,ID_VIEWOPT_TREESPEC);
		 ssh_menu_mark_item(menu,0,ID_VIEWOPT_TREETROPHY);
		 ssh_menu_mark_item(menu,0,ID_VIEWOPT_TREEDEFEND);
		 ssh_menu_mark_item(menu,0,ID_VIEWOPT_TREETRSPEC);
		 ssh_menu_mark_item(menu,0,ID_VIEWOPT_TREEDESPEC);
		 ssh_menu_mark_item(menu,0,ID_VIEWOPT_TREESIZE);
		 ssh_realize_menu(menu);
		 this->replot();
		}
		return 1;
	case ID_VIEWOPT_TREETROPHY:
		{
		 MojSwiat->ZmienKoloryDrzewaFil(klad::K_TROFIA);
		 ssh_menu_handle menu=ssh_main_menu();
		 ssh_menu_mark_item(menu,0,ID_VIEWOPT_TREESPEC);
		 ssh_menu_mark_item(menu,1,ID_VIEWOPT_TREETROPHY);
		 ssh_menu_mark_item(menu,0,ID_VIEWOPT_TREEDEFEND);
		 ssh_menu_mark_item(menu,0,ID_VIEWOPT_TREETRSPEC);
		 ssh_menu_mark_item(menu,0,ID_VIEWOPT_TREEDESPEC);
		 ssh_menu_mark_item(menu,0,ID_VIEWOPT_TREESIZE);
		 ssh_realize_menu(menu);
		 this->replot();
		}
		return 1;
	case ID_VIEWOPT_TREEDEFEND:
		{
		 MojSwiat->ZmienKoloryDrzewaFil(klad::K_OBRONA);
		 ssh_menu_handle menu=ssh_main_menu();
		 ssh_menu_mark_item(menu,0,ID_VIEWOPT_TREESPEC);
		 ssh_menu_mark_item(menu,0,ID_VIEWOPT_TREETROPHY);
		 ssh_menu_mark_item(menu,1,ID_VIEWOPT_TREEDEFEND);
		 ssh_menu_mark_item(menu,0,ID_VIEWOPT_TREETRSPEC);
		 ssh_menu_mark_item(menu,0,ID_VIEWOPT_TREEDESPEC);
		 ssh_menu_mark_item(menu,0,ID_VIEWOPT_TREESIZE);
		 ssh_realize_menu(menu);
		 this->replot();
		}
		return 1;
		case ID_VIEWOPT_TREETRSPEC:// " by trophic spec"
		{
		 MojSwiat->ZmienKoloryDrzewaFil(klad::K_SPECTROFIA);
		 ssh_menu_handle menu=ssh_main_menu();
		 ssh_menu_mark_item(menu,0,ID_VIEWOPT_TREESPEC);
		 ssh_menu_mark_item(menu,0,ID_VIEWOPT_TREETROPHY);
		 ssh_menu_mark_item(menu,0,ID_VIEWOPT_TREEDEFEND);
		 ssh_menu_mark_item(menu,1,ID_VIEWOPT_TREETRSPEC);
		 ssh_menu_mark_item(menu,0,ID_VIEWOPT_TREEDESPEC);
		 ssh_menu_mark_item(menu,0,ID_VIEWOPT_TREESIZE);
		 ssh_realize_menu(menu);
		 this->replot();
		}
		return 1;
		case ID_VIEWOPT_TREEDESPEC://" by defense spec"
		{
		 MojSwiat->ZmienKoloryDrzewaFil(klad::K_SPECOBRONA);
		 ssh_menu_handle menu=ssh_main_menu();
		 ssh_menu_mark_item(menu,0,ID_VIEWOPT_TREESPEC);
		 ssh_menu_mark_item(menu,0,ID_VIEWOPT_TREETROPHY);
		 ssh_menu_mark_item(menu,0,ID_VIEWOPT_TREEDEFEND);
		 ssh_menu_mark_item(menu,0,ID_VIEWOPT_TREETRSPEC);
		 ssh_menu_mark_item(menu,1,ID_VIEWOPT_TREEDESPEC);
		 ssh_menu_mark_item(menu,0,ID_VIEWOPT_TREESIZE);
		 ssh_realize_menu(menu);
		 this->replot();
		}
		return 1;
		case ID_VIEWOPT_TREESIZE://" by clad size"
		{
		 putchar('\a');
		 MojSwiat->ZmienKoloryDrzewaFil(klad::K_ILUBYLOIJEST);
		 ssh_menu_handle menu=ssh_main_menu();
		 ssh_menu_mark_item(menu,0,ID_VIEWOPT_TREESPEC);
		 ssh_menu_mark_item(menu,0,ID_VIEWOPT_TREETROPHY);
		 ssh_menu_mark_item(menu,0,ID_VIEWOPT_TREEDEFEND);
		 ssh_menu_mark_item(menu,0,ID_VIEWOPT_TREETRSPEC);
		 ssh_menu_mark_item(menu,0,ID_VIEWOPT_TREEDESPEC);
		 ssh_menu_mark_item(menu,1,ID_VIEWOPT_TREESIZE);
		 ssh_realize_menu(menu);
		 this->replot();
		}
		return 1;
	case ID_VIEWOPT_TRACETROPHICNET:
		{
		trace_trophi_netw=!trace_trophi_netw;
		ssh_menu_handle menu=ssh_main_menu();
		ssh_menu_mark_item(menu,trace_trophi_netw,ID_VIEWOPT_TRACETROPHICNET);
		ssh_realize_menu(menu);
		this->replot();
		}
		return 1;
	case ID_VIEWOPT_TRNETMORE:
		{
		  REJESTROWANY_ROZMIAR_WEZLA/=2;
		  if(REJESTROWANY_ROZMIAR_WEZLA<1)
				REJESTROWANY_ROZMIAR_WEZLA=0;
		  cerr<<endl<<lang("Granica zauważania węzłów sieci troficznej obniżona do ",
						   "Important nodes of food web are now from ")<<REJESTROWANY_ROZMIAR_WEZLA<<endl;
		  MojSwiat->UstalDetalicznoscSieci(REJESTROWANY_ROZMIAR_WEZLA);
		}
		return 1;
	case ID_VIEWOPT_TRNETLESS:
		{
		  if(REJESTROWANY_ROZMIAR_WEZLA<1)
			REJESTROWANY_ROZMIAR_WEZLA=1;
		  else
			REJESTROWANY_ROZMIAR_WEZLA*=2;
		  cerr<<endl<<lang("Granica zauważania węzłów sieci troficznej podwyższona do ",
						   "Important nodes of food web are now from ")<<REJESTROWANY_ROZMIAR_WEZLA<<endl;
		  MojSwiat->UstalDetalicznoscSieci(REJESTROWANY_ROZMIAR_WEZLA);
		}
		return 1;
//	#define ID_VIEWOPT_TRNETFULL            60012
	   case ID_VIEWOPT_TRNETFULL:
		{
		  REJESTROWANY_ROZMIAR_WEZLA=0;
		  cerr<<endl<<lang("Zniesiona granica zauważania węzłów sieci troficznej",
						   "Now all nodes of food web are important")<<endl;
		  MojSwiat->UstalDetalicznoscSieci(REJESTROWANY_ROZMIAR_WEZLA);
		}
		return 1;
//	#define ID_VIEWOPT_TRNETDEFA			60013
		case ID_VIEWOPT_TRNETDEFA:
		{
		  REJESTROWANY_ROZMIAR_WEZLA=REJESTROWANY_ROZMIAR_TAKSONU-1;
		  cerr<<endl<<lang( "Domyślna granica zauważania węzłów sieci troficznej (taka jak w drzewie fil.=",
                            "Treshold of importance for food web =")<<REJESTROWANY_ROZMIAR_WEZLA<<")"<<endl;
		  MojSwiat->UstalDetalicznoscSieci(REJESTROWANY_ROZMIAR_WEZLA);
		}
		return 1;
		//		ID_VIEWOPT_DMPNET               60014
		case  ID_VIEWOPT_DMPNET:
		{
			 //stan sieci troficznej
			wb_pchar bufor(1024);
			bufor.prn("%s_%07dMC.net",this->get_dump_name(),MojSwiat->daj_kroki_monte_carlo()	);
			ofstream netstateout(bufor.get());
			MojSwiat->ZapiszEkologieNET(netstateout); //Bez ograniczeń, tzn. z domyślnymi
			netstateout.close();
			bufor.prn("%s_%07dMC.vna",this->get_dump_name(),MojSwiat->daj_kroki_monte_carlo()); //printf
			netstateout.open(bufor.get());
			MojSwiat->ZapiszEkologieVNA(netstateout); //Bez ograniczeń, tzn. z domyślnymi
			netstateout.close();
		}
		return 1;
//ID_LOWMUTATIONS                 60015
		case  ID_LOWMUTATIONS:
		{
			UZYWAJ_PELNE_PROM=!UZYWAJ_PELNE_PROM;
			if(UZYWAJ_PELNE_PROM)
			{
				PROMIENIOWANIE/=10;
				cerr<<endl<<lang("Wsp. mutacji ponownie =1/","Mutation coefficient =1/")<<PROMIENIOWANIE<<endl;
			}
			else
			{
				PROMIENIOWANIE*=10;
				cerr<<endl<<lang("Wsp. mutacji =1/","Mutation coefficient =1/")<<PROMIENIOWANIE<<endl;
			}
			ssh_menu_handle menu=ssh_main_menu();
			ssh_menu_mark_item(menu,!UZYWAJ_PELNE_PROM,ID_LOWMUTATIONS);
			ssh_realize_menu(menu);
		}
		return 1;
	default:
		return 0;
	}
}

// Parsing parametrów wywołania programu
int parse_options(const int argc,const char* argv[])
{
const char* pom;
//koszty();
for(int i=1;i<argc;i++)
    {
	if( *argv[i]=='-' ) /* Opcja X lub symshell-a */
		continue;
	if((pom=strstr(argv[i],"WIDTHWIN="))!=NULL) //Nie NULL, czyli jest
	{
	SWIDTH=atol(pom+9);
	if(SWIDTH<50)
		{
		cerr<<lang("Za małe WIDTHWIN = ","Bad WIDTHWIN = ")<<SWIDTH<<lang(" (musi być"," (must be")<<" >50)"<<endl;
		return 0;
		}
	cerr<<"WIDTHWIN = "<<SWIDTH<<endl;
	}
	else
	if((pom=strstr(argv[i],"HEIGHTWIN="))!=NULL) //Nie NULL, czyli jest
	{
	SHEIGHT=atol(pom+10);
	if(SHEIGHT<50)
		{
		cerr<<lang("Za małe HEIGHTWIN = ","Bad HEIGHTWIN = ")<<SHEIGHT<<lang(" (musi być"," (must be")<<" >50)"<<endl;
		return 0;
		}
	cerr<<"HEIGHTWIN = "<<SHEIGHT<<endl;
	}
	else
	if((pom=strstr(argv[i],"SIDE="))!=NULL) //Nie NULL, czyli jest
	{
	IBOKSWIATA=atol(pom+4);
	if(IBOKSWIATA<20 || IBOKSWIATA>=SWIDTH)
		{
		cerr<<"SIDE="<<IBOKSWIATA<<lang(" !!! Jest za mały lub większy niż deklarowany ekran!\n",
				   "!!! It is too small or wider than screen")<<IBOKSWIATA;
		cerr<<lang("Wciśnij ENTER jeśli to akceptujesz lub przerwij program!",
				   "Press ENTER if it is acceptable or break the program");
		getchar();
		}
		else
		cerr<<lang("Długi bok (SIDE) świata ustawiony na: ",
				   "The longer side of simulation world SIDE=")<<IBOKSWIATA<<endl;
	}
	else
	//"FLEX=y/n - czy można zamieniać się z właścicielem komórki\n"
	//const unsigned	ZAMIANY=255; //Czy może przeciskać się na zajęte pola (0 na nie lub inna liczba na tak)
	if((pom=strstr(argv[i],"FLEX="))!=NULL) //Nie NULL, czyli jest
	{
	ZAMIANY=atol(pom+5);
	if(ZAMIANY==0)
		{
		if( (pom[5]=='y') || (pom[5]=='t') || (pom[5]=='Y') || (pom[5]=='T') || (pom[5]=='1') )
			ZAMIANY=255;
		}
	if (ZAMIANY>255)
		{
		cerr<<"FLEX="<<ZAMIANY<<lang("!!! Niewłaściwa wartość. Powinno być < 256",
									 "!!! Improper value, should be < 256")<<endl;
		cerr<<lang("Wciśnij ENTER jeśli to akceptujesz lub przerwij program!",
				   "Press ENTER if it is acceptable or break the program");
		getchar();
		}
	else
		cerr<<lang("Elastyczne wymijanie - ","Flexible agent moving - ")
			<<"FLEX: "<< (ZAMIANY==0 ? lang("nie","no"):lang("tak","yes") );
	}
	else
	// "RBIT=2^n - który bit odpowiada za zdolność ruchu\n"
	// const unsigned	BIT_RUCHU=1024;		// Jak poza maskę obrony to bez możliwości utraty ruchliwości
	if((pom=strstr(argv[i],"RBIT="))!=NULL) //Nie NULL, czyli jest
	{
	BIT_RUCHU=atol(pom+5);
	unsigned bit=bits(BIT_RUCHU);
	if (bit>1)
		{
		cerr<<lang("Bit ruchu (RBIT) musi być którąś potęgą liczby 2. Np. ",
				   "The move bit (RBIT) must by a power of two. E.g. ")
				   <<" 0,1,2,4,8,16,32,128..."<<endl;
		cerr<<lang("A podana wartość ma ",
				   "But given value have ")<<bit<<lang("bitów","bits")<<endl
			<<lang("Jeśli chcesz wyłączyć podaj bit spoza maski obrony",
				   "To turn off provide power larger than defence mask capability.")<<endl;
		return 0;
		}
	else
		{
		cerr<<lang("Podano wartość RBIT: ","Given value of RBIT is ")<<BIT_RUCHU<<endl;
		if(MAXBASE>BIT_RUCHU)
			cerr<<(BIT_RUCHU!=0?unsigned(log(1.0*BIT_RUCHU)/log(2.0)):0)
				<<lang(" bit maski obrony jest bitem ruchu",
					   "-th bit of defence mask is responsible on moving ability")<<endl;
		else
			cerr<<lang("Bit ruchu jest poza maską obrony - wszyscy agenci mają możliwość ruchu",
					   "The move bit is outside the defence mask - all agents are able to move")<<endl;
		}
	}
	else
	//MAXA=nnn - maksymalny wiek. Nie większy niż 255. Ujemne oznacza śmierci losowe\n"
	//const unsigned	MINIMALNY_WIEK=205;	// Rodzi się z tym wiekiem. Do śmierci ma 255-MINIMALNY_WIEK
	if((pom=strstr(argv[i],"MAXA="))!=NULL) //Nie NULL, czyli jest
	{
	unsigned MAX_WIEK=atol(pom+5);
	if(MAX_WIEK>255)
		{
		cerr<<lang("!!! Maksymalny wiek (MAXA=",
				   "!!! Maximal age of agent (MAXA=")<<MAX_WIEK
			<<lang(") nie może być większy niż 255!",
				   ") but it can not be greater than 255!")<<endl;
		return 0;
		}
		else
		{
		cerr<<lang("Maksymalny wiek (MAXA) ustawiony na ",
				   "Maximal age of agent (MAXA) set to ")<<MAX_WIEK<<endl;
		MINIMALNY_WIEK=255-MAX_WIEK;
		}
	}
	else
	// "IFER=NN - prawdopodobieństwo rozmnażania to 1/IFER\n"
	// const unsigned	NIEPLODNOSC=5;		// Prawdopodobieństwo rozmnażania jest 1/NIEPLODNOSC.
	if((pom=strstr(argv[i],"IFER="))!=NULL) //Nie NULL, czyli jest
	{
	NIEPLODNOSC=atol(pom+5);
	if(NIEPLODNOSC==0 || NIEPLODNOSC>1000)
		{
			cerr<<"!!! IFER = "<<NIEPLODNOSC
				<<lang("To musi być liczba całkowita w zakresie 1..1000!",
					"Have to be an integer in the range 1-1000!")<<endl;
		  return 0;
		}
		else
		{
		cerr<<"!!! IFER = "<<NIEPLODNOSC
			<<lang("czyli prawdopodobieństwo rozmnażania = ",
				   "ie the probability of reproduction = ")<<double(1.0/NIEPLODNOSC)<<endl;
		}
	}
	else
	if((pom=strstr(argv[i],"PRED="))!=NULL) //Nie NULL, czyli jest
	{
		PIRACTWO=(pom[5]=='Y')||(pom[5]=='1')||(pom[5]=='t');
		cerr<<"PRED="<<(PIRACTWO?lang("Tak","Yes"):lang("Nie","No"))<<" ";
		if(PIRACTWO)
		  cerr<<lang("- eksploatacja drapieżnicza","- predatory exploitation")<<endl;
		  else
		  cerr<<lang("- eksploatacja pasożytnicza","- parasitic exploitation")<<endl;
	}
	else
	//"POSA=0.XX - jaki ułamek siły  oddane potomkowi\n"
	//const double	WYPOSAZENIE_POTOMSTWA=0.05; // jaką cześć siły oddać potomkowi. Czy 0.1 to ZA DUŻO?
	if((pom=strstr(argv[i],"DOWR="))!=NULL) //Nie NULL, czyli jest
	{
	  WYPOSAZENIE_POTOMSTWA=atof(pom+5);
	  cerr<<"DOWR="<<WYPOSAZENIE_POTOMSTWA<<lang(" zapasów rodzica.","of parent reserve.")<<endl;
	  if(WYPOSAZENIE_POTOMSTWA<=0 || 1<WYPOSAZENIE_POTOMSTWA)
	  {
		 cerr<<lang("!!! LICZBA MUSI BYĆ W ZAKRESIE (0..1) !!!",
					"!!! BUT MUST BE IN RANGE (0..1) !!!")<<endl;
		 return 0;
	  }
	  else
	  if(WYPOSAZENIE_POTOMSTWA>0.1)
	  {
		 cerr<<lang("UWAGA! Wartość większa od 0.1 pozwala łatwo rozmnażać się z posagu!!!",
					"NOTE! Value greater than 0.1 allows one to easily reproduces basing on dowry!!!")<<endl;
	  }
	}
	else
	//Współczynnik mutacji na bit genomu
	if((pom=strstr(argv[i],"MUTD="))!=NULL) //Nie NULL, czyli jest
	{
		PROMIENIOWANIE=atol(pom+5);
		cerr<<"MUTD="<<PROMIENIOWANIE
			<<lang("Współczynnik mutacji wynosi 1/",
				"Mutation rate is 1/)")<<PROMIENIOWANIE<<" = "<<(1/PROMIENIOWANIE)<<endl;
		if(PROMIENIOWANIE<16) //Jeśli mniej niż długość genomu to algorytm losowania bitu zle działa!
		{
			cerr<<lang("MUTD musi być większe lub równe 16",
				   "MUTD have to be greater or equal to 16")<<endl;
			return 0;
		}
	}
	else
	if((pom=strstr(argv[i],"CATA="))!=NULL) //Nie NULL, czyli jest
	{
		WSP_KATASTROF=atol(pom+5);
		cerr<<lang("Współczynnik katastrof CATA=",
			   "Coefficient of disasters CATA=")<<WSP_KATASTROF<<endl;
		if(WSP_KATASTROF<=0)
			cerr<<lang(" - zatem katastrofy wyłączone"," - so disasters are off")<<endl;
		else
			cerr<<lang(" - zatem katastrofy możliwe"," - so disasters are possible")<<endl;
	}
	else
	if((pom=strstr(argv[i],"AUTO="))!=NULL) //Nie NULL, czyli jest
	{
		EFEKTYWNOSC_AUTOTROFA=atof(pom+5);
		cerr<<lang("Produktywność autotrofów AUTO=",
				   "Productivity of autotrophs")<<EFEKTYWNOSC_AUTOTROFA;
		if(EFEKTYWNOSC_AUTOTROFA<=0)
		{
			cerr<<lang("Nie może byc ujemna!!!",
					   "Can not be negative!!!")<<endl;
			return 0;
		}
		else
		if(EFEKTYWNOSC_AUTOTROFA>=1)
		{
		  cerr<<lang("Efektywnosc autotrofa powinna być w zakresię 0..0.99 ale spróbuje",
					 "Should be lower than 1 but we can try :-) ")<<endl;
		}
	}
	else
	if((pom=strstr(argv[i],"STMX="))!=NULL) //Nie NULL, czyli jest
	{
		MAX_ITERATIONS=atol(pom+4);
		cerr<<lang("Maksymalna liczba iteracji modelu STMX=",
				   "The maximum number of iterations of the model STMX=")<<MAX_ITERATIONS<<endl;
		if(MAX_ITERATIONS<=0)
		{
			cerr<<lang("Nie moze byc <= 0!!!","It can not be <= 0");
			return 0;
		}
	}
	else
	if((pom=strstr(argv[i],"DIST="))!=NULL) //Nie NULL, czyli jest
	{
		DYS_RUCHU=atol(pom+5);
		cerr<<lang("Maksymalny ruch agenta (DIST) ustawiony na ",
				   "Maximum movement of the agent (DIST) set to")<<DYS_RUCHU<<endl;
		if(DYS_RUCHU<=0)
		{
			cerr<<lang("Nie może być <= 0","It can not be <= 0")<<endl;
			return 0;
		}
	}
	else
	if((pom=strstr(argv[i],"LOGR="))!=NULL) //Nie NULL czyli jest
	{
		LogRatio=atol(pom+5);
		cerr<<lang("Częstość zapisu do logu (LOGR) ustawiona na ",
				   "The frequency of writing to the log (LOGR) set to ")<<LogRatio<<endl;
		if(LogRatio<=0)
		{
			cerr<<lang("Nie może być <= 0","It can not be <= 0")<<endl;
			return 0;
		}
	}
	else
	//"INLO=NNNN - długość wewnętrznych logów\n"
	// const unsigned długość_logow=50000;
	if((pom=strstr(argv[i],"INLO="))!=NULL) //Nie NULL, czyli jest
	{
		dlugosc_logow=atol(pom+5);
		cerr<<lang("Długość wewnętrznych logów (INLO) wynosi ",
				   "Length of the internal logs (INLO) is ")<<dlugosc_logow<<endl;
		if(dlugosc_logow<100)
		{
			cerr<<lang("Nie może być < 100","It can not be < 100")<<endl;
			return 0;
		}
	}
	else
	if((pom=strstr(argv[i],"LOGF="))!=NULL) //Nie NULL, czyli jest
	{
		strcpy(LogName,pom+5);
		cerr<<lang("Plik logu (LOGF) ustawiony na ",
				   "Log file name LOGF=")<<LogName<<endl;
	}
	else
	if((pom=strstr(argv[i],"MAPF="))!=NULL) //Nie NULL, czyli jest
	{
		strcpy(MappName,pom+5);
		cerr<<lang("Plik mapy (MAPF) ustawiony na ",
				   "Map file name MAPF=")<<MappName<<endl;
	}
	else
	//"DMPF=name - rdzen/początek nazwy plików dump\n"
	//char     SCREENDUMPNAME[256]="CO-EVO-3a";
	if((pom=strstr(argv[i],"DMPF="))!=NULL) //Nie NULL, czyli jest
	{
		strcpy(SCREENDUMPNAME,pom+5);
		cerr<<lang("Rdzeń (DMPF) nazwy zrzutów graficznych: ",
				   "The core (DMPF) of screen dumps name: ")<<SCREENDUMPNAME<<endl;
	}
	else
	if((pom=strstr(argv[i],"TAXS="))!=NULL) //Nie NULL, czyli jest
	{
		REJESTROWANY_ROZMIAR_TAKSONU=atol(pom+5);
		cerr<<lang("Rozmiar rejestrowanych taksonów (TAXS) ustawiono na ",
				   "Minimal size of registered taxon (TAXS) ")<<REJESTROWANY_ROZMIAR_TAKSONU<<endl;
		if(REJESTROWANY_ROZMIAR_TAKSONU<10)
		{
			cerr<<lang("!!! Ale musi być >=10 !!!","!!! But have to be >=10 !!!")<<endl;
			return 0;
		}
	}
	else /* Ostatecznie wychodzi, że nie ma takiej opcji */
	{
	cerr<<lang("Niezidentyfikowana opcja ",
				"Undefined option ")<<argv[i]<<endl;
	cerr<<"===================================================================="<<endl;
	cerr<<lang("MOŻLIWE TO: (W nawiasach wartości domyślne)",
				"POSSIBLE ONES ARE: (default values in parentheses)")<<endl;
	cerr<<lang("SIDE=NN - dłuższy bok obszaru symulacji ",
			   "SIDE=NN - the longer side of simulation world")<<'('<<IBOKSWIATA<<')'<<endl; //unsigned	IBOKSWIATA=300; // FAKTYCZNIE UŻYWANY BOK ŚWIATA
	cerr<<lang("MAPF=plik_init.gif - nazwa pliku z mapa zamieszkiwalności",
			   "MAPF=init_file.gif - name of availability map file")<<'('<<MappName<<')'<<endl; //Inicjalizacja świata. Pola czarne, miejsca niezamieszkiwalne.
	cerr<<lang("AUTO=0.XX - efektywność autotrofów ",
			   "AUTO=0.XX - productivity of autotrophs ")<<'('<<EFEKTYWNOSC_AUTOTROFA<<')'<<endl; //double	EFEKTYWNOSC_AUTOTROFA=0.5; // jaka część światła używa autotrof
	cerr<<lang("PRED=0/1 - gospodarka pasożytnicza versus drapieżnicza ",
			   "PRED=0/1 - predatory vs. parasitic exploitation ")<<'('<<PIRACTWO<<')'<<endl; //unsigned int	PIRACTWO=1; // Czy eksploatacja piracka, czy pasożytnicza
	cerr<<lang("DIST=NN - maksymalny krok ruchu agenta ",
			   "DIST=NN - maximal one step walk of agent ")<<'('<<DYS_RUCHU<<')'<<endl; //unsigned	DYS_RUCHU=1; // Dystans ruchu w jednym kierunku
	cerr<<lang("FLEX=y/n - czy można zamieniać się z właścicielem komórki ",
			   "FLEX=y/n - exchange position when agents bump")<<'('<<ZAMIANY<<')'<<endl; //const unsigned	ZAMIANY=255; //Czy może przeciskać się na zajęte pola (0 na nie lub inna liczba na tak)
	cerr<<lang("RBIT=2^n - który bit odpowiada za zdolność ruchu ",
			   "RBIT=2^n - weight of bit responsible for moving ability ")<<'('<<BIT_RUCHU<<')'<<endl; //const unsigned	BIT_RUCHU=1024;	// Poza maskę to bez możliwości utraty ruchliwości
	cerr<<lang("MUTD=NNN - co ile kopiowanych bitów trafia się mutacja ",
			   "MUTD=NNN - how often mutation occurs when copying bits ")<<"(every "<<PROMIENIOWANIE<<" bits)"<<endl; //unsigned	PROMIENIOWANIE=BITS_PER_GENOM*100; // Co ile kopiowanych bitów nastepuje mutacja
	cerr<<lang("DOWR=0.XX - jaki ułamek energii oddane potomkowi ",
			   "DOWR=0.XX - dowry for offspring as % of parent energy ")<<'('<<WYPOSAZENIE_POTOMSTWA<<')'<<endl; //const double	WYPOSAZENIE_POTOMSTWA=0.05; // jaka część siły  oddać potomkowi 0.1 to ZA DUZO!!! {???}
	cerr<<lang("IFER=NN - prawdopodobieństwo rozmnażania to. 1/IFER = 1/",
			   "IFER=NN - how often agent try to make child. 1/IFER = 1/")<<'('<<NIEPLODNOSC<<')'<<endl; //const unsigned	NIEPLODNOSC=5;		// Prawdopodobieństwo rozmnażania jest 1/NIEPLODNOSC
	cerr<<lang("MAXA=nnn - maksymalny wiek. Nie większy niż 255. ",
			   "MAXA=nnn - maximal age. Not grater than 255. ")<<'('<<255-MINIMALNY_WIEK<<')'<<endl; // Ujemne BĘDZIE oznaczać śmierci losowe"<<'('<<xxx<<')'<<endl; //const unsigned	MINIMALNY_WIEK=205;	// Rodzi się z tym wiekiem. Do smierci ma 255-MINIMALNY_WIEK
	cerr<<lang("CATA=NN - wykładnik częstości katastrof ",
			   "CATA=NN - ..............................")<<'('<<WSP_KATASTROF<<')'<<endl; //int	WSP_KATASTROF=0; //10-100 Wykładnik rozkładu katastrof - 0 -wyłączone
	cerr<<lang("TAXS=NN - minimalny rozmiar taksonu rejestrowanego w drzewie. Nie mniej niż 10! ",
			   "TAXS=NN - minimal taxon size to be registered in phylogenetic tree. No less than 10! ")<<'('<<REJESTROWANY_ROZMIAR_TAKSONU<<')'<<endl; //unsigned REJESTROWANY_ROZMIAR_TAKSONU=MINIMALNY_ROZMIAR_TAKSONU; //Domyslna wartość  kasowania w wizualizacji drzewa
					//unsigned int	ODWROCONY_KOSZT_OSLONY=0; //1 - czy koszty osłony są odwrócone
					//unsigned int	ODWROCONY_KOSZT_ATAKU=0; //1 - czy koszty ataku są odwrócone

					//"MITA=NNN - minimalny rozmiar taksonu z def.\n" //unsigned	 MINIMALNY_ROZMIAR_TAKSONU=100;  //Arbitralny rozmiar klonu, żeby go uznać za zarejestrowany takson
	cerr<<lang("INLO=NNNN - długość wewnętrznych logów ",
			   "INLO=NNNN - ")<<'('<<dlugosc_logow<<')'<<endl; // const unsigned dlugosc_logow=50000;
	cerr<<lang("LOGF=nazwa.log - nazwa pliku z logiem ",
			   "LOGF=name.log - ")<<'('<<LogName<<')'<<endl;
	cerr<<lang("LOGR=N - częstość zapisów do zewnętrznego logu ",
			   "LOGR=N - ")<<'('<<LogRatio<<')'<<endl; //unsigned	LogRatio=1; // Co ile kroków zapisywać do logu
	cerr<<lang("DMPF=nazwa - rdzeń/początek nazwy plików dump ",
			   "DMPF=name - ")<<'('<<SCREENDUMPNAME<<')'<<endl; //char     SCREENDUMPNAME[256]="CO-EVO-3a";
	cerr<<lang("STMX=NNNN - największa możliwa liczba kroków symulacji ",
			   "STMX=NNNN - ")<<'('<<MAX_ITERATIONS<<')'<<endl; //unsigned long	MAX_ITERATIONS=0xffffffff; // największa liczba iteracji
	cerr<<lang("WIDTHWIN=,HEIGHTWIN= - rozmiary obszaru roboczego okna ",
			   "WIDTHWIN=,HEIGHTWIN= - ")<<'('<<SWIDTH<<'x'<<SHEIGHT<<')'<<endl;	//unsigned SWIDTH=1200; //750; //1200; Tez zadziała
																				    //unsigned SHEIGHT=750; //550; //1000;
	return 0;
	}
	}
//POPRAWIANIE ZALEŻNOŚCI
if(DYS_RUCHU>IBOKSWIATA/4)
	 DYS_RUCHU=IBOKSWIATA/4;
return 1;
}

// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Reactivated source code from Windows (2022.07)
/// @author Wojciech Borkowski
/// FOR @LICENCE SEE HERE: https://github.com/borkowsk/Coevo2_model
// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

