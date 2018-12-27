/* Program symulujacy KOEWOLUCJE */
/* Kazdy osobnik ma swoj bitowy wzorzec odzywiania i bitowy wzorzec */
/* strategi oslony. Jesli ATAKOWANY.OSLONA AND ATAKUJACY.Trofia>0 to  */
/* znaczy ze atak zakonczyl sie powodzeniem.			    */
/* Osobniki rozmnazaja sie kosztem wolnego miejsca i zgromadzonej energii */
/* Ruchy wlasne, ofiara ataku, jak i moment rozmnazania wybierane sa */
/* losowo, aby nie zaciemniac modelu dodatkowymi parametrami. 	     */

#ifdef NDEBUG
const char*	   ProgramName="CO-EVOLUTION wer. 3.05b (c)1994-2008: Wojciech Borkowski [idea i realizacja]";
#else
const char*	   ProgramName="CO-EVOLUTION wer. 3.05b DEBUG (c)1994-2008 Wojciech Borkowski";
#endif

#pragma warn -aus 

//Wersja 3 - nowe mo¿liwoœci
//1. Ró¿ne dane do kolorowania drzewa filogenetycznego wybierane z menu
//2. Rozbudowa listy parametrów "wyprowadzonych" na zewn¹trz programu i usuniêcie blokady na "œwiaty" wiêksze od ekranu
//3. Dodanie miernika czasu procesora w g³ownej pêtli i wyprowadzenie wraz z krokiem na konsole
//4. 25.04.08 Usuniêcie b³edu w bibliotecznym menagerze lufcików wywalaj¹cego program przy robieniu "TILE"
//5. 11.09.08 Wypisywanie do pliku kompletnej filogenezy 
//.....

//Istotne nowosci w wersji 2.5
//1. Odwrotnosc prawdopodobienstwa mutacji wyprowadzona jako parametr RTG
//Mutacje w okolicy 1/16000 dla obszary 400x200 doprowadzaja do wyparcia heterotrofow 70000:100
//i to przy roznych wartosciach globalnosci oddzialywan.
//Przy poziomie 1/1600 jest zachowana rownowaga podobna do tej z typowego poziomu 1/160
//2. Wprowadzenie parametru wewnetrznego "ZAMIANY" umozliwiajacego "elastyczne" przeplywanie obok siebie obojetnych osobnikow
//Ma ono zapobiegac formowaniu sie frontow - czyli blokowaniu rozprzestrzeniania sie skutecznych taksonow przez "gestosc zaludnienia"
//3. Wprowadzenie maksymalnego dystansu skoku pozwala zbadac zaleznosc od "globalnosci ekologii"
//4. Dodano tez wyswietlanie wieku taksonow
//5. Dodano modu³ "kladystyka" oparty o dane z klonalinfo i s³uz¹cy preparowaniu prymitywnego (na razie) drzewa filogenetycznego
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

#define USES_RANDG
#include "INCLUDE/random.hpp" //WB random generators for "C"

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

#include "SYMSHELL/sshmenuf.h"
#include "ResSrc/coewo3rc.h"


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
#include "intersourc.hpp"//Zrodla specjalne "and" i ...
and_interaction_source	AndDemo("Trofia AND Oslona");       //Zrodla specjalne do celow demonstracyjnych
and_exploatation_source ExpDemo("(G AND O)/O * (G AND O)/G");//... tzn ladne dywaniki oddzialywan

#include "klonalinfo.hpp"
#include "troficinfo.h"
#include "kladystyka.hpp"
#include "ekologia.hpp"
#include "co_agent.hpp"

//BARDZO WA¯NE WSKA¯NIKI DO MASEK CZYLI GENÓW BÊD¥CYCH SK£ADOWYMI STRUKTURY 
//base agent::* geba_ptr=&agent::w.w.geba;      //TAK POWINNO BYC - ALE NIE KOMPILUJE
//base agent::* oslona_ptr=&agent::w.w.oslona;

//RECZNE WPISYWANIE - ZALEZNE OD "ENDIAN"
#define RECZNIE_WPISZ_PTR 

#ifdef  RECZNIE_WPISZ_PTR 
base agent::* geba_ptr=NULL;
base agent::* oslona_ptr=NULL;
#else
base agent::* geba_ptr=&agent::w.w.geba;
base agent::* oslona_ptr=&agent::w.w.oslona;
//base agent::* geba_ptr=NULL;
//base agent::* oslona_ptr=NULL;//(base agent::*)sizeof(base);
#endif    

//TAK MSVC KOMPILUJE ALE WPISUJE 0
//base agent::* geba_ptr=&agent::w.wzor::w.bity_wzoru::geba;
//base agent::* oslona_ptr=&agent::w.wzor::w.bity_wzoru::oslona;

//Wazne domyslne ustawienia
char     SCREENDUMPNAME[256]="CO-EVO-3a";
char     LogName[256]="coewo3.0a.out";
char     MappName[256]=""/*coewo2008ini.gif"*/;

//Czesto zmieniane parametry kompilacji i wykonania samego programu
unsigned SWIDTH=1200;//750;//1200; Tez zadziala
unsigned SHEIGHT=750;//550;//1000;

//const unsigned TAX_OUT=256; //PO CO TO?  zamiast REJESTROWANY_ROZMIAR_TAKSONU?

unsigned dlugosc_logow=50000; //Dlugoœæ wewnetrznych logów podrêcznych - do wizualizacji
unsigned LogRatio=1;				// Co ile krokow zapisywac do logu


//PODSTAWOWE PARAMETRY MODELU
unsigned IBOKSWIATA=300;                // FAKTYCZNIE UZYWANY BOK SWIATA
unsigned MINIMALNY_ROZMIAR_TAKSONU=100;  //Arbitralny rozmiar klonu zeby go uznac za zarejestrowany takson
unsigned REJESTROWANY_ROZMIAR_TAKSONU=MINIMALNY_ROZMIAR_TAKSONU;//Domyslna wartosc kasowania w wizualizacji drzewa

int			WSP_KATASTROF=0;//10-100		// Wykladnik rozkladu katastrof - 0 -wylaczone
unsigned	PROMIENIOWANIE=BITS_PER_GENOM*100;	// Co ile kopiowanych bitow nastepuje mutacja
double		EFEKTYWNOSC_AUTOTROFA=0.5;	// jaka czesc swiatla uzywa autotrof
unsigned	NIEPLODNOSC=5;		// Prawdopodobienstwo rozmnazania jest 1/NIEPLODNOSC
double		WYPOSAZENIE_POTOMSTWA=0.05; // jaka czesc sily oddac potomkowi 0.1 to ZA DUZO!!! {???}

unsigned	MINIMALNY_WIEK=205;	// Rodzi sie z tym wiekiem. Do smierci ma 255-MINIMALNY_WIEK
unsigned int	ODWROCONY_KOSZT_OSLONY=0;//1 - czy koszty oslony sa odwrocone
unsigned int	ODWROCONY_KOSZT_ATAKU=0;//1 - czy koszty ataku sa odwrocone

