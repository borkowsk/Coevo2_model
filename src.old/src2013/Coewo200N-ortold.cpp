/* Program symulujący KOEWOLUCJE */
/* Każdy osobnik ma swój bitowy wzorzec odżywiania i bitowy wzorzec */
/* strategii osłony. Jeśli ATAKOWANY.OSLONA AND ATAKUJACY.Trofia>0 to  */
/* znaczy ze atak zakończył sie powodzeniem.			    */
/* Osobniki rozmnażają sie kosztem wolnego miejsca i zgromadzonej energii */
/* Ruchy własne, ofiara ataku, jak i moment rozmnażania wybierane są */
/* losowo, aby nie zaciemniać modelu dodatkowymi parametrami. 	     */
//W razie czego:  __sbh_heap_check  albo podobne jeśli jest…


#ifdef NDEBUG
const char*	   ProgramName="CO-EVOLUTION wer. 3.6a (c)1994-2011: Wojciech Borkowski [idea i realizacja]";
#else
const char*	   ProgramName="CO-EVOLUTION wer. 3.6a DEBUG (c)1994-2011 Wojciech Borkowski";
#endif

#pragma warn -aus //??? i jaki kompilator?
unsigned lang_selector=0;     //Jaki jêzyk
//WERSJA 3.6
//1. Angielskie nazewnictwo okien i oraz tłumaczenie innych elementów
//2. Zmienne w czasie nas³onecznienie
//3. Angielskie nazwy parametrów i komunikaty

//Wersja 3.5
//Uporz¹dkowanie liczników kroków - przejœcie na kroki Monte-Carlo tam gdzie siê da...
//Zosta³y licznik kroków oraz osobno liczone 'wersje' danych.
//Wizualizacja 'on run' serii czasowych daje zawsze równe odstêpy punktów, niezaleznie
//od aktualnej multiplikacji MC. Ale w pliku log jest seria z w³aœciwymi wartoœciami MC

//Wersja 3 - nowe mo¿liwoœci
//1. Ró¿ne dane do kolorowania drzewa filogenetycznego wybierane z menu
//2. Rozbudowa listy parametrów 'wyprowadzonych' na zewn¹trz programu i usuniêcie blokady na 'œwiaty' wiêksze od ekranu
//3. Dodanie miernika czasu procesora w g³ownej pêtli i wyprowadzenie wraz z krokiem na konsole
//4. 25.04.08 Usuniêcie b³edu w bibliotecznym menagerze lufcików wywalaj¹cego program przy robieniu TILE
//5. 11.09.08 Wypisywanie do pliku kompletnej filogenezy
//6. Wersja 3.1 - poprawki techniczne i kompilowalnoœc pod MSVC 2008
//7. Uelastycznienie wizualizacji sieci ekologicznej i jej zapisywania
//8. Umo¿liwienie ci¹g³ych zrzutów zawartoœci okna graficznego i seci ekologicznej w dwu formatach
//9. Regulacja czêstoœc zrzutów - NIEDOKOÑCZONA - POWODUJE B£ÊDN¥ NUMERACJE PLIKÓW I KROKÓW

//Istotne nowosci w wersji 2.5
//1. Odwrotnosc prawdopodobienstwa mutacji wyprowadzona jako parametr RTG
//Mutacje w okolicy 1/16000 dla obszary 400x200 doprowadzaja do wyparcia heterotrofow 70000:100
//i to przy roznych wartosciach globalnosci oddzialywan.
//Przy poziomie 1/1600 jest zachowana rownowaga podobna do tej z typowego poziomu 1/160
//2. Wprowadzenie parametru wewnetrznego `ZAMIANY' umozliwiajacego elastyczne przeplywanie obok siebie obojetnych osobnikow
//Ma ono zapobiegac formowaniu sie frontow - czyli blokowaniu rozprzestrzeniania sie skutecznych taksonow przez 'gestosc zaludnienia'
//3. Wprowadzenie maksymalnego dystansu skoku pozwala zbadac zaleznosc od 'globalnosci ekologii'
//4. Dodano tez wyswietlanie wieku taksonow
//5. Dodano modu³ 'kladystyka' oparty o dane z klonalinfo i s³uz¹cy preparowaniu prymitywnego (na razie) drzewa filogenetycznego
//   kolorowanego specjalizacj¹
//6. Nieco uelastyczniono sposob ustalania co jest klonem a co taksonem - treshold jest teraz zmienna
//7. Dodano modul ekologiczny rejestrujacy i analizujacy siec troficzna
//8. Uzupelniono interface o mozliwosci wlasnych opcji uzytkownika - np. wylaczanie oblicznia sieci ekologicznej i kladystyki
//9. Zmieniono na szary tlo niektorych grafów
//15.02.2006
//  Rekompilacja z poprawion¹ bibliotek¹ wspierajaca
//  ...

#include <limits.h>
#include <assert.h>
#include <stdio.h>
#include <process.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <signal.h>
#include <time.h>

#include "INCLUDE/platform.hpp"

#define USES_RANDG
#include "INCLUDE/random.h" //WB random generators for C-lang  (MACROS!)

#define HIDE_WB_PTR_IO	0 //Musi byc IO dla wb_dynarray
#include "INCLUDE/wb_ptr.hpp"
#include "INCLUDE/wb_ptrio.h"
#include "INCLUDE/wb_cpucl.hpp"

#include "SYMSHELL/datasour.hpp"
#include "SYMSHELL/simpsour.hpp"
#include "SYMSHELL/filtsour.hpp"
#include "SYMSHELL/bifiltso.hpp"
#include "SYMSHELL/statsour.hpp"
#include "SYMSHELL/dhistosou.hpp"
#include "SYMSHELL/fifosour.hpp"
#include "SYMSHELL/funcsour.hpp"
#include "SYMSHELL/sourmngr.hpp"
#include "SYMSHELL/logfile.hpp"
#include "SYMSHELL/layer.hpp"
#include "SYMSHELL/areamngr.hpp"
#include "SYMSHELL/gadgets.hpp"
#include "SYMSHELL/textarea.hpp"
#include "SYMSHELL/graphs.hpp"
#include "SYMSHELL/mainmngr.hpp"

#include "SYMSHELL/sshutils.hpp"
#include "SYMSHELL/sshmenuf.h"
#include "ResSrc/coewo3rc.h"

#ifdef __BORLANDC__
#ifndef _getpid
#define _getpid()  (getpid())
#endif
#endif

class swiat; //Zapowiedz g³ównej klasy symulacji

//Menager lufcikow ze uzupelniona obsluga i/o (z menu)
class my_area_menager:public main_area_menager
{
	bool trace_fillog_tree;
	bool trace_trophi_netw;
	swiat*   MojSwiat;
	virtual int _pre_process_input(int input_char);//Przed obsluga domyslna. Zwraca 1 jesli obsluzyl.
public:

	my_area_menager(size_t size, //Konstruktor dajacy zarzadce o okreslonym rozmiarze listy
				int width,int height,
				unsigned ibkg=default_color):main_area_menager(size,width,height,ibkg),
				trace_fillog_tree(true),
				trace_trophi_netw(true),
				MojSwiat(NULL)
	{}
	int start(const char* wintitle,int argc=0,const char** argv=NULL,int double_buffering=-1);//Zwraca 1 jesli ok
	void connect(swiat* sw) {MojSwiat=sw;}
	bool trace_filogenetic_tree_enabled() { return trace_fillog_tree;}
	bool trace_trophic_network_enabled() { return trace_trophi_netw;}
};

#include "intersourc.hpp"//Zrodla specjalne AND i ...
and_interaction_source	AndDemo(lang("Trofia AND Oslona","Trophy AND Defence"));       //Zrodla specjalne do celow demonstracyjnych
and_exploatation_source ExpDemo(lang("(T AND O)/O * (T AND O)/T","(T AND D)/D * (T AND D)/T"));//... tzn ladne dywaniki oddzialywan

#include "klonalinfo.hpp"
#include "troficinfo.h"
#include "kladystyka.hpp"
#include "ekologia.hpp"
#include "co_agent.hpp"

//BARDZO WA¯NE WSKA¯NIKI DO MASEK CZYLI GENÓW BÊD¥CYCH SK£ADOWYMI STRUKTURY
//base agent::* geba_ptr=&agent::w.w.geba;      //TAK POWINNO BYC - ALE NIE KOMPILUJE
//base agent::* oslona_ptr=&agent::w.w.oslona;

#define RECZNIE_WPISZ_PTR //RECZNE WPISYWANIE - ZALEZNE OD 'ENDIAN' I WIDZIMISIE TWORCÓW KOMPILATORA

#ifdef  RECZNIE_WPISZ_PTR
base agent::* geba_ptr=NULL; //Tu w zasadzie tylko deklaracja...
base agent::* oslona_ptr=NULL;//Przypisanie jest dalej, te¿ zabezpieczone ifdef RECZNIE_WPISZ_PTR
#else
base agent::* geba_ptr=&(agent::w.w.geba); //TAK NIE DZIA£A
base agent::* oslona_ptr=&(agent::w.w.oslona);
#endif

//TAK MSVC KOMPILUJE ALE WPISUJE 0
//base agent::* geba_ptr=&agent::w.wzor::w.bity_wzoru::geba;
//base agent::* oslona_ptr=&agent::w.wzor::w.bity_wzoru::oslona;

//Wazne domyslne ustawienia
char     SCREENDUMPNAME[256]="CO-EVO-3a";
char     LogName[256]="coewo3.0a.out";
char     MappName[256]=""/*coewo2008ini.gif"*/;

//Czesto zmieniane parametry kompilacji i wykonania samego programu
unsigned SWIDTH=1200;//750;//960;//1200;//;//1200; Tez zadziala
unsigned SHEIGHT=900;//550;//750;//750//;//1000;

//const unsigned TAX_OUT=256; //PO CO TO?  zamiast REJESTROWANY_ROZMIAR_TAKSONU?
//PODSTAWOWE PARAMETRY MODELU
unsigned IBOKSWIATA=300;                // FAKTYCZNIE UZYWANY BOK SWIATA
unsigned MINIMALNY_ROZMIAR_TAKSONU=100;  //Arbitralny rozmiar klonu zeby go uznac za zarejestrowany takson
unsigned REJESTROWANY_ROZMIAR_TAKSONU=MINIMALNY_ROZMIAR_TAKSONU;//Domyslna wartosc kasowania w wizualizacji drzewa
unsigned REJESTROWANY_ROZMIAR_WEZLA=0;//MINIMALNY_ROZMIAR_TAKSONU-1;//Domyslna wartosc kasowania w wizualizacji sieci eko

unsigned	PROMIENIOWANIE=BITS_PER_GENOM*500;	// Co ile kopiowanych bitow nastepuje mutacja
bool		UZYWAJ_PELNE_PROM=true;				//Czy u¿ywamy pe³nego promieniowania, czy obni¿onego?     NIE U¯YWANE???

