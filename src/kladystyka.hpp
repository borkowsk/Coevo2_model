// klonalinfo.hpp: interface for the klonalinfo class.
//
//*////////////////////////////////////////////////////////////////////

#if !defined(AFX_KLADYSTYKA_HPP__5A44210C_F5AD_4B72_9E75_E043CCEEAB52__INCLUDED_)
#define AFX_KLADYSTYKA_HPP__5A44210C_F5AD_4B72_9E75_E043CCEEAB52__INCLUDED_

//#include "SYMSHELL/arrasour.hpp"
#include "klonalinfo.hpp"
#include "troficinfo.h"
#include "co_agent.hpp"

// Obiekt zbierający statystyki z dowolnego (pod)drzewa filogenetycznego
class klad
{
public:
	//Klasa trzymająca dostęp informacji klonalnej potomków
	struct line_info
	{
		size_t          parent_index; //Indeks rodzica na liście
		agent::informacja_klonalna*    klon; //Wskaźnik do oryginalnej informacji
		size_t          number_of_childs; //Liczba dzieci
		unsigned long   disperse; //Położenie tego klonu na szerokości drzewa
		
		void set(size_t pind,
				 agent::informacja_klonalna* k,
				 unsigned long  d=0)
			{
				parent_index=pind;
				klon=k;
				disperse=d;
			}
		
		line_info():
			parent_index(0),
			klon(NULL),
			number_of_childs(0),
			disperse(0){}
	};

	// Informacja o węzłach układających gałęzie drzewa
	struct node_info
	{
		size_t   my_line;
		unsigned long Time; //Położenie punktu w czasie
		unsigned long Disperse; //Położenie na osi 'rozpraszającej' - (rozmieszczenie)
		unsigned typ_wezla:2; //0 na gałęzi rodzica, 1 początkowy, 2 końcowy,3 wadliwy

		void set(unsigned typ,size_t li,unsigned long t,unsigned long d=0)
		{   typ_wezla=typ;assert(typ<3);
			my_line=li;
			Time=t;Disperse=d;
		}
		node_info():my_line(0),typ_wezla(3),Time(0),Disperse(0){}
	};

	//Informacja o połączeniach między węzłami
	struct connection
	{
		size_t start_node;
		size_t end_node;
		unsigned long node_weight;
		void set_attr(size_t s,size_t e,unsigned long w=0)
			{start_node=s;end_node=e;node_weight=w;}
		connection():start_node(-1),end_node(0),node_weight(0){}
		//connection(size_t s,size_t e):start_node(s),end_node(e),weight(1){}       
	};
	
private:
	agent::informacja_klonalna* ancestor; //Wskaźnik do wspólnego przodka
//	unsigned (agent::informacja_klonalna::*how_weighted)();//Wskaźnik do funkcji zwracającej jakiś atrybut klonu jako wagę połączenia
	size_t _for_simple_dispersing;
	
	struct_array_source<node_info,unsigned long>* pNodeTime;   //Punkty czasowe węzłów drzewa: specjacji, początku i konca istnienia klonu (po 3 na klon)
	struct_array_source<node_info,unsigned long>* pNodeSpread; //Sztuczny rozrzut węzłów dla czytelności drzewa
//  method_array_source<node_info,unsigned long>* pNodeWeights; //Kolorystyczne markery punktów. Takie jak linii (np. żeby była narysowana skala)
	struct_array_source<connection,size_t>* pLineStarts; //Indeksy początków linii łączących węzły drzewa (po 2 na klon)
	struct_array_source<connection,size_t>* pLineEnds;   //Indeksy końców linii łączących węzły drzewa (po 2 na klon)
	struct_array_source<connection,unsigned long>* pLineWeights; //Kolorystyczne markery linii
   
	void _update_source_ptrs(); //Poprawia wskaźniki do tablic, które mogą się dezaktualizować podczas wypełniania list
	void _empty_source_ptrs();  //Zmienia źródła na puste (o długości 0)
	void _disperse_nodes1();    //Algorytm rozstawiania drzewa (minimalny sensowny)
	void _disperse_nodes2();    //Algorytm rozstawiania drzewa (bardziej estetyczny - nie zaimplementowany)
protected:
	//LISTY SŁUŻĄCE DO BUDOWANIA DRZEWA
	array_template<line_info> descendants; //Lista wszystkich taksonów/klonów potomnych
	array_template<node_info>  nodes; //Lista punktów rozpinających drzewo
	array_template<connection> lines; //Lista linii rozpinających
	
