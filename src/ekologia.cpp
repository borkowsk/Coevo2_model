// ekologia.cpp: implementation of the ekologia class.
//*////////////////////////////////////////////////////////////////////
#include <math.h>
#include "lingo.hpp"
#include "ekologia.hpp"
#include "sshutils.hpp"

/*
/// Compute base-2 logarithm of X.
static
double log2(double v)
{
    return log(v)/log(2.0);
}
*/

//Akcja do wykonania na węźle drzewa taksonów.
//Jeśli zwróci false, to trawersowanie jest przerywane.
bool _moja_akcja_trawersowania(informacja_klonalna* current,void* user_data)
{
    ekologia* Self=reinterpret_cast<ekologia*>(user_data);
	if(current->ile_zywych()>Self->tax_size_tres()) //Tylko dla realne taksony, chyba ze user zechce inaczej
    {        
        informacja_troficzna* what=dynamic_cast<informacja_troficzna*>(current);
        Self->_zarejestruj_z_sortowaniem(what,current->identyfikator());
    }

    return true; // Nie ma powodu przerywać.
}

// Akcja do wykonania na węźle drzewa taksonów.
// Usuwa kontakty z poprzedniego kroku.
static
bool _trawersowanie_kasujace(informacja_klonalna* current,void* /*user_data*/)
{
    if(current->ile_zywych()>0) //Tylko dla wciąż istniejących
    {        
        informacja_troficzna* what=dynamic_cast<informacja_troficzna*>(current);
        what->czysc_kontakty();       
    }

    return true; // Nie ma powodu przerywać
}

inline
void ekologia::_swap(ekologia::node_info& f,ekologia::node_info& s)
{
    char buf[sizeof(node_info)];
    memcpy(buf,&s,sizeof(node_info));
    memcpy(&s,&f,sizeof(node_info));
    memcpy(&f,buf,sizeof(node_info));
}

// Mądrze rejestruje, jeśli jeszcze nie było, a 'updejtuje' i ewentualnie przesuwa na liście, gdy już był.
bool ekologia::_zarejestruj_z_sortowaniem(informacja_troficzna* what,unsigned ident)
{                               assert(what!=NULL); assert(ident<=informacja_klonalna::ile_klonow());
    size_t s=nodes.CurrSize();  
    unsigned i=0;
    
	for(;i<s;i++)
    {
        if(nodes[i].Identyfikator==ident) //Gdy taki już był na liście
        {
            nodes[i].Zywych=what->Waga(); //Uaktualnia liczbę żywych, czyli realną wagę klonu.
            goto SORTOWANKO;
        }
    }

    // NIE ZNALEZIONO TAKIEGO NA LIŚCIE
    nodes(i).set(ident,what); //To wstawiamy nowy

SORTOWANKO: // Żeby najliczniejsze klony były na początku, bo mają szanse być na wielu listach.
    for(;i>0 && nodes[i].Zywych>nodes[i-1].Zywych;i--)
    {
        _swap(nodes[i],nodes[i-1]);
    }
    
    return true;
}

size_t ekologia::Znajdz(unsigned Ident)
{
    size_t s=nodes.CurrSize();  
    
    for(unsigned i=0;i<s;i++)
    {
        if(nodes[i].Identyfikator==Ident) //Jeśli taki już był na liście
        {
            return i;
        }
	}

    return 0xffffffff;
}

