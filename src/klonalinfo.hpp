// klonalinfo.hpp: interface for the klonalinfo class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_KLONALINFO_HPP__5A44210C_F5AD_4B72_9E75_E043CCEEAB52__INCLUDED_)
#define AFX_KLONALINFO_HPP__5A44210C_F5AD_4B72_9E75_E043CCEEAB52__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "arrays.hpp"

//Przynaleznosc klonalna agenta i zwiazane z tym informacje
class informacja_klonalna
{   
   static unsigned long     licznik; //Ile bylo klonów od poczatku
   static unsigned long     usunieto;//Ile zostalo usunietych jako nieudane
   static unsigned long*    zrodlo_makerow_czasu;//Referencja dostarczana przez uzytkownika klasy
   static unsigned long     tresh_kasowania; //Przy jakim maksymalnym rozmiarze klonu wolno podjac probe jego kasowania 

   informacja_klonalna* parent; //Wskaznik do klonu macierzystego
   array_template<informacja_klonalna*> childs; //Wskazniki do klonów potomnych

   unsigned long wlasna_wartosc; //Wlasny numer klonu
   unsigned long data_powstania; //Kiedy powstal klon
   unsigned long ostatnia_smierc; //"Data" ostatniej smierci
   unsigned long zywych;         //Ile jest w tej chwili zywych
   unsigned long martwych;       //Ile jest juz martwych od poczatku istnienia klonu
public:
   informacja_klonalna(informacja_klonalna* parent);//Konstruktor	
   virtual ~informacja_klonalna();             //Destruktor chyba niepotrzebny, ale jest na wszelki wypadek
   virtual void dolicz_indywiduum();           //Zliczanie urodzin
   virtual bool odlicz_indywiduum();           //Zliczanie smierci. Jesli "true" to trzeba usunac obiekt

   static void podlacz_marker_czasu(unsigned long* gdzie); //Mozna wywolac tylko raz na program!!!
   static void ustaw_maksimum_kasowania(unsigned long ile);//Mozna wywolac tylko raz na program!!!
   static unsigned long ile_klonow() { return licznik; }
   static unsigned long ile_taksonow() { return licznik-usunieto; } 
};

#endif // !defined(AFX_KLONALINFO_HPP__5A44210C_F5AD_4B72_9E75_E043CCEEAB52__INCLUDED_)
