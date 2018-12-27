// klonalinfo.cpp: implementation of the klonalinfo class.
//
//////////////////////////////////////////////////////////////////////

#include "klonalinfo.hpp"
#include "assert.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

informacja_klonalna::informacja_klonalna(informacja_klonalna* iparent):
    parent(NULL),
    data_powstania(*zrodlo_makerow_czasu),
    ostatnia_smierc(0),
    zywych(0),
    martwych(0),
    wlasna_wartosc(0xFADEC0DE)
{
    assert(zrodlo_makerow_czasu!=NULL);
    wlasna_wartosc=++licznik;//Zwiekszenie i zapamientanie aktualnej wartosci licznika klonow

    parent=iparent;          //Zapamientanie klonu macierzystego/ojcowskiego
    
    if(parent!=NULL)//Jesli klon ma "ojca"
        parent->childs(parent->childs.CurrSize())=this;//I rejestracja nowego klonu jako dziecka klonu macierzystego
}

informacja_klonalna::~informacja_klonalna() //Destruktor uzywany glownie dla zbyt malych
{
    usunieto++;
    wlasna_wartosc=0xFADEC0DE;
}
      
bool informacja_klonalna::OK(informacja_klonalna* who)
//Sprawdza wartosc wlasna klonu - czy jest wlasciwa 
{
    if(who==NULL) 
            return false;
    if(who->wlasna_wartosc==0xFADEC0DE) 
            return false;
    if(who->wlasna_wartosc>licznik)
            return false;
    return true;
}

void informacja_klonalna::dolicz_indywiduum()
                       //Zliczanie urodzin
{
    zywych++;
}

bool informacja_klonalna::odlicz_indywiduum()
  //Zliczanie smierci
{
    zywych--;   assert(zywych<0xffffffff);
    martwych++; 

    if(zywych==0) //Ekstynkcja klonu
    {
        ostatnia_smierc=*zrodlo_makerow_czasu;
        
        if(tresh_kasowania>martwych //jesli klon bardzo maly
            &&                      //to podejmujemy probe usuniecia
           childs.CurrSize()==0     //o ile nie ma potomków 
            &&
           parent!=NULL             //i ma rodzica 
            )
        {                           
            for(unsigned i=0;i<parent->childs.CurrSize();i++)
                if(parent->childs[i]==this) //Szuka siebie na liscie dzieci swojego rodzica
                {
                    parent->childs.Del(i);
                    return true;
                }
        }
    }

    return false;
}

unsigned long informacja_klonalna::czas_zycia_klonu()
{
    unsigned long end=ostatnia_smierc;
    if(end==0) end=*zrodlo_makerow_czasu;
    unsigned long ret=end-data_powstania;
    return ret;
}

void informacja_klonalna::podlacz_marker_czasu(unsigned long* gdzie)
//Mozna wywolac tylko raz na program!!!
{
    static int only_one_call=0;
    assert(only_one_call==0);
    only_one_call++;

    //Ustalenie zrodla markerow czasu
    zrodlo_makerow_czasu=gdzie;
}

void informacja_klonalna::ustaw_maksimum_kasowania(unsigned long ile)
//Mozna wywolac tylko raz na program!!!
{
    static int only_one_call=0;
    assert(only_one_call==0);
    only_one_call++;

    tresh_kasowania=ile;
}

bool informacja_klonalna::for_each_child(_akcja_trawersowania what_to_do,void* user_data)
//Wykonanie akcji dla kazdego dziecka
{
 for(int i=0;i<childs.CurrSize();i++)
     if(childs[i]!=NULL)
    {
        bool ret=what_to_do(childs[i],user_data); 
        if(!ret) return false;
    }
 return true;   
}

bool informacja_klonalna::trawersuj_drzewo_pre(_akcja_trawersowania what_to_do,void* user_data)
//Trawersowanie z akcja wykonywana przed zajrzeniem do galezi
{
 bool ret=what_to_do(this,user_data);
 
 if(!ret) return false;
 
 for(int i=0;i<childs.CurrSize();i++)
     if(childs[i]!=NULL)
    {
        ret=childs[i]->trawersuj_drzewo_pre(what_to_do,user_data); 
        if(!ret) return false;
    }

 return true;   
}


bool informacja_klonalna::trawersuj_drzewo_post(_akcja_trawersowania what_to_do,void* user_data)
//Trawersowanie z akcja wykonywana po zajrzeniu do galezi
{
 for(int i=0;i<childs.CurrSize();i++)
   if(childs[i]!=NULL)
    {
        bool ret=childs[i]->trawersuj_drzewo_post(what_to_do,user_data); 
        if(!ret) return false;
    }

 return what_to_do(this,user_data);
}

unsigned long informacja_klonalna::tresh_kasowania=0;
unsigned long* informacja_klonalna::zrodlo_makerow_czasu=NULL;//Nie zainicjowana sensownie
unsigned long informacja_klonalna::licznik=0; //Ile jest klonów
unsigned long informacja_klonalna::usunieto=0;