unsigned int	PIRACTWO=1;				//Czy eksploatacja piracka czy pasozytnicza
//const unsigned BIT_RUCHU=128;		//Wyzerowanie ktorych bitow oslony odpowiada za zdolnosc ruchu
unsigned	BIT_RUCHU=1024;		//Poza maske - bez mozliwosci utraty ruchliwosci
unsigned	DYS_RUCHU=1;      //Dystans ruchu w jednym kierunku					
unsigned	ZAMIANY=255;        //Czy moze przeciskac sie na zajete pola (0 na nie lub inna liczba na tak)


unsigned long	MAX_ITERATIONS=0xffffffff; // najwieksza liczba iteracji
double          MonteCarloMult=10; //Ewentualna modyfikacja multiplikacji dlugosci kroku MonteCarlo
									//... ale dlaczego double ??????
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
/////////////////////////////////
unsigned long	licznik;		// licznik krokow symulacji
unsigned long	monte_carlo_licz;//licznik krokow monte-carlo

unsigned 		klon_auto;		// Licznik klonow autotroficznych
unsigned 		tax_auto;		// Licznik taksonow autotroficznych
unsigned 		autotrofy;		// Licznik agentow autrotroficznych

size_t			DLUG_WIERSZA;	// Obwod torusa

//Warstwy symulacji (sa torusami - bo taka jest wspolna geometria)
////////////////////////////////////////////////////////////////////////
rectangle_unilayer<unsigned char> zdatnosc; //Warstwa definiujaca zdatnosc do zasiedlenia
rectangle_layer_of_agents<agent>    ziemia; //Wlaœciwa warstwa agentow zasiedlajacych
rectangle_layer_of_struct<inte_stat> stats; //Warstwa pamietajaca lokalne statystyki zdazen

//Informacje statystyczne z drzewa filogenetycznego
klad filogeneza;

//Informacje statystyczne dla sieci ekologicznej
ekologia trophNet;

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
//KONSTRUKCJA DESTRUKCJA
swiat(size_t szerokosc,
	  char* logname,   //Nazwa pliku do zapisywania histori
	  char* mappname,
	  my_area_menager& AreaMenager
	  ); //Nazwa (bit)mapy inicjujacej "zdatnosc"
~swiat(){}

//AKCJE
void init();	  // stan startowy symulacji
void krok();	  // kolejny krok symulacji
void kataklizm(); // wygenerowanie katastrofy - reczne lub losowe

//Wspolpraca z menagerem wyswietlania
//---------------------------------------------
void wskazniki(); // aktualizacja nowych wartosci wskaznikow
void tworz_lufciki();//Tworzy domyslne lufciki

//METODY POMOCNICZE
agent& Ziemia(size_t Kolumna,size_t Wiersz) {return ziemia.get(Kolumna,Wiersz);}
unsigned long	get_licznik(){return licznik;}		// licznik krokow symulacji
unsigned long	get_monte_carlo_steps() {return monte_carlo_licz;}
//Do wizualizacji
void ZmienKoloryDrzewaFil(klad::Kolorowanie co);
void UstalDetalicznoscSieci(int Ile,bool Relative=true);
//Do statystyk
void print_params(ostream& out);
void ZapiszFilogeneze(ostream& out,unsigned min_time=0,unsigned max_time=0,unsigned size_tres=0);
void ZapiszEkologie(ostream& out,unsigned size_tres=0);
};