int    WSP_KATASTROF=0;//10-100		// Wykladnik rozkladu katastrof - 0 -wylaczone
int    DZIELNIK_1_OKRESU_MILANKOVICA=50;   //Przez ile dzieliæ kroki M C ¿eby uzuskac cos do zmian oœwietlenia
double POWER_COS_NASLONECZNIENIA=0;        //"Potêga cyklu s³onecznego": 0 - wy³¹czone  1 - zwyk³y cosinus   2 - tropikalny 3 - pory roku

double		EFEKTYWNOSC_AUTOTROFA=0.99;	// jaka czesc swiatla uzywa autotrof
unsigned	NIEPLODNOSC=5;		// Prawdopodobienstwo rozmnazania jest 1/NIEPLODNOSC
double		WYPOSAZENIE_POTOMSTWA=0.05; // jaka czesc sily oddac potomkowi 0.1 to ZA DUZO!!! {???}

unsigned	MINIMALNY_WIEK=205;	// Rodzi sie z tym wiekiem. Do smierci ma 255-MINIMALNY_WIEK
unsigned int	ODWROCONY_KOSZT_OSLONY=0;//1 - czy koszty oslony sa odwrocone
unsigned int	ODWROCONY_KOSZT_ATAKU=0;//1 - czy koszty ataku sa odwrocone

unsigned int	PIRACTWO=1;				//Czy eksploatacja piracka czy pasozytnicza
unsigned	DYS_RUCHU=1;      //Dystans ruchu w jednym kierunku
//unsigned BIT_RUCHU=128;		//Wyzerowanie ktorych bitow oslony odpowiada za zdolnosc ruchu
unsigned	BIT_RUCHU=1024;//1		//Gdy poza maske - bez mozliwosci utraty ruchliwosci
unsigned	ZAMIANY=1;//0//255;        //Czy moze przeciskac sie na zajete pola (0 na nie lub inna liczba na tak)

//Zewnêtrzne zarz¹dzanie symulacj¹ i zbieraniem danych
unsigned long	MAX_ITERATIONS=0xffffffff; // najwieksza liczba iteracji
int          MonteCarloMultiplic=10; //Ewentualna modyfikacja multiplikacji dlugosci kroku MonteCarlo
									//... ale dlaczego było double ??????
unsigned dlugosc_logow=100000; //Dlugoœæ wewnetrznych logów podrêcznych - do wizualizacji
unsigned LogRatio=1;				// Co ile krokow zapisywac do logu

bool DumpScreenContinously=false;
bool DumpNETContinously=false;
bool DumpVNAContinously=false;

//unsigned        textY=(IBOKSWIATA>TAX_OUT?IBOKSWIATA:TAX_OUT);         ???


/* Czesc niezalezna od platformy */
/*********************************/


//??? Gdzie to jest uzywane?
struct inte_stat//Struktura do przechowywania lokalnej informacji statystycznej
//-----------------------------------------------------------------------------
{
	unsigned N;//Ilosc interakcji
	double  sE;//Suma energii przejetej w interakcjach
	double  sP;//Suma udzialow (efektywnosci) interacji

	friend
	ostream& operator<< (ostream& o, const inte_stat& inst)
	{   //Bardzo prymitywne, bez kontroli b³êdów!!!
		o<<inst.N<<'\t'<<inst.sE<<'\t'<<inst.sP<<'\t';
		return o;
	}

	friend
	istream& operator >> (istream& in, inte_stat& inst)
	{   //Bardzo prymitywne, bez kontroli b³êdów!!!
		in>>inst.N;
		in>>inst.sE;
		in>>inst.sP;
		return in;
	}

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
size_t			DLUG_WIERSZA;	// Obwod torusa
double 			globalneSwiatlo; //Bazowa globalna iloœæ œwiat³a -  sta³a
double 			AktualneSwiatlo; //Aktualna iloœæ swiat³a - mo¿e siê zmieniaæ w czasie

// Statystyki i liczniki
/////////////////////////////////
unsigned long	licznik_krokow_w;// licznik wywo³an procedury kroku symulacji
unsigned long	monte_carlo_licz;//licznik realnie wykonanych krokow monte-carlo

unsigned 		klon_auto;		// Licznik klonow autotroficznych
unsigned 		tax_auto;		// Licznik taksonow autotroficznych
unsigned 		autotrofy;		// Licznik agentow autrotroficznych

klad filogeneza;                //Informacje statystyczne z drzewa filogenetycznego
ekologia trophNet;              //Informacje statystyczne dla sieci ekologicznej

//Warstwy symulacji (sa torusami - bo taka jest wspolna geometria)
////////////////////////////////////////////////////////////////////////
rectangle_unilayer<unsigned char> zdatnosc; //Warstwa definiujaca zdatnosc do zasiedlenia
rectangle_layer_of_agents<agent>    ziemia; //Wlaœciwa warstwa agentow zasiedlajacych
rectangle_layer_of_struct<inte_stat> stats; //Warstwa pamietajaca lokalne statystyki zdazen

//Zarzadzanie zrodlami danych
///////////////////////////////////
sources_menager								 Sources;  //Zarzadca seri przekaznikowych
matrix_source<unsigned>						 *Liczniki;//Udostepnienie licznikow taxonow
struct_matrix_source<agent,unsigned char >	 *Wiek;
struct_matrix_source<agent,unsigned>		 *Sila;
struct_matrix_source<agent,base >			 *Trofia;
struct_matrix_source<agent,base >			 *Oslona;
method_matrix_source<inte_stat,double>		 *EDynamism;//Lokalna dynamika przeplywu energii
method_matrix_source<inte_stat,double>		 *PDynamism;//Lokalne efektywnosc eksploatacji
logfile										 log;	// plik z zapisem histori symulacji

//Obszary wyswietlania
////////////////////////////////////
my_area_menager&	MyAreaMenager;
net_graph*			FilogeneticTree;	//Drzewo filogenetyczne
net_graph*			TrophicNet;		//Sieæ zaleznoœci pokarmowych
text_area*			OutArea;//Obszar do wypisywania statusu symulacji

public:
wb_pchar dumpmarker;//Uchwyt do nazwy eksperymentu

//KONSTRUKCJA DESTRUKCJA
swiat(size_t szerokosc,
	  char* logname,   //Nazwa pliku do zapisywania histori
	  char* mappname,
	  my_area_menager& AreaMenager
	  ); //Nazwa (bit)mapy inicjujacej 'zdatnosc'
~swiat(){}

//AKCJE
void inicjuj();	  // stan startowy symulacji
void krok();	  // kolejny krok symulacji
void kataklizm(); // wygenerowanie katastrofy - reczne lub losowe

//Wspolpraca z menagerem wyswietlania
//---------------------------------------------
void wskazniki(); // aktualizacja nowych wartosci wskaznikow
void tworz_lufciki();//Tworzy domyslne lufciki

//Dostêp do g³ownych danych
agent& Ziemia(size_t Kolumna,size_t Wiersz) {return ziemia.get(Kolumna,Wiersz);} //Dostêp do ziemii
float  Swiatlo(size_t Kolumna,size_t Wiersz) {return globalneSwiatlo;} //Dostêp do swiatla - tu globalnego

//INNE METODY POMOCNICZE
///////////////////////////////////////////
//Do wizualizacji
void ZmienKoloryDrzewaFil(klad::Kolorowanie co);
void UstalDetalicznoscSieci(int Ile,bool Relative=true);
//Do statystyk
unsigned long	daj_licznik_krokow(){return licznik_krokow_w;}		// licznik krokow symulacji
unsigned long   daj_kroki_monte_carlo(){return monte_carlo_licz;}   //suma realnie wykonanych kroków MC
void print_params(ostream& out);
//Do dodatkowych logów
void ZapiszFilogeneze(ostream& out,unsigned min_time=0,unsigned max_time=0,unsigned size_tres=0);
void ZapiszEkologieNET(ostream& out,unsigned size_tres=0,double weight_tres=0);
void ZapiszEkologieVNA(ostream& out,unsigned size_tres=0,double weight_tres=0);
};

