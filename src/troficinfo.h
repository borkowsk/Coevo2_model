// troficinfo.h: interface for the informacja_troficzna class.
// MODYFIKACJE DLA UMOŻLIWIENIA KOMPILACJI 2022.07!
//*////////////////////////////////////////////////////////////////////

#if !defined(AFX_TROFICINFO_H__D045F0BF_A73E_4B64_8BD7_9E70DFC79225__INCLUDED_)
#define AFX_TROFICINFO_H__D045F0BF_A73E_4B64_8BD7_9E70DFC79225__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "arrays.hpp"
using ::wbrtm::array_template;

class informacja_troficzna;

// Klasa służąca do śledzenie kontaktów (troficznych) agenta lub całego klonu
class informacja_troficzna  
{
    struct contacts
    {
        unsigned long int marker;
        unsigned long int counter;
        double weight;

        contacts():marker(0),counter(0),weight(0){}
        void set(unsigned long int m,double w){marker=m;weight=w;counter=1;}
        void inc(double w){weight+=w;counter++;}
    };
public:
	informacja_troficzna();
	virtual ~informacja_troficzna();
    //Czyszczenie kontaktów
    virtual void czysc_kontakty(bool freememory=false);
    //Zapisywanie tablicy kontaktów
    virtual void dolicz_kontakt(unsigned long int marker_z_kim,double waga);
    //void dolicz_kontakt(void* marker_z_kim,double waga); //jednak niepotrzebne

    //Dostęp do odczytu tablicy kontaktów
    //const array_template<const contacts>& tablica_kontaktow(); //???
    const contacts& operator [] (size_t ind);     
    unsigned AktualnaLiczbaKontaktow();   //Proste czytanie aktualnej liczby kontaktów
    unsigned MaksymalnaLiczbaKontaktow(); //Dostęp do maksymalnej liczby kontaktów
    virtual
    unsigned Waga()=0; //Domyślna waga dla węzła (liczba osobników lub biomasa)
    virtual  
    unsigned X()=0; //X domyślnego położenia węzła, np. maska ataku
    virtual 
    unsigned Y()=0; //Y domyślnego położenia węzła, np. maska obrony
private:
    unsigned max_num_of_contacts;      //Maksymalna liczba kontaktów w życiu klonu
	array_template<contacts> kontakty; //Tablica na zapis kontaktów
    void _swap(size_t i,size_t j);     //Zamienia elementy w powyższej tablicy
};

inline
const informacja_troficzna::contacts& informacja_troficzna::operator [] (size_t ind)
{
                            assert(ind<kontakty.CurrSize());
    return kontakty[ind];
}

/*
inline
const array_template<const informacja_troficzna::contacts>& informacja_troficzna::tablica_kontaktow()
{
    //Strasznie mocne, ale jak inaczej przekazać modyfikowalna tablice jako niemodyfikowalną?
    return *reinterpret_cast< array_template<const contacts>* >(&kontakty);
}
*/

inline
unsigned informacja_troficzna::MaksymalnaLiczbaKontaktow()
{
    if(max_num_of_contacts<kontakty.CurrSize())
        max_num_of_contacts=kontakty.CurrSize();
    return max_num_of_contacts;
}

inline
unsigned informacja_troficzna::AktualnaLiczbaKontaktow()
{
    return kontakty.CurrSize();
}

inline
unsigned informacja_troficzna::Waga()
{
    return 0;//Bo tak naprawdę to trzeba to reimplementować w klasach potomnych
}

inline
unsigned informacja_troficzna::X()
{
    return 0;//Bo tak naprawdę to trzeba to reimplementować w klasach potomnych
}

inline
unsigned informacja_troficzna::Y()
{
    return 0;//Bo tak naprawdę to trzeba to reimplementować w klasach potomnych
}


/*inline
void informacja_troficzna::dolicz_kontakt(void* marker_z_kim,double waga)
{
    dolicz_kontakt((unsigned long int)(marker_z_kim),waga);
}*/

#endif // !defined(AFX_TROFICINFO_H__D045F0BF_A73E_4B64_8BD7_9E70DFC79225__INCLUDED_)

// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Reactivated source code from Windows (2022.07)
/// @author Wojciech Borkowski
/// FOR @LICENCE SEE HERE: https://github.com/borkowsk/Coevo2_model
// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