void swiat::tworz_lufciki()
//Tworzy domyslne lufciki
{
if(0)   //SPECJALNY BLOK OBSLUGI BLEDOW
{
DO_OBSLUGI_BLEDOW: ;

	perror("Nie mozna utworzyc obiektow wizualizujacych");
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
*hack1=2;//sizeof(base)?
int *hack2=(int*)&geba_ptr;
*hack2=1;
}
#endif

{
//Zerowa seria w menagerze danych powinna byc pusta - sluzy
//do kontroli wersji danych. Menager moze tworzyc ja sam,
//ale zawsze mozna podmienic
ptr_to_scalar_source<unsigned long>* sca=new ptr_to_scalar_source<unsigned long>(&licznik,"Step#");
if(!sca) goto DO_OBSLUGI_BLEDOW;
Sources.replace(0,sca);

//Tylko ze wzgledow demonstracyjnych
array_source<unsigned>* arr=new array_source<unsigned>(MAXBASE2+1,agent::liczniki,"liczniki taksonow - liniowo");
if(!arr) goto DO_OBSLUGI_BLEDOW;
Sources.insert(arr);

//ZRODLA SUROWYCH WLASCIWOSCI AGENTÓW
Sila=ziemia.make_source("energia",&agent::sila);
if(!Sila) goto DO_OBSLUGI_BLEDOW;
Sources.insert(Sila);

Trofia=ziemia.make_source("trofia",geba_ptr);
if(!Trofia) goto DO_OBSLUGI_BLEDOW;
Sources.insert(Trofia);
Trofia->setminmax(0,255);

Oslona=ziemia.make_source("wrazliwosc",oslona_ptr);
if(!Oslona) goto DO_OBSLUGI_BLEDOW;
Sources.insert(Oslona);
Oslona->setminmax(0,255);

Wiek=ziemia.make_source("wiek",&agent::wiek);
if(!Wiek) goto DO_OBSLUGI_BLEDOW;
Wiek->setminmax(0,255);
Sources.insert(Wiek);

EDynamism=stats.make_source("Eksploatacja",&inte_stat::MeanEnergyOfInteraction);
if(!EDynamism) goto DO_OBSLUGI_BLEDOW;
Sources.insert(EDynamism);
EDynamism->set_missing(-1);

//ZRODLO METODOWE CZYTAJACE WIEK TAKSONU DO KTOREGO NALEZY AGENT 
method_matrix_source<agent,unsigned long>* WiekFilog=ziemia.make_source("WiekFilogenetyczny",&agent::how_old_taxon);
if(!WiekFilog) goto DO_OBSLUGI_BLEDOW;
//WiekFilog->setminmax(0,1);
WiekFilog->set_missing(0xffffffff);
int inWiekFilog=Sources.insert(WiekFilog);


//ZRODLO METODOWE FILTRUJACE ZYWE
method_matrix_source<agent,bool>* Zycie=ziemia.make_source("martwe/zywe",&agent::jest_zywy);
if(!Zycie) goto DO_OBSLUGI_BLEDOW;
Zycie->setminmax(0,1);
int inZycie=Sources.insert(Zycie);

GT_filter<method_matrix_source<agent,bool> >* filtrZycia=//Filtr na "Tylko Zywe"
	new GT_filter<method_matrix_source<agent,bool> >(0.5,Zycie,default_missing<double>(),"Zywe");//
if(!filtrZycia)goto DO_OBSLUGI_BLEDOW;
int inFiltrZycia=Sources.insert(filtrZycia);

//SAME ZYWE
if_then_source* GebaZywych=new if_then_source(filtrZycia,Trofia,"TrofiaZ");//Ale ma klopot z missing_val i przez to zle liczy min i max
if(!GebaZywych) goto DO_OBSLUGI_BLEDOW;
Sources.insert(GebaZywych);

if_then_source* OslonaZywych=new if_then_source(filtrZycia,Oslona,"OslonaZ");//Ale ma klopot z missing_val i przez to zle liczy min i max
if(!OslonaZywych) goto DO_OBSLUGI_BLEDOW;
//OslonaZywych->if_then_source::
Sources.insert(OslonaZywych);

if_then_source* WiekZywych=new if_then_source(filtrZycia,Wiek,"WiekZ");
if(!WiekZywych) goto DO_OBSLUGI_BLEDOW;
Sources.insert(WiekZywych);

if_then_source* WiekFilogZ=new if_then_source(filtrZycia,WiekFilog,"WiekFilo");
if(!WiekFilogZ) goto DO_OBSLUGI_BLEDOW;
Sources.insert(WiekFilogZ);

if_then_source* SilaZywych=new if_then_source(filtrZycia,Sila,"EnergiaZ");
if(!SilaZywych) goto DO_OBSLUGI_BLEDOW;
Sources.insert(SilaZywych);

//Trofia
pom=new carpet_graph(0,wys_map,szer_map-1,2*wys_map-1,//domyslne wspolrzedne
						//TrofiaZ);//I zrodlo danych
						GebaZywych);//I alternatywne zrodlo danych
if(!pom) goto DO_OBSLUGI_BLEDOW;
//pom->setdatacolors(0,255);//Pierwsze 25 kolorow bedzie slabo widoczne
pom->settitle("TROFIA AGENTÓW");
pom->settitlecolo(255,default_transparent);
pom->setbackground(default_half_gray);
MyAreaMenager.insert(pom);

//OSLONA
pom=new carpet_graph(0,2*wys_map,szer_map-1,3*wys_map-1,//domyslne wspolrzedne
//						Oslona);//I zrodlo danych
						OslonaZywych);//I alternatywne zrodlo danych

if(!pom) goto DO_OBSLUGI_BLEDOW;
//pom->setdatacolors(0,255);//Pierwsze 25 kolorow bedzie slabo widoczne
pom->settitle("PODATNOŒÆ AGENTÓW NA ATAK");
pom->settitlecolo(255,default_transparent);
pom->setbackground(default_half_gray);
MyAreaMenager.insert(pom);

//Energia
pom=new carpet_graph(MyAreaMenager.getwidth()-80,87+21+1+21+21,MyAreaMenager.getwidth()-1,87+21+1+21+21+21,					                   
//						Sila);//I zrodlo danych
                        SilaZywych);//I alternatywne zrodlo danych
pom->setdatacolors(1,254);//Zeby nie uzywac bialego, ktory jest tlem i calkiem czarnych
pom->setbackground(default_half_gray);
pom->settitle("ZASOBY ENERGII AGENTÓW");
pom->settitlecolo(255,default_transparent);
MyAreaMenager.insert(pom);

//ZRODLA O SPECJALIZACJI
method_matrix_source<agent,int>* Specjalizacja=ziemia.make_source("bity ustawione",&agent::how_specialised);
if(!Specjalizacja) goto DO_OBSLUGI_BLEDOW;
Specjalizacja->setminmax(1,16);
Specjalizacja->set_missing(-1);
int inSpecjalizacja=Sources.insert(Specjalizacja);

pom=new carpet_graph(0,3*wys_map,szer_map-1,4*wys_map,//domyslne wspolrzedne
						Specjalizacja);//I zrodlo danych
//pom->setdatacolors(1,254);//Zeby nie uzywac bialego, ktory jest tlem i calkiem czarnych
pom->setbackground(default_half_gray);
pom->settitlecolo(255,default_transparent);
pom->settitle("POZIOM SPECJALIZACJI AGENTÓW");
MyAreaMenager.insert(pom);

method_matrix_source<agent,int>* SpecjalizacjaA=ziemia.make_source("bity autotrofów",&agent::how_specialised_autotrof);
if(!SpecjalizacjaA) goto DO_OBSLUGI_BLEDOW;
SpecjalizacjaA->setminmax(1,16);
SpecjalizacjaA->set_missing(-1);
int inSpecjalizacjaA=Sources.insert(SpecjalizacjaA);

method_matrix_source<agent,int>* SpecjalizacjaH=ziemia.make_source("bity heterotrofów",&agent::how_specialised_heterotrof);
if(!SpecjalizacjaH) goto DO_OBSLUGI_BLEDOW;
SpecjalizacjaH->setminmax(1,16);
SpecjalizacjaH->set_missing(-1);
int inSpecjalizacjaH=Sources.insert(SpecjalizacjaH);

//HISTORIA OSTATNICH <dlugosc_logow> KROKOW
template_scalar_source_base<unsigned long>* allclons=new ptr_to_fuction_source<unsigned long>(informacja_klonalna::ile_klonow,"klonów od poczatku");
if(!allclons) goto DO_OBSLUGI_BLEDOW;
Sources.insert(allclons);
fifo_source<unsigned long>* flpom=new fifo_source<unsigned long>(allclons,dlugosc_logow);
if(!flpom)goto DO_OBSLUGI_BLEDOW;
int inAllclons=Sources.insert(flpom);

template_scalar_source_base<unsigned long>* alltax=new ptr_to_fuction_source<unsigned long>(informacja_klonalna::ile_taksonow,"taksonów od poczatku");
if(!alltax) goto DO_OBSLUGI_BLEDOW;
Sources.insert(alltax);
flpom=new fifo_source<unsigned long>(alltax,dlugosc_logow);
if(!flpom)goto DO_OBSLUGI_BLEDOW;
int inAlltax=Sources.insert(flpom);

template_scalar_source_base<unsigned>* staxa=new ptr_to_scalar_source<unsigned>(&tax_auto,"taksony autotroficzne");// ile jest autotroficznych taksonow (liczniejszych niz treshold)
if(!staxa) goto DO_OBSLUGI_BLEDOW;
Sources.insert(staxa);

fifo_source<unsigned>* fpom=new fifo_source<unsigned>(staxa,dlugosc_logow);
if(!fpom) goto DO_OBSLUGI_BLEDOW;
int fiftaxa=Sources.insert(fpom);

template_scalar_source_base<unsigned>* sklona=new ptr_to_scalar_source<unsigned>(&klon_auto,"klony autotroficzne");// ile klonow autotroficznych
if(!sklona) goto DO_OBSLUGI_BLEDOW;
Sources.insert(sklona);

fpom=new fifo_source<unsigned>(sklona,dlugosc_logow);
if(!fpom) goto DO_OBSLUGI_BLEDOW;
int fifklona=Sources.insert(fpom);

template_scalar_source_base<unsigned>* stax=new ptr_to_scalar_source<unsigned>(&agent::ile_big_tax,"taksony");// ile taxonow liczniejszych niz niz treshold
if(!stax) goto DO_OBSLUGI_BLEDOW;
Sources.insert(stax);

fpom=new fifo_source<unsigned>(stax,dlugosc_logow);
if(!fpom) goto DO_OBSLUGI_BLEDOW;
int fiftax=Sources.insert(fpom); 

template_scalar_source_base<unsigned>* sklon=new ptr_to_scalar_source<unsigned>(&agent::ile_tax,"klony");// ile w ogóle istniejacych klonow
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
template_scalar_source_base<unsigned>* sind=new ptr_to_scalar_source<unsigned>(&agent::ile_ind,"liczba agentow");// ile agentow niezerowych
if(!sind) goto DO_OBSLUGI_BLEDOW;
Sources.insert(sind);

fpom=new fifo_source<unsigned>(sind,dlugosc_logow);
if(!fpom) goto DO_OBSLUGI_BLEDOW;
int fifagents=Sources.insert(fpom);

template_scalar_source_base<unsigned>* sinda=new ptr_to_scalar_source<unsigned>(&autotrofy,"liczba autotrofow");// ile agentow autotroficznych
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
										fifagents,fifautoagents,
										-1
										).get_ptr_val(),
							   1/*Wspolne minimum/maximum*/);