// Aktualizacja listy węzłów
void ekologia::aktualizuj_liste_wezlow(bool /*leftrec*/)
{
    //Etap I: usuwanie klonów wymarłych dawniej i zerowanie pozostałych
    {
    for(size_t i=0;i<nodes.CurrSize();i++)
	{
        if(nodes[i].Zywych==0) //Dawniej usuniety
        {
            //cerr<<'{'<<nodes[i].Identyfikator<<'}';
            nodes.Del(i);
            i--; //Bo jednego ubyło i ten indeks trzeba raz jeszcze sprawdzić
		}
        else
            nodes[i].Zywych=0; //Jeśli klon wciąż istnieje, to zaraz zostanie zaktualizowany,
							   //a jak wymarł, to zostanie usunięty w następnej 'kolejce'.
	}
	}

    //Etap II: aktualizacja z listy klonów
	bool ret=ekologia::ancestor->trawersuj_drzewo_pre(_moja_akcja_trawersowania,this);

    //Etap III: przygotowanie listy połączeń. Niestety kosztowne, wiec dobrze jest pominąć cześć połączeń.
    {
		lines.Truncate(0); //Usuwamy stare połączenia, bo odnawianie byłoby kosztowniejsze.
        long i=nodes.CurrSize();
        if(i>2)
		for(i-1;i>=0;i--)
        {
            if(nodes[i].Zywych>0)
            {
				size_t limit=_lowest_index_for_treshold(i);
				for(long j=limit;j>=0;j--) //nodes[i].co->AktualnaLiczbaKontaktow()-1
                {
                    size_t kolejny=lines.CurrSize();
					unsigned Ident=nodes[i].co->operator [](j).marker;
                    unsigned long Waga= nodes[i].co->operator [](j).weight;
                    //unsigned Licz= nodes[i].co->operator [](j).counter;//Debug 

                    size_t indeks_zrodla=Znajdz( Ident); //To powoduje ze złożoność jest kwadratowa
                    
                    if(indeks_zrodla==-1)
                        continue; //Nie znaleziono. Prawdopodobnie klon powstał i zaginał w jednym kroku.
                    
					lines(kolejny).set( indeks_zrodla, i, Waga);
				}
            }
        }
	}

	//Etap V: usuwanie przeanalizowanych
    ret=ekologia::ancestor->trawersuj_drzewo_pre(_trawersowanie_kasujace,NULL);    

	_update_source_ptrs(); //Możliwe nowe wskaźniki do danych dla serii
}

void ekologia::ZapiszWFormacieNET(ostream& out,unsigned size_tres,double weight_tres)
{
	//Wypisywanie
	extern unsigned BIT_RUCHU; //=128 Wyzerowanie których bitów osłony odpowiada za zdolności ruchu. W pliku z main-em.

    out<<nodes.CurrSize()<<endl;
    node_info* Node=NULL;

    for(size_t i=0;i<nodes.CurrSize();i++)
    //if(nodes[i].Zywych>=size_tres)
    {
        Node=&nodes[i];
        out<<dec<<Node->X_lub_maska_ataku<<'.'<<Node->Identyfikator<<'\t';
        out<<dec<<Node->Y_lub_maska_obrony<<'.'<<Node->Identyfikator<<'\t';
        out<<dec<<Node->Zywych<<" ";
        if(Node->X_lub_maska_ataku==AUTOTROF)
            out<<"Au";
        else
            out<<hex<<Node->X_lub_maska_ataku;
        out<<'_'<<hex<<Node->Y_lub_maska_obrony;
        if(Node->Y_lub_maska_obrony  &  BIT_RUCHU)
            out<<'S';
        else
            out<<'_';
        out<<dec<<Node->Identyfikator;
        out<<endl;
     }

     double MaxWeight=0;
     unsigned ignored=0;
     for(size_t j=0;j<lines.CurrSize();j++) //lines(kolejny).set( indeks_zrodla, i, Waga)
        if(lines[j].weight>=weight_tres)
        {
            out<<lines[j].start_node<<'\t'<<lines[j].end_node<<'\t'<<lines[j].weight<<endl;
            //Szukanie maksimum
            if(MaxWeight<lines[j].weight)
                 MaxWeight=lines[j].weight;
         }
         else ignored++;

		cerr<<endl<<lang("Plik NET: Maksymalna waga=","NET file: Max Weight = ")<<MaxWeight
                  <<lang("ale 'cienkie' krawędzie są ignorowane!!! ->"," but v.light edges ignored!!! -> ")
                  <<ignored<<endl;
		out<<flush;
}

