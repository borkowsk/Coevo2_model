//
//Zrodla do demonstracji interakcji drapieznik/ofiara za pomoca masek bitowych
/////////////////////////////////////////////////////////////////////////////////
#ifndef __COEWO_INTERACTIONS_SOURCES_HPP_
#define __COEWO_INTERACTIONS_SOURCES_HPP_

#include "SYMSHELL/datasour.hpp"
//Klasa udostepniajaca dowolna tablice prostokatna oraz jej wycinki.
//Jesli zdefiniowany wycinek wykracza poza tablice zrodlowa to funkcja
//get zwraca wartosc 'miss' podawana w konstruktorze.Alternatywnie
//wycinek moze realizowac geometrie torusa i wtedy miss nie jest
//potrzebne.

class and_interaction_source:public rectangle_source_base
//--------------------------------------------------------------
{
unsigned bit_and(unsigned A,unsigned B) //Po co? Takie sobie æwiczenie?
	{  return A&B;  }

public:
// Constructor
and_interaction_source( const char* itit):
	rectangle_source_base(itit,256,256,1/*,subs,imiss*/){}

~and_interaction_source()
	{
#ifndef NDEBUG
	cerr<<"~and_interaction_source():"<<name()<<'\n';
#endif
	}

void  bounds(size_t& num,double& min,double& max)
//Ile elementow,wartosc minimalna i maksymalna
	{
	num=getrectgeometry()->get_width()*getrectgeometry()->get_height();

	if(ymin<ymax)//Sa dane
		{
		min=ymin;max=ymax;
		return;
		}

	//Nie sa dane wiec probkujemy - co troche kosztuje
	min=0;
	max=255;
	}

double get(iteratorh& p)
//Daje nastepna z la*lb liczb!!!
	{   assert("get(iteratorh& p) NOT IMPLEMENTED!!!"==NULL);
	//assert(p!=NULL);
	if(p==NULL) return miss;
	/*
	size_t pom=_next( p );
	if(pom!=ULONG_MAX)
		return and(A,B);
		else */
		return miss;
	}

double get(size_t index)//Przetwarza index uzyskany z geometri
	{ //na wartosc z serii, o ile jest mozliwe czytanie losowe
	assert(index<getrectgeometry()->get_size());
	unsigned A=index/256;
	unsigned B=index%256;
	return bit_and(A,B);
	}
};


class and_exploatation_source:public rectangle_source_base
//--------------------------------------------------------------
{
double comp_and(unsigned A,unsigned B)
	{
	//double(w.w.oslona & zabojca.w.w.geba)/(zabojca.w.w.geba)*
	//		double(w.w.oslona & zabojca.w.w.geba)/(w.w.oslona)
    if(A!=0 && B!=0)
	    return double(A & B)/double(B) * double(A & B)/double(A);
    else
        return miss;
	}

public:
// Constructor
and_exploatation_source( const char* itit):
	rectangle_source_base(itit,256,256,1/*,subs,imiss*/){}

~and_exploatation_source()
	{
#ifndef NDEBUG
	cerr<<"~and_exploatation_source():"<<name()<<'\n';
#endif
	}

void  bounds(size_t& num,double& min,double& max)
//Ile elementow,wartosc minimalna i maksymalna
	{
	num=getrectgeometry()->get_width()*getrectgeometry()->get_height();

	if(ymin<ymax)//Sa dane
		{
		min=ymin;max=ymax;
		return;
		}

	min=0;           //Brak dopasowania
	max=comp_and(255,255);//Pelne dopasowanie maski ataku do maski ochrony. 100% trawienia.
	}

double get(iteratorh& p)
//Daje nastepna z la*lb liczb!!!
	{
	assert("get(iteratorh& p) NOT IMPLEMENTED!!!"==NULL);
	//assert(p!=NULL);
	if(p==NULL) return miss;
	/*
	size_t pom=_next( p );
	if(pom!=ULONG_MAX)
		return and(A,B);
		else */
		return miss;
	}

double get(size_t index)//Przetwarza index uzyskany z geometri
	{ //na wartosc z serii, o ile jest mozliwe czytanie losowe
	assert(index<getrectgeometry()->get_size());
	unsigned A=index/256;
	unsigned B=index%256;
	return comp_and(A,B);
	}
};


#endif