void swiat::tworz_lufciki()
//Tworzy domyslne lufciki
{
if(0)   //SPECJALNY BLOK OBSLUGI BLEDOW
{
DO_OBSLUGI_BLEDOW: ;

	perror(lang("Nie mozna utworzyc obiektow wizualizujacych","I'm not able to make visualisation stuff"));
	//Tu tylko w wypadku b³êdu
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
???? *hack2=0;//sizeof(base)?
*hack1=1;//?
#elif __GNUC__
*hack2=0;//sizeof(base)?
*hack1=1;//?
#else //BDS2006
*hack2=1;//sizeof(base)?
*hack1=2;
#endif

}
#endif

{
//Zerowa seria w menagerze danych powinna byc pusta - sluzy
//do kontroli wersji danych. Menager moze tworzyc ja sam,
//ale zawsze mozna podmienic
ptr_to_scalar_source<unsigned long>* sca=new ptr_to_scalar_source<unsigned long>(&monte_carlo_licz,lang("KrokMC","MCStep"));
if(!sca) goto DO_OBSLUGI_BLEDOW;
data_source_base* first=Sources.get(0);
int index=0;
if(first!=NULL)
	Sources.replace(first->name(),sca);
	else
	index=Sources.insert(sca);        assert(index==0);

//Tylko ze wzgledow demonstracyjnych
array_source<unsigned>* arr=new array_source<unsigned>(MAXBASE2+1,agent::liczniki,lang("liczniki taksonow - liniowo","taxon counters"));
if(!arr) goto DO_OBSLUGI_BLEDOW;
Sources.insert(arr);

//ZRODLA SUROWYCH WLASCIWOSCI AGENTÓW
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

//ZRODLO METODOWE CZYTAJACE WIEK TAKSONU DO KTOREGO NALEZY AGENT
method_matrix_source<agent,unsigned long>* WiekFilog=ziemia.make_source(lang("WiekFilogenetyczny","PhylogeneticAge"),&agent::how_old_taxon);
if(!WiekFilog) goto DO_OBSLUGI_BLEDOW;
//WiekFilog->setminmax(0,1);
WiekFilog->set_missing(0xffffffff);
int inWiekFilog=Sources.insert(WiekFilog);


//ZRODLO METODOWE FILTRUJACE ZYWE
method_matrix_source<agent,bool>* Zycie=ziemia.make_source(lang("martwe/zywe","alive or not"),&agent::jest_zywy);
if(!Zycie) goto DO_OBSLUGI_BLEDOW;
Zycie->setminmax(0,1);
int inZycie=Sources.insert(Zycie);

GT_filter<method_matrix_source<agent,bool> >* filtrZycia=//Filtr na "Tylko Zywe"
	new GT_filter<method_matrix_source<agent,bool> >(0.5,Zycie,default_missing<double>(),lang("Zywe","Alive"));//
if(!filtrZycia)goto DO_OBSLUGI_BLEDOW;
int inFiltrZycia=Sources.insert(filtrZycia);

//SAME ZYWE
if_then_source* GebaZywych=new if_then_source(filtrZycia,Trofia,lang("TrofiaZ","TrophyA"));//Ale ma klopot z missing_val i przez to zle liczy min i max
if(!GebaZywych) goto DO_OBSLUGI_BLEDOW;
Sources.insert(GebaZywych);

if_then_source* OslonaZywych=new if_then_source(filtrZycia,Oslona,lang("ObronaZ","DefenceA"));//Ale ma klopot z missing_val i przez to zle liczy min i max
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

//Trofia
pom=new carpet_graph(0,wys_map,szer_map-1,2*wys_map-1,//domyslne wspolrzedne
						//TrofiaZ);//I zrodlo danych
						GebaZywych);//I alternatywne zrodlo danych
if(!pom) goto DO_OBSLUGI_BLEDOW;
//pom->setdatacolors(0,255);//Pierwsze 25 kolorow bedzie slabo widoczne
pom->settitle(lang("PREFERENCJE TROFICZNE (maski ataku)","TROPHIC PREFERENCES (masks of attack)"));
pom->settitlecolo(255,default_transparent);
pom->setbackground(default_half_gray);
MyAreaMenager.insert(pom);

//OSLONA
pom=new carpet_graph(0,2*wys_map,szer_map-1,3*wys_map-1,//domyslne wspolrzedne
//						Oslona);//I zrodlo danych
						OslonaZywych);//I alternatywne zrodlo danych

if(!pom) goto DO_OBSLUGI_BLEDOW;
//pom->setdatacolors(0,255);//Pierwsze 25 kolorow bedzie slabo widoczne
pom->settitle(lang("PODATNOŒÆ AGENTÓW NA ATAK (maski obrony)","SENSITIVITY TO THE ATTACK (mask of defence)"));
pom->settitlecolo(255,default_transparent);
pom->setbackground(default_half_gray);
MyAreaMenager.insert(pom);

//Energia
pom=new carpet_graph(MyAreaMenager.getwidth()-80,87+21+1+21+21,MyAreaMenager.getwidth()-1,87+21+1+21+21+21,
//						Sila);//I zrodlo danych
                        SilaZywych);//I alternatywne zrodlo danych
pom->setdatacolors(1,254);//Zeby nie uzywac bialego, ktory jest tlem i calkiem czarnych
pom->setbackground(default_half_gray);
pom->settitle(lang("ZASOBY ENERGII AGENTÓW","ENERGY RESOURCES OF AGENTS"));
pom->settitlecolo(255,default_transparent);
MyAreaMenager.insert(pom);

//ZRODLA O SPECJALIZACJI
method_matrix_source<agent,int>* Specjalizacja=ziemia.make_source(lang("bity ustawione","how much specialised"),&agent::how_specialised);
if(!Specjalizacja) goto DO_OBSLUGI_BLEDOW;
Specjalizacja->setminmax(1,16);
Specjalizacja->set_missing(-1);
int inSpecjalizacja=Sources.insert(Specjalizacja);

pom=new carpet_graph(0,3*wys_map,szer_map-1,4*wys_map,//domyslne wspolrzedne
						Specjalizacja);//I zrodlo danych
//pom->setdatacolors(1,254);//Zeby nie uzywac bialego, ktory jest tlem i calkiem czarnych
pom->setbackground(default_half_gray);
pom->settitlecolo(255,default_transparent);
pom->settitle(lang("POZIOM SPECJALIZACJI AGENTÓW","LEVEL OF SPECIALISATION"));
MyAreaMenager.insert(pom);

method_matrix_source<agent,int>* SpecjalizacjaA=ziemia.make_source(lang("bity autotrofów","spec. bits of autotrophs"),&agent::how_specialised_autotrof);
if(!SpecjalizacjaA) goto DO_OBSLUGI_BLEDOW;
SpecjalizacjaA->setminmax(1,16);
SpecjalizacjaA->set_missing(-1);
int inSpecjalizacjaA=Sources.insert(SpecjalizacjaA);

method_matrix_source<agent,int>* SpecjalizacjaH=ziemia.make_source(lang("bity heterotrofów","spec. bits of heterotrophs"),&agent::how_specialised_heterotrof);
if(!SpecjalizacjaH) goto DO_OBSLUGI_BLEDOW;
SpecjalizacjaH->setminmax(1,16);
SpecjalizacjaH->set_missing(-1);
int inSpecjalizacjaH=Sources.insert(SpecjalizacjaH);

//HISTORIA OSTATNICH <dlugosc_logow> KROKOW
//Parametry 'abiotyczne'
//AktualneSwiatlo????
template_scalar_source_base<double>* currLight=new ptr_to_scalar_source<double>(&AktualneSwiatlo,lang("Nas³onecznienie","Insolation"));
if(!currLight) goto DO_OBSLUGI_BLEDOW;
Sources.insert(currLight);
fifo_source<double>* curlpom=new fifo_source<double>(currLight,dlugosc_logow);
if(!curlpom)goto DO_OBSLUGI_BLEDOW;
int inCurrLight=Sources.insert(curlpom);

//Bogactwo klonów i gatunków
template_scalar_source_base<unsigned long>* allclons=
					new ptr_to_fuction_source<unsigned long>(informacja_klonalna::ile_klonow,lang("klonów od poczatku","clones from the beginning"));
if(!allclons) goto DO_OBSLUGI_BLEDOW;
Sources.insert(allclons);
fifo_source<unsigned long>* flpom=new fifo_source<unsigned long>(allclons,dlugosc_logow);
if(!flpom)goto DO_OBSLUGI_BLEDOW;
int inAllclons=Sources.insert(flpom);

template_scalar_source_base<unsigned long>* alltax=
					new ptr_to_fuction_source<unsigned long>(informacja_klonalna::ile_taksonow,lang("eko-gatunków od poczatku","species from the begining"));
if(!alltax) goto DO_OBSLUGI_BLEDOW;
Sources.insert(alltax);
flpom=new fifo_source<unsigned long>(alltax,dlugosc_logow);
if(!flpom)goto DO_OBSLUGI_BLEDOW;
int inAlltax=Sources.insert(flpom);

template_scalar_source_base<unsigned>* staxa=new ptr_to_scalar_source<unsigned>(&tax_auto,lang("eko-gatunki autotroficzne","autotrophic species"));// ile jest autotroficznych taksonow (liczniejszych niz treshold)
if(!staxa) goto DO_OBSLUGI_BLEDOW;
Sources.insert(staxa);

fifo_source<unsigned>* fpom=new fifo_source<unsigned>(staxa,dlugosc_logow);
if(!fpom) goto DO_OBSLUGI_BLEDOW;
int fiftaxa=Sources.insert(fpom);

template_scalar_source_base<unsigned>* sklona=new ptr_to_scalar_source<unsigned>(&klon_auto,lang("klony autotroficzne","autotrophic clones"));// ile klonow autotroficznych
if(!sklona) goto DO_OBSLUGI_BLEDOW;
Sources.insert(sklona);

fpom=new fifo_source<unsigned>(sklona,dlugosc_logow);
if(!fpom) goto DO_OBSLUGI_BLEDOW;
int fifklona=Sources.insert(fpom);

template_scalar_source_base<unsigned>* stax=new ptr_to_scalar_source<unsigned>(&agent::ile_big_tax,lang("eko-gatunki","species"));// ile taxonow liczniejszych niz niz treshold
if(!stax) goto DO_OBSLUGI_BLEDOW;
Sources.insert(stax);

fpom=new fifo_source<unsigned>(stax,dlugosc_logow);
if(!fpom) goto DO_OBSLUGI_BLEDOW;
int fiftax=Sources.insert(fpom);

template_scalar_source_base<unsigned>* sklon=new ptr_to_scalar_source<unsigned>(&agent::ile_tax,lang("klony","clones"));// ile w ogóle istniejacych klonow
if(!sklon) goto DO_OBSLUGI_BLEDOW;
Sources.insert(sklon);

fpom=new fifo_source<unsigned>(sklon,dlugosc_logow);
if(!fpom) goto DO_OBSLUGI_BLEDOW;
int fifklon=Sources.insert(fpom);

//ROZNE STATYSTYKI
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

//HISTORIA LICZEBNOSCI AGENTÓW
template_scalar_source_base<unsigned>* sind=new ptr_to_scalar_source<unsigned>(&agent::ile_ind,lang("liczba agentow","num. of agents"));// ile agentow niezerowych
if(!sind) goto DO_OBSLUGI_BLEDOW;
Sources.insert(sind);

fpom=new fifo_source<unsigned>(sind,dlugosc_logow);
if(!fpom) goto DO_OBSLUGI_BLEDOW;
int fifagents=Sources.insert(fpom);

template_scalar_source_base<unsigned>* sinda=new ptr_to_scalar_source<unsigned>(&autotrofy,lang("liczba autotrofow","num. of autotrophs"));// ile agentow autotroficznych
if(!sinda) goto DO_OBSLUGI_BLEDOW;
Sources.insert(sinda);

fpom=new fifo_source<unsigned>(sinda,dlugosc_logow);
if(!fpom) goto DO_OBSLUGI_BLEDOW;
int fifautoagents=Sources.insert(fpom);


//WYSWIETLACZ HISTORI LICZEBNOSCI
graph* pom1=new sequence_graph(
                               //szer_map,300,szer_map+300,400,//z wycieciem na male kwadratowe okienko
                               szer_map,300,MyAreaMenager.getwidth()-1,400,//rownolegle do okna liczby taksonow
							   3,Sources.make_series_info(
										inCurrLight,fifagents,fifautoagents,
										-1
										).get_ptr_val(),
							  // 1/*Wspolne minimum/maximum*/);
                                 0/*reskalowanie*/);
if(!pom1) goto DO_OBSLUGI_BLEDOW;
pom1->setframe(128);
pom1->settitle(lang("AGENCI - HISTORIA LICZEBNOŒCI","History of the number of agents"));
//pom1->setdatacolors()
MyAreaMenager.insert(pom1);

//Historia sily
//szer_map+301,300,MyAreaMenager.getwidth()-1,400,

//Historia liczby taksonow
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


//WIZUALIZACJA FILTRU ZYCIA
pom=new carpet_graph(MyAreaMenager.getwidth()-80,0,MyAreaMenager.getwidth()-1,20,//domyslne wspolrzedne
						filtrZycia);//I zrodlo danych
if(!pom) goto DO_OBSLUGI_BLEDOW;
pom->setdatacolors(50,254);//Potrzebne tylko dwa kolory naprawde
pom->setframe(32);
pom->setbackground(default_half_gray);
pom->settitlecolo(255,default_transparent);
pom->settitle(lang("ZYWI AGENCI","ALIVE AGENTS"));
MyAreaMenager.insert(pom);

//UDOSTEPNIANIE I WYSWIETLANIE BACKGROUNDU SYMULACJI
rectangle_source_base* Background=zdatnosc.make_source(lang("przydatnoœæ terenu","suitability of areas"));
if(!Background) goto DO_OBSLUGI_BLEDOW;
Sources.insert(Background);
Background->setminmax(0,255);//255 odcieni (szarosci) - czarne niezdatne
pom=new carpet_graph(MyAreaMenager.getwidth()-80,21,MyAreaMenager.getwidth()-1,42,//domyslne wspolrzedne
					 Background);//I zrodlo danych

if(!pom) goto DO_OBSLUGI_BLEDOW;
pom->setdatacolors(0,255);//Pierwsze 25 kolorow bedzie slabo widoczne
pom->setframe(64);
pom->setbackground(default_half_gray);
pom->settitlecolo(255,default_transparent);
pom->settitle(lang("Mapa zdatnych obszarów","Map of useful/unusable areas"));
MyAreaMenager.insert(pom);

//Demo interakcji AND
pom=new carpet_graph(MyAreaMenager.getwidth()-80,43,MyAreaMenager.getwidth()-1,64,//domyslne wspolrzedne
						&AndDemo);//I zrodlo danych
if(!pom) goto DO_OBSLUGI_BLEDOW;
//pom->setdatacolors(0,255);//Potrzebne tylko dwa kolory naprawde
pom->setframe(64);
pom->settitle(lang("AND(Trofia,Oslona)","AND(Trophy,Defense)"));
MyAreaMenager.insert(pom);

//Demo exploatacji potenclalnej (AND*K)
pom=new carpet_graph(MyAreaMenager.getwidth()-80,65,MyAreaMenager.getwidth()-1,86,//domyslne wspolrzedne
						&ExpDemo);//I zrodlo danych
if(!pom) goto DO_OBSLUGI_BLEDOW;
//pom->setdatacolors(0,255);//Potrzebne tylko dwa kolory naprawde
pom->setframe(64);
pom->settitle(lang("Eksploatacja potencjalna","exploitation potential"));
MyAreaMenager.insert(pom);

//Wiek
pom=new carpet_graph(MyAreaMenager.getwidth()-80,87,MyAreaMenager.getwidth()-1,87+21,//domyslne wspolrzedne
						Wiek);//I zrodlo danych
pom->setdatacolors(0,255);//Pierwsze 25 kolorow bedzie slabo widoczne
pom->setframe(32);
pom->setbackground(default_half_gray);
pom->settitlecolo(255,default_transparent);
pom->settitle(lang("WIEK AGENTÓW","AGE OF AGENTS"));
MyAreaMenager.insert(pom);

//OKNO HISTORI SILY
//dalej zakladamy ze SilaStat nie umieszcza sam podzrodel w MyAreaMenagerze danych
//ale latwo sprawic by to zalozenie nie bylo prawdziwe - wystarczy wywolac
//SilaStat->link_source_MyAreaMenager(Sources);
fifo_source<double>* dpom=new fifo_source<double>(SilaStat->Mean(),dlugosc_logow);//Fifo ze sredniej sily
if(!dpom) goto DO_OBSLUGI_BLEDOW;
int MeanSilaFifoIndex=Sources.insert(dpom);

dpom=new fifo_source<double>(SilaStat->Max(),dlugosc_logow);//Fifo ze sredniej sily
if(!dpom) goto DO_OBSLUGI_BLEDOW;
int MaxSilaFifoIndex=Sources.insert(dpom);

dpom=new fifo_source<double>(SilaStat->SD(),dlugosc_logow);//Fifo ze sredniej sily
if(!dpom) goto DO_OBSLUGI_BLEDOW;
int SdSilaFifoIndex=Sources.insert(dpom);

pom1=new sequence_graph(//szer_map,300,szer_map+300,400,
                        MyAreaMenager.getwidth()-80,87+21+1,MyAreaMenager.getwidth()-1,87+21+21,//domyslne wspolrzedne
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

//Lokalne przeplywy energii
pom=new carpet_graph(MyAreaMenager.getwidth()-80,87+21+1+21,MyAreaMenager.getwidth()-1,87+21+1+21+21,
						EDynamism);//I zrodlo danych
pom->setdatacolors(0,255);//Pierwsze 25 kolorow bedzie slabo widoczne
//pom->setframe(32);
pom->setbackground(default_half_gray);
pom->settitlecolo(255,default_transparent);
pom->settitle(lang("LOKALNY PRZEP£YW ENERGII","LOCAL ENERGY FLOW"));
MyAreaMenager.insert(pom);

//Dywanowa mapa uzywanych nisz ekologicznych
/////////////////////////////////////////////////////
Liczniki=new matrix_source<unsigned>(lang("liczebnosc taksonow","abundance of taxa"),
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
pom=new carpet_graph(MyAreaMenager.getwidth()-80,87+21+1+21+21+21,MyAreaMenager.getwidth()-1,87+21+1+21+21+21+21,//domyslne wspolrzedne
                     //MyAreaMenager.getwidth()-80,87+21+1+21+21,MyAreaMenager.getwidth()-1,87+21+1+21+21+21,
						fl,1);//I zrodlo danych
if(!pom) goto DO_OBSLUGI_BLEDOW;

//pom->setbackground(0);
pom->setdatacolors(0,255);
pom->settitlecolo(255,default_transparent);
pom->setbackground(default_dark_gray);
pom->settitle(lang("MAPA U¯YWANYCH NISZ EKOLOGICZNYCH","USAGE OF ECOLOGICAL NICHES"));
MyAreaMenager.insert(pom);

//Wiek filogenetyczny klonow
////////////////////////////////////////////////////
pom1=new carpet_graph(0,0,szer_map-1,wys_map-1,//domyslne wspolrzedne
					  //MyAreaMenager.getwidth()-80,87+21+1+21+21,MyAreaMenager.getwidth()-1,87+21+1+21+21+21,
						WiekFilogZ);//I zrodlo danych
pom1->setdatacolors(0,255);//Pierwsze 25 kolorow bedzie slabo widoczne
pom1->setbackground(default_half_gray);
pom1->settitlecolo(255,default_transparent);
pom1->settitle(lang("FILOGENETYCZNY WIEK AGENTÓW","PHYLOGENETIC AGE OF AGENTS"));
MyAreaMenager.insert(pom1);

//Okno historii liczby taksonow od poczatku
//////////////////////////////////////////////////
pom1=new sequence_graph(//szer_map+301,300,MyAreaMenager.getwidth()-1,400,//domyslne wspolrzedne
						//MyAreaMenager.getwidth()-80,199,MyAreaMenager.getwidth()-1,299,//nowe domyslne wspolrzedne
						MyAreaMenager.getwidth()-80,87+21+1+21+21+21+21,MyAreaMenager.getwidth()-1,87+21+1+21+21+21+21+21,//domyslne wspolrzedne
								2,Sources.make_series_info(
									inAllclons,inAlltax,
													-1
										).get_ptr_val(),
								0/* Z reskalowaniem */);
							   //1/*Wspolne minimum/maximum*/);
if(!pom1) goto DO_OBSLUGI_BLEDOW;
pom1->setframe(32);
pom1->settitle(lang("PRZYROST DRZEWA FILOGENETYCZNEGO","GROWTH OF PHYLOGENETIC TREE"));
MyAreaMenager.insert(pom1);

//HISTOGRAM SPECJALIZACJI AUTOTROFóW
////////////////////////////////////////////////////////////////
/*	discrete_histogram_source(
		int    LowestClass,     //Najnizsza klasa
		size_t HowManyClass,    //Ile klas od niej
		DATA_SOURCE* ini=NULL,  //Klasa zrodlowa
								//Jesli nie pokrywa sie z minX-maxX to faktycznie liczony jest wycinek
		const char* format="DISCR.DISTRIBUTION(%s[%d..%d])",
		sources_menager_base* MyMenager=NULL,
		size_t table_size=11	//BEZ ZAPASU
		):
		*/
generic_discrete_histogram_source*  histspecjA=
		new generic_discrete_histogram_source(
				1,16,
				SpecjalizacjaA,
				"histogram(%s[%d..%d])",//format
				&Sources,
				17//table_size BEZ ZAPASU?*/
			);
if(!histspecjA) goto DO_OBSLUGI_BLEDOW;
Sources.insert(histspecjA);
pom=new bars_graph(//szer_map+1,130,szer_map+119,130+84,//domyslne wspolrzedne
					MyAreaMenager.getwidth()-80,87+21+1+21+21+21+21+21,MyAreaMenager.getwidth()-1,87+21+1+21+21+21+21+21+21,//domyslne wspolrzedne
				   histspecjA);
pom->setdatacolors(200,250);//max kolor to kolor ostatniego slupka
pom->setframe(232);
pom->settitle(lang("SPECJALIZACJA AUTOTROFÓW","SPECIALISATION OF AUTOTHROPS"));
MyAreaMenager.insert(pom);

//HISTOGRAM SPECJALIZACJI HETEROTROFÓW
/////////////////////////////////////////////////////////////////
generic_discrete_histogram_source*  histspecjH=
		new generic_discrete_histogram_source(
				1,16,
				SpecjalizacjaH,
				"histogram(%s[%d..%d])",//format
				&Sources,
				17//table_size BEZ ZAPASU?
			);
if(!histspecjH) goto DO_OBSLUGI_BLEDOW;
Sources.insert(histspecjH);
pom=new bars_graph(//szer_map+1,130+85,szer_map+119,299,//domyslne wspolrzedne
				   MyAreaMenager.getwidth()-80,87+21+21+1+21+21+21+21+21,MyAreaMenager.getwidth()-1,87+21+1+21+21+21+21+21+21+21,//domyslne wspolrzedne
					histspecjH);
pom->setdatacolors(0,64);//max kolor to kolor ostatniego slupka
pom->setframe(32);
pom->settitle(lang("SPECJALIZACJA HETEROTROFÓW","SPECIALISATION OF HETEROTROPHS"));
MyAreaMenager.insert(pom);

//FILOGENEZA TAXONÓW -  MOZE BYC KOSZTOWNE W RYSOWANIU
//////////////////////////////////////////////////////////////////////////
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
pom=FilogeneticTree=new net_graph(szer_map+170,130,MyAreaMenager.getwidth()-81,299,//domyslne wspolrzedne
				  Tm,0,
				  Sp,0,
                  Ls,0,
				  Le,0,
				  Lw,0,//Kolory linii jako punktow - powoduje narysowanie skali barwnej, ale dziala tylko dlatego ze domyslna figura jest NULL!
                  NULL,0,//Rozmiary punktow faktycznie nie sa potrzebne
                  NULL,0,//I rozmiary grotow tez nie
				  Lw,0
				  );
pom->setframe(128);
pom->setbackground(default_light_gray);
wb_pchar buf(1024);
buf.prn(lang("FILOGENEZA - taksony powy¿ej %d","PHYLOGENETIC TREE - taxons from %d"),REJESTROWANY_ROZMIAR_TAKSONU);
pom->settitle(buf.get());
MyAreaMenager.insert(pom);

//Sieciowa mapa zaleznosci ekologicznych
////////////////////////////////////////////////////////////////
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
//Sieæ zaleznoœci pokarmowych
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

//INFORMACJE STATYSTYCZNE I STATUSOWE
/////////////////////////////////////////////////////
OutArea=new text_area(
                      szer_map+1,0,MyAreaMenager.getwidth()-81,87+21+21,//domyslne wspolrzedne
					  lang("Informacje","Information_area"),
					  default_white,default_black,128,40);
if(!OutArea) goto DO_OBSLUGI_BLEDOW;
OutArea->settitle("STATUS:");
int out_area=MyAreaMenager.insert(OutArea);


//USTALANIE KOLUMN DO WYDRUKU
log.insert(sca);//Licznik krokow == wersji
log.insert(sind); //liczba indywiduuw
log.insert(sinda);//liczba autotrofow
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


//Ostatnie przygotowania do wyswietlania
MyAreaMenager.maximize(out_area);
return;
}
}
/*
unsigned MINIMALNY_ROZMIAR_TAKSONU=100;  //Arbitralny rozmiar klonu zeby go uznac za zarejestrowany takson
unsigned REJESTROWANY_ROZMIAR_TAKSONU=MINIMALNY_ROZMIAR_TAKSONU;//Domyslna wartosc kasowania w wizualizacji drzewa
enum Kolorowanie {K_SPECJALIZACJA=1,K_TROFIA=2,K_OBRONA=3}      */
void swiat::ZmienKoloryDrzewaFil(klad::Kolorowanie co)
{
	filogeneza.ChangeLineWeightsSource(co);
}

void swiat::UstalDetalicznoscSieci(int Ile,bool Relative)
{
	this->trophNet.set_tax_size_tres(Ile);//Relative jest ignorowane. Na razie?
}

void swiat::ZapiszFilogeneze(ostream& out,unsigned min_time,unsigned max_time,unsigned size_tres)
{
	//Tu byæ mo¿e trzeba cos dodaæ, ale na razie tak - bez kontroli
	//...
	//Informacje nag³owkowe
	out<<lang("ISTOTNIEJSZE LINIE FILOGENETYCZNE","IMPORTANT FILOGENETIC LINES");
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

/* IMPLEMENTACJA CZESCI NIEZALEZNEJ OD PLATFORMY */
/*************************************************/

//DEFINICJA KONSTRUKTORA
swiat::swiat(size_t szerokosc,char* logname,char* mappname,my_area_menager& AreaMenager):
	MyAreaMenager(AreaMenager),
	Sources(255),
	zdatnosc(szerokosc,(szerokosc/2),128/*INICJALIZACJA*/),
	ziemia(szerokosc,(szerokosc/2)/*DOMYSLNA INICJALIZACJA KONSTRUKTOREM*/), //Prostokat np 20x10
	stats(szerokosc,(szerokosc/2)),
	DLUG_WIERSZA(szerokosc),
	OutArea(NULL),		//Obszar statusu
	FilogeneticTree(NULL),	//Drzewo filogenetyczne
	TrophicNet(NULL),		//Sieæ zaleznoœci pokarmowych
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
{//!!!Nie mozna tu jeszcze polegac na wirtualnych metodach klasy swiat
if(mappname)
    zdatnosc.init_from_bitmap(mappname);
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
lang("ROZMIAR_SWIATA=\t","WORLD_SIZE=\t")<<DLUG_WIERSZA<<'x'<<DLUG_WIERSZA/2<<'\n'<<
lang("MAPA=\t","THE_MAPP=\t")<<MappName<<'\n'<<
lang("UPOSAZENIE_POTOMSTWA=\t","DOWRY_OF_OFFSPRING=\t")<<WYPOSAZENIE_POTOMSTWA<<'\n'<<
lang("EFEKTYWNOSC_AUTOTROFA=\t","AUTOTROPHS_EFFECTIVENESS=\t")<<EFEKTYWNOSC_AUTOTROFA<<'\n'<<
lang("MAX_WIEK=\t","MAX_AGE=\t")<<(255-MINIMALNY_WIEK)<<'\n'<<
lang("PLODNOSC=\t","FERTILITY=\t")<<1./NIEPLODNOSC<<'\n'<<
	"RTG=\t"<<1./PROMIENIOWANIE<<'\n'<<
lang("PRAWDOPODOBIENSTWO_KATASTROF=\t","LIKELIHOOD_OF_DISASTERS=\t")<<WSP_KATASTROF<<(WSP_KATASTROF<=0?"(disabled)":"")<<'\n'<<
lang("EKSPLOATACJA DRAPIEZNICZA=\t","PREDATORY EXPLOITATION=\t")<<(PIRACTWO?lang("TAK","YES"):lang("NIE","NO"))<<'\n'<<
lang("DYSTANS_RUCHU=\t","MAX_MOVE=\t")<<DYS_RUCHU<<'\n'<<      //Dystans ruchu w jednym kierunku
lang("BIT_STABILIZACJI_RUCHU=\t","DONT_MOVE_BIT=\t")<<BIT_RUCHU<<'\n'<<		//Poza maske - bez mozliwosci utraty ruchliwosci
lang("ELASTYCZNE_MIJANIE=\t","FLEXIBLE_MOVING=\t")<<(ZAMIANY?lang("TAK","YES"):lang("NIE","NO"))<<'\n'<<        //Czy moze przeciskac sie na zajete pola
"MONTECARLO STEPS per BIG STEP=\t"<<MonteCarloMultiplic<<endl;
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
if(w.w.oslona==0 || !(sila!=0 && wiek!=0) )
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

    klon=new informacja_klonalna(NULL,w);//Klon bez 'rodzica'
}

//Operacje na licznikach klonow i taksonow
klon->dolicz_indywiduum();//Klonalny licznik indywiduów
ile_ind++;                //Globalny licznik indywiduów

base2 iwfull=w.w.geba*256+w.w.oslona;
liczniki[iwfull]++;                                            assert(*(liczniki+iwfull));//Tablica dla aktualnych 'nisz ekologicznych'

if(liczniki[iwfull]>max)
		{
		max=liczniki[iwfull];
		max_change=1;
		}

if( liczniki[iwfull]==1 ) // pierwszy przedstawiciel taxonu
		ile_tax++;   // wiec liczba klonów wzrasta

if( liczniki[iwfull]==informacja_klonalna::tresh_taksonow()+1 ) // osiagnal w góre wartosc == duzy taxon - rozwojowy
		ile_big_tax++;

return 1;
}

int agent::kill()
// Kazde zabicie agenta
{
assert( w._full>0 );

//DEREJESTRACJA AGENTA
ile_ind--;
base2 wfull=w.w.geba*256+w.w.oslona;//Zeby bylo niezalezne od 'endian'
liczniki[wfull]--;
if( liczniki[wfull]==0 )	//ostatni przedstawiciel tego taxonu
	ile_tax--;
if( liczniki[wfull]==informacja_klonalna::tresh_taksonow()) // osiagnal w dól wartosc == tresh_kasowania --> maly taxon
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
            klon=new informacja_klonalna(rodzic.klon,w);//Historia filogenezy. Efemeryczne i bezpotomne klony moga byc czasem usuwane
        }
        else
        {
            klon=rodzic.klon;
        }

		return init(w.w.geba,w.w.oslona,uposazenie);   // prawdziwa inicjacja i wywolanie klon->dolicz_indywiduum()
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

assert(zabojca.sila!=0);//I zabojca nie powinien 'umrzec z przejedzenia' (przekret licznika)

if(sila==0) //Czy zjedzony w calosci
	return kill();
	else
	return 0;
}

int agent::uplyw_czasu(float IloscSwiatla)
// prawo czasu - wszystko sie starzeje i traci energie
{
assert(sila>0);
assert(w.w.oslona>0);
assert(w._full>0);

wiek++;	       // Normalne starzenie sie

if(w.w.geba==AUTOTROF) //JEST  A U T O T R O F E M
	{
	sila+=unsigned(IloscSwiatla*EFEKTYWNOSC_AUTOTROFA-1);// bez zutu gnu sie czepia a tak ma byc!
	assert(sila!=0);
	}
	else
	sila--;// Metaboliczne zurzycie energii

if(sila==0 || wiek==0) //Czy CALKIEM brak sily, lub zycie sie 'przekrecilo'
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
{ int x,y; };

void swiat::krok()
{
stats.Reinitialise();//Czyszczenie lokalnej statystyki

if(WSP_KATASTROF!=0)
	kataklizm(); // najwyżej jeden, chocby bardzo bardzo maly na krok symulacji

long ile= long( sqr(double(DLUG_WIERSZA))/2.0*MonteCarloMultiplic ); // ile na krok MonteCarlo. /2.0 bo to prostokat a nie kwadrat
licznik_krokow_w++;//Kolejny krok symulacji
monte_carlo_licz+=MonteCarloMultiplic;//Licznik kroków MonteCarlo - bezwzglêdna jednostka czasu

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
		continue;			//obrob nastepnego

	if(!Ziemia(x,y).jest_zywy() )//jest martwy
		continue;		         //obrob nastepnego

	if(Ziemia(x,y).uplyw_czasu(AktualneSwiatlo)==1)    //Koszty czasu zycia
		continue;				 //Zginal ze starosci

	//Losowanie sasiedniego miejsca
	unsigned	a=0,licznik=0;
	int			x1,y1;
	do{ //Losowanie kierunku ruchu - ??? mozna by trochê przyœpieszyæ ale chyba tylko trochê...
	ZNOWU:
		vector2 dxy={DYS_RUCHU-RANDOM(DYS_RUCHU*2+1),DYS_RUCHU-RANDOM(DYS_RUCHU*2+1)};
		if(dxy.x==0 && dxy.y==0) goto ZNOWU;//Musi byc jakies przesuniecie
		if((licznik++)==8) goto JESZCZE_RAZ;//Wyskok kontynuujacy petle zewnetrzn¹
											//gdy nie mozna trafic w cos zdatnego
		x1=(x+dxy.x);
		y1=(y+dxy.y);
	}while(zdatnosc.get(x1,y1)==0);//Dopuki nie wylosujemy zdatnego pola

	if(!Ziemia(x1,y1).jest_zywy()) //Jesli wylosowane pole jest wolne
	   {//------------------------------------------------------------
		if(RANDOM(NIEPLODNOSC)==0)  //Proponuje sie rozmnazac
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
			   //wzor v=Ziemia(x,y).w;//Debug only
			   //wzor w=Ziemia(x1,y1).w;

			   unsigned long kogo=Ziemia(x1,y1).klon->identyfikator();//Trzeba zapamietac bo obiekt x1-y1 jak i jego klon moze zostac zniszczony
			   unsigned energy=0;   //Na energie podana przez procedury zjadania

			   if(PIRACTWO)
				   Ziemia(x1,y1).kill(Ziemia(x,y),energy);	//Zabicie sasiada
			   else
				   Ziemia(x1,y1).parasit(Ziemia(x,y),energy);	//Eksploatacja sasiada

			   if(MyAreaMenager.trace_trophic_network_enabled())
				   Ziemia(x,y).klon->dolicz_zezarcie(kogo,energy);

			   stats.get(x,y).sE+=energy;
			   stats.get(x,y).N++;
		   }
		   else //Jesli nie moze go uszczknac to moze sie z nim zamienic na miejsca
		   if(ZAMIANY && !(Ziemia(x1,y1).w.w.oslona&BIT_RUCHU ))// ... o ile nie jest on 'przyrosniety'
		   {
			  ziemia.swap(x1,y1,x,y);
		   }
	   }
	continue;	//NASTEPNY NAWROT PETLI
	JESZCZE_RAZ://AWARYJNE WYJSCIE
		   ;
	}


	//Koniec kroków monte-carlo po agentach
	//------------------------------------------  NIE W TYM MIEJSCU!!!



	//AKCJE DO ZROBIENIA PO KROKU SYMULACJI
	////////////////////////////////////////////////////////////////////////////
	//Uaktualnienie informacji o frakcjach autotrofow
	klon_auto=0;
	tax_auto=0;
	autotrofy=0;
	for(unsigned i=0xffff;i>0xffff-0x100;i--)//Ile niezerowych klonow autotroficznych
	{
		unsigned pom=agent::liczniki[i];
		autotrofy+=pom;
		if(pom>0)
			klon_auto++;
		if(pom>informacja_klonalna::tresh_taksonow())
			tax_auto++;
	}

	if(Liczniki)
		Liczniki->setminmax(0,agent::max);//Oszczedza liczenia max

	Sources.new_data_version(1,1);//Oznajmia seriom ze dane sie uaktualnily

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

double poison(int n); // generuje rozklad para-poison w zakresie 0..1 o n stopniach

void  swiat::kataklizm(void)
{
double power;
int x,y,r;
if(WSP_KATASTROF<=0)
   return; //spontaniczne katastrofy wylaczone
x=RANDOM(DLUG_WIERSZA);
y=RANDOM(DLUG_WIERSZA/2);
power=poison(WSP_KATASTROF);						assert(power>=0 && power<=1);
r=power*DLUG_WIERSZA/2;
if(r>1)
	{
	cerr<<endl<<lang("Katastrofa w punkcie ","Disaster occured at ")<<"("<<x<<","<<y<<") r="<<r<<endl;
	ziemia.clean_circle(x,y,r);
	}
}



//IMPLEMENTACJA WIZUALIZACJI Z UZYCIEM MENAGERA
//--------------------------------------------------

void  swiat::wskazniki(void)
{
//* LOSOWANIE SKAMIENIALOSCI
unsigned x=RANDOM(DLUG_WIERSZA);
unsigned y=RANDOM(DLUG_WIERSZA/2);

if(this->monte_carlo_licz%LogRatio==0)      //Czy tu?????????????????????
		log.try_writing();

char bufor[2048];//Bufor jest co prawda ze sporym zapasem
bufor[2047]='A';//Mo¿e zostaæ albo A albo NULL ale jak coœ innego to przepelniony

sprintf(bufor,lang("%ld KROK MONTE CARLO [%lu - symulacji]   PID=%lu\n"
							"LICZBA AGENTOW:%lu "
							"(%lu AUTOTROFICZNYCH)\n"
							"LICZBA EKO-GATUNKÓW:%lu "
							"(%lu AUTOTROFICZNYCH)\n"
							"LICZBA KLONOW:%lu "
							"(%lu AUTOTROFICZNYCH)\n"
							"ILE KLONÓW OD POCZATKU:%lu "
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
	(unsigned long)_getpid(), //ISO C++ standard name
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
	OutArea->clean();//Stare linie usuwamy - czyste okno
	OutArea->add_text(bufor);
	}

}

/*  OGOLNA FUNKCJA MAIN */
/************************/
int parse_options(const int argc,const char* argv[]);

int main(const int argc,const char* argv[])
{
printf(lang("CO-EWOLUCJA: program symulujacy kooewolucje wielu gatunkow (do 65000)\n","CO-EVOlution: this program simulate co-evolution of many species (up to 65000)\n"));
printf(ProgramName);
printf("\nCompilation: " __DATE__ " " __TIME__ );

if((sizeof(base)*2)!=sizeof(base2))
	{
	fprintf(stderr,"Niewlasciwe rozmiary dla typow bazowych:2*%u!=%u\n",
		sizeof(base),sizeof(base2));
	exit(1);
	}
printf(lang("\nROZMIAR GENOMU: %uB\nLICZBA MOZLIWYCH KLONOW=%lu\nMAXINT=%d\n",
			"\nSIZE OF GENOM: %uB\nPOSSIBLE CLONES=%lu\nMAXINT=%d\n"),sizeof(bity_wzoru),(unsigned long)MAXBASE2,INT_MAX);

RANDOMIZE();
if(!parse_options(argc,argv))
		exit(1);

my_area_menager Lufciki(255,SWIDTH,SHEIGHT,28);//255 mo¿liwych lufcików
swiat& tenSwiat=*new swiat(IBOKSWIATA,LogName,MappName,Lufciki);
if(&tenSwiat==NULL)
{
	fprintf(stderr,lang("Brak pamieci!\n","Not enought memory!\n"));
	exit(1);
}
Lufciki.connect(&tenSwiat);

if(!Lufciki.start(ProgramName,argc,argv,1/*Buffered*/))
{
	printf("%s\n",lang("Nie moge wystartowac grafiki","Can't initialize graphics"));
    exit(1);
}

tenSwiat.inicjuj();
tenSwiat.tworz_lufciki();

//Utworzenie sensownej nazwy pliku(-ów) do zrzutow ekranu
{
wb_pchar buf(strlen(SCREENDUMPNAME)+20);
buf.prn("%s_%s",SCREENDUMPNAME,tenSwiat.dumpmarker.get());
Lufciki.set_dump_name(buf.get());
printf(lang("\nPID procesu: %u Marker zrzutów ekranu: %lu","\nProcess PID: %u Dump files marker: %lu"),_getpid(),tenSwiat.dumpmarker.get());
}

//zamiast Lufciki.run_input_loop();
cerr<<endl;
while(tenSwiat.daj_kroki_monte_carlo()<MAX_ITERATIONS) //Do za³o¿onego koñca
	{
	wb_cpu_clock fulltimer;
	tenSwiat.wskazniki();//Aktualizacja informacji
	Lufciki._replot();//Odrysowanie
	Lufciki.flush();
	Lufciki.reset_dump_number(tenSwiat.daj_kroki_monte_carlo()		);
	Lufciki.process_input();//Obsluga zdarzen zewnetrznych

	if(!Lufciki.should_continue())
				break;//Albo koniec zabawy wymuszony "klikiem"

	wb_cpu_clock steptimer;
	tenSwiat.krok();  //Kolejny krok symulacji
	double czas_kroku=steptimer;

	 //Ci¹g³e zrzuty plików jesli jest postrzeba
	 if(DumpScreenContinously)  //Obrazek ekranu
	 {
		Lufciki.dump_screen();
	 }
	 if(DumpNETContinously)	 //stan sieci troficznej w formacie NET
	 {
		wb_pchar bufor(1024);
		bufor.prn("%s_%07dMC.net",Lufciki.get_dump_name(),tenSwiat.daj_kroki_monte_carlo());
		ofstream netstateout(bufor.get());
		tenSwiat.ZapiszEkologieNET(netstateout);//Bez ograniczeñ, tzn. z domyœlnymi
		netstateout.close();
	 }
	 if(DumpVNAContinously)	 //stan sieci troficznej w formacie VNA
	 {
		wb_pchar bufor(1024);
		bufor.prn("%s_%07dMC.vna",Lufciki.get_dump_name(),tenSwiat.daj_kroki_monte_carlo());
		ofstream netstateout(bufor.get());
		tenSwiat.ZapiszEkologieVNA(netstateout);//Bez ograniczeñ, tzn. z domyœlnymi
		netstateout.close();
	 }

	//OSTATECZNIE POMIAR CZASU OBROTU PÊTLI I NOWA PÊTLA
		cout<<'\r'<<'\t'<<tenSwiat.daj_kroki_monte_carlo()<<"MC "<<tenSwiat.daj_licznik_krokow()
			<<lang(" krok. Czas:\t","-th step. Time:\t")<<double(fulltimer)
			<<lang("s. w tym symul.:\t","s. but simulation:\t")<<double(czas_kroku)<<" s.  ";
	}

//Ostateczne zrzuty plików do analizy
//Pelna filogeneza
ofstream filogout("lastfilog.out");
tenSwiat.ZapiszFilogeneze(filogout);//Bez ograniczeñ, tzn. z domyœlnymi
filogout.close();

//Ostatni stan sieci troficznej
ofstream netstateout("lasttrophnet.net");
tenSwiat.ZapiszEkologieNET(netstateout);//Bez ograniczeñ, tzn. z domyœlnymi
netstateout.close();
netstateout.open("lasttrophnet.vna");
tenSwiat.ZapiszEkologieVNA(netstateout);//Bez ograniczeñ, tzn. z domyœlnymi
netstateout.close();

//Dealokacja swiata wraz ze wszystkimi skladowymi
cerr<<endl;
delete &tenSwiat;
printf(lang("Bye, bye!","Do widzenia!!!\n"));
return 0;
}

double poison(int n)
// generuje bardzo skosny rozklad w zakresie 0..1 o n "stopniach" skosnosci
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
unsigned agent::ile_big_tax=0;// ile taxonow liczniejszych niz informacja_klonalna::tresh_taksonow()
const /*unsigned long*/size_t TAXNUMBER=(unsigned long)MAXBASE2+1;
unsigned agent::liczniki[ /*TAXNUMBER*/MAXBASE2+1 ];// Liczniki liczebnosci taxonow
								//Musi miec mozliwosc posiadania 0xffff+1 elementow

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



//Reimplementacja start() zeby obslugiwac wyspecjalizowane menu
int my_area_menager::start(const char* wintitle,int argc,const char** argv,int double_buffering)
//Zwraca 1 jesli ok
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

int my_area_menager::_pre_process_input(int input_char)
//Przed obsluga domyslna. Zwraca 1 jesli obsluzyl.
{
										assert(MojSwiat!=NULL);
static bool firs_change_viewfreq=true;	//Do ostrze¿enia przy pierwszej
								//zmianie czêstotliowosci wizualizacji
	switch(input_char){
	case ID_VIEWOPT_DUMPCO:     //Sta³e zrzucanie grafiki
		{
			DumpScreenContinously=!DumpScreenContinously;
			ssh_menu_handle menu=ssh_main_menu();
			ssh_menu_mark_item(menu,DumpScreenContinously,ID_VIEWOPT_DUMPCO);
			ssh_realize_menu(menu);
		}
		return 1;
	case ID_VIEWOPT_DMPCNET:		//Sta³e zrzucanie sieci troficznej
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
                <<lang("Czestosc wizializacji zmieniona na ","Visualisation freqency changed into ")<<MonteCarloMultiplic<<endl;
			if(firs_change_viewfreq)
				{cerr<<lang("WIZUALIZACJA SERII MOZE BYC NIESPOJNA!!!","TIME SERIES VISUALISATION MAY BE INCONSISTENT!!!")<<endl;
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
                <<lang("Czestosc wizializacji zmieniona na ","Visualisation freqency changed into ")<<MonteCarloMultiplic<<endl;
			if(firs_change_viewfreq)
				{cerr<<lang("WIZUALIZACJA SERII MOZE BYC NIESPOJNA!!!","TIME SERIES VISUALISATION MAY BE INCONSISTENT!!!")<<endl;
				firs_change_viewfreq=false;}
		}
		return 1;
	case ID_VIEWOPT_DMPCVNA:      	//Sta³e zrzucanie sieci troficznej w formacie VNA
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
		  cerr<<endl<<lang("Granica zauważania wêz³ów sieci troficznej podwyższona do ",
						   "Important nodes of food web are now from ")<<REJESTROWANY_ROZMIAR_WEZLA<<endl;
		  MojSwiat->UstalDetalicznoscSieci(REJESTROWANY_ROZMIAR_WEZLA);
		}
		return 1;
//	#define ID_VIEWOPT_TRNETFULL            60012
	   case ID_VIEWOPT_TRNETFULL:
		{
		  REJESTROWANY_ROZMIAR_WEZLA=0;
		  cerr<<endl<<lang("Zniesiona granica zauważania wêz³ów sieci troficznej",
						   "Now all nodes of food web are important")<<endl;
		  MojSwiat->UstalDetalicznoscSieci(REJESTROWANY_ROZMIAR_WEZLA);
		}
		return 1;
//	#define ID_VIEWOPT_TRNETDEFA			60013
		case ID_VIEWOPT_TRNETDEFA:
		{
		  REJESTROWANY_ROZMIAR_WEZLA=REJESTROWANY_ROZMIAR_TAKSONU-1;
		  cerr<<endl<<lang( "Domyœlna granica zauważania wêz³ów sieci troficznej (taka jak w drzewie fil.=",
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
			MojSwiat->ZapiszEkologieNET(netstateout);//Bez ograniczeñ, tzn. z domyœlnymi
			netstateout.close();
			bufor.prn("%s_%07dMC.vna",this->get_dump_name(),MojSwiat->daj_kroki_monte_carlo());//printf
			netstateout.open(bufor.get());
			MojSwiat->ZapiszEkologieVNA(netstateout);//Bez ograniczeñ, tzn. z domyœlnymi
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


int parse_options(const int argc,const char* argv[])
{
const char* pom;
//koszty();
for(int i=1;i<argc;i++)
    {
	if( *argv[i]=='-' ) /* Opcja X lub symshella */
		continue;
	if((pom=strstr(argv[i],"WIDTHWIN="))!=NULL) //Nie NULL czyli jest
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
	if((pom=strstr(argv[i],"HEIGHTWIN="))!=NULL) //Nie NULL czyli jest
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
	if((pom=strstr(argv[i],"SIDE="))!=NULL) //Nie NULL czyli jest
	{
	IBOKSWIATA=atol(pom+4);
	if(IBOKSWIATA<20 || IBOKSWIATA>=SWIDTH)
		{
		cerr<<"SIDE="<<IBOKSWIATA<<lang(" !!! Jest za ma³y lub wiêkszy niż deklarowany ekran!\n",
				   "!!! It is too small or wider than screen")<<IBOKSWIATA;
		cerr<<lang("Wciœnij ENTER jeœli to akceptujesz lub przerwij program!",
				   "Press ENTER if it is acceptable or break the program");
		getchar();
		}
		else
		cerr<<lang("D³ugi bok (SIDE) œwiata ustawiony na: ",
				   "The longer side of simulation world SIDE=")<<IBOKSWIATA<<endl;
	}
	else
	//"FLEX=y/n - czy mo¿na zamieniaæ siê z w³aœcicielem komorki\n"
	//const unsigned	ZAMIANY=255;//Czy moze przeciskac sie na zajete pola (0 na nie lub inna liczba na tak)
	if((pom=strstr(argv[i],"FLEX="))!=NULL) //Nie NULL czyli jest
	{
	ZAMIANY=atol(pom+5);
	if(ZAMIANY==0)
		{
		if( (pom[5]=='y') || (pom[5]=='t') || (pom[5]=='Y') || (pom[5]=='T') || (pom[5]=='1') )
			ZAMIANY=255;
		}
	if (ZAMIANY>255)
		{
		cerr<<"FLEX="<<ZAMIANY<<lang("!!! Niew³aœciwa wartoœæ. Powinno byæ < 256",
									 "!!! Improper value, should be < 256")<<endl;
		cerr<<lang("Wciœnij ENTER jeœli to akceptujesz lub przerwij program!",
				   "Press ENTER if it is acceptable or break the program");
		getchar();
		}
	else
		cerr<<lang("Elastyczne wymijanie - ","Flexible agent moving - ")
			<<"FLEX: "<<ZAMIANY==0?lang("nie","no"):lang("tak","yes");
	}
	else
	//"RBIT=2^n - który bit odpowiada za zdolnoœæ ruchu\n"
	//const unsigned	BIT_RUCHU=1024;		// Poza maske obrony - bez mozliwosci utraty ruchliwosci
	if((pom=strstr(argv[i],"RBIT="))!=NULL) //Nie NULL czyli jest
	{
	BIT_RUCHU=atol(pom+5);
	unsigned bit=bits(BIT_RUCHU);
	if (bit>1)
		{
		cerr<<lang("Bit ruchu (RBIT) musi byæ któr¹œ potêg¹ liczby 2. Np. ",
				   "The move bit (RBIT) must by a power of two. E.g. ")
				   <<" 0,1,2,4,8,16,32,128..."<<endl;
		cerr<<lang("A podana wartoœæ ma ",
				   "But given value have ")<<bit<<lang("bitów","bits")<<endl
			<<lang("Jeœli chcesz wy³¹czyæ podaj bit spoza maski obrony",
				   "To turn off provide power larger than defence mask capability.")<<endl;
		return 0;
		}
	else
		{
		cerr<<lang("Podano wartoœæ RBIT: ","Given value of RBIT is ")<<BIT_RUCHU<<endl;
		if(MAXBASE>BIT_RUCHU)
			cerr<<(BIT_RUCHU!=0?unsigned(log(1.0*BIT_RUCHU)/log(2.0)):0)
				<<lang(" bit maski obrony jest bitem ruchu",
					   "-th bit of defence mask is responsible on moving ability")<<endl;
		else
			cerr<<lang("Bit ruchu jest poza mask¹ obrony - wszyscy agenci mają możliwość ruchu",
					   "The move bit is outside the defence mask - all agents are able to move")<<endl;
		}
	}
	else
	//MAXA=nnn - maksymalny wiek. Nie wiêkszy ni¿ 255. Ujemne oznacza œmierci losowe\n"
	//const unsigned	MINIMALNY_WIEK=205;	// Rodzi sie z tym wiekiem. Do smierci ma 255-MINIMALNY_WIEK
	if((pom=strstr(argv[i],"MAXA="))!=NULL) //Nie NULL czyli jest
	{
	unsigned MAX_WIEK=atol(pom+5);
	if(MAX_WIEK>255)
		{
		cerr<<lang("!!! Maksymalny wiek (MAXA=",
				   "!!! Maximal age of agent (MAXA=")<<MAX_WIEK
			<<lang(") nie może byæ wiêkszy niż 255!",
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
	//"IFER=NN - prawdopodobieñstwo rozmna¿ania to 1/IFER\n"
	//const unsigned	NIEPLODNOSC=5;		// Prawdopodobienstwo rozmnazania jest 1/NIEPLODNOSC
	if((pom=strstr(argv[i],"IFER="))!=NULL) //Nie NULL czyli jest
	{
	NIEPLODNOSC=atol(pom+5);
	if(NIEPLODNOSC==0 || NIEPLODNOSC>1000)
		{
			cerr<<"!!! IFER = "<<NIEPLODNOSC
				<<lang("To musi byæ liczba ca³kowita w zakresie 1..1000!",
					"Have to be an integer in the range 1-1000!")<<endl;
		  return 0;
		}
		else
		{
		cerr<<"!!! IFER = "<<NIEPLODNOSC
			<<lang("czyli prawdopodobieñstwo rozmnażania = ",
				   "ie the probability of reproduction = ")<<double(1.0/NIEPLODNOSC)<<endl;
		}
	}
	else
	if((pom=strstr(argv[i],"PRED="))!=NULL) //Nie NULL czyli jest
	{
		PIRACTWO=(pom[5]=='Y')||(pom[5]=='1')||(pom[5]=='t');
		cerr<<"PRED="<<(PIRACTWO?lang("Tak","Yes"):lang("Nie","No"))<<" ";
		if(PIRACTWO)
		  cerr<<lang("- eksploatacja drapieznicza","- predatory exploitation")<<endl;
		  else
		  cerr<<lang("- eksploatacja pasożytnicza","- parasitic exploitation")<<endl;
	}
	else
	//"POSA=0.XX - jaki u³amek sily oddane potomkowi\n"
	//const double	WYPOSAZENIE_POTOMSTWA=0.05; // jaka czesc sily oddac potomkowi 0.1 to ZA DUZO!!! {???}
	if((pom=strstr(argv[i],"DOWR="))!=NULL) //Nie NULL czyli jest
	{
	  WYPOSAZENIE_POTOMSTWA=atof(pom+5);
	  cerr<<"DOWR="<<WYPOSAZENIE_POTOMSTWA<<lang(" zapasów rodzica.","of parent reserve.")<<endl;
	  if(WYPOSAZENIE_POTOMSTWA<=0 || 1<WYPOSAZENIE_POTOMSTWA)
	  {
		 cerr<<lang("!!! LICZBA MUSI BYÆ W ZAKRESIE (0..1) !!!",
					"!!! BUT MUST BE IN RANGE (0..1) !!!")<<endl;
		 return 0;
	  }
	  else
	  if(WYPOSAZENIE_POTOMSTWA>0.1)
	  {
		 cerr<<lang("UWAGA! Wartoœæ wiêksza od 0.1 pozwala ³atwo rozmnażać siê z posagu!!!",
					"NOTE! Value greater than 0.1 allows one to easily reproducts basing on dowry!!!")<<endl;
	  }
	}
	else
	//Współczynnik mutacji na bit genomu
	if((pom=strstr(argv[i],"MUTD="))!=NULL) //Nie NULL czyli jest
	{
		PROMIENIOWANIE=atol(pom+5);
		cerr<<"MUTD="<<PROMIENIOWANIE
			<<lang("Wspó³czynnik mutacji wynosi 1/",
				"Mutation rate is 1/)")<<PROMIENIOWANIE<<" = "<<(1/PROMIENIOWANIE)<<endl;
		if(PROMIENIOWANIE<16) //Jesli mniej niz dlugosc genomu to algorytm losowania bitu zle dziala
		{
			cerr<<lang("MUTD musi być większe lub równe 16",
				   "MUTD have to be greater or equal to 16")<<endl;
			return 0;
		}
	}
	else
	if((pom=strstr(argv[i],"CATA="))!=NULL) //Nie NULL czyli jest
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
	if((pom=strstr(argv[i],"AUTO="))!=NULL) //Nie NULL czyli jest
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
		  cerr<<lang("Efektywnosc autotrofa powinna byæ w zakresie 0..0.99 ale spróbuje",
					 "Should be lower than 1 but we can try :-) ")<<endl;
		}
	}
	else
	if((pom=strstr(argv[i],"STMX="))!=NULL) //Nie NULL czyli jest
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
	if((pom=strstr(argv[i],"DIST="))!=NULL) //Nie NULL czyli jest
	{
		DYS_RUCHU=atol(pom+5);
		cerr<<lang("Maksymalny ruch agenta (DIST) ustawiony na ",
				   "Maximum movement of the agent (DIST) set to")<<DYS_RUCHU<<endl;
		if(DYS_RUCHU<=0)
		{
			cerr<<lang("Nie moze być <= 0","It can not be <= 0")<<endl;
			return 0;
		}
	}
	else
	if((pom=strstr(argv[i],"LOGR="))!=NULL) //Nie NULL czyli jest
	{
		LogRatio=atol(pom+5);
		cerr<<lang("Czestosc zapisu do logu (LOGR) ustawiona na ",
				   "The frequency of writing to the log (LOGR) set to ")<<LogRatio<<endl;
		if(LogRatio<=0)
		{
			cerr<<lang("Nie moze być <= 0","It can not be <= 0")<<endl;
			return 0;
		}
	}
	else
	//"INLO=NNNN - dlugoœæ wewnêtrznych logów\n"
	// const unsigned dlugosc_logow=50000;
	if((pom=strstr(argv[i],"INLO="))!=NULL) //Nie NULL czyli jest
	{
		dlugosc_logow=atol(pom+5);
		cerr<<lang("D³ugoœæ wewnêtrznych logów (INLO) wynosi ",
				   "Length of the internal logs (INLO) is ")<<dlugosc_logow<<endl;
		if(dlugosc_logow<100)
		{
			cerr<<lang("Nie moze być < 100","It can not be < 100")<<endl;
			return 0;
		}
	}
	else
	if((pom=strstr(argv[i],"LOGF="))!=NULL) //Nie NULL czyli jest
	{
		strcpy(LogName,pom+5);
		cerr<<lang("Plik logu (LOGF) ustawiony na ",
				   "Log file name LOGF=")<<LogName<<endl;
	}
	else
	if((pom=strstr(argv[i],"MAPF="))!=NULL) //Nie NULL czyli jest
	{
		strcpy(MappName,pom+5);
		cerr<<lang("Plik mapy (MAPF) ustawiony na ",
				   "Map file name MAPF=")<<MappName<<endl;
	}
	else
	//"DMPF=name - rdzen/pocz¹tek nazwy plików dump\n"
	//char     SCREENDUMPNAME[256]="CO-EVO-3a";
	if((pom=strstr(argv[i],"DMPF="))!=NULL) //Nie NULL czyli jest
	{
		strcpy(SCREENDUMPNAME,pom+5);
		cerr<<lang("Rdzeñ (DMPF) nazwy zrzutów graficznych: ",
				   "The core (DMPF) of screen dumps name: ")<<SCREENDUMPNAME<<endl;
	}
	else
	if((pom=strstr(argv[i],"TAXS="))!=NULL) //Nie NULL czyli jest
	{
		REJESTROWANY_ROZMIAR_TAKSONU=atol(pom+5);
		cerr<<lang("Rozmiar rejestrowanych taksonów (TAXS) ustawiono na ",
				   "Minimal size of registered taxon (TAXS) ")<<REJESTROWANY_ROZMIAR_TAKSONU<<endl;
		if(REJESTROWANY_ROZMIAR_TAKSONU<10)
		{
			cerr<<lang("!!! Ale musi byæ >=10 !!!","!!! But have to be >=10 !!!")<<endl;
			return 0;
		}
	}
	else /* Ostatecznie wychodzi ze nie ma takiej opcji */
	{
	cerr<<lang("Niezidentyfikowana opcja ","Undefined option ")<<argv[i]<<endl;
	cerr<<lang("MOZLIWE TO: (W nawiasach wartoœci domyœlne)","POSSIBLE ONES ARE: (default values in parentheses)")<<endl;
	cerr<<lang("SIDE=NN - dłuższy bok obszaru symulacji ",
			   "SIDE=NN - the longer side of simulation world")<<'('<<IBOKSWIATA<<')'<<endl;//unsigned	IBOKSWIATA=300;// FAKTYCZNIE UZYWANY BOK SWIATA
	cerr<<lang("MAPF=plik_init.gif - nazwa pliku z mapa zamieszkiwalnoœci",
			   "MAPF=init_file.gif - name of availability map file")<<'('<<MappName<<')'<<endl;//Inicjalizacja œwiata - czarne, miejsca niezamieszkiwalne
	cerr<<lang("AUTO=0.XX - efektywność autotrofów ",
			   "AUTO=0.XX - productivity of autotrophs")<<'('<<EFEKTYWNOSC_AUTOTROFA<<')'<<endl;//double	EFEKTYWNOSC_AUTOTROFA=0.5;// jaka czesc swiatla uzywa autotrof
	cerr<<lang("PRED=0/1 - gospodarka pasozytnicza versus drapieznicza ",
			   "PRED=0/1 - predatory vs. parasitic exploitation")<<'('<<PIRACTWO<<')'<<endl;//unsigned int	PIRACTWO=1;// Czy eksploatacja piracka czy pasozytnicza
	cerr<<lang("DIST=NN - maksymalny krok ruchu agenta ",
			   "DIST=NN - ")<<'('<<DYS_RUCHU<<')'<<endl;//unsigned	DYS_RUCHU=1;// Dystans ruchu w jednym kierunku
	cerr<<lang("FLEX=y/n - czy można zamieniaæ siê z w³aœcicielem komorki ",
			   "FLEX=y/n -")<<'('<<ZAMIANY<<')'<<endl;//const unsigned	ZAMIANY=255;//Czy moze przeciskac sie na zajete pola (0 na nie lub inna liczba na tak)
	cerr<<lang("RBIT=2^n - który bit odpowiada za zdolnoœæ ruchu ",
			   "")<<'('<<BIT_RUCHU<<')'<<endl;//const unsigned	BIT_RUCHU=1024;		// Poza maske - bez mozliwosci utraty ruchliwosci
	cerr<<lang("MUTD=NNN - co ile kopiowanych bitów trafia sie mutacja ",
			   "")<<'('<<PROMIENIOWANIE<<')'<<endl;//unsigned	PROMIENIOWANIE=BITS_PER_GENOM*100;// Co ile kopiowanych bitow nastepuje mutacja
	cerr<<lang("DOWR=0.XX - jaki u³amek energii oddane potomkowi ",
			   "DOWR=0.XX - ")<<'('<<WYPOSAZENIE_POTOMSTWA<<')'<<endl;//const double	WYPOSAZENIE_POTOMSTWA=0.05; // jaka czesc sily oddac potomkowi 0.1 to ZA DUZO!!! {???}
	cerr<<lang("IFER=NN - prawdopodobieñstwo rozmnażania to 1/IFER ",
			   "IFER=NN - ")<<'('<<NIEPLODNOSC<<')'<<endl;//const unsigned	NIEPLODNOSC=5;		// Prawdopodobienstwo rozmnazania jest 1/NIEPLODNOSC
	cerr<<lang("MAXA=nnn - maksymalny wiek. Nie wiêkszy niż 255. ",
			   "MAXA=nnn - ")<<'('<<255-MINIMALNY_WIEK<<')'<<endl;// Ujemne BÊDZIE oznaczaæ œmierci losowe"<<'('<<xxx<<')'<<endl;//const unsigned	MINIMALNY_WIEK=205;	// Rodzi sie z tym wiekiem. Do smierci ma 255-MINIMALNY_WIEK
	cerr<<lang("CATA=NN - wykladnik czestosci katastrof ",
			   "CATA=NN - ")<<'('<<WSP_KATASTROF<<')'<<endl;//int	WSP_KATASTROF=0;//10-100 Wykladnik rozkladu katastrof - 0 -wylaczone
	cerr<<lang("TAXS=NN - min. rozmiar taksonu rejestrowanego w drzewie ",
			   "TAXS=NN - ")<<'('<<REJESTROWANY_ROZMIAR_TAKSONU<<')'<<endl;//unsigned	 REJESTROWANY_ROZMIAR_TAKSONU=MINIMALNY_ROZMIAR_TAKSONU;//Domyslna wartosc kasowania w wizualizacji drzewa
					//unsigned int	ODWROCONY_KOSZT_OSLONY=0;//1 - czy koszty oslony sa odwrocone
					//unsigned int	ODWROCONY_KOSZT_ATAKU=0;//1 - czy koszty ataku sa odwrocone

					//"MITA=NNN - minimalny rozmiar taksonu z def.\n"//unsigned	 MINIMALNY_ROZMIAR_TAKSONU=100;  //Arbitralny rozmiar klonu zeby go uznac za zarejestrowany takson
	cerr<<lang("INLO=NNNN - dlugoœæ wewnêtrznych logów ",
			   "INLO=NNNN - ")<<'('<<dlugosc_logow<<')'<<endl;// const unsigned dlugosc_logow=50000;
	cerr<<lang("LOGF=nazwa.log - nazwa pliku z logiem ",
			   "LOGF=name.log - ")<<'('<<LogName<<')'<<endl;
	cerr<<lang("LOGR=N - czestosc zapisow do zewnetrznego logu ",
			   "LOGR=N - ")<<'('<<LogRatio<<')'<<endl;//unsigned	LogRatio=1;// Co ile krokow zapisywac do logu
	cerr<<lang("DMPF=nazwa - rdzen/pocz¹tek nazwy plików dump ",
			   "DMPF=name - ")<<'('<<SCREENDUMPNAME<<')'<<endl;//char     SCREENDUMPNAME[256]="CO-EVO-3a";
	cerr<<lang("STMX=NNNN - najwieksza mozliwa liczba krokow symulacji ",
			   "STMX=NNNN - ")<<'('<<MAX_ITERATIONS<<')'<<endl;//unsigned long	MAX_ITERATIONS=0xffffffff;// najwieksza liczba iteracji
	cerr<<lang("WIDTHWIN=,HEIGHTWIN= - rozmiary obszaru roboczego okna ",
			   "WIDTHWIN=,HEIGHTWIN= - ")<<'('<<SWIDTH<<'x'<<SHEIGHT<<')'<<endl;	//unsigned SWIDTH=1200;//750;//1200; Tez zadziala
																				//unsigned SHEIGHT=750;//550;//1000;
	return 0;
	}
	}
//POPRAWIANIE ZALEZNOSCI
if(DYS_RUCHU>IBOKSWIATA/4)
	 DYS_RUCHU=IBOKSWIATA/4;
return 1;
}