void ekologia::ZapiszWFormacieVNA(ostream& out,unsigned size_tres,double weight_tres)
{
    //Wypisywanie
    extern unsigned BIT_RUCHU; //=128 Wyzerowanie których bitów osłony odpowiada za zdolności ruchu. W pliku z main-em.

    out<<"*Node data "<<endl;
    out<<"ID\tLABEL\tSIZE\tATTACK\tDEFENSE\tMOVEABLE\tCLONE_ID"<<endl;
    for(size_t i=0;i<nodes.CurrSize();i++)
    //if(nodes[i].Zywych>=size_tres)
    {
        node_info* Node=&nodes[i];
        out<<dec<<i+1<<'\t'; //ID mogłoby być nazwą, ale to utrudnia
                             //wczytywanie w prostych programach w Pascalu/Delphi

        if(Node->X_lub_maska_ataku==AUTOTROF) //LABEL - kompletna "nazwa" eko-fil taksonu
            out<<"Aut";
        else
            out<<'H'<<hex<<Node->X_lub_maska_ataku;
        out<<'_'<<hex<<Node->Y_lub_maska_obrony;
        if(Node->Y_lub_maska_obrony  &  BIT_RUCHU)
            out<<'S';else out<<'_';
        out<<dec<<Node->Identyfikator<<'\t';

        out<<dec<<Node->Zywych<<'\t'; //SIZE czyli prawdziwa liczebność taksonu
        out<<dec<<Node->X_lub_maska_ataku<<'\t'; //ATTACK
        out<<dec<<Node->Y_lub_maska_obrony<<'\t'; //DEFENSE

        if(Node->Y_lub_maska_obrony  &  BIT_RUCHU) //MOVEABLE
            out<<'N';else out<<'Y';

        out<<"\t"<<dec<<Node->Identyfikator; //CLONE_ID

        out<<endl;
    }

    out<<"*Node properties "<<endl;
    out<<"ID\tx\ty\tcolor\tshape\tsize\tlabeltext\tlabelsize"<<endl;
    //ID x y color shape size labeltext labelsize labelcolor gapx gapy active
    for(size_t i=0;i<nodes.CurrSize();i++)
    //if(nodes[i].Zywych>=size_tres)
    {
        node_info* Node=&nodes[i];
        unsigned Color=0;

        out<<dec<<i+1<<'\t'; //ID mogloby byc nazwą, ale ...
        out<<dec<<Node->X_lub_maska_ataku<<'.'<<Node->Identyfikator<<'\t';//x
        out<<dec<<Node->Y_lub_maska_obrony<<'.'<<Node->Identyfikator<<'\t';//y

        //color  heksadecymalne RGB
        if(Node->X_lub_maska_ataku==AUTOTROF)
            Color=0x00FF00; //Green
            else
            Color=Node->X_lub_maska_ataku; //Some red no green
        Color+=Node->Y_lub_maska_obrony<<16; //blue - maska obrony
        out<<"0x"<<hex<<Color<<"\t";

        //out<<dec<<int(1+log2(Node->Y_lub_maska_obrony))<<'\t'; //shape - waga maski obrony    ()
        if(Node->X_lub_maska_ataku==AUTOTROF) //Shape
            out<<"1\t"; //Autotrofy - w jednym 'kształcie'
            else
            out<<"2\t"; //Heterotrofy w drugim

        out<<dec<<int(1+log10(1.0+Node->Zywych)*2)<<'\t'; //size - zlogarytmowany rozmiar klonu

        if(Node->X_lub_maska_ataku==AUTOTROF) //shortlabel - 'nazwa' ekologiczna
            out<<"Au";
        else
            out<<hex<<Node->X_lub_maska_ataku;
        out<<'_'<<hex<<Node->Y_lub_maska_obrony;
        if(Node->Y_lub_maska_obrony  &  BIT_RUCHU)
            out<<'S';else out<<'M';

        out<<"\t6"<<endl; //Labelsize
     }

     out<<"*Tie data"<<endl;
     out<<"from\tto\tstrength\tlog_str"<<dec<<endl;
     double MaxWeight=0;
     unsigned ignored=0;
     for(size_t j=0;j<lines.CurrSize();j++) //lines(kolejny).set( indeks_zrodla, i, Waga)
        if(lines[j].weight>=weight_tres && lines[j].start_node!=lines[j].end_node) //Krawędzie zwrotne nie wczytują się w NETDRAW-a
        {
            out<<dec<<lines[j].start_node+1<<'\t'<<dec<<lines[j].end_node+1<<'\t'
            <<lines[j].weight<<'\t'
            <<log10(1.0+lines[j].weight)<<'\t'
            <<endl;
            //Szukanie maksimum dla następnej pętli
            if(MaxWeight<lines[j].weight)
                 MaxWeight=lines[j].weight;
        }
        else ignored++;

    //cerr<<endl<<'VNA Max Weight = <<MaxWeight<<' but small and selfedges ignored!!! -> <<ignored<<endl;
    cerr<<endl<<lang("Plik VNA: Maksymalna waga=","VNA file: Max Weight = ")<<MaxWeight
        <<lang("ale 'cienkie' krawędzie i pętle własne są ignorowane!!! ->"," but small- and self- edges ignored!!! -> ")<<ignored<<endl;

    out<<"*Tie properties"<<endl;
    out<<"from\tto\tcolor\tsize\theadcolor"
        <<endl;
    //FROM TO color size headcolor headsize active
    for(size_t j=0;j<lines.CurrSize();j++)//lines(kolejny).set( indeks_zrodla, i, Waga)
        if(lines[j].weight>=weight_tres && lines[j].start_node!=lines[j].end_node)
        {
            unsigned colw=255-(255*(lines[j].weight/MaxWeight));
            unsigned colorek=(colw<<16)+(colw<<8)+colw;

           //	cerr<<"Max="<<MaxWeight<<" W="<<dec<<lines[j].weight<<"-->"
           //		<<(lines[j].weight/MaxWeight)<<" c="<<dec<<colw<<" ccc="
           //		<<hex<<colorek<<" = "<<dec<<colorek<<endl;

            out<<dec<<lines[j].start_node+1<<'\t'
                <<dec<<lines[j].end_node+1<<'\t'//from to
                //<<"0x"<<hex<<colorek<<'\t' //color
                <<dec<<colorek<<'\t' //color
                <<dec<<int(log10(1.0+lines[j].weight))+1<<'\t'//size
                <<dec<<(colw<<16)+(colw<<8)<<'\t'   //cyan
               <<endl;
        }
    out<<endl<<flush;
}

