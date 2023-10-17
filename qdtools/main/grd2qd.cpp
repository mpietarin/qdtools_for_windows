/**
 * grd2qd-filtteri lukee Matin gdal:illa tuottamia grd-tiedostoja,
 * ja muuttaa sen querydataksi ja sitten tallettaa sen haluttuun tiedostoon.
 * Seuraavassa datan formaatti. Täyttö järjestys on vasemmalta oikealle
 * ja rivit ylhäältä alas.
 * Osaa lukea myös NFmiDataMatrix-formaattia -m optiolla.
 */
/*
ncols        2400
nrows        3200
xllcorner    -180.000000000000
yllcorner    -50.000000000000
cellsize     0.025000000000
-4932 -4668 -4536 -4642 -4705 -4801 -4955 -5094...
...
 */

#ifdef _MSC_VER
#pragma warning(disable : 4109 4068)  // Disables many warnings that MSVC++ 7.1 generates
#endif

#include <fstream>
#include <iostream>

#include <newbase/NFmiAreaFactory.h>
#include <newbase/NFmiCmdLine.h>
#include <newbase/NFmiCommentStripper.h>
#include <newbase/NFmiFastQueryInfo.h>
#include <newbase/NFmiFileString.h>
#include <newbase/NFmiFileSystem.h>
#include <newbase/NFmiProducerName.h>
#include <newbase/NFmiQueryData.h>
#include <newbase/NFmiQueryDataUtil.h>
#include <newbase/NFmiStreamQueryData.h>
#include <newbase/NFmiTimeList.h>

using namespace std;

static void Domain(int argc, const char *argv[]);

// Pitää tehdä sekä paramdescriptor.
// Paramdescriptorissa on sellaisia parametri nimiä kun halutaan käyttää.
static NFmiParamDescriptor MakeDefaultParamDesc(const NFmiProducer &producer,
                                                const NFmiParam &param)
{
  NFmiParamBag parBag;
  NFmiDataIdent dataIdent(param, producer);
  parBag.Add(NFmiDataIdent(dataIdent));

  return NFmiParamDescriptor(parBag);
}

static NFmiTimeDescriptor MakeTimeDesc(void)
{
  NFmiMetTime aTime;
  NFmiTimeBag timeBag(aTime, aTime, 60);
  return NFmiTimeDescriptor(aTime, timeBag);
}

static NFmiHPlaceDescriptor MakeHPlaceDesc(const NFmiGrid &theGrid)
{
  NFmiHPlaceDescriptor hplace(theGrid);
  return hplace;
}

static NFmiQueryInfo MakeQueryInfo(NFmiParamDescriptor &params, const NFmiGrid &theGrid)
{
  NFmiTimeDescriptor times(::MakeTimeDesc());
  NFmiHPlaceDescriptor hplace(::MakeHPlaceDesc(theGrid));
  NFmiQueryInfo info(params, times, hplace, NFmiVPlaceDescriptor());
  return info;
}

static NFmiQueryData *MakeQueryData(NFmiParamDescriptor &params,
                                    const NFmiGrid &theGrid,
                                    NFmiDataMatrix<float> &theDataMatrix,
                                    bool fFlipRowOrder)
{
  NFmiQueryInfo info = ::MakeQueryInfo(params, theGrid);
  NFmiQueryData *data = NFmiQueryDataUtil::CreateEmptyData(info);
  if (data)
  {
    NFmiFastQueryInfo fastInfo(data);
    fastInfo.First();
    size_t nx = theDataMatrix.NX();
    size_t ny = theDataMatrix.NY();
    for (size_t j = 0; j < ny; j++)
    {
      for (size_t i = 0; i < nx; i++)
      {
        unsigned long locationIndex =
            static_cast<unsigned long>(fFlipRowOrder ? (ny - j - 1) * nx + i : j * nx + i);
        if (fastInfo.LocationIndex(locationIndex))
        {
          fastInfo.FloatValue(theDataMatrix[i][j]);
        }
        else
          throw runtime_error(
              "Error in MakeQueryData: index out of bounds, error in application, stopping "
              "program...");
      }
    }
    return data;
  }
  throw runtime_error(
      "Error: Unable to create querydata in MakeQueryDataFromBlocks, stopping program...");
}

// ----------------------------------------------------------------------
// Kaytto-ohjeet
// ----------------------------------------------------------------------
void Usage(const std::string &theExecutableName)
{
  NFmiFileString fileNameStr(theExecutableName);
  std::string usedFileName(
      fileNameStr.FileName().CharPtr());  // ota pois mahd. polku executablen nimestä

  cerr << "Usage: " << usedFileName.c_str() << " [options] grdfilename > output" << endl
       << endl
       << "Program reads NFmiDataMatrix style grid from file and" << endl
       << "converts it to queryData." << endl
       << endl
       << "Options:" << endl
       << endl
       << "\t-p <projection>\tWanted projection, not really an option" << endl
       << "\t\t (e.g. -p stereographic,20,90,60:6,51.3,49,70.2)." << endl
       << "\t-f \tFlip row order in gridfile from upward to downward," << endl
       << "\t\t default is from bottom to top order." << endl
       << "\t-m \tRead NFmiDataMatrix format instead." << endl
       << "\t-P <parId,parName>\tWanted parameter." << endl
       << "\t-d <prodId,prodName>\tWanted producer." << endl
       << endl;
}

