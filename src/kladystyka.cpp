// kladystyka.cpp: implementation of the klad class.
//
//*////////////////////////////////////////////////////////////////////
#include <assert.h>

#include "sshutils.hpp"
#include "dataclasses/simpsour.hpp"
#include "dataclasses/arrasour.hpp" //analogiczne do #include "mattsour.hpp"

#include "kladystyka.hpp"

// Powinno być zadeklarowane w klasie klad, ale wtedy nie chce się kompilować!!! TODO
unsigned long (agent::informacja_klonalna::*how_weighted)(); //Wskaźnik do funkcji zwracającej jakiś atrybut klonu jako wagę połączenia

//*////////////////////////////////////////////////////////////////////
// Construction/Destruction
//*////////////////////////////////////////////////////////////////////

// Konstruktor
klad::klad(agent::informacja_klonalna* TheAncestor):
	ancestor(TheAncestor),
//	how_weighted(NULL), //Funkcja kolorowania
	pNodeTime(NULL),   //Punkty czasowe węzłów drzewa: specjacji, początku i konca istnienia klonu (po 3 na klon)
	pNodeSpread(NULL), //Sztuczny rozrzut węzłów dla czytelności drzewa
	pLineStarts(NULL), //Indeksy początków linii łączących węzły drzewa (po 2 na klon)
	pLineEnds(NULL),   //Indeksy końców linii łączących węzły drzewa (po 2 na klon)
	pLineWeights(NULL) //Domyślne kolorowanie linii
{
	how_weighted=&agent::informacja_klonalna::how_specialised; //Musi być globalnie, bo w klasie nie wiem jak wywołać :-D TODO
}

// Destruktor. Na wszelki wypadek.
klad::~klad()
{
	zapomnij_liste(true ); //Zapominamy listę
	ancestor=NULL;
}

// Akcje tekstowego wypisywania klonów od konkretnego węzła ich drzewa
void klad::ZapiszTxt(ostream& out,unsigned min_time,unsigned max_time,unsigned size_tres)
{    
	_filogOutPutInfo Info(*this,out,min_time,max_time,size_tres,ancestor); //Przodkiem wspólnego przodka jest on sam :-)
	if(Info.max_time==0) 
			Info.max_time=ULONG_MAX;
	out<<"FULLCODE"<<'\t'
		<<"ORDNUMBER"<<'\t'
		<<"ANCORDNUM"<<'\t'
		<<"BEGSTEP"<<'\t'
		<<"ENDSTEP"<<'\t'
		<<"LIFETIME"<<'\t'
		<<"CURRAGENTS"<<'\t'
		<<"ALLAGENTS"<<'\t'
		<<"FEEDNISHE"<<'\t'
		<<"DEFFNISHE"<<'\t'
		<<"FEEDSPEC"<<'\t'
		<<"DEFFSPEC"<<'\t'
		<<"SPECJALIS"<<'\t'
		<<"NISHENAME"<<endl;
	_wypisz_poddrzewo(ancestor,Info);
}

bool klad::_wypisz_poddrzewo(agent::informacja_klonalna* klon,_filogOutPutInfo& Info)
{
extern unsigned BIT_RUCHU; //=128 Wyzerowanie których bitów osłony odpowiada za zdolności ruchu. W pliku z main-em
if(	Info.min_time<=klon->data_powstania_klonu() && 
	klon->data_powstania_klonu()<=Info.max_time &&
	klon->wszystkich()>Info.size_tres)
	{
		if(klon->feeding_niche()==AUTOTROF)
			Info.out<<"Au";
		else
			Info.out<<hex<<klon->feeding_niche();
		Info.out<<'_'<<klon->defense_niche();
		if(klon->defense_niche() &  BIT_RUCHU)
		  Info.out<<'S';
		else
		  Info.out<<'_';
		Info.out<<dec<<klon->identyfikator()<<'\t'
				<<dec<<klon->identyfikator()<<'\t'
				<<Info.Klon->identyfikator()<<'\t'
				<<klon->data_powstania_klonu()<<'\t';
				
		if(klon->data_wymarcia_klonu()==0) //Jeszcze nie był wymarły
			Info.out<<'\t'; //Pusta kolumna
		else //byl wymarły
			Info.out<<klon->data_wymarcia_klonu()<<'\t';
	
		Info.out<<klon->czas_zycia_klonu()<<'\t'
				<<klon->ile_zywych()<<'\t'
				<<klon->wszystkich()<<'\t'
				<<klon->feeding_niche()<<'\t'
				<<klon->defense_niche()<<'\t'
				<<klon->how_specialised_in_feeding()<<'\t'
				<<klon->how_specialised_in_defense()<<'\t'
				<<klon->how_specialised()<<'\t'
				<<hex<<klon->feeding_niche()<<klon->defense_niche()<<'\t'<<dec;
	 Info.out<<endl;
	 }
// I rekurencyjne wywołanie akcji dla dzieci, i to nawet jak samego nie wypisano!
// //////////////////////////////////////////////////////////////////////////////
//_filogOutPutInfo MyInfo(...);   NIEPOTRZEBNE???
agent::informacja_klonalna* Parent=Info.Klon;
Info.Klon=klon; //Dla dzieci klonu to on jest rodzicem
klon->for_each_child(_wypisz_takson,&Info); //TU JEST UKRYTA PĘTLA I REKURENCJA!!!
Info.Klon=Parent; //Przywracanie 'dziadka' - dla rodzeństwa
return true;
}

