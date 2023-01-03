// ekologia.hpp.h: interface for the ekologia class.
// MODYFIKACJE DLA UMOŻLIWIENIA KOMPILACJI 2022.07!
//*////////////////////////////////////////////////////////////////////

#ifndef _EKOLOGIA_HPP_
#define _EKOLOGIA_HPP_

#include "arrasour.hpp"
using namespace ::wbrtm;
//using wbrtm::array_template;

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
            Identyfikator=Ident; //Unikalny identyfikator klonu, bo wskaźnik do 'ori' może zostać użyty powtórnie
            X_lub_maska_ataku=X;
            Y_lub_maska_obrony=Y;
            Zywych=Z; //Pomaga też do wykrywania realokacji klonów (skasowanych)
            co=ori;
        }

        void set(unsigned Ident,informacja_troficzna* ori)
        {
            Identyfikator=Ident; //Unikalny identyfikator klonu, bo wskaźnik do 'ori' może zostać użyty powtórnie
            X_lub_maska_ataku=ori->X();
            Y_lub_maska_obrony=ori->Y();
            Zywych=ori->Waga(); //Pomaga też do wykrywania realokacji klonów (skasowanych)
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
    unsigned user_tax_tres;  //Treshold użytkownika, do użycia, jeśli inny niż z 'informacja_klonalna'.
	double conn_treshold;    //Wizualizuje tylko te połączenia, które stanowią większość zasilającej biomasy.
    agent::informacja_klonalna* ancestor; //Wskaźnik do wspólnego przodka. Żeby znajdować jego żywe klony.
    array_template<node_info>  nodes; //Lista węzłów sieci ekologicznej.
    array_template<connection> lines; //Lista linii rozpinających.

    struct_array_source<node_info,unsigned int>* pNodeX; //Współrzędne poziome węzłów w aranżacji
    struct_array_source<node_info,unsigned int>* pNodeY; //Współrzędne pionowe węzłów w aranżacji
    struct_array_source<node_info,unsigned int>* pNodeWeight; //Waga węzłów
	struct_array_source<connection,size_t>* pConnPrey; //Indeksy początków linii łączących węzły sieci
	struct_array_source<connection,size_t>* pConnPred; //Indeksy końców linii łączących węzły sieci
	struct_array_source<connection,unsigned long>* pConnWeight; //Przepływ w danej linii

	void _update_source_ptrs(); //Poprawia wskaźniki do tablic, które mogą się dezaktualizować podczas wypełniania list
	void _empty_source_ptrs(); //Zmienia źródła na puste (o długości 0)

	friend bool _moja_akcja_trawersowania(informacja_klonalna* current,void* user_data);

    //Mądrze rejestruje, jeśli jeszcze nie było, 'updejtuje' i ewentualnie przesuwa na liście
	bool _zarejestruj_z_sortowaniem(informacja_troficzna* what,unsigned ident);

	size_t _lowest_index_for_treshold(size_t node);
public:
	size_t HowManyNodes() { return nodes.CurrSize();}
	double connection_tres(){ return conn_treshold;}  //Jaka granica wizualizacji połączeń
	unsigned tax_size_tres();  //Jak istotne dla ekosystemu taksony wizualizuje
	void set_tax_size_tres(size_t T); //Ustawia granice wizualizacji (od następnego updajtu!)

	// Udostępnianie podstawowych serii danych do budowania sieci
	linear_source_base* NodeX(); //Położenie węzła, np. maska ataku
	linear_source_base* NodeY(); //Położenie węzła, np. maska obrony
	linear_source_base* NodeWeight(); //Waga węzła, np. liczba osobników (ewentualnie biomasa)
	//linear_source_base* NodeScaledWeight(); //Pierwiastkowana waga węzła, lepsze dla kółek

	// Informacje o połączeniach
	linear_source_base* ConnPrey(); //Start połączenia w ofierze (prey)
	linear_source_base* ConnPred(); //Koniec połączenia w drapieżniku (predator)
	linear_source_base* ConnWeight(); //Liczba ofiar lub pozyskana biomasa ofiar

	// Interfejs konieczny
	ekologia(agent::informacja_klonalna* TheAncestor,
			 unsigned int user_node_treshold=-1,
			 double connection_treshold=0);
	virtual ~ekologia();
	
	void operator = (agent::informacja_klonalna* TheAncestor);

	virtual void aktualizuj_liste_wezlow(bool leftrec=true); //Aktualizacja listy węzłów
	virtual void zapomnij_liste(bool zwolnij_pamiec=false); //Zapominanie listy z ewentualnym zwalnianiem pamięci
	size_t Znajdz(unsigned int Identyfikator);  //Znajduje węzeł o podanym identyfikatorze i zwraca jego indeks na liście

	void ZapiszWFormacieNET(ostream& out,unsigned size_tres=0,double weight_tres=0);
	void ZapiszWFormacieVNA(ostream& out,unsigned size_tres=0,double weight_tres=0);
};

// /////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Podręczna implementacja
// /////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Informacja od ilu osobników liczy się takson.
// Program wizualizuje tylko istotne dla ekosystemu taksony.
inline
unsigned ekologia::tax_size_tres()
{ 
    if(user_tax_tres!=-1)
		return user_tax_tres;
	else
		return informacja_klonalna::tresh_taksonow();
}

// Ustawia granice wizualizacji (od następnego updajtu!)
inline
void ekologia::set_tax_size_tres(size_t Tres)
{
	 user_tax_tres=Tres;
}

inline 
void ekologia::operator = (agent::informacja_klonalna* TheAncestor)
    {
        if(ancestor!=TheAncestor)
            zapomnij_liste(); //Zapominamy stare dane
        ancestor=TheAncestor;
    }


#endif // !defined(AFX_EKOLOGIA_HPP_H__5ED2EEC1_E452_4836_AAEC_BC300462CFB9__INCLUDED_)

// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Reactivated source code from Windows (2022.07)
/// @author Wojciech Borkowski
/// FOR @LICENCE SEE HERE: https://github.com/borkowsk/Coevo2_model
// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

