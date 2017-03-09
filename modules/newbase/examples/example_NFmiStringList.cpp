// ======================================================================
/*!
 * \file example_NFmiStringList.cpp
 *
 * T�m�n esimerkkiohjelman ainoa tarkoitus on testata, toimiiko
 * uusi STL versio luokasta oikein. K�yt� aina mieluummin
 * std::list tai std::vector luokkia.
 *
 */
// ======================================================================

#include "NFmiStringList.h"
#include "NFmiFileSystem.h"
#include <iostream>
#include <fstream>
#include <cassert>

using namespace std;

int main(void)
{
  NFmiStringList slist;
  slist.Add(new NFmiString("Elementti 1"));
  slist.Add(new NFmiString("Elementti 2"));
  slist.Add(new NFmiString("Elementti 3"));
  slist.Add(new NFmiString("Elementti 4"));
  cout << "Alistettu lista:" << endl << slist;

  cout << "Elementtien lukum��r� = " << slist.NumberOfItems() << endl;

  // Luku ja kirjoitus
  {
	ofstream out("stringlist.tst");
	assert(out);
	out << slist;
	out.close();
 
	ifstream in ("stringlist.tst");
	NFmiStringList slist2;
	assert(in);
	in >> slist2;
	cout << "Kirjoitettu ja luettu lista:" << endl << slist2;

	NFmiFileSystem::RemoveFile("stringlist.tst");
  }

  // Kopiointi
  {
	NFmiStringList ob1 = slist;
	cout << "Kopiointi kopiokonstruktorilla:" << endl << ob1;

	NFmiStringList ob2;
	ob2 = slist;
	cout << "Kopiointi = operaattorilla:" << endl << ob2;
  }

  // Iterointi
  {
	cout << "Iterointi:" << endl;
	slist.Reset();
	while(slist.Next())
	  cout << *(slist.Current());
  }

  // Etsiminen
  {
	if(slist.Find(NFmiString("Elementti 0")))
	  cout << "Elementti 0 l�ytyi, mik� on v��rin!" << endl;
	else
	  cout << "Elementti� 0 ei l�ydy, mik� on oikein" << endl; 

	cout << "Yritet��n poistaa elementti" << endl;
	slist.Remove();
	cout << "Lista on nyt:" << endl << slist;

	if(slist.Find(NFmiString("Elementti 2")))
	  cout << "Elementti 2 l�ytyi, mik� on oikein!" << endl;
	else
	  cout << "Elementti� 2 ei l�ydy, mik� on v��rin" << endl; 

	cout << "Yritet��n poistaa elementti" << endl;
	slist.Remove();
	cout << "Lista on nyt:" << endl << slist;

  }

  slist.Clear(true);
  cout << "Lista on siivoamisen j�lkeen:" << endl << slist;

  return 0;
}

