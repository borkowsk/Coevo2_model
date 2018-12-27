// kladystyka.cpp: implementation of the klad class.
//
//////////////////////////////////////////////////////////////////////
#include <assert.h>
#include "SYMSHELL/sshutils.hpp"
#include "SYMSHELL/simpsour.hpp"
#include "SYMSHELL/arrasour.hpp" //analogiczne do #include "mattsour.hpp"
#include "kladystyka.hpp"

//Powinno byæ zadeklarowane w klasie klad, ale wtedy nie chce siê kompilowaæ!!!
unsigned long (agent::informacja_klonalna::*how_weighted)();//WskaŸnik do funkcji zwracaj¹cej jakiœ atrybut klonu jako wagê pol¹czenia

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
klad::klad(agent::informacja_klonalna* TheAncestor)://Konstruktor	
	ancestor(TheAncestor),
//	how_weighted(NULL), //Funkjca kolorowania
	pNodeTime(NULL),//Punkty czasowe wezlow drzewa: specjacji, poczatku i konca istnienia klonu (po 3 na klon)
	pNodeSpread(NULL),//Sztuczny rozrzut wezlów  dla czytelnosci drzewa
	pLineStarts(NULL),//Indeksy poczatków linii laczacych wezly drzewa (po 2 na klon)
	pLineEnds(NULL),//Indeksy koncow linii laczacych wezly drzewa (po 2 na klon)
	pLineWeights(NULL)//Domyslne kolorowanie linii
{
	how_weighted=&agent::informacja_klonalna::how_specialised; //Musi byæ globalnie, bo w klasie nie wiem jak wywo³aæ
}

klad::~klad()//Destruktor - na wszelki wypadek
{
	zapomnij_liste(true/*zwolnij_pamiec*/);//Zapominamy liste
	ancestor=NULL;
}


//Akcje tekstowego wypisywania klonów od konkretnego wezla ich drzewa
void klad::ZapiszTxt(ostream& out,unsigned min_time,unsigned max_time,unsigned size_tres)
{    
	_filogOutPutInfo Info(*this,out,min_time,max_time,size_tres,ancestor);//Przodkiem wspólnego przodka jest on sam :-)
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
extern unsigned BIT_RUCHU; //=128 Wyzerowanie ktorych bitow oslony odpowiada za zdolnosc ruchu. W pliku z main-em
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
				
		if(klon->data_wymarcia_klonu()==0) //Jeszcze nie by³ wymar³y
			Info.out<<'\t';//Pusta kolumna
		else //byl wymarly
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
//I rekurencyjne wywolanie akcji dla dzieci - nawet jak samego nie wypisano!
////////////////////////////////////////////////////////////////////////////////
//_filogOutPutInfo MyInfo(...);   NIEPOTRZEBNE???
agent::informacja_klonalna* Parent=Info.Klon;
Info.Klon=klon;//Dla dzieci klonu to on jest rodzicem
klon->for_each_child(_wypisz_takson,&Info);//TU JEST UKRYTA PÊTLA I REKURENCJA!!!
Info.Klon=Parent;//Przywracanie 'dziadka' - dla rodzeñstwa
return true;
}

//Akcje wype³niania struktur danych wizualizacyjnych od konkretnego wezla drzewa klonow 
bool klad::_wypelniaj_listy(agent::informacja_klonalna* klon,_trawersal_info& Info)
{
														assert(klon!=NULL);
size_t MyIndex=Info.This.descendants.CurrSize();
size_t PointIndex=Info.This.nodes.CurrSize();
size_t LineIndex=Info.This.lines.CurrSize();

//Tworzenie informacji o aktualnym klonie
Info.This.descendants(MyIndex).set(Info.Index,klon);//klon->identyfikator()
if(Info.Klon!=NULL)//Dopiecie do macierzystego
{
	Info.This.nodes(PointIndex).set(0,Info.Index,klon->data_powstania_klonu(),Info.Klon->identyfikator());//Wezel utworzenia na obiekcie macierzystym
	PointIndex++;
	Info.This.lines(LineIndex).set_attr( PointIndex-1,PointIndex,(klon->*how_weighted)());//wezel koncowy powstanie za chwile
	LineIndex++;
}

//Reprezentacja istnienia klonu na osi czasu
Info.This.nodes(PointIndex++).set(1,MyIndex,klon->data_powstania_klonu(),klon->identyfikator());//Startowy wezel klonu
Info.This.nodes(PointIndex).set(2,MyIndex,klon->data_powstania_klonu()+klon->czas_zycia_klonu(),klon->identyfikator());//Koncowy wezel klonu

Info.This.lines(LineIndex).set_attr(PointIndex-1,PointIndex,(klon->*how_weighted)());

//I rekurencyjne wywolanie akcji dla dzieci
//////////////////////////////////////////
_trawersal_info MyInfo(Info,MyIndex,klon);
klon->for_each_child(_dodaj_do_listy,&MyInfo);//TU JEST UKRYTA PÊTLA I REKURENCJA!!!

//Prosty dispersing drzewa
Info.This.descendants[MyIndex].disperse=Info.This._for_simple_dispersing++;

//Zapamietanie liczby linii potomnych tego klonu
Info.This.descendants[MyIndex].number_of_childs=MyInfo.RetNumberOfDescendans;

//Powiekszenie liczby linii potomnych rodzica o liczbe linii potomnych tego klonu
Info.RetNumberOfDescendans+=1+MyInfo.RetNumberOfDescendans;//Do odczytania wyzej

return true;
}

