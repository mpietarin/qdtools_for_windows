/*!
 *  \file
 *
 *  Tekijä: Marko (29.8.2001)
 *
 *  Ohjelma lukee stdin:in annetun querydatan ja argumenttina annetun
 *  hilamäärityksen (NFmiGrid-olio). Ohjelma luo qdata:sta uuden qdatan,
 *  jossa on uusi haluttu hila. Uudessa qdatassa on muut systeemit otettu
 *  alkuperäisesta qdatasta (time, param ja level).
 *
 *  Ohjelma tulostaa syntyneen qdatan stdout:iin.
 *
 *  Tekee oletuksena yleisesti lineaarisen-interpoloinnin.
 *  Halutessa voidaan laittaa tekemään lagrange interpolointia antamalla
 *  2. argumenttina 5 (ks. nurero eri menetelmille FmiGlobals.h ja
 *  FmiInterpolationMethod-enum).
 *
 *  Jos parametri on yhdistelmäparametri tai sen parametrin interpolointi
 *  metodiksi on asetettu nearestPoint, ei lagrangea voi tehdä kyseisille
 *  parametreille.
 */

#include <newbase/NFmiAreaFactory.h>
#include <newbase/NFmiCmdLine.h>
#include <newbase/NFmiGrid.h>
#include <newbase/NFmiQueryData.h>
#include <newbase/NFmiQueryDataUtil.h>
#include <newbase/NFmiStringTools.h>
#include <newbase/NFmiWindFix.h>
#include <algorithm>
#include <fstream>

using namespace std;  // tätä ei saa sitten laittaa headeriin, eikä ennen includeja!!!!

// ----------------------------------------------------------------------
// Kaytto-ohjeet
// ----------------------------------------------------------------------

void usage(void)
{
  cout << "Usage: qdinterpolatearea [options] controlGridFile < inputData > outputData" << endl
       << endl
       << "Options:" << endl
       << endl
       << "\t-s <Columns_x_Rows>\tWanted grid size, default is 1x1" << endl
       << "\t-p <projection>\tWanted projection, if given, overrides controlGridFile (fileName not "
          "given then)."
       << endl
       << "\t-i <inputfile>\tInput file instead of standard input" << endl
       << "\t-o <outputfile>\tOutput file instead of standard output" << endl
       << endl
       << "\tExample 1: qdinterpolatearea myGrid.dat < myData.sqd > newData.sqd" << endl
       << "\t        2: qdinterpolatearea -p stereographic,10,90,60:-19.711,25.01,62.93,62.845 -s "
          "40x50  < myData.sqd > newData.sqd"
       << endl
       << "\t        3: qdinterpolatearea -p stereographic,20,90,60:6,51.3,49,70.2 -s 63x70  < "
          "myData.sqd > newData.sqd"
       << endl
       << "\t        4: qdinterpolatearea -p stereographic,20,90,60:6,51.3,49,70.2 -s 10x10km -i "
          "myData.sqd -o newData.sqd"
       << endl
       << "\t        5: qdinterpolatearea -p stereographic,20,90,60:6,51.3,49,70.2:1000,1000m -i "
          "myData.sqd -o newData.sqd"
       << endl
       << endl;
}

void get_grid_size(const string &theGridSizeString, int &theXSize, int &theYSize)
{
  if (theGridSizeString == "")
    throw runtime_error("Grid size option (-s) was empty.");
  else
  {
    string::size_type pos = theGridSizeString.find('x');
    if (pos != string::npos)
    {
      string wantedXSizeStr(theGridSizeString.begin(), theGridSizeString.begin() + pos);
      string wantedYSizeStr(theGridSizeString.begin() + pos + 1, theGridSizeString.end());
      theXSize = NFmiStringTools::Convert<int>(wantedXSizeStr);
      theYSize = NFmiStringTools::Convert<int>(wantedYSizeStr);
      if (theXSize < 0 || theXSize < 0)
        throw runtime_error("Grid size option (-s) had negative values.");
      return;  // pakko palauttaa onnistumisen merkiksi
    }
  }
  throw runtime_error("Grid size option (-s) was bad.");
}

void run(int argc, const char *argv[])
{
  NFmiCmdLine cmdline(argc, argv, "p!s!i!o!");

  string inputfile = "-";
  string outputfile = "-";

  // Tarkistetaan optioiden oikeus:
  if (cmdline.Status().IsError())
  {
    cerr << "Error: Invalid command line:" << endl << cmdline.Status().ErrorLog().CharPtr() << endl;
    usage();
    throw runtime_error("");  // tässä piti ensin tulostaa cerr:iin tavaraa ja sitten vasta usage,
                              // joten en voinut laittaa virheviesti poikkeuksen mukana.
  }

  if (cmdline.isOption('i')) inputfile = cmdline.OptionValue('i');

  if (cmdline.isOption('o')) outputfile = cmdline.OptionValue('o');

  bool projectionFromOptions = cmdline.isOption('p') != 0;
  int minParams = projectionFromOptions ? 0 : 1;
  if (cmdline.NumberofParameters() < minParams)
  {
    cerr << "Error: more parameters expected" << endl;
    usage();
    // tässä piti ensin tulostaa cerr:iin tavaraa ja sitten vasta usage,
    // joten en voinut laittaa virheviesti poikkeuksen mukana.
    throw runtime_error("");
  }

  NFmiGrid *wantedGrid = 0;
  if (projectionFromOptions)
  {
    string projectionString = cmdline.OptionValue('p');
    if (cmdline.isOption('s'))
    {
      string grid = cmdline.OptionValue('s');
      projectionString += ':' + grid;
    }

    boost::shared_ptr<NFmiArea> outputArea = NFmiAreaFactory::Create(projectionString);
    wantedGrid = new NFmiGrid(outputArea.get(), outputArea->Width(), outputArea->Height());
  }
  else
  {
    string gridFileName = cmdline.Parameter(1);

    wantedGrid = new NFmiGrid();
    ifstream inGrid(gridFileName.c_str(), ios::binary);
    if (inGrid)
    {
      inGrid >> *wantedGrid;
      inGrid.close();
    }
    else
    {
      string errStr("Hilatiedostoa ");
      errStr += gridFileName.c_str();
      errStr += " ei voitu avata!";
      throw runtime_error(errStr);
    }
  }

  NFmiQueryData qd(inputfile);

  boost::shared_ptr<NFmiQueryData> newData(
      NFmiQueryDataUtil::Interpolate2OtherGrid(&qd, wantedGrid, nullptr));

  // Temporary fix until newbase interpolation has been corrected
  NFmiWindFix::FixWinds(*newData); 
  
  newData->Write(outputfile);
}

int main(int argc, const char *argv[])
{
  try
  {
    run(argc, argv);
  }
  catch (exception &e)
  {
    cerr << e.what() << endl;
    return 1;
  }
  return 0;  // homma meni putkeen palauta 0 onnistumisen merkiksi!!!!
}