size_t ekologia::_lowest_index_for_treshold(size_t i)
{
	node_info& Node=nodes[i];

	size_t N=Node.co->AktualnaLiczbaKontaktow();
	if(N>0)
	{
		double Suma=0;
		size_t j;
		for(j=0;j<N;j++) //Obliczenie 100%
			Suma+=Node.co->operator [](j).weight;

		if(Suma>0)
		{
			double Suma2=0;
			for(j=0;j<N;j++)
			{
				Suma2+=Node.co->operator [](j).weight;
				if(Suma2/Suma > conn_treshold)
					return j;   //treshold przekroczony na j-tym połączeniu
			}
		}
//		else  cerr<<endl<<"Strange node "<<i<<" S.of "<<N<<" Weight="<<Suma<<endl;
	}

	return Node.co->AktualnaLiczbaKontaktow()-1; //Oddaje ostatni, jeśli nie wyszedł wcześniej
}

//Zapominanie listy z ewentualnym zwalnianiem pamięci
void ekologia::zapomnij_liste(bool zwolnij_pamiec)
{ 
    nodes.Truncate(0);
	lines.Truncate(0);
	if(zwolnij_pamiec)
	{
		nodes.Deallocate();
        lines.Deallocate();
        _empty_source_ptrs();
    }
}

// Poprawia wskaźniki do tablic, które mogą się dezaktualizować podczas wypełniania list
void ekologia::_update_source_ptrs()
{
    //if(pNodeTime!=NULL) 
    //    pNodeTime->set_source(nodes.CurrSize(),nodes.GetTabPtr()); // Punkty czasowe węzlów drzewa: specjacji,
    //                                                               // początku i konca istnienia klonu (po 3 na klon)
    if(pNodeX!=NULL)
        pNodeX->set_source(nodes.CurrSize(),nodes.GetTabPtr());
    if(pNodeY!=NULL)
        pNodeY->set_source(nodes.CurrSize(),nodes.GetTabPtr());
    if(pNodeWeight!=NULL)
        pNodeWeight->set_source(nodes.CurrSize(),nodes.GetTabPtr());
    if(pConnPrey!=NULL)
        pConnPrey->set_source(lines.CurrSize(),lines.GetTabPtr());
    if(pConnPred!=NULL)
        pConnPred->set_source(lines.CurrSize(),lines.GetTabPtr());
    if(pConnWeight!=NULL)
        pConnWeight->set_source(lines.CurrSize(),lines.GetTabPtr());  
}


