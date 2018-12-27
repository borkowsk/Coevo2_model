/* INTERFACE DO ROZMAITYCH GENERATOROW LICZB PSEUDOLOSOWYCH */
/*----------------------------------------------------------*/
/* USES_RANDG - Random generator writed in C based on "Numerical Recipes" */
/* USES_BSD_RANDOM - Random generator from BSD UNIX  */
/* USES_SVR4_RANDOM - Random generator from System V UNIX */
/* USES_STDC_RAND - Reandom generator buildin standard C */

#ifndef _RANDOMS_H_INCLUDED_
#define _RANDOMS_H_INCLUDED_  1

/* time headers included for time() */
#include <time.h>
#ifndef unix
#include <sys/timeb.h>
#endif 

#ifdef __cplusplus
extern "C" {
#endif

float  randg(); 			 /* Num.Rec. random number generator */
void   srandg(short int);    /* Seed setting for generator */

#ifdef __cplusplus
};
#endif


#if   defined( USES_RANDG )

#	define RANDOM_MAX  ( MAXINT )
#	define RAND()      ( (int)((randg)()*RANDOM_MAX) )
#	define RANDOM(_I_) ( (int)((randg)()*(_I_)) )
#	define SRAND(_P_)  { (srandg)(- (_P_) ); }
#	define DRAND()	   ( (randg)() )
#	define RANDOMIZE() { (srandg)( (unsigned) time(NULL) ); }

#elif defined( USES_BSD_RANDOM )

#	if defined(__IRIX32__)||defined(__IRIX64__)
#	include <math.h>
#	endif

#	define RANDOM_MAX  ( 0x7fffffffL )
#	define RAND()      ( (random)() )
#	define RANDOM(_I_) ( (int) (((double) (random)() * (_I_) ) / ((double)RANDOM_MAX+1) ) )
#	define SRAND(_P_)  { (srandom)(_P_);}
#	define DRAND()     ( (double)(random)()/((double)(RANDOM_MAX)+1) )
#	define RANDOMIZE() { (srandom)( (unsigned) time(NULL) ); }

#elif defined( USES_SVR4_DRAND )

#	define RANDOM_MAX  ( MAXINT )
#	define RAND() 	   ( (lrand48)() ) /* CHECK RANGE! */
#	define RANDOM(_I_) ( (drand48)()*(_I_))
#	define SRAND(_P_)  { (srand48)( _P_ ); }
#	define DRAND()     ( (drand48)() )
#	define RANDOMIZE() { (srand48)( (long) time(NULL) ); }

#elif defined( USES_STDC_RAND )

#	define RANDOM_MAX  ( RAND_MAX )
#	define RAND() 	   ( (rand)() )
#	define RANDOM(_I_) (int)(((double)(rand)()*(_I_))/((double)RAND_MAX+1))
#	define SRAND(_P_)  { (srand)( _P_ ); }
#	define DRAND()     ( (double)(rand)()/(double)RAND_MAX )
#	define RANDOMIZE() {  (srand)( (unsigned)time(NULL) ); }

#elif defined( UNDEF_RAND )

#	undef RANDOM_MAX    
#	undef RAND
#	undef RANDOM
#	undef SRAND
#	undef DRAND
#	undef RANDOMIZE

#else /* NO USEABLE RANDOM FUNCTIONS */

#	define RANDOM_MAX  ( SELECT_RANDOM_NOT_DEFINED_FOR_THIS_CODE  )
#	define RAND() 	   ( SELECT_RANDOM_NOT_DEFINED_FOR_THIS_CODE  )
#	define RANDOM(_I_) ( SELECT_RANDOM_NOT_DEFINED_FOR_THIS_CODE  )
#	define SRAND(_P_)  { SELECT_RANDOM_NOT_DEFINED_FOR_THIS_CODE  }
#	define DRAND()     ( SELECT_RANDOM_NOT_DEFINED_FOR_THIS_CODE  )
#	define RANDOMIZE() { SELECT_RANDOM_NOT_DEFINED_FOR_THIS_CODE  }

#endif


#endif
/********************************************************************/
/*           THIS CODE IS DESIGNED & COPYRIGHT  BY:                 */
/*            W O J C I E C H   B O R K O W S K I                   */
/* Zaklad Systematyki i Geografii Roslin Uniwersytetu Warszawskiego */
/*  & Instytut Studiow Spolecznych Uniwersytetu Warszawskiego       */
/*        WWW:  http://moderato.iss.uw.edu.pl/~borkowsk             */
/*        MAIL: borkowsk@iss.uw.edu.pl                              */
/*                               (Don't change or remove this note) */
/********************************************************************/