	//ŁAŻENIE PO DRZEWIE
	//Klasa do przekazywania informacji zwrotnej
	struct _trawersal_info
	{
	klad&  This;  //Dostęp do obiektu "klad", który jest budowany
	size_t Index; //Indeks klonu macierzystego
	agent::informacja_klonalna* Klon; //Wskaźnik do klonu macierzystego
	size_t RetNumberOfDescendans; //Liczba wszystkich potomków, do wypełnienia i zwrócenia
	//Konstrukcja
	_trawersal_info(klad& t,size_t i=0,agent::informacja_klonalna* k=NULL):This(t),Klon(k),Index(i),RetNumberOfDescendans(0){}
	_trawersal_info(const _trawersal_info& s,size_t i=0,agent::informacja_klonalna* k=NULL):This(s.This),Klon(k),Index(i),RetNumberOfDescendans(0){}
	};
	
	// Chodzenie po drzewie klonów w celu wizualizacji i nie tylko
	// Wypełnianie list do wizualizacji drzewa
	static
	bool _wypelniaj_listy(agent::informacja_klonalna* klon,_trawersal_info& Info);

	static //Opakowanie bez typów zgodne z wymaganiami metody for_each_child
	bool _dodaj_do_listy(informacja_klonalna* klon,void* user_data); //'Downgrade typów' żeby uzgodnić z metodą for_each_child

	//Chodzenie po drzewie klonów w celu wypisania do pliku
	struct _filogOutPutInfo
	{
	klad&  This; //Dostęp do obiektu 'klad', który jest wypisywany
	agent::informacja_klonalna* Klon; //Wskaźnik do klonu macierzystego
	ostream& out;
	unsigned min_time;
	unsigned max_time;
	unsigned size_tres;
	_filogOutPutInfo(klad& t,ostream& iout,unsigned imin_time,unsigned imax_time,unsigned isize_tres,agent::informacja_klonalna* k=NULL):
		This(t),out(iout),min_time(imin_time),max_time(imax_time),size_tres(isize_tres),Klon(k)
	{}
	};
	
	static 
	bool _wypisz_poddrzewo(agent::informacja_klonalna* klon,_filogOutPutInfo& Info);
	static //Opakowanie bez typów zgodne z wymaganiami metody for_each_child
	bool _wypisz_takson(informacja_klonalna* klon,void* user_data); //'Downgrade typów' żeby uzgodnić z metodą for_each_child
public:
	// Udostępnianie podstawowych serii danych do budowania drzewa
	linear_source_base* NodeTime();   //Punkty czasowe węzłów drzewa: specjacji, początku i konca istnienia klonu (po 3 na klon)
	linear_source_base* NodeSpread(); //Sztuczny rozrzut węzłów dla czytelności drzewa
	linear_source_base* LineStarts(); //Indeksy początków linii łączących węzły drzewa (po 2 na klon)
	linear_source_base* LineEnds();   //Indeksy końców linii łączących węzły drzewa (po 2 na klon)

	//Informacje o węzłach
	linear_source_base* NodeWeight(); //Domyślne kolorystyczne markery punktów
	//Informacje dodatkowe o kladach
//    linear_source_base* LineSpec(); //Specjalizacja linii wyrażona liczba wyzerowanych bitów
//    linear_source_base* LineTrof(); //Trofia linii wyrażona wartością 0..1^Liczba_bitów_maski_ataku
//    linear_source_base* LineDefe(); //Obronność linii wyrażona wartością 0..1^Liczba_bitów_maski_obrony
	

	//Interfejs konieczny
	klad(agent::informacja_klonalna* TheAncestor);  //Konstruktor

	virtual ~klad();     //Destruktor. Na wszelki wypadek
	
	void operator = (agent::informacja_klonalna* TheAncestor);
	
	virtual void aktualizuj_liste_zstepnych(bool leftrec=true); //Aktualizacja listy

    virtual void zapomnij_liste(bool zwolnij_pamiec=false); //Zapomina listy i ewentualnie zwalnia pamięć.

    size_t HowManyClones() { return descendants.CurrSize();}

	void ZapiszTxt(ostream& out,unsigned min_time=0,unsigned max_time=0,unsigned size_tres=0);
	
	enum Kolorowanie {K_SPECJALIZACJA=1,K_TROFIA,K_OBRONA,K_SPECTROFIA,K_SPECOBRONA,K_ILUBYLOIJEST};

    //Domyślne kolorystyczne markery linii
	linear_source_base* LineWeights(); //Domyślne kolorystyczne markery linii
	void ChangeLineWeightsSource(Kolorowanie co); //Zmiana kolorowania kladów
};

// ///////////////////////////////////////////////////////////
// Podręczna implementacja
// ////////////////////////////////////////////////////////////
inline 
void klad::operator = (agent::informacja_klonalna* TheAncestor)
    {
        if(ancestor!=TheAncestor)
			zapomnij_liste(); //Zapominamy stare dane
        ancestor=TheAncestor;
    }

#endif // !defined(AFX_KLONALINFO_HPP__5A44210C_F5AD_4B72_9E75_E043CCEEAB52__INCLUDED_)

// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Reactivated source code from Windows (2022.07)
/// @author Wojciech Borkowski
/// FOR @LICENCE SEE HERE: https://github.com/borkowsk/Coevo2_model
// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