//Akcje dla kazdego dziecka wezla drzewa
bool klad::_dodaj_do_listy(informacja_klonalna* klon,void* user_data)
{
	_trawersal_info& Info=*(reinterpret_cast< _trawersal_info* >(user_data));
	return Info.This._wypelniaj_listy(dynamic_cast<agent::informacja_klonalna*>(klon),Info);
}

bool klad::_wypisz_takson(informacja_klonalna* klon,void* user_data)//'Downgrade typow' zeby uzgodnic z metoda for_each_child
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

//Uruchomienie trawersowania drzewa klonow
void klad::aktualizuj_liste_zstepnych(bool leftrec)
{
	//Zapominamy stare listy, ale alokacja zostaje!
	descendants.Truncate(0);
	nodes.Truncate(0);
	lines.Truncate(0);
	_for_simple_dispersing=0;

	if(ancestor)//Tworzy nowe listy, jesli jest z czego
	{
	   //Utworzenie obiektu pomocniczego zawierajacego informacje
	   //o klonie macierzystym i o obiekcie drzewa kladystycznego
	   _trawersal_info RootInfo(*this);//Tu informacje o klonie macierzystym ustawione na same 0
	   
	   _wypelniaj_listy(ancestor,RootInfo);   
		
	   _disperse_nodes1();

	   _update_source_ptrs();//Poprawia wskazniki do tablic, ktore moga sie dezaktualizowac podczas wypelniania list
			  
	   //DEBUG
	   //cerr<<"lengh(des)="<<descendants.CurrSize()<<' '<<RootInfo.RetNumberOfDescendans<<endl;
	   //cerr<<"lengh(nod)="<<nodes.CurrSize()<<endl; 
	   //cerr<<"lengh(lin)="<<lines.CurrSize()<<endl<<endl;
	   ;
	}
}

void klad::_disperse_nodes1()//Algorytm rozstawiania drzewa - minimalny sensowny
{
	//Dla kazdej linii trzeba obliczyc polozenie na szerokosc drzewa
	//for(int l=0;l<descendants.CurrSize();l++){}

	//Dla kazdego wezla trzeba [teraz] przepisac polozenie na szerokosci drzewa
	for(unsigned i=0;i<nodes.CurrSize();i++)
	{
		size_t line=nodes[i].my_line;
		unsigned long disp=descendants[line].disperse;
		nodes[i].Disperse=disp;
	}
}

void klad::_disperse_nodes2()//Algorytm rozstawiania drzewa - bardziej estetyczny
{

}

void klad::_update_source_ptrs()
//Poprawia wskazniki do tablic, ktore moga sie dezaktualizowac podczas wypelniania list
{
    if(pNodeTime!=NULL) 
        pNodeTime->set_source(nodes.CurrSize(),nodes.GetTabPtr());//Punkty czasowe wezlow drzewa: specjacji, poczatku i konca istnienia klonu (po 3 na klon)
    if(pNodeSpread!=NULL) 
        pNodeSpread->set_source(nodes.CurrSize(),nodes.GetTabPtr());//Sztuczny rozrzut wezlów  dla czytelnosci drzewa
    if(pLineStarts!=NULL) 
        pLineStarts->set_source(lines.CurrSize(),lines.GetTabPtr());//Indeksy poczatków linii laczacych wezly drzewa (po 2 na klon)
    if(pLineEnds!=NULL) 
        pLineEnds->set_source(lines.CurrSize(),lines.GetTabPtr());//Indeksy koncow linii laczacych wezly drzewa (po 2 na klon)
    if(pLineWeights!=NULL)
        pLineWeights->set_source(lines.CurrSize(),lines.GetTabPtr());//Wagi lini najlepiej jako kolory
}


void klad::_empty_source_ptrs()
{    
    if(pNodeTime!=NULL) 
        pNodeTime->set_source(0,NULL);//Punkty czasowe wezlow drzewa: specjacji, poczatku i konca istnienia klonu (po 3 na klon)
    if(pNodeSpread!=NULL) 
        pNodeSpread->set_source(0,NULL);//Sztuczny rozrzut wezlów  dla czytelnosci drzewa
    if(pLineStarts!=NULL) 
        pLineStarts->set_source(0,NULL);//Indeksy poczatków linii laczacych wezly drzewa (po 2 na klon)
    if(pLineEnds!=NULL) 
        pLineEnds->set_source(0,NULL);//Indeksy koncow linii laczacych wezly drzewa (po 2 na klon)
    if(pLineWeights!=NULL)
        pLineWeights->set_source(0,NULL);//Wagi lini najlepiej jako kolory
}