if(!pom1) goto DO_OBSLUGI_BLEDOW;
pom1->setframe(128);
pom1->settitle("AGENCI - HISTORIA LICZEBNOŒCI");
MyAreaMenager.insert(pom1);

//Historia sily
//szer_map+301,300,MyAreaMenager.getwidth()-1,400,

//Historia liczby taksonow
pom1=new sequence_graph(szer_map,401,MyAreaMenager.getwidth()-1,MyAreaMenager.getheight()-1,
							   5,Sources.make_series_info(
										fiftaxa,fifklona,
										fiftax,fifklon,
													-1
										).get_ptr_val(),
							   1/*Wspolne minimum/maximum*/);
if(!pom1) goto DO_OBSLUGI_BLEDOW;
pom1->setframe(128);
pom1->settitle("HISTORIA LICZBY TAKSONÓW");
MyAreaMenager.insert(pom1);


//WIZUALIZACJA FILTRU ZYCIA
pom=new carpet_graph(MyAreaMenager.getwidth()-80,0,MyAreaMenager.getwidth()-1,20,//domyslne wspolrzedne
						filtrZycia);//I zrodlo danych
if(!pom) goto DO_OBSLUGI_BLEDOW;
pom->setdatacolors(50,254);//Potrzebne tylko dwa kolory naprawde
pom->setframe(32);
pom->setbackground(default_half_gray);
pom->settitlecolo(255,default_transparent);
pom->settitle("ZYWE");
MyAreaMenager.insert(pom);

//UDOSTEPNIANIE I WYSWIETLANIE BACKGROUNDU SYMULACJI
rectangle_source_base* Background=zdatnosc.make_source("Mapa zdatnych obszarów");
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
pom->settitle("SIEDLISKO");
MyAreaMenager.insert(pom);

//Demo interakcji AND
pom=new carpet_graph(MyAreaMenager.getwidth()-80,43,MyAreaMenager.getwidth()-1,64,//domyslne wspolrzedne
						&AndDemo);//I zrodlo danych
if(!pom) goto DO_OBSLUGI_BLEDOW;
//pom->setdatacolors(0,255);//Potrzebne tylko dwa kolory naprawde
pom->setframe(64);
pom->settitle("AND(Trofia,Oslona)");
MyAreaMenager.insert(pom);

//Demo exploatacji potenclalnej (AND*K)
pom=new carpet_graph(MyAreaMenager.getwidth()-80,65,MyAreaMenager.getwidth()-1,86,//domyslne wspolrzedne
						&ExpDemo);//I zrodlo danych
if(!pom) goto DO_OBSLUGI_BLEDOW;
//pom->setdatacolors(0,255);//Potrzebne tylko dwa kolory naprawde
pom->setframe(64);
pom->settitle("Eksploatacja potencjalna");
MyAreaMenager.insert(pom);

//Wiek
pom=new carpet_graph(MyAreaMenager.getwidth()-80,87,MyAreaMenager.getwidth()-1,87+21,//domyslne wspolrzedne
						Wiek);//I zrodlo danych
pom->setdatacolors(0,255);//Pierwsze 25 kolorow bedzie slabo widoczne
pom->setframe(32);
pom->setbackground(default_half_gray);
pom->settitlecolo(255,default_transparent);
pom->settitle("WIEK AGENTÓW");
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
							    3,Sources.make_series_info(
								MeanSilaFifoIndex,SdSilaFifoIndex,MaxSilaFifoIndex,
													-1
										).get_ptr_val(),
								0/* Z reskalowaniem */);
							   //1/*Wspolne minimum/maximum*/);
if(!pom1) goto DO_OBSLUGI_BLEDOW;
pom1->setframe(128);
pom1->settitle("HISTORIA ENERGII AGENTÓW");
MyAreaMenager.insert(pom1);

//Lokalne przeplywy energii
pom=new carpet_graph(MyAreaMenager.getwidth()-80,87+21+1+21,MyAreaMenager.getwidth()-1,87+21+1+21+21,
						EDynamism);//I zrodlo danych
pom->setdatacolors(0,255);//Pierwsze 25 kolorow bedzie slabo widoczne
//pom->setframe(32);
pom->setbackground(default_half_gray);
pom->settitlecolo(255,default_transparent);
pom->settitle("LOKALNY PRZEP£YW ENERGII");
MyAreaMenager.insert(pom);