int main(int argc, const char *argv[])
{
  try
  {
    ::Domain(argc, argv);
  }
  catch (exception &e)
  {
    cerr << e.what() << endl;
    return 1;
  }
  return 0;  // homma meni putkeen palauta 0 onnistumisen merkiksi!!!!
}

static bool ReadGrdFile(std::istream &in, NFmiDataMatrix<float> &gridData)
{
  std::string dummy;
  in >> dummy;
  int columnSize = 0;
  in >> columnSize;
  in >> dummy;
  int rowSize = 0;
  in >> rowSize;
  in >> dummy >> dummy >> dummy >> dummy >> dummy >> dummy;
  gridData.Resize(columnSize, rowSize, kFloatMissing);
  size_t gridPointCount = gridData.NX() * gridData.NY();
  for (size_t i = 0; i < gridPointCount; i++)
  {
    float value = kFloatMissing;
    in >> value;
    gridData[i % columnSize][i / columnSize] = value;
  }
  return in.good();
}

void Domain(int argc, const char *argv[])
{
  NFmiCmdLine cmdline(argc, argv, "p!fmP!d!");

  // Tarkistetaan optioiden oikeus:
  if (cmdline.Status().IsError())
  {
    cout << "Error: Invalid command line:" << endl << cmdline.Status().ErrorLog().CharPtr() << endl;
    ::Usage(argv[0]);
    throw runtime_error("");  // tässä piti ensin tulostaa cout:iin tavaraa ja sitten vasta Usage,
                              // joten en voinut laittaa virheviesti poikkeuksen mukana.
  }

  int numOfParams = cmdline.NumberofParameters();
  if (numOfParams < 1)
  {
    cout << "Error: 1 parameter expected, 'grdfilename'\n\n";
    ::Usage(argv[0]);
    throw runtime_error("");  // tässä piti ensin tulostaa cout:iin tavaraa ja sitten vasta Usage,
                              // joten en voinut laittaa virheviesti poikkeuksen mukana.
  }

  string gridDataFileName = cmdline.Parameter(1);

  NFmiParam param(kFmiTemperature,
                  "T",
                  kFloatMissing,
                  kFloatMissing,
                  kFloatMissing,
                  kFloatMissing,
                  "%.1f",
                  kLinearly);
  if (cmdline.isOption('P'))
  {
    string paramString;
    paramString = cmdline.OptionValue('P');
    vector<string> parVec = NFmiStringTools::Split(paramString);
    if (parVec.size() != 2)
      throw runtime_error(
          "Error: -P option (parameter info) was incorrect syntax, use -P parId,parName, stopping "
          "program...");
    unsigned long parId = NFmiStringTools::Convert<unsigned long>(parVec[0]);
    string paramName = parVec[1];
    param = NFmiParam(parId,
                      paramName,
                      kFloatMissing,
                      kFloatMissing,
                      kFloatMissing,
                      kFloatMissing,
                      "%.1f",
                      kLinearly);
  }

  NFmiProducer producer(1064, "prod");
  if (cmdline.isOption('d'))
  {
    string producerString;
    producerString = cmdline.OptionValue('d');
    vector<string> prodVec = NFmiStringTools::Split(producerString);
    if (prodVec.size() != 2)
      throw runtime_error(
          "Error: -d option (producer info) was incorrect syntax, use -d prodId,prodName, stopping "
          "program...");
    unsigned long prodId = NFmiStringTools::Convert<unsigned long>(prodVec[0]);
    string prodName = prodVec[1];
    producer = NFmiProducer(prodId, prodName);
  }

  NFmiParamDescriptor params(::MakeDefaultParamDesc(producer, param));
  if (NFmiFileSystem::FileExists(gridDataFileName) == false)
    throw runtime_error(string("Error: given grdfilename:\n") + gridDataFileName +
                        "\ndoesn't exist, stopping program...");

  NFmiAreaFactory::return_type outputArea;
  if (cmdline.isOption('p'))
  {
    string projectionString;
    projectionString = cmdline.OptionValue('p');
    outputArea = NFmiAreaFactory::Create(projectionString);
  }
  else
    throw runtime_error(
        "Error: you have to give the projection string option \ne.g. -p "
        "stereographic,20,90,60:6,51.3,49,70.2,\n its not really an option, stopping program...");

  bool flipRowOrder = false;
  if (cmdline.isOption('f')) flipRowOrder = true;

  bool readDataMatrix = false;
  if (cmdline.isOption('m')) readDataMatrix = true;

  NFmiDataMatrix<float> gridData;
  ifstream in(gridDataFileName.c_str());
  if (!in)
    throw runtime_error(string("Error: given gridfilename:\n") + gridDataFileName +
                        "\ncan't be opened, stopping program...");
  if (readDataMatrix)
    in >> gridData;
  else
    ::ReadGrdFile(in, gridData);

  NFmiGrid aGrid(outputArea.get(),
                 static_cast<unsigned long>(gridData.NX()),
                 static_cast<unsigned long>(gridData.NY()));

  NFmiQueryData *newQData = ::MakeQueryData(params, aGrid, gridData, flipRowOrder);
  if (newQData == 0)
    throw runtime_error("Error: Unable to create querydata from METAR data, stopping program...");

  NFmiStreamQueryData sQOutData(newQData);  // tämä myös tuhoaa qdatan
  if (!sQOutData.WriteCout())
    throw runtime_error("Error: Couldn't write combined qdata to stdout.");
}
