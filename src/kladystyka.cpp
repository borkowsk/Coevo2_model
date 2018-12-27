// kladystyka.cpp: implementation of the klad class.
//
//////////////////////////////////////////////////////////////////////
#include <assert.h>
#include "simpsour.hpp"
#include "kladystyka.hpp"
#include "arrasour.hpp" //analogiczne do #include "mattsour.hpp"

//Akcje dla konkretnego wezla drzewa klonow 
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
    Info.This.lines(LineIndex).set(PointIndex-1,PointIndex,klon->how_specialised());//wezel koncowy powstanie za chwile
    LineIndex++;
}
//Reprezentacja istnienia klonu na osi czasu
Info.This.nodes(PointIndex++).set(1,MyIndex,klon->data_powstania_klonu(),klon->identyfikator());//Startowy wezel klonu
Info.This.nodes(PointIndex).set(2,MyIndex,klon->data_powstania_klonu()+klon->czas_zycia_klonu(),klon->identyfikator());//Koncowy wezel klonu
Info.This.lines(LineIndex).set(PointIndex-1,PointIndex,klon->how_specialised());

//I rekurencyjne wywolanie akcji dla dzieci
//////////////////////////////////////////
_trawersal_info MyInfo(Info,MyIndex,klon);
klon->for_each_child(_dodaj_do_listy,&MyInfo);

//Prosty dispersing drzewa
Info.This.descendants[MyIndex].disperse=Info.This._for_simple_dispersing++;

//Zapamietanie liczby lini potomnych tego klonu
Info.This.descendants[MyIndex].number_of_childs=MyInfo.RetNumberOfDescendans;

//Powiekszenie liczby linii potomnych rodzica o liczbe linii potomnych tego klonu
Info.RetNumberOfDescendans+=1+MyInfo.RetNumberOfDescendans;//Do odczytania wyzej

return true;
}

//Akcja dla kazdego dziecka wezla drzewa
bool klad::_dodaj_do_listy(informacja_klonalna* klon,void* user_data)
{
    _trawersal_info& Info=*(reinterpret_cast< _trawersal_info* >(user_data));        
    return Info.This._wypelniaj_listy(dynamic_cast<agent::informacja_klonalna*>(klon),Info);
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
    for(int i=0;i<nodes.CurrSize();i++)
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
            (lines.CurrSize(),lines.GetTabPtr(),&connection::weight,"Specialisation");
        pLineWeights->setminmax(0,16);//Specjalizacja: Od 0 do 16 wyzerowanych bitow
        return pLineWeights;
    }
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
klad::klad(agent::informacja_klonalna* TheAncestor)://Konstruktor	
    ancestor(TheAncestor),
    pNodeTime(NULL),//Punkty czasowe wezlow drzewa: specjacji, poczatku i konca istnienia klonu (po 3 na klon)
    pNodeSpread(NULL),//Sztuczny rozrzut wezlów  dla czytelnosci drzewa
    pLineStarts(NULL),//Indeksy poczatków linii laczacych wezly drzewa (po 2 na klon)
    pLineEnds(NULL),//Indeksy koncow linii laczacych wezly drzewa (po 2 na klon)
    pLineWeights(NULL)//Domyslne kolorowanie linii
{
}

klad::~klad()//Destruktor - na wszelki wypadek
{
    zapomnij_liste(true/*zwolnij_pamiec*/);//Zapominamy liste
    ancestor=NULL;
}

