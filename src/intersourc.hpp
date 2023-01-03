// Żródła do demonstracji interakcji drapieżnik/ofiara za pomocą masek bitowych
//*///////////////////////////////////////////////////////////////////////////////
#ifndef __COEWO_INTERACTIONS_SOURCES_HPP_
#define __COEWO_INTERACTIONS_SOURCES_HPP_

#include "datasour.hpp"

//Klasa udostępniająca dowolna tablice prostokątna oraz jej wycinki.
//Jeśli zdefiniowany wycinek wykracza poza tablice źródłowa to funkcja
//'get' zwraca wartość 'miss' podawaną w konstruktorze. Alternatywnie
//wycinek może realizować geometrie torusa i wtedy miss nie jest
//potrzebne.
class and_interaction_source:public rectangle_source_base
//--------------------------------------------------------------
{
unsigned bit_and(unsigned A,unsigned B) //Po co? Takie sobie ćwiczenie?
	{  return A&B;  }

public:
// Constructor
and_interaction_source( const char* itit):
	rectangle_source_base(itit,256,256,1 /*,subs,imiss*/ ){}

~and_interaction_source()
	{
#ifndef NDEBUG
	cerr<<"~and_interaction_source():"<<name()<<'\n';
#endif
	}

    //Ile elementów, wartość minimalna i maksymalna
    void  bounds(size_t& num,double& min,double& max)
	{
	num=getrectgeometry()->get_width()*getrectgeometry()->get_height();

	if(ymin<ymax) //Sa dane
		{
		min=ymin;max=ymax;
		return;
		}

	// Nie są dane, wiec próbkujemy. Co trochę niestety kosztuje!
	min=0;
	max=255;
	}

//Daje następna z la*lb liczb!!!
double get(iteratorh& p)
	{                                                              assert("get(iteratorh& p) NOT IMPLEMENTED!!!"==NULL);
	//assert(p!=NULL);
	if(p==NULL) return miss;
	/*
	size_t pom=_next( p );
	if(pom!=ULONG_MAX)
		return and(A,B);
		else */
		return miss;
	}

//Przetwarza index uzyskany z geometrii
double get(size_t index)
	{ //na wartość z serii, o ile jest możliwe czytanie losowe
	assert(index<getrectgeometry()->get_size());
	unsigned A=index/256;
	unsigned B=index%256;
	return bit_and(A,B);
	}
};


class and_exploatation_source:public rectangle_source_base
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

    //Ile elementów, wartość minimalna i maksymalna
    void  bounds(size_t& num,double& min,double& max)
	{
	num=getrectgeometry()->get_width()*getrectgeometry()->get_height();

	if(ymin<ymax)//Sa dane
		{
		min=ymin;max=ymax;
		return;
		}

	min=0;           //Brak dopasowania
	max=comp_and(255,255); //Pełne dopasowanie maski ataku do maski ochrony. 100% trawienia.
	}

    //Daje następną z la*lb liczb!!!
    double get(iteratorh& p)
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

    //Przetwarza index uzyskany z geometrii
    //na wartość z serii, o ile jest możliwe czytanie losowe.
    double get(size_t index)
	{
	assert(index<getrectgeometry()->get_size());
	unsigned A=index/256;
	unsigned B=index%256;
	return comp_and(A,B);
	}
};


#endif

// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Reactivated source code from Windows (2022.07)
/// @author Wojciech Borkowski
/// FOR @LICENCE SEE HERE: https://github.com/borkowsk/Coevo2_model
// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
