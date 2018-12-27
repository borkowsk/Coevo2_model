// klonalinfo.hpp: interface for the klonalinfo class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_KLADYSTYKA_HPP__5A44210C_F5AD_4B72_9E75_E043CCEEAB52__INCLUDED_)
#define AFX_KLADYSTYKA_HPP__5A44210C_F5AD_4B72_9E75_E043CCEEAB52__INCLUDED_

#include "arrasour.hpp"
#include "klonalinfo.hpp"
#include "co_agent.hpp"

//Obiekt zbierajacy statystyki z dowolnego (pod)drzewa filogenetycznego
class klad
{
public:
    //Klasa trzymajaca dostep informacji klonalnej potomkow 
    struct line_info
    {
        size_t          parent_index;//Indeks rodzica na liscie
        agent::informacja_klonalna*    klon;//Wskaznik do oryginalnej informacji
        size_t          number_of_childs;//Liczba dzieci
        unsigned long   disperse;//Polozenie tego klonu szerokosci drzewa
        void set(size_t pind,
                 agent::informacja_klonalna* k,
                 unsigned long  d=0)
            {parent_index=pind;klon=k;disperse=d;}
        line_info():parent_index(0),klon(NULL),number_of_childs(0),disperse(0){}
    };
    //Informacja o wezlach ukladajacych galezie drzewa
    struct node_info
    {
        size_t   my_line;
        unsigned long Time;//Polozenie punktu w czasie
        unsigned long Disperse;//Polozenie na osi "rozpraszajacej"(rozmieszczenie)
        unsigned typ_wezla:2;//0 na galezi rodzica, 1 poczatkowy, 2 koncowy,3 wadliwy
        void set(unsigned typ,size_t li,unsigned long t,unsigned long d=0)
        {   typ_wezla=typ;assert(typ<3);
            my_line=li;
            Time=t;Disperse=d;
        }
        node_info():my_line(0),typ_wezla(3),Time(0),Disperse(0){}
    };
    //Informacja o polaczeniach miedzy wezlami
    struct connection
    {
        size_t start_node;
        size_t end_node;
        unsigned long weight;
        void set(size_t s,size_t e,unsigned long w=0)
            {start_node=s;end_node=e;weight=w;}
        connection():start_node(-1),end_node(0),weight(0){}
        //connection(size_t s,size_t e):start_node(s),end_node(e),weight(1){}       
    };
    
private:
    agent::informacja_klonalna* ancestor; //Wskaznik do wspólnego przodka
    size_t _for_simple_dispersing;
    struct_array_source<node_info,unsigned long>* pNodeTime;//Punkty czasowe wezlow drzewa: specjacji, poczatku i konca istnienia klonu (po 3 na klon)
    struct_array_source<node_info,unsigned long>* pNodeSpread;//Sztuczny rozrzut wezlów  dla czytelnosci drzewa
//  method_array_source<node_info,unsigned long>* pNodeWeights;//Kolorystyczne markery punktów - takie jak linii (np zeby byla narysowana skala)
    struct_array_source<connection,size_t>* pLineStarts;//Indeksy pocz¹tków linii laczacych wezly drzewa (po 2 na klon)
    struct_array_source<connection,size_t>* pLineEnds;//Indeksy koncow linii laczacych wezly drzewa (po 2 na klon)
    struct_array_source<connection,unsigned long>* pLineWeights;//Kolorystyczne markery lini
    void _update_source_ptrs();//Poprawia wskazniki do tablic, ktore moga sie dezaktualizowac podczas wypelniania list
    void _disperse_nodes1();//Algorytm rozstawiania drzewa - minimalny sensowny
    void _disperse_nodes2();//Algorytm rozstawiania drzewa - bardziej estetyczny
protected:
    //LISTY SLUZACE DO BYDOWANIA DRZEWA
    array_template<line_info> descendants; //Lista wszystkich taksonów/klonów potomnych
    array_template<node_info>  nodes;//Lista punktów rozpinajacych drzewo
    array_template<connection> lines;//Lista lini rozpinajacych
    
    //£AZENIE PO DRZEWIE
    //Klasa do przekazywania informacji zwrotnej
    struct _trawersal_info
    {
    klad&  This; //Dostep do kladu ktory jest budowany
    size_t Index;//Indeks klonu macierzystego
    agent::informacja_klonalna* Klon;//Wskaznik do klonu macierzystego
    size_t RetNumberOfDescendans;//Liczba wszystkich potomkow, do wypelnienia i zwrócenia
    //Konstrukcja
    _trawersal_info(klad& t,size_t i=0,agent::informacja_klonalna* k=NULL):This(t),Klon(k),Index(i),RetNumberOfDescendans(0){}
    _trawersal_info(const _trawersal_info& s,size_t i=0,agent::informacja_klonalna* k=NULL):This(s.This),Klon(k),Index(i),RetNumberOfDescendans(0){}
    };
    //Wlasciwe akcje do wypelninia list, glownie przez chodzenie po drzewie
    static
    bool _wypelniaj_listy(agent::informacja_klonalna* klon,_trawersal_info& Info);
    static
    bool _dodaj_do_listy(informacja_klonalna* klon,void* user_data);//"Downgrade typow" zeby uzgodnic z metoda for_each_child
public:
    //Udostepnianie podstawowych serii danych do budowania drzewa
    linear_source_base* NodeTime();//Punkty czasowe wezlow drzewa: specjacji, poczatku i konca istnienia klonu (po 3 na klon)
    linear_source_base* NodeSpread();//Sztuczny rozrzut wezlów  dla czytelnosci drzewa
    linear_source_base* LineStarts();//Indeksy poczatków linii laczacych wezly drzewa (po 2 na klon)
    linear_source_base* LineEnds();//Indeksy koncow linii laczacych wezly drzewa (po 2 na klon)
    //Informacje o wezlach
    linear_source_base* NodeWeight();//Domyslne kolorystyczne markery punktów
    //Informacje dodatkowe o liniach
    linear_source_base* LineSpec();//Specjalizacja lini wyrazona liczba wyzerowanych bitów
    linear_source_base* LineTrof();//Trofia lini wyrazona wartosci¹ 0..1^Liczba_bitów_maski_ataku
    linear_source_base* LineDefe();//Obronnosc lini wyrazona wartosci¹ 0..1^Liczba_bitów_maski_obrony
    //Domyslne kolorystyczne markery linii
    linear_source_base* LineWeights();//Domyslne kolorystyczne markery linii

    //Interfejs konieczny
    klad(agent::informacja_klonalna* TheAncestor);//Konstruktor	
    virtual ~klad();     //Destruktor - na wszelki wypadek
    
    void operator = (agent::informacja_klonalna* TheAncestor);
    
    virtual void aktualizuj_liste_zstepnych(bool leftrec=true); //Aktualizacja listy

    size_t HowManyClones() { return descendants.CurrSize();}
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Podreczna implementacja

inline 
void klad::operator = (agent::informacja_klonalna* TheAncestor)
    {
        if(ancestor!=TheAncestor)
            descendants.Truncate(0);//Zapominamy stara liste
        ancestor=TheAncestor;
    }

#endif // !defined(AFX_KLONALINFO_HPP__5A44210C_F5AD_4B72_9E75_E043CCEEAB52__INCLUDED_)
