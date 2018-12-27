// ekologia.cpp: implementation of the ekologia class.
//
//////////////////////////////////////////////////////////////////////
#include <math.h>
#include "ekologia.hpp"
#include "SYMSHELL/sshutils.hpp"


static
bool _moja_akcja_trawersowania(informacja_klonalna* current,
                              void* user_data)
//Akcja do wykonania na wezle drzewa taksonow
//Jesli zwroci false to trawersowanie jest przerywane   
{
    ekologia* Self=reinterpret_cast<ekologia*>(user_data);
	if(current->ile_zywych()>Self->tax_size_tres())//Tylko dla realne taksony, chyba ze user zechce inaczej
    {        
        informacja_troficzna* what=dynamic_cast<informacja_troficzna*>(current);
        Self->_zarejestruj_z_sortowaniem(what,current->identyfikator());
    }

    return true;//Nie ma powodu przerywac
}
          
static
bool _trawersowanie_kasujace(informacja_klonalna* current,
                              void* user_data)
//Akcja do wykonania na wezle drzewa taksonow
//-- usuwa kontakty z poprzedniego kroku
{
    if(current->ile_zywych()>0)//Tylko dla wci¹z istniejacych
    {        
        informacja_troficzna* what=dynamic_cast<informacja_troficzna*>(current);
        what->czysc_kontakty();       
    }

    return true;//Nie ma powodu przerywac
}
         
inline
void ekologia::_swap(ekologia::node_info& f,ekologia::node_info& s)
{
    char buf[sizeof(node_info)];
    memcpy(buf,&s,sizeof(node_info));
    memcpy(&s,&f,sizeof(node_info));
    memcpy(&f,buf,sizeof(node_info));
}
                                                         
bool ekologia::_zarejestruj_z_sortowaniem(informacja_troficzna* what,unsigned ident)
//madrze rejestruje, jesli jeszcze nie bylo, 'updejtuje' i ewentualnie przesuwa na liscie
{                               assert(what!=NULL); assert(ident<=informacja_klonalna::ile_klonow());
    size_t s=nodes.CurrSize();  
    unsigned i=0;
    
	for(;i<s;i++)
    {
        if(nodes[i].Identyfikator==ident)//Jesli taki juz byl na liscie
        {
            nodes[i].Zywych=what->Waga();//Uaktualnia liczbe zywych - realna wage klonu            
            goto SORTOWANKO;
        }
    }
//NIE ZNALEZIONO TAKIEGO NA LISCIE    
    nodes(i).set(ident,what);//To wstawiamy nowy

SORTOWANKO://Zeby najliczniejsze klony byly na poczatku bo maja szanse byc na wielu listach
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
        if(nodes[i].Identyfikator==Ident)//Jesli taki juz byl na liscie
        {
            return i;
        }
	}

    return 0xffffffff;
}

void ekologia::aktualizuj_liste_wezlow(bool /*leftrec*/)
//Aktualizacja listy wezlów
{
    //Etap I - usuwanie klonow wymarlych dawniej i zerowanie pozostalych
    {
    for(size_t i=0;i<nodes.CurrSize();i++)
	{
        if(nodes[i].Zywych==0) //Dawniej usuniety
        {
            //cerr<<'{'<<nodes[i].Identyfikator<<'}';
            nodes.Del(i);
            i--;//Bo jednego ubylo i ten indeks trzeba raz jeszcze sprawdzic
		}
        else
            nodes[i].Zywych=0; //Jesli klon wciaz istnieje to zaraz zostanie zaktualizowany
							  //a jak wymarl to zostanie usuniety w nastepnej 'kolejce'
	}
	}

    //Etap II - aktualizacja z listy klonow
	bool ret=ekologia::ancestor->trawersuj_drzewo_pre(_moja_akcja_trawersowania,this);

    //Etap III - przygotowanie listy polaczen - niestety kosztowne, wiec dobrze jest pominac czesc polaczen
    {
		lines.Truncate(0);//Usuwamy stare polaczenia bo odnawianie byloby kosztowniejsze
		for(long i=nodes.CurrSize()-1;i>=0;i--)
        {
            if(nodes[i].Zywych>0)
            {
				size_t limit=_lowest_index_for_treshold(i);
				for(long j=limit;j>=0;j--)//nodes[i].co->AktualnaLiczbaKontaktow()-1
                {
                    size_t kolejny=lines.CurrSize();
					unsigned Ident=nodes[i].co->operator [](j).marker;
                    unsigned long Waga= nodes[i].co->operator [](j).weight;
                    //unsigned Licz= nodes[i].co->operator [](j).counter;//Debug 

                    size_t indeks_zrodla=Znajdz( Ident);//To powoduje ze slozonosc jest kwadratowa
                    
                    if(indeks_zrodla==-1)
                        continue;//Nie znaleziono - prawdopodobnie klon powstal i zaginal w jednym kroku
                    
					lines(kolejny).set( indeks_zrodla, i, Waga);
				}
            }
        }
	}

	//Etap V - usuwanie przeanalizowanych
    ret=ekologia::ancestor->trawersuj_drzewo_pre(_trawersowanie_kasujace,NULL);    

	_update_source_ptrs();//Mozliwe nowe wskazniki do danych dla serii
}

