#ifndef _COEWO_AGENT_HPP_
#define _COEWO_AGENT_HPP_
/* ROZMIAR TYPU BASE DECYDUJE O MOZLIWEJ KOMPLIKACJI SWIATA */
/* JEST TYLE MOZLIWYCH TAXONOW ILE WZORCOW BITOWYCH W base2 */
#include "bits.h"
typedef unsigned char base;   // musi byc bez znaku
typedef unsigned short base2; // musi miescic 2 zmienne base

const unsigned BITS_PER_GENOM=16;   //!!! TO JEST ZAFIKSOWANE PRZEZ ROZMIAR base i base2
const base2 MAXBASE2=(base2)0xffffffffffffffffUL;
const base  MAXBASE =(base)MAXBASE2;
const base  AUTOTROF=MAXBASE;// wzor bitowy autotrofa - swiat go zywi



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

//WLASCIWA DEKLARACJA AGENTA
///////////////////////////////////////////////////
class agent//:public agent_base
{    
public:
    class informacja_klonalna:public ::informacja_klonalna
    {
        wzor genom; //zeby bylo wiadomo co to za klon
        int _specjalised;
//        int _defensespec;
//        int _feedingspec;
    public:
        informacja_klonalna(informacja_klonalna* parent,wzor wgenom):
          genom(wgenom),::informacja_klonalna(parent)
          {
              _specjalised=bits(genom.w.geba)+bits(genom.w.oslona);
//              _defensespec=bits(genom.w.oslona);
//              _feedingspec=bits(genom.w.geba);
          }
          int   how_specialised() //Poziom specjalizacji mierzony w bitach
          {     return _specjalised;    }
/*          
          int   how_specialised_in_defense() //Poziom specjalizacji mierzony w bitach
          {     return _defensespec;    }
          
          int   how_specialised_in_feeding() //Poziom specjalizacji mierzony w bitach
          {     return _feedingspec;    }
*/
    };

    wzor			 w; // wzor bitowy taxonu
    
    unsigned char wiek; // ile razy byl u sterowania, po przerolowaniu - smierc
    unsigned	  sila; // zgromadzona energia
    informacja_klonalna* klon;//Przynaleznosc klonalna i zwiazane z tym informacje
  
    /* Informacje statystyczne dotyczace indywiduow i klonów */
    static unsigned max;//jaki jest najwiekszy taxon
    static unsigned max_change;//i czy ostatnio max sie zmienil
    static unsigned ile_ind;// ile jest zywych indywiduow
    static unsigned ile_tax;// ile taxonow niezerowych
    static unsigned ile_big_tax;// ile taxonow liczniejszych niz treshold
    static unsigned liczniki[/*(size_t)MAXBASE2+1*/];// Liczniki liczebnosci aktualnie istniejacych 'taxonow ekologicznych" wg bitów wzoru
     
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
    bool  jest_zywy()
    { 
        if(sila!=0 && wiek!=0) 
        {
            assert(klon!=NULL);
            return true;
        }
        else
            return false;
    }
  
    bool  is_alive() { return jest_zywy();} //Bo wymaga tego klasa layer i pochodne
    
    unsigned long  how_old_taxon()
    {
       if(!jest_zywy()) return 0xffffffff;
       else return klon->data_powstania_klonu();
    }

    int   how_specialised() //Poziom specjalizacji mierzony w bitach
    {
        if(!jest_zywy()) return -1;
        else return klon->how_specialised();
    }
    
    int   how_specialised_autotrof() //Poziom specjalizacji mierzony w bitach tylko dla autotrofów
    {
        if(!jest_zywy() || !(w.w.geba==0xff)) return -1;
        else return klon->how_specialised();
    }
    
    int   how_specialised_heterotrof() //Poziom specjalizacji mierzony w bitach tylko dla autotrofów
    {
        if(!jest_zywy() || (w.w.geba==0xff)) return -1;
        else return klon->how_specialised();
    }
/*
    int   how_specialised_in_defense() //Poziom specjalizacji maski obrony mierzony w bitach
    {
        if(!jest_zywy()) return -1;
        else return klon->how_specialised_in_defense();
    }

    int   how_specialised_in_feeding() //Poziom specjalizacji maski ataku mierzony w bitach
    {
        if(!jest_zywy()) return -1;
        else return klon->how_specialised_in_feeding();
    }
*/
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

#endif