void ekologia::_empty_source_ptrs()
{
    if(pNodeX!=NULL)
        pNodeX->set_source(0,NULL);
    if(pNodeY!=NULL)
        pNodeY->set_source(0,NULL);
    if(pNodeWeight!=NULL)
        pNodeWeight->set_source(0,NULL);
    if(pConnPrey!=NULL)
        pConnPrey->set_source(0,NULL);
    if(pConnPred!=NULL)
        pConnPred->set_source(0,NULL);
    if(pConnWeight!=NULL)
        pConnWeight->set_source(0,NULL);  
}

//Położenie poziome węzła na wykresie. Może być np. maska ataku
linear_source_base* ekologia::NodeX()
{
    if(pNodeX !=NULL) 
    {
        return pNodeX;
    }
    else
    {
        return pNodeX  =  new struct_array_source<node_info,unsigned int>
            (nodes.CurrSize(),nodes.GetTabPtr(),&node_info::X_lub_maska_ataku,"X");
    }
}

//Położenie pionowe węzła na wykresie. Może być np. maska obrony
linear_source_base* ekologia::NodeY()
{
    if(pNodeY !=NULL) 
    {
        return pNodeY;
    }
    else
    {
        return pNodeY  =  new struct_array_source<node_info,unsigned int>
            (nodes.CurrSize(),nodes.GetTabPtr(),&node_info::Y_lub_maska_obrony,"Y");
    }
}

// Waga węzła. Może być np. liczba osobników (a ewentualnie biomasa)
linear_source_base* ekologia::NodeWeight()
{
    if(pNodeWeight !=NULL) 
    {
        return pNodeWeight;
    }
    else
    {
        return pNodeWeight  =  new struct_array_source<node_info,unsigned int>
            (nodes.CurrSize(),nodes.GetTabPtr(),&node_info::Zywych,"Weight");
    }
}

//linear_source_base* NodeScaledWeight(){  } //Pierwiastkowana waga węzła. Lepsze dla kółek.

// Informacje o połączeniach predator-prey.
// Start połączenia w ofierze (prey)
linear_source_base* ekologia::ConnPrey()
{
    if(pConnPrey !=NULL) 
    {
        return pConnPrey;
    }
    else
    {
        return pConnPrey  = new struct_array_source<connection,size_t>
            (lines.CurrSize(),lines.GetTabPtr(),&connection::start_node,"-Prey-");
    }
}

// Informacje o połączeniach predator-prey.
// Koniec połączenia w drapieżniku (predator).
linear_source_base* ekologia::ConnPred()
{
    if(pConnPred !=NULL) 
    {
        return pConnPred;
    }
    else
    {
        return pConnPred  =  new struct_array_source<connection,size_t>
            (lines.CurrSize(),lines.GetTabPtr(),&connection::end_node,"-Pred-");
    }
}

// Liczba ofiar lub pozyskana biomasa ofiar
linear_source_base* ekologia::ConnWeight()
{
    if(pConnWeight !=NULL) 
    {
        return pConnWeight;
    }
    else
    {
        pConnWeight  =  new struct_array_source<connection,unsigned long>
            (lines.CurrSize(),lines.GetTabPtr(),&connection::weight,"Prey biomas");       
        return pConnWeight;
    }
}


// ////////////////////////////////////////////////////////////////////
// Construction/Destruction
// ////////////////////////////////////////////////////////////////////

//Konstruktor
ekologia::ekologia(agent::informacja_klonalna* TheAncestor,
					unsigned int user_node_treshold, //=-1,
					double connection_treshold       //=0
					):
	user_tax_tres(user_node_treshold),
	conn_treshold(connection_treshold),
	ancestor(TheAncestor),
	pNodeX(NULL),
	pNodeY(NULL),
	pNodeWeight(NULL),
	pConnPrey(NULL),  //Indeksy początków linii łączących węzły sieci
	pConnPred(NULL),  //Indeksy końców linii łączących węzły sieci
	pConnWeight(NULL) //Przepływ w danej linii
{
	  cerr<<endl;
	  cerr<<"user_tax_tres: "<<user_tax_tres<<endl;
	  cerr<<"conn_treshold: "<<conn_treshold<<endl;
	  cerr<<endl;
}

ekologia::~ekologia()
{
    zapomnij_liste(true/*zwolnij_pamiec*/); //Zapominamy listę
    ancestor=NULL;
}


// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Reactivated source code from Windows (2022.07)
/// @author Wojciech Borkowski
/// FOR @LICENCE SEE HERE: https://github.com/borkowsk/Coevo2_model
// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
