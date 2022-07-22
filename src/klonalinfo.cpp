// klonalinfo.cpp: implementation of the klonalinfo class.
//
//*////////////////////////////////////////////////////////////////////

#include <assert.h>
#include <math.h>
#include "klonalinfo.hpp"


// ////////////////////////////////////////////////////////////////////
// Construction/Destruction
// ////////////////////////////////////////////////////////////////////

informacja_klonalna::informacja_klonalna(informacja_klonalna* iparent):
    parent(NULL),
    data_powstania(*zrodlo_makerow_czasu),
    ostatnia_smierc(0),
    zywych(0),
    martwych(0),
    wlasna_wartosc(0xFADEC0DE)
{
    assert(zrodlo_makerow_czasu!=NULL);
    wlasna_wartosc=++licznik; //Zwiększenie i zapamiętanie aktualnej wartości licznika klonów

    parent=iparent;           //Zapamiętanie klonu macierzystego/ojcowskiego
    
    if(parent!=NULL) //Jeśli klon ma "ojca"
        parent->childs(parent->childs.CurrSize())=this; //I rejestracja nowego klonu jako dziecka klonu macierzystego
}

informacja_klonalna::~informacja_klonalna() //Destruktor używany głównie dla zbyt małych klonów.
{
    usunieto++;
    wlasna_wartosc=0xFADEC0DE;
}

// Sprawdza wartość własną klonu. Czy jest właściwa?
bool informacja_klonalna::OK(informacja_klonalna* who)
{
    if(who==NULL) 
            return false;
    if(who->wlasna_wartosc==0xFADEC0DE) 
            return false;
    if(who->wlasna_wartosc>licznik)
            return false;
    return true;
}

// Zliczanie urodzin
void informacja_klonalna::dolicz_indywiduum()
{
    zywych++;
}

// Zliczanie śmierci
bool informacja_klonalna::odlicz_indywiduum()
{
    zywych--;                                                                                 assert(zywych<0xffffffff);
    martwych++; 

    if(zywych==0) //Ekstynkcja klonu
    {
        ostatnia_smierc=*zrodlo_makerow_czasu;
        
        if(tresh_kasowania>martwych //jeśli klon bardzo mały
            &&                      //to podejmujemy próbę usunięcia
           childs.CurrSize()==0     //o ile nie ma potomków 
            &&
           parent!=NULL             //i ma rodzica 
            )
        {                           
            for(unsigned i=0;i<parent->childs.CurrSize();i++)
                if(parent->childs[i]==this) //Szuka siebie na liście dzieci swojego rodzica
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

// Można wywołać tylko raz na program!!!
void informacja_klonalna::podlacz_marker_czasu(unsigned long* gdzie)
{
    static int only_one_call=0;
    assert(only_one_call==0);
    only_one_call++;

    //Ustalenie źródła markerów czasu
    zrodlo_makerow_czasu=gdzie;
}

// Można wywołać tylko raz na program!!!
void informacja_klonalna::ustaw_maksimum_kasowania(unsigned long ile)
{
    static int only_one_call=0;
    assert(only_one_call==0);
    only_one_call++;

    tresh_kasowania=ile;
}

// Wykonanie akcji dla każdego dziecka
bool informacja_klonalna::for_each_child(_akcja_trawersowania what_to_do,void* user_data)
{
 for(unsigned i=0;i<childs.CurrSize();i++)
     if(childs[i]!=NULL)
    {
        bool ret=what_to_do(childs[i],user_data); 
        if(!ret) return false;
    }
 return true;   
}

// Trawersowanie z akcją wykonywana przed zajrzeniem do gałęzi
bool informacja_klonalna::trawersuj_drzewo_pre(_akcja_trawersowania what_to_do,void* user_data)
{
 bool ret=what_to_do(this,user_data);
 
 if(!ret) return false;
 
 for(unsigned i=0;i<childs.CurrSize();i++)
     if(childs[i]!=NULL)
    {
        ret=childs[i]->trawersuj_drzewo_pre(what_to_do,user_data); 
        if(!ret) return false;
    }

 return true;   
}

// Trawersowanie z akcją wykonywana po zajrzeniu do gałęzi
bool informacja_klonalna::trawersuj_drzewo_post(_akcja_trawersowania what_to_do,void* user_data)
{
 for(unsigned i=0;i<childs.CurrSize();i++)
   if(childs[i]!=NULL)
    {
        bool ret=childs[i]->trawersuj_drzewo_post(what_to_do,user_data); 
        if(!ret) return false;
    }

 return what_to_do(this,user_data);
}


unsigned long informacja_klonalna::magnitude() 
{
	double pom=wszystkich();
	if(pom==0) return 0;
	pom=log(pom)/log(2.0);
	return (unsigned long)(pom);
} 	

unsigned long informacja_klonalna::tresh_kasowania=0;
unsigned long* informacja_klonalna::zrodlo_makerow_czasu=NULL; //Nie zainicjowana sensownie
unsigned long informacja_klonalna::licznik=0; //Ile jest klonów
unsigned long informacja_klonalna::usunieto=0;


// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Reactivated source code from Windows (2022.07)
/// @author Wojciech Borkowski
/// FOR @LICENCE SEE HERE: https://github.com/borkowsk/Coevo2_model
// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
