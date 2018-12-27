// troficinfo.cpp: implementation of the informacja_troficzna class.
//
//////////////////////////////////////////////////////////////////////

#include "troficinfo.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

informacja_troficzna::informacja_troficzna():max_num_of_contacts(0)
{

}

informacja_troficzna::~informacja_troficzna()
{
    czysc_kontakty(false);//Nie trzeba fizycznie bo i tak zaraz tablice diabli wezma
    max_num_of_contacts=0;
}

void informacja_troficzna::czysc_kontakty(bool freememory)
{
    if(max_num_of_contacts<kontakty.CurrSize())
        max_num_of_contacts=kontakty.CurrSize();

    kontakty.Truncate(0);
    if(freememory)
        kontakty.Deallocate();
}
   
inline void informacja_troficzna::_swap(size_t i,size_t j)
{
   char buf[sizeof(contacts)];
   memcpy(buf,&kontakty[i],sizeof(contacts));
   memcpy(&kontakty[i],&kontakty[j],sizeof(contacts)); 
   memcpy(&kontakty[j],buf,sizeof(contacts));  
}

//Zapisywanie tablicy kontaktow
void informacja_troficzna::dolicz_kontakt(unsigned long int marker_z_kim,double waga)
{
    size_t ile_slotow=kontakty.CurrSize();
    size_t i=0;//pozycja slotu ktory modyfikujemy
    
    //szukanie czy juz byl taki i inkrementacja
    for(;i<ile_slotow;i++)
        if(kontakty[i].marker==marker_z_kim)
        {   //Znaleziono
            kontakty[i].inc(waga);//Inkrementacja wagi kontaktu            
            goto SORTOWANKO;//CO BYLO DO ZROBIENIA
        }
    
    //Jesli sie tu znalezlismy to jest to na pewno nowy kontakt
    assert(i==ile_slotow);//czyli gdy 'i' jest za tablica
    kontakty(i).set(marker_z_kim,waga);

SORTOWANKO://Jak juz napewno kontakt zarejestrowany to moze trzeba go przesunac blizej czolo listy
        for(;i>0 && kontakty[i].weight>kontakty[i-1].weight;i--)
        {
            _swap(i,i-1);
        }
}
 