// Akcje wypełniania struktur danych wizualizacyjnych od konkretnego węzła drzewa klonów
bool klad::_wypelniaj_listy(agent::informacja_klonalna* klon,_trawersal_info& Info)
{
														                                             assert(klon!=NULL);
size_t MyIndex=Info.This.descendants.CurrSize();
size_t PointIndex=Info.This.nodes.CurrSize();
size_t LineIndex=Info.This.lines.CurrSize();

// Tworzenie informacji o aktualnym klonie
Info.This.descendants(MyIndex).set(Info.Index,klon);//klon->identyfikator()

if(Info.Klon!=NULL) //Dopięcie do macierzystego
{
    // Węzeł utworzenia na obiekcie macierzystym
	Info.This.nodes(PointIndex).set(0,Info.Index,klon->data_powstania_klonu(),Info.Klon->identyfikator());
	PointIndex++;
    // Węzeł końcowy powstanie za chwile
	Info.This.lines(LineIndex).set_attr( PointIndex-1,PointIndex,(klon->*how_weighted)());
	LineIndex++;
}

// Reprezentacja istnienia klonu na osi czasu
Info.This.nodes(PointIndex++).set(1,MyIndex,klon->data_powstania_klonu(),klon->identyfikator()); //Startowy węzeł klonu
Info.This.nodes(PointIndex).set(2,MyIndex,klon->data_powstania_klonu()+klon->czas_zycia_klonu(),klon->identyfikator()); //Końcowy węzeł klonu

Info.This.lines(LineIndex).set_attr(PointIndex-1,PointIndex,(klon->*how_weighted)());

//  I rekurencyjne wywołanie akcji dla dzieci
//*/////////////////////////////////////////////
_trawersal_info MyInfo(Info,MyIndex,klon);
klon->for_each_child(_dodaj_do_listy,&MyInfo); //TU JEST UKRYTA PĘTLA I REKURENCJA!!!

//Prosty dispersing drzewa
Info.This.descendants[MyIndex].disperse=Info.This._for_simple_dispersing++;

// Zapamiętanie liczby linii potomnych tego klonu
Info.This.descendants[MyIndex].number_of_childs=MyInfo.RetNumberOfDescendans;

// Powiększenie liczby linii potomnych rodzica o liczbę linii potomnych tego klonu
Info.RetNumberOfDescendans+=1+MyInfo.RetNumberOfDescendans; //Do odczytania wyżej

return true;
}

// Akcje dla każdego dziecka węzła drzewa
bool klad::_dodaj_do_listy(informacja_klonalna* klon,void* user_data)
{
	_trawersal_info& Info=*(reinterpret_cast< _trawersal_info* >(user_data));
	return Info.This._wypelniaj_listy(dynamic_cast<agent::informacja_klonalna*>(klon),Info);
}

bool klad::_wypisz_takson(informacja_klonalna* klon,void* user_data) //'Downgrade typów' żeby uzgodnić z metodą for_each_child
{
	_filogOutPutInfo& Info=*(reinterpret_cast< _filogOutPutInfo* >(user_data));
	return Info.This._wypisz_poddrzewo(dynamic_cast<agent::informacja_klonalna*>(klon),Info);
}

void klad::zapomnij_liste(bool zwolnij_pamiec)
{
	descendants.Truncate(0);
	nodes.Truncate(0);
	lines.Truncate(0);
	_for_simple_dispersing=0;
	
	if(zwolnij_pamiec)
	{
		descendants.Deallocate();
		nodes.Deallocate();
		lines.Deallocate();
		_empty_source_ptrs();
	}
}