void ekologia::ZapiszWFormacieNET(ostream& out,unsigned size_tres,double weight_tres)
{
	//Wypisywanie
	extern unsigned BIT_RUCHU; //=128 Wyzerowanie ktorych bitow oslony odpowiada za zdolnosc ruchu. W pliku z main-em

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
		 for(size_t j=0;j<lines.CurrSize();j++)//lines(kolejny).set( indeks_zrodla, i, Waga)
			if(lines[j].weight>=weight_tres)
			{
				out<<lines[j].start_node<<'\t'<<lines[j].end_node<<'\t'<<lines[j].weight<<endl;
				//Szukanie maksimum
				if(MaxWeight<lines[j].weight)
					 MaxWeight=lines[j].weight;
			 }
			 else ignored++;

		cerr<<endl<<lang("Plik NET: Maksymalna waga=","NET file: Max Weight = ")<<MaxWeight<<lang("ale 'cienkie' krawêdzie s¹ ignorowane!!! ->"," but v.light edges ignored!!! -> ")<<ignored<<endl;
		out<<flush;

}

static
double log2(double v)
{
return log(v)/log(2.0); //(v > 0)  
}

void ekologia::ZapiszWFormacieVNA(ostream& out,unsigned size_tres,double weight_tres)
{
		//Wypisywanie
	extern unsigned BIT_RUCHU; //=128 Wyzerowanie ktorych bitow oslony odpowiada za zdolnosc ruchu. W pliku z main-em

		out<<"*Node data "<<endl;
		out<<"ID\tLABEL\tSIZE\tATTACK\tDEFENSE\tMOVEABLE\tCLONE_ID"<<endl;
		for(size_t i=0;i<nodes.CurrSize();i++)
		//if(nodes[i].Zywych>=size_tres)
		{
			node_info* Node=&nodes[i];
			out<<dec<<i+1<<'\t'; //ID mogloby byc nazw¹, ale to utrudnia
								 //wczytywanie w prostych programach w pascalu

			if(Node->X_lub_maska_ataku==AUTOTROF) //LABEL - kompletna "nazwa" eko-fil taksonu
				out<<"Aut";
			else
				out<<'H'<<hex<<Node->X_lub_maska_ataku;
			out<<'_'<<hex<<Node->Y_lub_maska_obrony;
			if(Node->Y_lub_maska_obrony  &  BIT_RUCHU)
				out<<'S';else out<<'_';
			out<<dec<<Node->Identyfikator<<'\t';

			out<<dec<<Node->Zywych<<'\t'; //SIZE - prawdziwa liczebnoœæ taksonu
			out<<dec<<Node->X_lub_maska_ataku<<'\t'; //ATTACK
			out<<dec<<Node->Y_lub_maska_obrony<<'\t'; //DEFENSE

			if(Node->Y_lub_maska_obrony  &  BIT_RUCHU) //MOVEABLE
				out<<'N';else out<<'Y';

			out<<"\t"<<dec<<Node->Identyfikator;//CLONE_ID

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

			out<<dec<<i+1<<'\t'; //ID mogloby byc nazw¹, ale ...
			out<<dec<<Node->X_lub_maska_ataku<<'.'<<Node->Identyfikator<<'\t';//x
			out<<dec<<Node->Y_lub_maska_obrony<<'.'<<Node->Identyfikator<<'\t';//y

			//color  heksadecymalne RGB
			if(Node->X_lub_maska_ataku==AUTOTROF)
				Color=0x00FF00;//Green
				else
				Color=Node->X_lub_maska_ataku;//Some red no green
			Color+=Node->Y_lub_maska_obrony<<16;//blue - maska obrony
			out<<"0x"<<hex<<Color<<"\t";

			//out<<dec<<int(1+log2(Node->Y_lub_maska_obrony))<<'\t';//shape - waga maski obrony    ()
			if(Node->X_lub_maska_ataku==AUTOTROF)//Shape
				out<<"1\t";//Autotrofy - w jednym 'kszta³cie'
				else
				out<<"2\t";//Heterotrofy w drugim

			out<<dec<<int(1+log10(1.0+Node->Zywych)*2)<<'\t'; //size - zlogarytmowany rozmiar klonu

			if(Node->X_lub_maska_ataku==AUTOTROF) //shortlabel - 'nazwa' ekologiczna
				out<<"Au";
			else
				out<<hex<<Node->X_lub_maska_ataku;
			out<<'_'<<hex<<Node->Y_lub_maska_obrony;
			if(Node->Y_lub_maska_obrony  &  BIT_RUCHU)
				out<<'S';else out<<'M';

			out<<"\t6"<<endl;//Labelsize
		 }

		 out<<"*Tie data"<<endl;
		 out<<"from\tto\tstrength\tlog_str"<<dec<<endl;
		 double MaxWeight=0;
		 unsigned ignored=0;
		 for(size_t j=0;j<lines.CurrSize();j++)//lines(kolejny).set( indeks_zrodla, i, Waga)
			if(lines[j].weight>=weight_tres && lines[j].start_node!=lines[j].end_node) //Krawêdzie zwrotne nie wczytuj¹ siê w NETDRAWa
			{
				out<<dec<<lines[j].start_node+1<<'\t'<<dec<<lines[j].end_node+1<<'\t'
				<<lines[j].weight<<'\t'
				<<log10(1.0+lines[j].weight)<<'\t'
				<<endl;
				//Szukanie maksimum dla nastêpnej pêtli
				if(MaxWeight<lines[j].weight)
					 MaxWeight=lines[j].weight;
			}
			else ignored++;

		//cerr<<endl<<'VNA Max Weight = <<MaxWeight<<' but small and selfedges ignored!!! -> <<ignored<<endl;
		cerr<<endl<<lang("Plik VNA: Maksymalna waga=","VNA file: Max Weight = ")<<MaxWeight
			<<lang("ale 'cienkie' krawêdzie i petle w³asne s¹ ignorowane!!! ->"," but small- and self- edges ignored!!! -> ")<<ignored<<endl;

		out<<"*Tie properties"<<endl;
		out<<"from\tto\tcolor\tsize\theadcolor"
			<<endl;
		//FROM TO color size headcolor headsize active
		for(size_t j=0;j<lines.CurrSize();j++)//lines(kolejny).set( indeks_zrodla, i, Waga)
			if(lines[j].weight>=weight_tres && lines[j].start_node!=lines[j].end_node)
			{
				unsigned colw=0;
				unsigned colorek=0;
				colw=255-(255*(lines[j].weight/MaxWeight));
				colorek=(colw<<16)+(colw<<8)+colw;

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
		for(j=0;j<N;j++)//Oblicznie 100%
			Suma+=Node.co->operator [](j).weight;
		if(Suma>0)
		{
			double Suma2=0;
			for(j=0;j<N;j++)
			{
				Suma2+=Node.co->operator [](j).weight;
				if(Suma2/Suma > conn_treshold)
					return j;   //treshold przekroczony na j-otym polaczeniu
			}
		}
//		else  cerr<<endl<<"Strange node "<<i<<" S.of "<<N<<" Weight="<<Suma<<endl;
	}
	return Node.co->AktualnaLiczbaKontaktow()-1;//Oddaje ostatni jesli nie wyszedla wczesniej
}

