// klonalinfo.hpp: interface for the klonalinfo class.
//*////////////////////////////////////////////////////////////////////

#if !defined(AFX_KLONALINFO_HPP__5A44210C_F5AD_4B72_9E75_E043CCEEAB52__INCLUDED_)
#define AFX_KLONALINFO_HPP__5A44210C_F5AD_4B72_9E75_E043CCEEAB52__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "arrays.hpp"
using namespace ::wbrtm;

class informacja_klonalna;

// Akcja do wykonania na węźle drzewa taksonów.
// Jeśli zwróci false to trawersowanie jest przerywane
typedef bool (*_akcja_trawersowania)(informacja_klonalna* current,
									 void* user_data
                                     );
   
// Przynależność klonalna agenta i związane z tym informacje
class informacja_klonalna
{   
   static unsigned long     licznik; //Ile było klonów od początku? Może byc tylko powiększany. Służy zapewnieniu unikalnych identyfikatorów klonów
   static unsigned long     usunieto; //Ile zostało usuniętych jako nieudane
   static unsigned long*    zrodlo_makerow_czasu; //Referencja dostarczana przez użytkownika klasy
   static unsigned long     tresh_kasowania; //Przy jakim maksymalnym rozmiarze klonu wolno podjąć probe jego kasowania

   informacja_klonalna* parent; //Wskaźnik do klonu macierzystego
   array_template<informacja_klonalna*> childs; //Wskaźniki do klonów potomnych
   
   unsigned long data_powstania; //Kiedy powstał klon
   unsigned long ostatnia_smierc; //'Data' ostatniej śmierci
   unsigned long zywych;         //Ile jest w tej chwili żywych
   unsigned long martwych;       //Ile jest już martwych od początku istnienia klonu
   unsigned long wlasna_wartosc; //Własny numer klonu

public:
   // Składowe statyczne, czyli wspólne dla klasy
   static bool OK(informacja_klonalna* who);   //Sprawdza wartość własną klonu. Czy jest właściwa?
   static void podlacz_marker_czasu(unsigned long* gdzie);  //Można wywołać tylko raz na program!!!
   static void ustaw_maksimum_kasowania(unsigned long ile); //Można wywołać tylko raz na program!!!
   static unsigned long tresh_taksonow() { return tresh_kasowania;} //Jaki jest w tej symulacji próg na rozmiar taksonu?
   static unsigned long ile_klonow() { return licznik; }
   static unsigned long ile_taksonow() { return licznik-usunieto; } 

   // Tworzenie/niszczenie
   informacja_klonalna(informacja_klonalna* parent);//Konstruktor	
   virtual ~informacja_klonalna();             //Destruktor chyba niepotrzebny, ale jest na wszelki wypadek
	  
   // Składowe indywidualne
   virtual void dolicz_indywiduum();           //Zliczanie urodzin
   virtual bool odlicz_indywiduum();           //Zliczanie śmierci. Jeśli 'true' to trzeba/można usunać obiekt

   unsigned long identyfikator(); 
   unsigned long data_powstania_klonu(); 
   unsigned long data_wymarcia_klonu();
   unsigned long czas_zycia_klonu();
   unsigned long ile_zywych();
   unsigned long wszystkich();
   unsigned long magnitude();
   
   bool for_each_child(_akcja_trawersowania what_to_do,void* user_data); //Wykonanie akcji dla każdego dziecka
   bool trawersuj_drzewo_pre(_akcja_trawersowania what_to_do,void* user_data); //Trawersowanie z akcją wykonywana przed zajrzeniem do gałęzi
   bool trawersuj_drzewo_post(_akcja_trawersowania what_to_do,void* user_data); //Trawersowanie z akcją wykonywana po zajrzeniu do gałęzi
};


//*/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Podręczna implementacja
//*/////////////////////////////////////////////////////////////////////////////////////////////////////////////
inline
unsigned long informacja_klonalna::identyfikator()
	{ return wlasna_wartosc; }

inline 
unsigned long informacja_klonalna::data_powstania_klonu() 
	{ return data_powstania; }

inline 
unsigned long informacja_klonalna::data_wymarcia_klonu() 
	{ return ostatnia_smierc; }

inline 
unsigned long informacja_klonalna::ile_zywych() 
	{ return zywych; }

inline 
unsigned long informacja_klonalna::wszystkich() 
	{ return zywych+martwych; }

#endif // !defined(AFX_KLONALINFO_HPP__5A44210C_F5AD_4B72_9E75_E043CCEEAB52__INCLUDED_)

// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Reactivated source code from Windows (2022.07)
/// @author Wojciech Borkowski
/// FOR @LICENCE SEE HERE: https://github.com/borkowsk/Coevo2_model
// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