// Uruchomienie trawersowania drzewa klonów
void klad::aktualizuj_liste_zstepnych(bool leftrec)
{
	// Zapominamy stare listy, ale alokacja zostaje!
	descendants.Truncate(0);
	nodes.Truncate(0);
	lines.Truncate(0);
	_for_simple_dispersing=0;

	if(ancestor)//Tworzy nowe listy, jeśli jest z czego
	{
	   // Utworzenie obiektu pomocniczego zawierającego informacje
	   // o klonie macierzystym i o obiekcie drzewa kladystycznego
	   _trawersal_info RootInfo(*this); //Tu informacje o klonie macierzystym ustawione na same 0
	   
	   _wypelniaj_listy(ancestor,RootInfo);   
		
	   _disperse_nodes1();

	   _update_source_ptrs(); //Poprawia wskaźniki do tablic, które mogą się dezaktualizować podczas wypełniania list
			  
	   //DEBUG
	   //cerr<<"lengh(des)="<<descendants.CurrSize()<<' '<<RootInfo.RetNumberOfDescendans<<endl;
	   //cerr<<"lengh(nod)="<<nodes.CurrSize()<<endl; 
	   //cerr<<"lengh(lin)="<<lines.CurrSize()<<endl<<endl;
	   ;
	}
}

// Algorytm rozstawiania drzewa. Ten tylko minimalny sensowny.
void klad::_disperse_nodes1()
{
	// Dla każdej linii trzeba obliczyć położenie na szerokości drzewa
	//for(int l=0;l<descendants.CurrSize();l++){}

	//Dla każdego węzła trzeba [teraz] przepisać położenie na szerokości drzewa
	for(unsigned i=0;i<nodes.CurrSize();i++)
	{
		size_t line=nodes[i].my_line;
		unsigned long disp=descendants[line].disperse;
		nodes[i].Disperse=disp;
	}
}

// Algorytm rozstawiania drzewa. Bardziej estetyczny
// NIGDY NIE ZAIMPLEMENTOWANY W TYM MODELU.
void klad::_disperse_nodes2()
{
    //...TODO
}

// Poprawia wskaźniki do tablic, które mogą się dezaktualizować podczas wypełniania list
void klad::_update_source_ptrs()
{
    if(pNodeTime!=NULL) 
        pNodeTime->set_source(nodes.CurrSize(),nodes.GetTabPtr());   //Punkty czasowe węzłów drzewa: specjacji, początku i konca istnienia klonu (po 3 na klon)
    if(pNodeSpread!=NULL) 
        pNodeSpread->set_source(nodes.CurrSize(),nodes.GetTabPtr()); //Sztuczny rozrzut węzłów dla czytelności drzewa
    if(pLineStarts!=NULL) 
        pLineStarts->set_source(lines.CurrSize(),lines.GetTabPtr()); //Indeksy początków linii łączących węzły drzewa (po 2 na klon)
    if(pLineEnds!=NULL) 
        pLineEnds->set_source(lines.CurrSize(),lines.GetTabPtr());   //Indeksy końców linii łączących węzły drzewa (po 2 na klon)
    if(pLineWeights!=NULL)
        pLineWeights->set_source(lines.CurrSize(),lines.GetTabPtr()); //Wagi linii najlepiej jako kolory
}


void klad::_empty_source_ptrs()
{    
    if(pNodeTime!=NULL) 
        pNodeTime->set_source(0,NULL);  //Punkty czasowe węzłów drzewa: specjacji, początku i konca istnienia klonu (po 3 na klon)
    if(pNodeSpread!=NULL) 
        pNodeSpread->set_source(0,NULL); //Sztuczny rozrzut węzłów dla czytelności drzewa
    if(pLineStarts!=NULL) 
        pLineStarts->set_source(0,NULL); //Indeksy początków linii łączących węzły drzewa (po 2 na klon)
    if(pLineEnds!=NULL) 
        pLineEnds->set_source(0,NULL); //Indeksy końców linii łączących węzły drzewa (po 2 na klon)
    if(pLineWeights!=NULL)
        pLineWeights->set_source(0,NULL); //Wagi linii najlepiej jako kolory
}

//*////////////////////////////////////////////////////////////////////
//  Udostępnianie danych
//*////////////////////////////////////////////////////////////////////

