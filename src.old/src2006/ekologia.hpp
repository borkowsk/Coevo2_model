// ekologia.hpp.h: interface for the ekologia class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_EKOLOGIA_HPP_H__5ED2EEC1_E452_4836_AAEC_BC300462CFB9__INCLUDED_)
#define AFX_EKOLOGIA_HPP_H__5ED2EEC1_E452_4836_AAEC_BC300462CFB9__INCLUDED_

#include "arrasour.hpp"
#include "klonalinfo.hpp"
#include "troficinfo.h"
#include "co_agent.hpp"

class ekologia  
{
    struct node_info
    {
        
        unsigned X_lub_maska_ataku;
        unsigned Y_lub_maska_obrony;
        unsigned Zywych;
        unsigned Identyfikator;
        informacja_troficzna* co;

        void set(unsigned Ident,unsigned X,unsigned Y,unsigned Z,informacja_troficzna* ori)
        {
            Identyfikator=Ident;//Unikalny identyfikator klonu - bo wskaznik do ori moze zostac uzyty powtornie
            X_lub_maska_ataku=X;
            Y_lub_maska_obrony=Y;
            Zywych=Z;//Pomaga tez do wykrywania realokacji klon�w (skasowanych)
            co=ori;
        }

        void set(unsigned Ident,informacja_troficzna* ori)
        {
            Identyfikator=Ident;//Unikalny identyfikator klonu - bo wskaznik do ori moze zostac uzyty powtornie
            X_lub_maska_ataku=ori->X();
            Y_lub_maska_obrony=ori->Y();
            Zywych=ori->Waga();//Pomaga tez do wykrywania realokacji klon�w (skasowanych)
            co=ori;
        }
    };

    struct connection
    {
        size_t start_node;
        size_t end_node;
        unsigned long weight;
        void set(size_t s,size_t e,unsigned long w=0)
            {start_node=s;end_node=e;weight=w;}
        connection():start_node(-1),end_node(0),weight(0){}
    };

    static
    void _swap(node_info& f,node_info& s);
private:
    unsigned user_tax_tres;  //Treshold uzytkownika - jesli inny niz z informacja_klonalna
    double conn_treshold;    //Wizualizuje tylko te polaczenia, ktore stanowia "gro" zasilajacej biomasy
    agent::informacja_klonalna* ancestor; //Wskaznik do wsp�lnego przodka - zeby znajdowac zywe klony
    array_template<node_info>  nodes;//Lista wez��w sieci ekologicznej
    array_template<connection> lines;//Lista lini rozpinajacych

    struct_array_source<node_info,unsigned int>* pNodeX;//Wspolrzedne wezlow w aranzacji
    struct_array_source<node_info,unsigned int>* pNodeY;//------------//----------------
    struct_array_source<node_info,unsigned int>* pNodeWeight;//Waga wezlow
    struct_array_source<connection,size_t>* pConnPrey;//Indeksy pocz�tk�w linii laczacych wezly sieci
    struct_array_source<connection,size_t>* pConnPred;//Indeksy koncow linii laczacych wezly sieci
    struct_array_source<connection,unsigned long>* pConnWeight;//Przeplyw w danej linii
   

    void _update_source_ptrs();//Poprawia wskazniki do tablic, ktore moga sie dezaktualizowac podczas wypelniania list
    void _empty_source_ptrs();//Zmienia zrodla na puste (o dlugosci 0)

    friend bool _moja_akcja_trawersowania(informacja_klonalna* current,void* user_data);
    bool _zarejestruj_z_sortowaniem(informacja_troficzna* what,unsigned ident);//madrze rejestruje, jesli jeszcze nie bylo, 'updejtuje" i ewentualnie przesuwa na liscie
    size_t _lowest_index_for_treshold(size_t node);
public:
    //Udostepnianie podstawowych serii danych do budowania sieci
    linear_source_base* NodeX();//Polozenie wezla - np. maska ataku
    linear_source_base* NodeY();//Polozenie wezla - np. maska obrony
    linear_source_base* NodeWeight();//Waga wezla - np. liczba osobnik�w (ewentualnie biomasa)
    //linear_source_base* NodeScaledWeight();//Pierwiastkowana waga wezla - lepsze dla k�ek
    
    //Informacje o polaczeniach
    linear_source_base* ConnPrey();//Start polaczenia w ofierze (prey)
    linear_source_base* ConnPred();//Koniec polaczenia w drapiezniku (predator)
    linear_source_base* ConnWeight();//Liczba ofiar lub  pozyskana biomasa ofiar

    //Interfejs konieczny
	ekologia(agent::informacja_klonalna* TheAncestor);
	virtual ~ekologia();
    void operator = (agent::informacja_klonalna* TheAncestor);
 
    virtual void aktualizuj_liste_wezlow(bool leftrec=true); //Aktualizacja listy wezl�w
    virtual void zapomnij_liste(bool zwolnij_pamiec=false); //Zapominanie listy z ewentualnym zwalnianiem pamieci
    size_t Znajdz(unsigned int Identyfikator);  //Znajduje wezel o podanym identyfikatorze i zwraca jego indeks na liscie
    size_t HowManyNodes() { return nodes.CurrSize();}
    unsigned tax_size_tres();  //Wizualizuje tylko istotne dla ekosystemu taksony
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Podreczna implementacja
inline
unsigned ekologia::tax_size_tres() //Wizualizuje tylko istotne dla ekosystemu taksony
{ 
    if(user_tax_tres!=-1)
        return user_tax_tres;
    else
        return informacja_klonalna::tresh_taksonow(); 
}

inline 
void ekologia::operator = (agent::informacja_klonalna* TheAncestor)
    {
        if(ancestor!=TheAncestor)
            zapomnij_liste();//Zapominamy stare dane
        ancestor=TheAncestor;
    }


#endif // !defined(AFX_EKOLOGIA_HPP_H__5ED2EEC1_E452_4836_AAEC_BC300462CFB9__INCLUDED_)