//////////////////////////////////////////////////////////////////////
// Udostepnianie danych
//////////////////////////////////////////////////////////////////////
linear_source_base* klad::NodeTime()
//Punkty czasowe wezlow drzewa: specjacji, poczatku i konca istnienia klonu (po 3 na klon)
{
    if(pNodeTime !=NULL) 
    {
        return pNodeTime;
    }
    else
    {
        return pNodeTime  =  new struct_array_source<node_info,unsigned long>
			(nodes.CurrSize(),nodes.GetTabPtr(),&node_info::Time,"Time");
    }
}

linear_source_base* klad::NodeSpread()//Sztuczny rozrzut wezlów  dla czytelnosci drzewa
{
    if(pNodeSpread !=NULL) 
    {
        return pNodeSpread;
    }
    else
    {
        return pNodeSpread  =  new struct_array_source<node_info,unsigned long>
			(nodes.CurrSize(),nodes.GetTabPtr(),&node_info::Disperse,"-l-");;
    }
}

linear_source_base* klad::LineStarts()
//Indeksy poczatków linii laczacych wezly drzewa (po 2 na klon)
{
    if(pLineStarts !=NULL) 
    {
        return pLineStarts;
    }
    else
    {
        return pLineStarts  = new struct_array_source<connection,size_t>
			(lines.CurrSize(),lines.GetTabPtr(),&connection::start_node,"-st-");
	}
}

linear_source_base* klad::LineEnds()
//Indeksy koncow linii laczacych wezly drzewa (po 2 na klon)
{
   if(pLineEnds !=NULL) 
    {
        return pLineEnds;
    }
    else
    {
        return pLineEnds  =  new struct_array_source<connection,size_t>
            (lines.CurrSize(),lines.GetTabPtr(),&connection::end_node,"-en-");
    }
}

linear_source_base* klad::LineWeights()
//Indeksy koncow linii laczacych wezly drzewa (po 2 na klon)
{
   if(pLineWeights !=NULL) 
    {
		return pLineWeights;
    }
    else
    {
		pLineWeights  =  new struct_array_source<connection,unsigned long>
			(lines.CurrSize(),lines.GetTabPtr(),&connection::node_weight,lang("Specjalizacja","Specialisation"));
		pLineWeights->setminmax(0,16);//Specjalizacja: Od 0 do 16 wyzerowanych bitow
		return pLineWeights;
	}
}

void klad::ChangeLineWeightsSource(Kolorowanie co)//Zmiana kolorowania kladów
{
if(pLineWeights !=NULL) //Tylko jeœli ju¿ zosta³o zaalokowane       		//pLineWeights->reset();      ???
	switch(co){
	case K_ILUBYLOIJEST:
		how_weighted=&agent::informacja_klonalna::magnitude;
		pLineWeights->settitle(lang("jako log2 z D.orA agentów","in log2 of D.orA. agents."));
		pLineWeights->setminmax(0,0);//Czytanie aktualnego min i max
		aktualizuj_liste_zstepnych();
		break;
	case K_SPECTROFIA:
		how_weighted=&agent::informacja_klonalna::how_specialised_in_feeding;
		pLineWeights->settitle(lang("jako specjalizacja trof.","in trophy spec."));
		pLineWeights->setminmax(0,8);//Obrona jako wartoœæ 
		aktualizuj_liste_zstepnych();
		break;
	case K_SPECOBRONA:
		how_weighted=&agent::informacja_klonalna::how_specialised_in_defense;
		pLineWeights->settitle(lang("","in defence spec."));
		pLineWeights->setminmax(0,8);//Obrona jako wartoœæ 
		aktualizuj_liste_zstepnych();
		break;
	case K_OBRONA:
		how_weighted=&agent::informacja_klonalna::defense_niche;
		pLineWeights->settitle("Defence niches");
		pLineWeights->setminmax(0,255);//Obrona jako wartoœæ 
		aktualizuj_liste_zstepnych();
		break;
	case K_TROFIA:
		how_weighted=&agent::informacja_klonalna::feeding_niche;
		pLineWeights->settitle("Feeding niches");
		pLineWeights->setminmax(0,255);//Trofia jako wartosc
		aktualizuj_liste_zstepnych();
		break;		
	case K_SPECJALIZACJA:
	default:
		how_weighted=&agent::informacja_klonalna::how_specialised;
		pLineWeights->settitle("Specialisation");
		pLineWeights->setminmax(0,16);//Specjalizacja: Od 0 do 16 wyzerowanych bitow
		aktualizuj_liste_zstepnych();
		break;		
	}
}