// Punkty czasowe węzłów drzewa: specjacji, początku i konca istnienia klonu (po 3 na klon)
linear_source_base* klad::NodeTime()
{
    if(pNodeTime !=NULL) 
    {
        return pNodeTime;
    }
    else
    {
        return pNodeTime  =  new struct_array_source<node_info,unsigned long>
			(nodes.CurrSize(),nodes.GetTabPtr(),&node_info::Time,"Time");  //Now ERROR TODO
    }
}

// Sztuczny rozrzut węzłów dla czytelności drzewa
linear_source_base* klad::NodeSpread()
{
    if(pNodeSpread !=NULL) 
    {
        return pNodeSpread;
    }
    else
    {
        return pNodeSpread  =  new struct_array_source<node_info,unsigned long>
			(nodes.CurrSize(),nodes.GetTabPtr(),&node_info::Disperse,"-l-");  //Now ERROR TODO
    }
}

// Indeksy początków linii łączących węzły drzewa (po 2 na klon)
linear_source_base* klad::LineStarts()
{
    if(pLineStarts !=NULL) 
    {
        return pLineStarts;
    }
    else
    {
        return pLineStarts  = new struct_array_source<connection,size_t>
			(lines.CurrSize(),lines.GetTabPtr(),&connection::start_node,"-st-");  //Now ERROR TODO
	}
}

// Indeksy końców linii łączących węzły drzewa (po 2 na klon)
linear_source_base* klad::LineEnds()
{
   if(pLineEnds !=NULL) 
    {
        return pLineEnds;
    }
    else
    {
        return pLineEnds  =  new struct_array_source<connection,size_t>
            (lines.CurrSize(),lines.GetTabPtr(),&connection::end_node,"-en-"); //Now ERROR TODO
    }
}

// Indeksy końców linii łączących węzły drzewa (po 2 na klon)
linear_source_base* klad::LineWeights()
{
   if(pLineWeights !=NULL) 
    {
		return pLineWeights;
    }
    else
    {
		pLineWeights  =  new struct_array_source<connection,unsigned long>
			(lines.CurrSize(),lines.GetTabPtr(),&connection::node_weight,lang("Specjalizacja","Specialisation"));
		pLineWeights->setminmax(0,16); //Specjalizacja: Od 0 do 16 wyzerowanych bitów
		return pLineWeights;
	}
}

// Zmiana kolorowania kladów
void klad::ChangeLineWeightsSource(Kolorowanie co)
{
if(pLineWeights !=NULL) //Tylko jeśli już zostało zaalokowane       		//pLineWeights->reset();      ???
	switch(co){
	case K_ILUBYLOIJEST:
		how_weighted=&agent::informacja_klonalna::magnitude;
		pLineWeights->settitle(lang("jako log2 z D.orA agentów","in log2 of D.orA. agents."));
		pLineWeights->setminmax(0,0); //Czytanie aktualnego min i max
		aktualizuj_liste_zstepnych();
		break;
	case K_SPECTROFIA:
		how_weighted=&agent::informacja_klonalna::how_specialised_in_feeding;
		pLineWeights->settitle(lang("jako specjalizacja trof.","in trophy spec."));
		pLineWeights->setminmax(0,8); //Obrona jako wartość
		aktualizuj_liste_zstepnych();
		break;
	case K_SPECOBRONA:
		how_weighted=&agent::informacja_klonalna::how_specialised_in_defense;
		pLineWeights->settitle(lang("","in defence spec."));
		pLineWeights->setminmax(0,8); //Obrona jako wartość
		aktualizuj_liste_zstepnych();
		break;
	case K_OBRONA:
		how_weighted=&agent::informacja_klonalna::defense_niche;
		pLineWeights->settitle("Defence niches");
		pLineWeights->setminmax(0,255); //Obrona jako wartość
		aktualizuj_liste_zstepnych();
		break;
	case K_TROFIA:
		how_weighted=&agent::informacja_klonalna::feeding_niche;
		pLineWeights->settitle("Feeding niches");
		pLineWeights->setminmax(0,255); //Trofia jako wartość
		aktualizuj_liste_zstepnych();
		break;		
	case K_SPECJALIZACJA:
	default:
		how_weighted=&agent::informacja_klonalna::how_specialised;
		pLineWeights->settitle("Specialisation");
		pLineWeights->setminmax(0,16); //Specjalizacja: Od 0 do 16 wyzerowanych bitów
		aktualizuj_liste_zstepnych();
		break;		
	}
}

// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Reactivated source code from Windows (2022.07)
/// @author Wojciech Borkowski
/// FOR @LICENCE SEE HERE: https://github.com/borkowsk/Coevo2_model
// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////