//Dywanowa mapa uzywanych nisz ekologicznych
/////////////////////////////////////////////////////
Liczniki=new matrix_source<unsigned>("liczebnosc taksonow",
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
pom->settitle("MAPA U¯YWANYCH NISZ EKOLOGICZNYCH");
MyAreaMenager.insert(pom);

//Wiek filogenetyczny klonow
////////////////////////////////////////////////////
pom1=new carpet_graph(0,0,szer_map-1,wys_map-1,//domyslne wspolrzedne
					  //MyAreaMenager.getwidth()-80,87+21+1+21+21,MyAreaMenager.getwidth()-1,87+21+1+21+21+21,					
						WiekFilogZ);//I zrodlo danych
pom1->setdatacolors(0,255);//Pierwsze 25 kolorow bedzie slabo widoczne
pom1->setbackground(default_half_gray);
pom1->settitlecolo(255,default_transparent);
pom1->settitle("FILOGENETYCZNY WIEK AGENTÓW");
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
pom1->settitle("PRZYROST DRZEWA FILOGENETYCZNEGO");
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
				"HISTOGRAM(%s[%d..%d])",//format
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
pom->settitle("SPECJALIZACJA AUTOTROFÓW");
MyAreaMenager.insert(pom);

//HISTOGRAM SPECJALIZACJI HETEROTROFÓW
/////////////////////////////////////////////////////////////////
generic_discrete_histogram_source*  histspecjH=
		new generic_discrete_histogram_source(
				1,16,
				SpecjalizacjaH,
				"HISTOGRAM(%s[%d..%d])",//format   
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
pom->settitle("SPECJALIZACJA HETEROTROFÓW");
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
buf.prn("FILOGENEZA - taksony powy¿ej %d",REJESTROWANY_ROZMIAR_TAKSONU);
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
data_source_base*  Dummy=new function_source< constans< 5 > >(1000000,0,1000000,"5",0,10);
Sources.insert(Dummy,1);
//Sieæ zaleznoœci pokarmowych
pom=TrophicNet=new net_graph(szer_map+1,130,szer_map+169,299,
					  X,0,
					  Y,0,
					  Prey,0,
					  Pred,0,

					  W,0,
					  W,0,
					  Dummy,1,
					  CW,0,
					  new circle_point,1
					  );
pom->setframe(128);
//pom->setdatacolors(0,128);
pom->setbackground(default_light_gray);
pom->settitle("SIEÆ TROFICZNA");
MyAreaMenager.insert(pom);

//szer_map+1,130,szer_map+119,130+84
//szer_map+1,130+85,szer_map+119,299

//INFORMACJE STATYSTYCZNE I STATUSOWE
/////////////////////////////////////////////////////
OutArea=new text_area(
                      szer_map+1,0,MyAreaMenager.getwidth()-81,87+21+21,//domyslne wspolrzedne
                      "Information area",
					  default_white,default_black,128,40);
if(!OutArea) goto DO_OBSLUGI_BLEDOW;
OutArea->settitle("STATUS:");
int out_area=MyAreaMenager.insert(OutArea);


//USTALANIE KOLUMN DO WYDRUKU
//log.insert(sca);//Licznik krokow == wersji 
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
/*
void swiat::UstalDetalicznoscSieci(int Ile,bool Relative=true);
*/
void swiat::ZapiszFilogeneze(ostream& out,unsigned min_time,unsigned max_time,unsigned size_tres)
{
	//Tu byæ mo¿e trzeba cos dodaæ, ale na razie tak - bez kontroli
	//...
	//Informacje nag³owkowe
	out<<"\"IMPORTANT FILOGENETIC\" LINES";
	if(size_tres>0)
		out<<" larger than "<<size_tres<<" agents,";
	out<<" from "<<min_time<<"-th step";
	if(max_time>min_time)
		out<<" to "<<max_time<<" step";
		else
		{
			max_time=0;
			out<<" to global \"end of life\" in "<<licznik<<"-th step";
		}
	out<<endl<<"MODEL PARAMETERS:"<<endl;
	print_params(out);
	out<<endl;
	filogeneza.ZapiszTxt(out,min_time,max_time,size_tres);
}
/*
void swiat::ZapiszEkologie(ostream& out,unsigned size_tres=0);
*/

/* IMPLEMENTACJA CZESCI NIEZALEZNEJ OD PLATFORMY */
/*************************************************/

//DEFINICJA KONSTRUKTORA
swiat::swiat(size_t szerokosc,char* logname,char* mappname,my_area_menager& AreaMenager):	
	MyAreaMenager(AreaMenager), 	
	Sources(150), 
	zdatnosc(szerokosc,(szerokosc/2),128/*INICJALIZACJA*/),
	ziemia(szerokosc,(szerokosc/2)/*DOMYSLNA INICJALIZACJA KONSTRUKTOREM*/), //Prostokat np 20x10
	stats(szerokosc,(szerokosc/2)),
	DLUG_WIERSZA(szerokosc),
	OutArea(NULL),		//Obszar statusu
	FilogeneticTree(NULL),	//Drzewo filogenetyczne
	TrophicNet(NULL),		//Sieæ zaleznoœci pokarmowych
	Liczniki(NULL),
	licznik(0),
	Wiek(NULL),Sila(NULL),
	Trofia(NULL),Oslona(NULL),
	klon_auto(1),tax_auto(1),autotrofy(1),
	log(100,logname),
    filogeneza(NULL),
    trophNet(NULL)
{//!!!Nie mozna tu jeszcze polegac na wirtualnych metodach klasy swiat
if(mappname)
    zdatnosc.init_from_bitmap(mappname);
informacja_klonalna::podlacz_marker_czasu(&licznik);
informacja_klonalna::ustaw_maksimum_kasowania(REJESTROWANY_ROZMIAR_TAKSONU-1);
}

void swiat::init()
{
Ziemia(DLUG_WIERSZA/2,DLUG_WIERSZA/4).init(255,255,255);
filogeneza=Ziemia(DLUG_WIERSZA/2,DLUG_WIERSZA/4).klon;
trophNet=Ziemia(DLUG_WIERSZA/2,DLUG_WIERSZA/4).klon;

long t=time(NULL);
log.GetStream()<<"KO-EWOLUCJA ["<<t<<"]\n";
//fprintf(log,"%ux%u\tWYP_POT=%f\tEFEKT=%f\tMAX_WIEK=%d\tPLOD=%f\tRTG=%f\tBUM=0.5^%d\n",
print_params(log.GetStream());
}

void swiat::print_params(ostream& out)
{
out<<
"ROZMIAR=\t"<<DLUG_WIERSZA<<'x'<<DLUG_WIERSZA/2<<'\n'<<
"MAPA=\t"<<MappName<<'\n'<<
"WYPOSAZENIE_POTOMSTWA=\t"<<WYPOSAZENIE_POTOMSTWA<<'\n'<<
"EFEKTYWNOSC_AUTOTROFA=\t"<<EFEKTYWNOSC_AUTOTROFA<<'\n'<<
"MAX_WIEK=\t"<<(255-MINIMALNY_WIEK)<<'\n'<<
"PLODNOSC=\t"<<1./NIEPLODNOSC<<'\n'<<
"RTG=\t"<<1./PROMIENIOWANIE<<'\n'<<
"PRAWDOPODOBIENSTWO KATASTROF=\t"<<WSP_KATASTROF<<(WSP_KATASTROF<=0?"(disabled)":"")<<'\n'<<
"EKSPLOATACJA=\t"<<(PIRACTWO?"DRAPIEZNICZA":"PASOZYTNICZA")<<'\n'<<
"DYSTANS RUCHU=\t"<<DYS_RUCHU<<'\n'<<      //Dystans ruchu w jednym kierunku					
"BIT_STABILIZACJI_RUCHU=\t"<<BIT_RUCHU<<'\n'<<		//Poza maske - bez mozliwosci utraty ruchliwosci
"ELASTYCZNE MIJANIE=\t"<<(ZAMIANY?"YES":"NO")<<'\n'<<        //Czy moze przeciskac sie na zajete pola
"MONTECARLO STEPS per STEP=\t"<<MonteCarloMult<<endl;
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

    klon=new informacja_klonalna(NULL,w);//Klon bez "rodzica"   
}

//Operacje na licznikach klonow i taksonow
klon->dolicz_indywiduum();//Klonalny licznik indywiduów
ile_ind++;                //Globalny licznik indywiduów

base2 iwfull=w.w.geba*256+w.w.oslona;
liczniki[iwfull]++;                                            assert(*(liczniki+iwfull));//Tablica dla aktualnych "nisz ekologicznych"

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
base2 wfull=w.w.geba*256+w.w.oslona;//Zeby bylo niezalezne od "endian"
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
{ int x,y; };

void swiat::krok()
{
stats.Reinitialise();//Czyszczenie lokalnej statystyki
long ile= long( sqr(double(DLUG_WIERSZA))/2.0*MonteCarloMult ); // ile na krok MonteCarlo. /2.0 bo to prostokat a nie kwadrat
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
		   if(ZAMIANY && !(Ziemia(x1,y1).w.w.oslona&BIT_RUCHU ))// ... o ile nie jest on "przyrosniety"
		   {
			  ziemia.swap(x1,y1,x,y);
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
	if(pom>informacja_klonalna::tresh_taksonow())
		tax_auto++;
	}
}

if(Liczniki)
	{
	Liczniki->setminmax(0,agent::max);//Oszczedza liczenia max
	}

Sources.new_data_version(1,1);//Oznajmia seriom ze dane sie uaktualnily

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
		
char checker1[2]="AA";
char bufor[2048];//ze sporym zapasem
char checker2[2]="AA";
sprintf(bufor,"%ld KROK MONTE-CARLO [%lu - symulacji] PID=%lu\n"
							"LICZBA AGENTOW:%lu "
							"(%lu AUTOTROFICZNYCH)\n"
							"LICZBA TAKSONOW:%lu "
							"(%lu AUTOTROFICZNYCH)\n"
							"LICZBA KLONOW:%lu "	
							"(%lu AUTOTROFICZNYCH)\n"
                            "ILE KLONÓW OD POCZATKU:%lu "
                            "(%lu ISTOTNYCH)",
	//agent::nazwy[agent::plot_mode],
	(long)licznik*(long)MonteCarloMult,
	(unsigned long)licznik,
	(unsigned long)getpid(),
	(unsigned long)agent::ile_ind,
	(unsigned long)autotrofy,
	(unsigned long)agent::ile_big_tax,
	(unsigned long)tax_auto,
	(unsigned long)agent::ile_tax,
	(unsigned long)klon_auto,
    (unsigned long)informacja_klonalna::ile_klonow(),
    (unsigned long)informacja_klonalna::ile_taksonow()
    );
																assert(checker1[0]=='A');
																assert(checker1[1]=='A');
																assert(checker2[0]=='A');
																assert(checker2[1]=='A');
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
printf("CO-EWOLUCJA: program symulujacy kooewolucje wielu gatunkow\n");
printf(ProgramName);
printf("\nCompilation: " __DATE__ " " __TIME__ );
//printf("POLECENIA: 'g': Trofia 'o':OSLONA 's':SILA 'w':WIEK 'q':QUIT\n");
#if(sizeof(base)*2!=sizeof(base2))
	{
	fprintf(stderr,"Niewlasciwe rozmiary dla typow bazowych:2*%u!=%u\n",
		sizeof(base),sizeof(base2));
	exit(1);
	}
#endif
printf("\nROZMIAR GENOMU: %uB\nLICZBA MOZLIWYCH KLONOW=%lu\nMAXINT=%d\n",sizeof(bity_wzoru),(unsigned long)MAXBASE2,INT_MAX);

RANDOMIZE(); 
if(!parse_options(argc,argv))
		exit(1);

my_area_menager Lufciki(24,SWIDTH,SHEIGHT,28);
swiat& tenSwiat=*new swiat(IBOKSWIATA,LogName,MappName,Lufciki);
if(&tenSwiat==NULL)
{
	fprintf(stderr,"Brak pamieci!\n");
	exit(1);
}
Lufciki.connect(&tenSwiat);

if(!Lufciki.start(ProgramName,argc,argv,1/*Buffered*/))
{
    printf("%s\n","Can't initialize graphics");
    exit(1);
}

tenSwiat.init();
tenSwiat.tworz_lufciki();

//Utworzenie sensownej nazwy pliku(-ów) do zrzutow ekranu
{
wb_pchar buf(strlen(SCREENDUMPNAME)+20);
buf.prn("%s_%ld",SCREENDUMPNAME,time(NULL));
Lufciki.set_dump_name(buf.get());
printf("\nProcess PID: %u Dump files marker: %lu",getpid(),time(NULL));
}

//zamiast Lufciki.run_input_loop();
cerr<<endl;
while(tenSwiat.get_licznik()<MAX_ITERATIONS) //Do za³o¿onego koñca
	{
	wb_cpu_clock fulltimer;
	tenSwiat.wskazniki();//Aktualizacja informacji
	Lufciki._replot();//Odrysowanie
	Lufciki.flush();

	Lufciki.process_input();//Obsluga zdarzen zewnetrznych
	
	if(!Lufciki.should_continue())
				break;//Albo koniec zabawy wymuszony "klikiem"
				
	wb_cpu_clock steptimer;
	tenSwiat.krok();  //Kolejny krok symulacji
	cout<<'\r'<<'\t'<<tenSwiat.get_licznik()<<" krok. Czas pêtli:\t"<<double(fulltimer)<<"s. w tym krok:\t"<<double(steptimer)<<" s    ";
	}

//Ostateczne zrzuty plików do analizy

//Pelna filogeneza
ofstream filogout("lastfilog.out");
tenSwiat.ZapiszFilogeneze(filogout);
filogout.close();

//Dealokacja swiata wraz ze wszystkimi skladowymi
delete &tenSwiat;
printf("Do widzenia!!!\n");
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
		cerr<<"Bad WIDTHWIN = "<<SWIDTH<<" (must be >50)"<<endl;
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
		cerr<<"Bad HEIGHTWIN = "<<SHEIGHT<<" (must be >50)"<<endl;
		return 0;
		}
	cerr<<"HEIGHTWIN = "<<SHEIGHT<<endl;
	}
	else
	if((pom=strstr(argv[i],"BOK="))!=NULL) //Nie NULL czyli jest
	{
	IBOKSWIATA=atol(pom+4);
	if(IBOKSWIATA<20 || IBOKSWIATA>=SWIDTH)
		{
		cerr<<"!!! BOK == %u jest za ma³y lub wiêkszy ni¿ deklarowany ekran!\n"<<IBOKSWIATA;
		cerr<<"Wciœnij ENTER jeœli to akceptujesz lub przerwij program!";
		getchar();
		}
		else
		cerr<<"D³ugi BOK œwiata ustawiony na: "<<IBOKSWIATA<<endl;
	}
	else
	//"FLEX=y/n - czy mo¿na zamieniaæ siê z w³aœcicielem komorki\n"
	//const unsigned	ZAMIANY=255;//Czy moze przeciskac sie na zajete pola (0 na nie lub inna liczba na tak)
	if((pom=strstr(argv[i],"FLEX="))!=NULL) //Nie NULL czyli jest
	{
	ZAMIANY=atol(pom+5);
	if(ZAMIANY==0)
		{
		if(pom[5]=='y' || pom[5]=='t' || pom[5]=='Y' || pom[5]=='T')
			ZAMIANY=255;
		}
	if (ZAMIANY>255) 
		{
		printf("!!! Niew³aœciwa wartoœæ FLEX=%d Powinno byæ < 256\n",ZAMIANY);
		printf("Wciœnij ENTER jeœli to akceptujesz lub przerwij program!");
		getchar();
		}
	else
		printf("Wartoœæ FLEX ustawiona na %d. Wartoœæ 0 oznacza wy³¹czenie opcji.\n",ZAMIANY);
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
		printf("Bit ruchu musi byæ któr¹œ potêg¹ liczby 2. Np. 0,1,2,4,8,16,32,128...\n"
				"A podana wartoœæ ma %u bitów. Jeœli chcesz wy³¹czyæ podaj bit spoza maski obrony\n",
				bit);
		}
	else
		{
		printf("Podano wartoœæ RBIT=%u",BIT_RUCHU);
		if(MAXBASE>BIT_RUCHU)
			printf("; %u bit maski obrony jest bitem ruchu\n",(BIT_RUCHU!=0?unsigned(log(BIT_RUCHU)/log(2.0)):0));
		else
			printf("; %u bit jest poza mask¹ obrony\n",unsigned(log(BIT_RUCHU)/log(2.0)));//Tu 0 raczej sie nie pojawi
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
		cerr<<"!!! Maksymalny wiek (MAXA="<<MAX_WIEK<<") nie mo¿e byæ wiêkszy ni¿ 255!"<<endl;
		}
		else
		{
		cerr<<"Maksymalny wiek (MAXA) ustawiony na "<<MAX_WIEK<<endl;
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
		fprintf(stderr,"!!! IFER = %u. To musi byæ liczba ca³kowita w zakresie 1..1000!\n",NIEPLODNOSC);
		return 0;
		}
		else
		{
		fprintf(stderr,"IFER = %u, czyli prawdopodobieñstwo rozmna¿ania = %g!\n",NIEPLODNOSC,double(1.0/NIEPLODNOSC));		
		}
	}
	else
	//"POSA=0.XX - jaki u³amek sily oddane potomkowi\n"
	//const double	WYPOSAZENIE_POTOMSTWA=0.05; // jaka czesc sily oddac potomkowi 0.1 to ZA DUZO!!! {???}
	if((pom=strstr(argv[i],"POSA="))!=NULL) //Nie NULL czyli jest
	{
	  WYPOSAZENIE_POTOMSTWA=atof(pom+5);
	  fprintf(stderr,"POSA(g)=%g zapasów rodzica. ",WYPOSAZENIE_POTOMSTWA);
	  if(WYPOSAZENIE_POTOMSTWA<=0 || 1<WYPOSAZENIE_POTOMSTWA)
	  {
		 fprintf(stderr,"!!! LICZBA MUSI BYÆ W ZAKRESIE (0..1) !!!\n");
		 return 0;
	  }
	  else
	  if(WYPOSAZENIE_POTOMSTWA>0.1)
	  {
		 fprintf(stderr,"Wartoœæ wiêksza od 0.1 pozwala ³atwo rozmna¿aæ siê z posagu!!!\n");
	  }
	  else
		 fprintf(stderr,"OK.\n");
	}
	else
	if((pom=strstr(argv[i],"RTG="))!=NULL) //Nie NULL czyli jest
	{
	PROMIENIOWANIE=atol(pom+4);
	if(PROMIENIOWANIE<16) //Jesli mniej niz dlugosc genomu to algorytm losowania bitu zle dziala
		{
		fprintf(stderr,"Niewlasciwe RTG ==  %u\n",PROMIENIOWANIE);
		return 0;
		}
	cerr<<"Wspó³czynnik mutacji: 1/"<<PROMIENIOWANIE<<endl;
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
	printf("Efektywnosc autotrofa =g%\n",EFEKTYWNOSC_AUTOTROFA);
	if(EFEKTYWNOSC_AUTOTROFA<=0)
		{
		fprintf(stderr,"Efektywnosc autotrofa nie mo¿e byc ujemna 0..0.99\n");
		return 0;
		}
		else
		if(EFEKTYWNOSC_AUTOTROFA>=1)
		{
		fprintf(stderr,"Efektywnosc autotrofa powinna byæ w zakresie 0..0.99 ale spróbuje\n");
		return 0;
		}
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
	if((pom=strstr(argv[i],"DIST="))!=NULL) //Nie NULL czyli jest
	{
	DYS_RUCHU=atol(pom+5);
	if(DYS_RUCHU<=0)
		{
		fprintf(stderr,"Minimalny krok ruchu nie moze byc <=0\n");
		return 0;
		}
	printf("Maksymalny ruch agenta ustawiony na %lu\n",DYS_RUCHU);
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
	//"INLO=NNNN - dlugoœæ wewnêtrznych logów\n"
	// const unsigned dlugosc_logow=50000;
	if((pom=strstr(argv[i],"INLO="))!=NULL) //Nie NULL czyli jest
	{
	dlugosc_logow=atol(pom+5);
	if(dlugosc_logow<100)
		{
		fprintf(stderr,"!!!D³ugoœæ wewnêtrznych logów (INLO) nie moze byc mniejsza ni¿ 100!\n");
		return 0;
		}
		else
		{
		fprintf(stderr,"D³ugoœæ wewnêtrznych logów INLO=%u\n",dlugosc_logow);
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
	cerr<<"Plik logu: "<<LogName<<endl;
	}
	else
	if((pom=strstr(argv[i],"MAPP="))!=NULL) //Nie NULL czyli jest
	{
	strcpy(MappName,pom+5);
	cerr<<"Plik mapy: "<<MappName<<endl;
	}
	else
	//"DMPF=name - rdzen/pocz¹tek nazwy plików dump\n"
	//char     SCREENDUMPNAME[256]="CO-EVO-3a";
	if((pom=strstr(argv[i],"DMPF="))!=NULL) //Nie NULL czyli jest
	{
	strcpy(SCREENDUMPNAME,pom+5);
	cerr<<"Rdzeñ nazwy zrzutów graficznych: "<<SCREENDUMPNAME<<endl;
	}
	else
	if((pom=strstr(argv[i],"TAXS="))!=NULL) //Nie NULL czyli jest
	{
	REJESTROWANY_ROZMIAR_TAKSONU=atol(pom+5);
	if(REJESTROWANY_ROZMIAR_TAKSONU<10)
		{
		fprintf(stderr,"Rozmiar rejestrowanych taksonów musi byæ >=10\n");
		return 0;
		}
		else printf("Rozmiar rejestrowanych taksonów ustawiono na %d\n",REJESTROWANY_ROZMIAR_TAKSONU);
	}
	else /* Ostatecznie wychodzi ze nie ma takiej opcji */
	{
	cerr<<"Niezidentyfikowana opcja "<<argv[i]<<endl;
	cerr<<"MOZLIWE TO: (W nawiasach wartoœci domyœlne)"<<endl;
	cerr<<"BOK=NN - bok obszaru symulacji "<<'('<<IBOKSWIATA<<')'<<endl;//unsigned	IBOKSWIATA=300;// FAKTYCZNIE UZYWANY BOK SWIATA
	cerr<<"MAPP=init.gif - nazwa pliku z mapa zamieszkiwalnoœci "<<'('<<MappName<<')'<<endl;//Inicjalizacja œwiata - czarne, miejsca niezamieszkiwalne
	cerr<<"DIST=NN - maksymalny krok ruchu agenta "<<'('<<DYS_RUCHU<<')'<<endl;//unsigned	DYS_RUCHU=1;// Dystans ruchu w jednym kierunku
	cerr<<"FLEX=y/n - czy mo¿na zamieniaæ siê z w³aœcicielem komorki "<<'('<<ZAMIANY<<')'<<endl;//const unsigned	ZAMIANY=255;//Czy moze przeciskac sie na zajete pola (0 na nie lub inna liczba na tak)
	cerr<<"RBIT=2^n - który bit odpowiada za zdolnoœæ ruchu "<<'('<<BIT_RUCHU<<')'<<endl;//const unsigned	BIT_RUCHU=1024;		// Poza maske - bez mozliwosci utraty ruchliwosci
	cerr<<"AUTO=0.XX - efektywnosc autotrofa "<<'('<<EFEKTYWNOSC_AUTOTROFA<<')'<<endl;//double	EFEKTYWNOSC_AUTOTROFA=0.5;// jaka czesc swiatla uzywa autotrof
	cerr<<"RTG=NNN - co ile kopiowanych bitów trafia sie mutacja "<<'('<<PROMIENIOWANIE<<')'<<endl;//unsigned	PROMIENIOWANIE=BITS_PER_GENOM*100;// Co ile kopiowanych bitow nastepuje mutacja
	cerr<<"IFER=NN - prawdopodobieñstwo rozmna¿ania to 1/IFER "<<'('<<NIEPLODNOSC<<')'<<endl;//const unsigned	NIEPLODNOSC=5;		// Prawdopodobienstwo rozmnazania jest 1/NIEPLODNOSC
	cerr<<"POSA=0.XX - jaki u³amek sily oddane potomkowi "<<'('<<WYPOSAZENIE_POTOMSTWA<<')'<<endl;//const double	WYPOSAZENIE_POTOMSTWA=0.05; // jaka czesc sily oddac potomkowi 0.1 to ZA DUZO!!! {???}
	cerr<<"MAXA=nnn - maksymalny wiek. Nie wiêkszy ni¿ 255. "<<'('<<255-MINIMALNY_WIEK<<')'<<endl;// Ujemne BÊDZIE oznaczaæ œmierci losowe"<<'('<<xxx<<')'<<endl;//const unsigned	MINIMALNY_WIEK=205;	// Rodzi sie z tym wiekiem. Do smierci ma 255-MINIMALNY_WIEK
	cerr<<"PIRA=0/1 - gospodarka pasozytnicza versus drapieznicza "<<'('<<PIRACTWO<<')'<<endl;//unsigned int	PIRACTWO=1;// Czy eksploatacja piracka czy pasozytnicza
	cerr<<"BUM=NN - wykladnik czestosci katastrof "<<'('<<WSP_KATASTROF<<')'<<endl;//int	WSP_KATASTROF=0;//10-100 Wykladnik rozkladu katastrof - 0 -wylaczone
					//unsigned int	ODWROCONY_KOSZT_OSLONY=0;//1 - czy koszty oslony sa odwrocone
					//unsigned int	ODWROCONY_KOSZT_ATAKU=0;//1 - czy koszty ataku sa odwrocone

					//"MITA=NNN - minimalny rozmiar taksonu z def.\n"//unsigned	 MINIMALNY_ROZMIAR_TAKSONU=100;  //Arbitralny rozmiar klonu zeby go uznac za zarejestrowany takson
	cerr<<"TAXS=NN - min. rozmiar taksonu rejestrowanego w drzewie "<<'('<<REJESTROWANY_ROZMIAR_TAKSONU<<')'<<endl;//unsigned	 REJESTROWANY_ROZMIAR_TAKSONU=MINIMALNY_ROZMIAR_TAKSONU;//Domyslna wartosc kasowania w wizualizacji drzewa
	cerr<<"INLO=NNNN - dlugoœæ wewnêtrznych logów "<<'('<<dlugosc_logow<<')'<<endl;// const unsigned dlugosc_logow=50000;
	cerr<<"MAX=NNNN - najwieksza mozliwa liczba krokow symulacji "<<'('<<MAX_ITERATIONS<<')'<<endl;//unsigned long	MAX_ITERATIONS=0xffffffff;// najwieksza liczba iteracji
	cerr<<"LOGC=N - czestosc zapisow do zewnetrznego logu "<<'('<<LogRatio<<')'<<endl;//unsigned	LogRatio=1;// Co ile krokow zapisywac do logu
	cerr<<"LOGF=name.log - nazwa pliku z logiem "<<'('<<LogName<<')'<<endl;
	cerr<<"DMPF=name - rdzen/pocz¹tek nazwy plików dump "<<'('<<SCREENDUMPNAME<<')'<<endl;//char     SCREENDUMPNAME[256]="CO-EVO-3a";
	cerr<<"WIDTHWIN,HEIGHTWIN - rozmiary obszaru roboczego okna "<<'('<<SWIDTH<<'x'<<SHEIGHT<<')'<<endl;	//unsigned SWIDTH=1200;//750;//1200; Tez zadziala
																				//unsigned SHEIGHT=750;//550;//1000;
	return 0;
	}
	}
//POPRAWIANIE ZALEZNOSCI
if(DYS_RUCHU>IBOKSWIATA/4)
	 DYS_RUCHU=IBOKSWIATA/4;
return 1;
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
		ssh_realize_menu(menu);
	}
	return ret;
}

int my_area_menager::_pre_process_input(int input_char)
//Przed obsluga domyslna. Zwraca 1 jesli obsluzyl.
{
										assert(MojSwiat!=NULL);
	switch(input_char){
	case ID_VIEWOPT_DUMPCO:
		{

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

		}
		return 1;
	case ID_VIEWOPT_TRNETLESS:
		{

		}
		return 1;
	default:
		return 0;
	}
}


