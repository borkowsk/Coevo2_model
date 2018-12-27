// klonalinfo.cpp: implementation of the klonalinfo class.
//
//////////////////////////////////////////////////////////////////////

#include "klonalinfo.hpp"
#include "assert.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

informacja_klonalna::informacja_klonalna(informacja_klonalna* iparent):
    parent(NULL),wlasna_wartosc(0),data_powstania(*zrodlo_makerow_czasu),ostatnia_smierc(0),zywych(0),martwych(0)
{
    assert(zrodlo_makerow_czasu!=NULL);
    wlasna_wartosc=++licznik;//Zwiekszenie i zapamientanie aktualnej wartosci licznika klonow

    parent=iparent;          //Zapamientanie klonu macierzystego/ojcowskiego
    
    if(parent!=NULL)//Jesli klon ma "ojca"
        parent->childs(parent->childs.CurrSize())=this;//I rejestracja nowego klonu jako dziecka klonu macierzystego
}

informacja_klonalna::~informacja_klonalna() //Destruktor chyba niepoptrzebny, ale jest na wszelki wypadek
{
    usunieto++;
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
           parent!=NULL             //i ma radzica 
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

unsigned long informacja_klonalna::tresh_kasowania=0;
unsigned long* informacja_klonalna::zrodlo_makerow_czasu=NULL;//Nie zainicjowana sensownie
unsigned long informacja_klonalna::licznik=0; //Ile jest klonów
unsigned long informacja_klonalna::usunieto=0;