void ekologia::zapomnij_liste(bool zwolnij_pamiec)
//Zapominanie listy z ewentualnym zwalnianiem pamieci
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

void ekologia::_update_source_ptrs()
//Poprawia wskazniki do tablic, ktore moga sie dezaktualizowac podczas wypelniania list
{
    //if(pNodeTime!=NULL) 
    //    pNodeTime->set_source(nodes.CurrSize(),nodes.GetTabPtr());//Punkty czasowe wezlow drzewa: specjacji, poczatku i konca istnienia klonu (po 3 na klon)   
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


linear_source_base* ekologia::NodeX()
//Polozenie wezla - np. maska ataku
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

linear_source_base* ekologia::NodeY()
//Polozenie wezla - np. maska obrony
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

linear_source_base* ekologia::NodeWeight()
//Waga wezla - np. liczba osobników (ewentualnie biomasa)
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

//linear_source_base* NodeScaledWeight(){  }//Pierwiastkowana waga wezla - lepsze dla kó³ek

//Informacje o polaczeniach
linear_source_base* ekologia::ConnPrey()
//Start polaczenia w ofierze (prey)
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

linear_source_base* ekologia::ConnPred()
//Koniec polaczenia w drapiezniku (predator)
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

linear_source_base* ekologia::ConnWeight()
//Liczba ofiar lub  pozyskana biomasa ofiar
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


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

ekologia::ekologia(agent::informacja_klonalna* TheAncestor,
					unsigned int user_node_treshold,//=-1,
					double connection_treshold//=0
					)://Konstruktor
	user_tax_tres(user_node_treshold),
	conn_treshold(connection_treshold),
	ancestor(TheAncestor),
	pNodeX(NULL),
	pNodeY(NULL),
	pNodeWeight(NULL),
	pConnPrey(NULL),//Indeksy pocz¹tków linii laczacych wezly sieci
	pConnPred(NULL),//Indeksy koncow linii laczacych wezly sieci
	pConnWeight(NULL)//Przeplyw w danej linii
{
	  cerr<<endl;
	  cerr<<"user_tax_tres: "<<user_tax_tres<<endl;
	  cerr<<"conn_treshold: "<<conn_treshold<<endl;
	  cerr<<endl;
}

ekologia::~ekologia()
{
    zapomnij_liste(true/*zwolnij_pamiec*/);//Zapominamy liste
    ancestor=NULL;
} 