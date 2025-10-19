// T‰m‰ ohjelma lukee topografia dataa ja laskee siit‰ useita johdettuja parametreja:
// kFmiTopoDirectionToSea, kFmiTopoDistanceToSea, kFmiTopoDirectionToLand, kFmiTopoDistanceToLand
// calctopodata.cpp
// Tekij‰: Marko (19.9.2013)
#ifdef WIN32
#pragma warning(disable : 4786 4239 4390)  // poistaa n kpl VC++ k‰‰nt‰j‰n varoitusta
#endif

#include <newbase/NFmiCmdLine.h>
#include <newbase/NFmiGrid.h>
#include <newbase/NFmiMilliSecondTimer.h>
#include <newbase/NFmiQueryDataUtil.h>
#include <newbase/NFmiStreamQueryData.h>

#include <cassert>
#include <map>

#ifndef UNIX
#include <conio.h>
#endif

#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/math/special_functions/round.hpp>

using namespace std;  // t‰t‰ ei saa sitten laittaa headeriin, eik‰ ennen includeja!!!!

namespace
{
// Isoille datoille pit‰‰ optimoida ett‰ ei lasketa kaikkia pisteit‰ kun etsit‰‰n l‰hint‰
// maa/meripistett‰. S‰‰det‰‰n -s x optiolla. Esimerkkej‰:
// -s 0   = ei optimointia, k‰yd‰‰n kaikki pisteet l‰pi
    // -s 5 = kun ollaan 5+1 etsint‰laatikkokeh‰‰ laskentapisteest‰, aloitetaan laskemaan vain joka 2. hilapiste, 
    // kun ollaan 2*5+1 keh‰‰, aloitetaan laskemaan vain joka 3. hilapiste jne. Eli mit‰ isompi numero sit‰ v‰hemm‰n skippausta.
int g_SkipLevel = 0;
}

// Piti tehd‰ oma thread_group -luokka, koska boost:in omassa ei voi kysy‰ blokkaamatta, ett‰ onko
// kaikki
// threadit lopettaneet vai ei. Siell‰ on vain join_all-metodi, joka blokkaa.
// TƒMƒ on siis kopio thread_group.hpp:sta. En voinut peri‰ luokkaa ja lis‰t‰ ominaisuutta,
// koska peritty luokka ei p‰‰sisi k‰siksi mill‰‰n threads-dataosioon.
class thread_group_Marko
{
 private:
  thread_group_Marko(thread_group_Marko const &);
  thread_group_Marko &operator=(thread_group_Marko const &);

 public:
  thread_group_Marko() {}
  ~thread_group_Marko()
  {
    for (std::list<boost::thread *>::iterator it = threads.begin(), end = threads.end(); it != end;
         ++it)
    {
      delete *it;
    }
  }

  bool is_this_thread_in()
  {
    boost::thread::id id = boost::this_thread::get_id();
    boost::shared_lock<boost::shared_mutex> guard(m);
    for (std::list<boost::thread *>::iterator it = threads.begin(), end = threads.end(); it != end;
         ++it)
    {
      if ((*it)->get_id() == id) return true;
    }
    return false;
  }

  bool is_thread_in(boost::thread *thrd)
  {
    if (thrd)
    {
      boost::thread::id id = thrd->get_id();
      boost::shared_lock<boost::shared_mutex> guard(m);
      for (std::list<boost::thread *>::iterator it = threads.begin(), end = threads.end();
           it != end;
           ++it)
      {
        if ((*it)->get_id() == id) return true;
      }
      return false;
    }
    else
    {
      return false;
    }
  }

  void add_thread(boost::thread *thrd)
  {
    if (thrd)
    {
      BOOST_THREAD_ASSERT_PRECONDITION(
          !is_thread_in(thrd),
          thread_resource_error(system::errc::resource_deadlock_would_occur,
                                "thread_group_Marko: trying to add a duplicated thread"));

      boost::lock_guard<boost::shared_mutex> guard(m);
      threads.push_back(thrd);
    }
  }

  void remove_thread(boost::thread *thrd)
  {
    boost::lock_guard<boost::shared_mutex> guard(m);
    std::list<boost::thread *>::iterator const it = std::find(threads.begin(), threads.end(), thrd);
    if (it != threads.end())
    {
      threads.erase(it);
    }
  }

  // TƒMƒ on siis se toiminnallisuus, mit‰ halusin join_all-metodin block-ja-odota sijasta.
  // Odottaa halutun ajan joka thredin kohdalla kerrallaan. Jos threadi on k‰ynniss‰, sen
  // timed_join palauttaa false:n, jos kyseinen threadi on viel‰ k‰ynniss‰.
  // Eli jos kaikki threadit ovat lopettaneet, palauttaa true:n, muuten false.
  template <typename TimeDuration>
  bool timed_join_all(TimeDuration const &rel_time)
  {
    BOOST_THREAD_ASSERT_PRECONDITION(
        !is_this_thread_in(),
        thread_resource_error(system::errc::resource_deadlock_would_occur,
                              "thread_group_Marko: trying joining itself"));
    boost::shared_lock<boost::shared_mutex> guard(m);
    bool allThreadsEnded = true;

    for (std::list<boost::thread *>::iterator it = threads.begin(), end = threads.end(); it != end;
         ++it)
    {
      if ((*it)->joinable())  // t‰m‰ on mysteeri, mutta ilmeisesti joskus threadilta ei voi
                                     // kysy‰ timed_join-metodia, koska sen info-osio on jotenkin
                                     // 'nollautunut' (onko threadi jo lopettanut?)
        allThreadsEnded &= (*it)->timed_join(rel_time);
      if (!allThreadsEnded) break;
    }
    return allThreadsEnded;
  }

  void join_all()
  {
    BOOST_THREAD_ASSERT_PRECONDITION(
        !is_this_thread_in(),
        thread_resource_error(system::errc::resource_deadlock_would_occur,
                              "thread_group_Marko: trying joining itself"));
    boost::shared_lock<boost::shared_mutex> guard(m);

    for (std::list<boost::thread *>::iterator it = threads.begin(), end = threads.end(); it != end;
         ++it)
    {
      if ((*it)->joinable()) (*it)->join();
    }
  }

  size_t size() const
  {
    boost::shared_lock<boost::shared_mutex> guard(m);
    return threads.size();
  }

 private:
  std::list<boost::thread *> threads;
  mutable boost::shared_mutex m;
};

static void AddParam(NFmiParamBag &paramBag,
                     set<FmiParameterName> &dontCopyTheseParams,
                     FmiParameterName paramId,
                     const std::string &paramName,
                     const NFmiProducer &producer)
{
  dontCopyTheseParams.insert(paramId);
  if (!paramBag.SetCurrent(paramId))
    paramBag.Add(NFmiDataIdent(
        NFmiParam(paramId, paramName, kFloatMissing, kFloatMissing, 1, 0, "%.1", kLinearly),
        producer));
}

// saa sis‰‰n topo parametrilla varustetun infon.
// Luo samoilla spekseilla uuden infon, miss‰ topo parametrin lis‰ksi muutkin halutut
// parametrit. Lis‰ksi kopioi itse topo datan uuteen dataan.
NFmiQueryData *CreateNewQueryData(NFmiFastQueryInfo &theTopoInfo, bool copySourceParams)
{
  set<FmiParameterName> dontCopyTheseParams;
  NFmiParamBag params(theTopoInfo.ParamBag());
  if (!params.SetCurrent(kFmiTopoGraf))
    throw runtime_error(
        "Given source data didn't have topographic parameter (par-id 17), stopping execution...");
  NFmiProducer &producer = *theTopoInfo.Producer();
  ::AddParam(params, dontCopyTheseParams, kFmiTopoDirectionToSea, "Dir2Sea", producer);
  ::AddParam(params, dontCopyTheseParams, kFmiTopoDistanceToSea, "Dist2Sea", producer);
  ::AddParam(params, dontCopyTheseParams, kFmiTopoDirectionToLand, "Dir2Land", producer);
  ::AddParam(params, dontCopyTheseParams, kFmiTopoDistanceToLand, "Dist2Land", producer);
  ::AddParam(params, dontCopyTheseParams, kFmiTopoAzimuth, "SlopeDir", producer);

  NFmiQueryInfo innerInfo(NFmiParamDescriptor(params),
                          theTopoInfo.TimeDescriptor(),
                          theTopoInfo.HPlaceDescriptor(),
                          theTopoInfo.VPlaceDescriptor(),
                          theTopoInfo.InfoVersion());
  NFmiQueryData *data = NFmiQueryDataUtil::CreateEmptyData(innerInfo);
  theTopoInfo.First();
  if (data)
  {
    NFmiFastQueryInfo newInfo(data);
    NFmiDataMatrix<float> gridData;  // t‰m‰n avulla siirret‰‰n parametri eli hila kerrallaan
    // Kopioidaan uuteen dataan muut paitsi (mahdolliset) laskettavat  parametrit.
    // Oletus vain 1 aika ja level
    for (theTopoInfo.ResetParam(); theTopoInfo.NextParam();)
    {
      FmiParameterName copiedParam =
          static_cast<FmiParameterName>(theTopoInfo.Param().GetParamIdent());
      set<FmiParameterName>::iterator it = dontCopyTheseParams.find(copiedParam);
      if (copySourceParams || it == dontCopyTheseParams.end())
      {
        if (newInfo.Param(copiedParam))
        {
          theTopoInfo.Values(gridData);
          newInfo.SetValues(gridData);
        }
      }
    }
  }
  return data;
}

static bool IsMasked(float value, float landSeeMaskValue, bool fFromLand)
{
  if (landSeeMaskValue != kFloatMissing)
  {
    if (fFromLand && landSeeMaskValue == -1) return true;
    if (!fFromLand && landSeeMaskValue == 1) return true;

    return false;
  }

  if (value == kFloatMissing)
  {
    return false;
  }

  if (fFromLand ? (value <= 0.f) : (value > 0.f))
    return true;

  return false;
}

// theTopoInfo ja theLandSeeMaskInfo on asetettu tiettyyn pisteeseen.
bool CalcDistanceAndDirection(NFmiFastQueryInfo &theTopoInfo,
                              NFmiFastQueryInfo *theLandSeeMaskInfo,
                              int x,
                              int y,
                              bool fFromLand,
                              float &minDistance,
                              float &minDistDirection)
{
  float value = theTopoInfo.PeekLocationValue(x, y);
  float landSeeMaskValue = theLandSeeMaskInfo ? theLandSeeMaskInfo->PeekLocationValue(x, y) : kFloatMissing;
  if (::IsMasked(value, landSeeMaskValue, fFromLand))
  {
    NFmiLocation currentLoc(theTopoInfo.LatLon());
    NFmiPoint seePoint(theTopoInfo.PeekLocationLatLon(x, y));
    float distance = static_cast<float>(currentLoc.Distance(seePoint) /
                                        1000.f);  // et‰isyys kilometreissa => / 1000
    if (distance < minDistance)
    {
      minDistDirection = static_cast<float>(currentLoc.Direction(seePoint));
      if (minDistDirection < 0)
        minDistDirection += 360;  // muutetaan negatiiviset suunnat positiivisiksi
      minDistance = distance;
      return true;
    }
  }
  return false;
}

// Lasketaan hilan halutulla kohdalla olevan hilapisteen x-y -koko. Sen avulla voidaan arvioida,
// ollaanko etsitty
// rannikkoa tarpeeksi kaukaa.
static NFmiPoint CalcXYGridSizeInKM(NFmiFastQueryInfo &sourceInfo)
{
  NFmiPoint p1 = sourceInfo.PeekLocationLatLon(0, 0);  // keski piste
  NFmiPoint p2 = sourceInfo.PeekLocationLatLon(1, 0);  // keskelt‰ yksi oikealle
  if (p2 == NFmiPoint::gMissingLatlon)
    p2 = sourceInfo.PeekLocationLatLon(-1, 0);  // keskelt‰ yksi vasemmalle
  if (p2 == NFmiPoint::gMissingLatlon)
    throw runtime_error(
        "Error in CalcXYGridSizeInKM -function, error in program logic, can't find grid-point "
        "size, stopping...");
  NFmiPoint p3 = sourceInfo.PeekLocationLatLon(0, 1);  // keskelt‰ yksi ylˆs
  if (p3 == NFmiPoint::gMissingLatlon)
    p3 = sourceInfo.PeekLocationLatLon(0, -1);  // keskelt‰ yksi alas
  if (p3 == NFmiPoint::gMissingLatlon)
    throw runtime_error(
        "Error in CalcXYGridSizeInKM -function, error in program logic, can't find grid-point "
        "size, stopping...");
  NFmiLocation midLoc(p1);
  double gridSizeXInKm = midLoc.Distance(p2) / 1000.;
  double gridSizeYInKm = midLoc.Distance(p3) / 1000.;
  return NFmiPoint(gridSizeXInKm, gridSizeYInKm);
}

static void PrintProgress(unsigned long locationIndex)
{
  if (locationIndex && (locationIndex % 1000000 == 0))
    cerr << "#";
  else if (locationIndex && (locationIndex % 100000 == 0))
    cerr << "*";
  else if (locationIndex && (locationIndex % 10000 == 0))
    cerr << ":";
  else if (locationIndex && (locationIndex % 1000 == 0))
    cerr << ".";
}

static int CalcSkippingStep(int offset)
{ 
  if (g_SkipLevel <= 0) return 1;

  auto step = boost::math::iround(0.4999f + ((float)offset / g_SkipLevel));
  return std::max(1, step);
}

// Laskee et‰isyyden ja suunnan maalle tai merelle. Jos etsit‰‰n meri tapausta ja ollaan jo merell‰,
// et‰isyys ja suunta on 0.
// sourceTopoInfo sis‰lt‰‰ topografia datan
// sourceLandSeeMaskInfo, jos annettu, sis‰lt‰‰ maa/meri maskin, miss‰ maapisteet on arvolla 1 ja meripisteet on arvolla -1
// destDist2CoastInfo t‰h‰n lasketaan rannikon et‰isyys data
// destDir2CoastInfo t‰h‰n lasketaan suunta rannikolle
// Hila ja area pit‰‰ olla kaikissa infoissa sama! (ajalla ja level jutuilla ei ole v‰li‰)
void CalcDistanceAndDirection2CoastalLine(
    int threadIndex,
    NFmiLocationIndexRangeCalculator &theLocationIndexRangeCalculator,
    NFmiFastQueryInfo &sourceTopoInfo,
    NFmiFastQueryInfo *sourceLandSeeMaskInfo,
    NFmiFastQueryInfo &destDist2CoastInfo,
    NFmiFastQueryInfo &destDir2CoastInfo,
    bool fFromLand,
    NFmiThreadCallBacks *threadCallBacks)
{
  try
  {
    int xSize = destDist2CoastInfo.Grid()->XNumber();
    int ySize = destDist2CoastInfo.Grid()->YNumber();

    unsigned long startIndex = 0;
    unsigned long endIndex = 0;
    for (; theLocationIndexRangeCalculator.GetCurrentLocationRange(startIndex, endIndex);)
    {
      for (unsigned long locationIndex = startIndex; locationIndex <= endIndex; locationIndex++)
      {
        sourceTopoInfo.LocationIndex(locationIndex);
        if (sourceLandSeeMaskInfo) sourceLandSeeMaskInfo->LocationIndex(locationIndex);
        destDist2CoastInfo.LocationIndex(locationIndex);
        destDir2CoastInfo.LocationIndex(locationIndex);

        ::PrintProgress(locationIndex);
        // incrementaali tarkistus, jos molemmat parametrit ovat ei puuttuvia, menn‰‰n seuraavaan
        // pisteeseen
        if (destDist2CoastInfo.FloatValue() != kFloatMissing &&
            destDir2CoastInfo.FloatValue() != kFloatMissing)
          continue;

        NFmiQueryDataUtil::CheckIfStopped(threadCallBacks);
        float minDistance = 999999999.f;
        float minDistDirection = kFloatMissing;
        float value = sourceTopoInfo.FloatValue();
        float landSeeMaskValue =
            sourceLandSeeMaskInfo ? sourceLandSeeMaskInfo->FloatValue() : kFloatMissing;
        if (::IsMasked(value, landSeeMaskValue, fFromLand))
        {
          minDistance = 0.f;
          minDistDirection = 0.f;
        }
        else  // l‰hdet‰‰n etsim‰‰n rannikkoa kiert‰en peek:eill‰ laatikkoa kauemmas ja kauemmas
        {
          NFmiPoint gridSizeInKM = ::CalcXYGridSizeInKM(sourceTopoInfo);
          int xOffSet = 1;
          int yOffSet = 1;
          for (; xOffSet < xSize && yOffSet < ySize;
               xOffSet++, yOffSet++)  // kasvatetaan etsint‰ laatikon kokoa
          {
            NFmiQueryDataUtil::CheckIfStopped(threadCallBacks);
            // Katsotaan ensin orthogonaali pisteist‰, lˆytyykˆ rannikkoa ja oletetaan ett‰ t‰llˆin
            // ollaan automaattisesti
            // l‰himm‰ss‰ mahdollisessa pisteess‰ ja etsint‰ voidaan lopettaa.
            if (CalcDistanceAndDirection(sourceTopoInfo,
                                         sourceLandSeeMaskInfo,
                                         0,
                                         yOffSet,
                                         fFromLand,
                                         minDistance,
                                         minDistDirection))
              break;
            if (CalcDistanceAndDirection(sourceTopoInfo,
                                         sourceLandSeeMaskInfo,
                                         0,
                                         -yOffSet,
                                         fFromLand,
                                         minDistance,
                                         minDistDirection))
              break;
            if (CalcDistanceAndDirection(sourceTopoInfo,
                                         sourceLandSeeMaskInfo,
                                         xOffSet,
                                         0,
                                         fFromLand,
                                         minDistance,
                                         minDistDirection))
              break;
            if (CalcDistanceAndDirection(sourceTopoInfo,
                                         sourceLandSeeMaskInfo,
                                         -xOffSet,
                                         0,
                                         fFromLand,
                                         minDistance,
                                         minDistDirection))
              break;
            // Ei lˆytynyt ortho-pisteest‰ mit‰‰n, lasketaan teoreettinen min et‰isyys t‰m‰n
            // hilalaatikon
            // sis‰ll‰. Saatua limitDistance:ia voidaan k‰ytt‰‰ sitten searchMinDistMap:in
            // tarkasteluun, ett‰ jos
            // ei-ortho laatikon etsinn‰ss‰ on lˆytynyt l‰hempi piste kuin t‰m‰ teoreettinen,
            // voidaan lopettaa.
            float limitDistance = static_cast<float>(
                std::min((xOffSet - 1) * gridSizeInKM.X(), (yOffSet - 1) * gridSizeInKM.Y()));
            // Pienennet‰‰n limitDistancea hieman, jotta etsint‰ 's‰de' laajenee, huomasin 
            // pieni‰ virheit‰ datoisssa ilman t‰t‰ korjaus kerrointa
            limitDistance *= 0.95f;  
            // Isoille datoille t‰m‰ laatikon reunojen hilapisteidden osittais skippaus optimointi 
            // on t‰rke‰, koska muuten laskenta kest‰‰ ikuisuuden.
            auto skipStep = ::CalcSkippingStep(xOffSet);
            for (int i = 1; i <= yOffSet; i += skipStep)
            {
              CalcDistanceAndDirection(sourceTopoInfo,
                                       sourceLandSeeMaskInfo,
                                       i,
                                       yOffSet,
                                       fFromLand,
                                       minDistance,
                                       minDistDirection);
              CalcDistanceAndDirection(sourceTopoInfo,
                                       sourceLandSeeMaskInfo,
                                       i,
                                       -yOffSet,
                                       fFromLand,
                                       minDistance,
                                       minDistDirection);
              CalcDistanceAndDirection(sourceTopoInfo,
                                       sourceLandSeeMaskInfo,
                                       -i,
                                       yOffSet,
                                       fFromLand,
                                       minDistance,
                                       minDistDirection);
              CalcDistanceAndDirection(sourceTopoInfo,
                                       sourceLandSeeMaskInfo,
                                       -i,
                                       -yOffSet,
                                       fFromLand,
                                       minDistance,
                                       minDistDirection);
              CalcDistanceAndDirection(sourceTopoInfo,
                                       sourceLandSeeMaskInfo,
                                       xOffSet,
                                       i,
                                       fFromLand,
                                       minDistance,
                                       minDistDirection);
              CalcDistanceAndDirection(sourceTopoInfo,
                                       sourceLandSeeMaskInfo,
                                       xOffSet,
                                       -i,
                                       fFromLand,
                                       minDistance,
                                       minDistDirection);
              CalcDistanceAndDirection(sourceTopoInfo,
                                       sourceLandSeeMaskInfo,
                                       -xOffSet,
                                       i,
                                       fFromLand,
                                       minDistance,
                                       minDistDirection);
              CalcDistanceAndDirection(sourceTopoInfo,
                                       sourceLandSeeMaskInfo,
                                       -xOffSet,
                                       -i,
                                       fFromLand,
                                       minDistance,
                                       minDistDirection);
            }
            if (minDistance < limitDistance) break;
          }
        }
        destDist2CoastInfo.FloatValue(minDistance);
        destDir2CoastInfo.FloatValue(minDistDirection);
      }
    }
  }
  catch (NFmiStopThreadException &)
  {  // jos threadi on haluttu lopettaa, vain lopetetaan t‰m‰ ilman kummempia juttuja
  }
  catch (exception &e)
  {
    cerr << "\nError in thread " << threadIndex << ": " << e.what()
         << "\nStopping thread's execution..." << endl;
  }
  catch (...)
  {  // kaikki muut poikkeukset myˆs lopettavat threadin suorituksen
    cerr << "\nUnknown error in thread " << threadIndex << ", stopping thread's execution..."
         << endl;
  }
}

// Tekee joko maalle/merelle laskut.
// Jakaa teht‰v‰n osiin koneen s‰ikeille.
// K‰ytt‰j‰ voi keskeytt‰‰ suorituksen q-napin painallukseeen.
// Jos laskut suoritettu loppuun, palauttaa true, jos pakotettu keskeytys, palauttaa false.
static bool DoMultiThreadedCalculations(NFmiFastQueryInfo &topoInfo,
                                        NFmiFastQueryInfo *landSeeMaskInfo,
                                        NFmiFastQueryInfo &distParamInfo,
                                        NFmiFastQueryInfo &dirParamInfo,
                                        bool fFromLand,
                                        NFmiThreadCallBacks *threadCallBacks)
{
    // Otetaan kayttoon oikeat core:t - 2, eli jaetaan totaal lukumaara
    // kahdella, etta saadaan hyperthreadin 2x maara puoleen ja otetaan siita
    // viela pois 2, jotta kone ei ole ihan tukossa.
  int usedThreadCount = int(boost::thread::hardware_concurrency() * 0.8f);

  NFmiLocationIndexRangeCalculator locationIndexRangeCalculator(topoInfo.SizeLocations(), 10);
  std::vector<boost::shared_ptr<NFmiFastQueryInfo> > topoInfos(usedThreadCount);
  std::vector<boost::shared_ptr<NFmiFastQueryInfo> > landSeeMaskInfos(usedThreadCount);
  std::vector<boost::shared_ptr<NFmiFastQueryInfo> > distParamInfos(usedThreadCount);
  std::vector<boost::shared_ptr<NFmiFastQueryInfo> > dirParamInfos(usedThreadCount);
  for (int i = 0; i < usedThreadCount; i++)
  {
    topoInfos[i] = boost::shared_ptr<NFmiFastQueryInfo>(new NFmiFastQueryInfo(topoInfo));
    landSeeMaskInfos[i] = landSeeMaskInfo
        ? boost::shared_ptr<NFmiFastQueryInfo>(new NFmiFastQueryInfo(*landSeeMaskInfo)) : nullptr;
    distParamInfos[i] = boost::shared_ptr<NFmiFastQueryInfo>(new NFmiFastQueryInfo(distParamInfo));
    dirParamInfos[i] = boost::shared_ptr<NFmiFastQueryInfo>(new NFmiFastQueryInfo(dirParamInfo));
  }

  thread_group_Marko calcParts;
  for (int i = 0; i < usedThreadCount; i++)
    calcParts.add_thread(new boost::thread(CalcDistanceAndDirection2CoastalLine,
                                           i + 1,
                                           boost::ref(locationIndexRangeCalculator),
                                           *topoInfos[i].get(),
                                           landSeeMaskInfos[i].get(),
                                           *distParamInfos[i].get(),
                                           *dirParamInfos[i].get(),
                                           fFromLand,
                                           threadCallBacks));

  bool calculationsOk = true;
#ifndef UNIX
  for (;;)
  {
    if (calcParts.timed_join_all(boost::posix_time::milliseconds(30))) break;
    // UNIX does not need this rubbish, you just terminate with Ctrl-C
    // Huom! T‰m‰ ei ole roskaa, vaan keskeytetty‰ ajoa voidaan myˆhemmin jatkaa, jos painetaan
    // q-n‰pp‰int‰.
    // T‰llˆin tulos data talletetaan, ja jos ohjelma k‰ynnistet‰‰n uudestaan samoilla
    // argumenteilla,
    // jatkaa se laskuja siit‰ mist‰ se oli aiemmin ehtinyt. T‰m‰ voi olla oleellista, koska esim.
    // Tyynenmeren datojen laskut kestiv‰t toista vuorokautta ja ohjelma vie kaikki koneen CPU
    // resurssit.

    if (::_kbhit())  // _kbhit tarkistaa onko painettu mit‰‰n n‰pp‰int‰, ei j‰‰ odottamaan
    {
      int ch = _getch();  // jos oli painettu jotain n‰pp‰int‰, pit‰‰ tarkistaa oliko se
                          // mahdollisesti q, jolloin k‰ytt‰j‰ haluaa pakotetun lopetuksen
      if (ch == 'q' || ch == 'Q')
      {
        threadCallBacks->Stop(true);  // t‰m‰ antaa viestin eri threadeille lopettamisesta
        calculationsOk = false;
        break;
      }
    }
  }
#endif
  calcParts.join_all();  // varmuuden vuoksi viel‰ t‰m‰, jos ollaan tultu q:n painalluksella, pit‰‰
                         // viel‰ odottaa eri threadien pakotettua lopetusta
  return calculationsOk;
}

static bool CheckDestinationData(NFmiFastQueryInfo &sourceInfo,
                                 boost::shared_ptr<NFmiQueryData> &destinationData)
{
  if (destinationData)
  {
    NFmiFastQueryInfo destInfo(destinationData.get());
    // 1. pit‰‰ olla sama hila ja alue
    if (NFmiQueryDataUtil::AreGridsEqual(sourceInfo.Grid(), destInfo.Grid()))
    {
      // 2. pit‰‰ olla tietyt parametrit
      if (destInfo.Param(kFmiTopoGraf) && destInfo.Param(kFmiTopoDirectionToLand) &&
          destInfo.Param(kFmiTopoDirectionToSea) && destInfo.Param(kFmiTopoDistanceToLand) &&
          destInfo.Param(kFmiTopoDistanceToSea))
      {
        // 3. saa olla vain yksi aika ja level
        if (destInfo.SizeTimes() == 1 && destInfo.SizeLevels() == 1) return true;
      }
    }
  }
  return false;
}

// T‰m‰ ohjelma on siis incrementaalinen, eli se pyrkii jatkamaan keskeytetyn datan luomista jos
// mahdollista.
static boost::shared_ptr<NFmiQueryData> GetDestinationData(NFmiFastQueryInfo &sourceInfo,
                                                           const std::string &outpuFileName,
                                                           bool copySourceParams)
{
  boost::shared_ptr<NFmiQueryData> data;
  try
  {
    // 1. yrit‰ lukea qdata, jos outputFile sellaisen sis‰lt‰‰
    data = boost::shared_ptr<NFmiQueryData>(
        new NFmiQueryData(outpuFileName, false));  // false = ei memory mappausta, koska sellaista
                                                   // ei voi tallettaa takaisin tiedostoon,
                                                   // mem-mappaus on read-only moodia
    // 2. Jos dataa oli, takista oliko se sourceInfon mukaista
    if (::CheckDestinationData(sourceInfo, data)) return data;
  }
  catch (...)
  {
  }
  // 3. Jos tiedostossa ollut data ei ollut ok, luodaan uusi sourceInfon mukaan
  data = boost::shared_ptr<NFmiQueryData>(CreateNewQueryData(sourceInfo, copySourceParams));
  return data;
}

static float CalcNormalGrad(NFmiFastQueryInfo &sourceTopoInfo, double gridSizeInMeters, bool doX)
{
  float upper =
      doX ? sourceTopoInfo.PeekLocationValue(1, 0) : sourceTopoInfo.PeekLocationValue(0, 1);
  float lower =
      doX ? sourceTopoInfo.PeekLocationValue(-1, 0) : sourceTopoInfo.PeekLocationValue(0, -1);
  if (upper != kFloatMissing && lower != kFloatMissing)
  {
    float grad = static_cast<float>((upper - lower) / (2 * gridSizeInMeters));
    return grad;
  }
  else
    return kFloatMissing;
}

// Vasen- tai alareuna lasketaan n‰ill‰ kertoimilla, esim. vasenreuna:
// gradX = (-1 * peek(2, 0) + 4 * peek(1, 0) - 3 * peek(0, 0)) / (2*deltaX)
static float CalcLowerEdgeGrad(NFmiFastQueryInfo &sourceTopoInfo, double gridSizeInMeters, bool doX)
{
  float value1 =
      doX ? sourceTopoInfo.PeekLocationValue(2, 0) : sourceTopoInfo.PeekLocationValue(0, 2);
  float value2 =
      doX ? sourceTopoInfo.PeekLocationValue(1, 0) : sourceTopoInfo.PeekLocationValue(0, 1);
  float value3 = sourceTopoInfo.PeekLocationValue(0, 0);
  if (value1 != kFloatMissing && value2 != kFloatMissing && value3 != kFloatMissing)
  {
    float grad =
        static_cast<float>((-1 * value1 + 4 * value2 - 3 * value3) / (2 * gridSizeInMeters));
    return grad;
  }
  else
    return kFloatMissing;
}

// Oikea tai yl‰reuna lasketaan n‰ill‰ kertoimilla, esim. oikea reuna:
// gradX = (1 * peek(-2, 0) - 4 * peek(-1, 0) + 3 * peek(0, 0)) / (2*deltaX)
static float CalcUpperEdgeGrad(NFmiFastQueryInfo &sourceTopoInfo, double gridSizeInMeters, bool doX)
{
  float value1 =
      doX ? sourceTopoInfo.PeekLocationValue(-2, 0) : sourceTopoInfo.PeekLocationValue(0, -2);
  float value2 =
      doX ? sourceTopoInfo.PeekLocationValue(-1, 0) : sourceTopoInfo.PeekLocationValue(0, -1);
  float value3 = sourceTopoInfo.PeekLocationValue(0, 0);
  if (value1 != kFloatMissing && value2 != kFloatMissing && value3 != kFloatMissing)
  {
    float grad =
        static_cast<float>((1 * value1 - 4 * value2 + 3 * value3) / (2 * gridSizeInMeters));
    return grad;
  }
  else
    return kFloatMissing;
}

static float CalcTopoGradX(NFmiFastQueryInfo &sourceTopoInfo, double gridSizeInMeters)
{
  NFmiPoint gridPoint = sourceTopoInfo.Grid()->GridPoint(sourceTopoInfo.LocationIndex());
  if (gridPoint.X() == 0)  // ollaan vasemmassa reunassa
    return ::CalcLowerEdgeGrad(sourceTopoInfo, gridSizeInMeters, true);
  else if (gridPoint.X() == sourceTopoInfo.GridXNumber() - 1)  // ollaan oikeassa reunassa
    return ::CalcUpperEdgeGrad(sourceTopoInfo, gridSizeInMeters, true);
  else
    return ::CalcNormalGrad(sourceTopoInfo, gridSizeInMeters, true);
}

static float CalcTopoGradY(NFmiFastQueryInfo &sourceTopoInfo, double gridSizeInMeters)
{
  NFmiPoint gridPoint = sourceTopoInfo.Grid()->GridPoint(sourceTopoInfo.LocationIndex());
  if (gridPoint.Y() == 0)  // ollaan ala reunassa
    return ::CalcLowerEdgeGrad(sourceTopoInfo, gridSizeInMeters, false);
  else if (gridPoint.Y() == sourceTopoInfo.GridYNumber() - 1)  // ollaan yl‰ reunassa
    return ::CalcUpperEdgeGrad(sourceTopoInfo, gridSizeInMeters, false);
  else
    return ::CalcNormalGrad(sourceTopoInfo, gridSizeInMeters, false);
}

// atan2 funktio palauttaa kulman, jonka origo on 90 asteen kohdalla ja se menee
// anti-clock-wise suuntaan, joten jouduin k‰‰nt‰m‰‰n suunnan ja siirt‰m‰‰n origoa, ett‰
// saisimme maantieteelliset asteet k‰yttˆˆn (pohj. = 0, it‰ = 90, etel‰ 180 ja l‰nsi = 270 astetta)
static float CalcAngleFromAtanRad(double atan2Rad)
{
  double tmpDegreeValue = atan2Rad * (180 / kPii);
  double finalValue = -tmpDegreeValue + 90;  // k‰‰nnet‰‰n ja siirret‰‰n origoa
  if (finalValue < 0) finalValue += 360;
  return static_cast<float>(finalValue);
}

static float CalcSlopeDirAngle(NFmiFastQueryInfo &sourceTopoInfo, const NFmiPoint &gridSizeInKM)
{
  float gradX = ::CalcTopoGradX(sourceTopoInfo, gridSizeInKM.X() * 1000);
  float gradY = ::CalcTopoGradY(sourceTopoInfo, gridSizeInKM.Y() * 1000);
  if (gradX != kFloatMissing && gradY != kFloatMissing)
  {
    if (gradX == 0 && gradY == 0) return 0;  // t‰m‰ on poikkeustapaus, mit‰ atan2 ei hanskaa
    float atan2Rad = atan2(gradY, gradX);
    float slopeDirAngle = ::CalcAngleFromAtanRad(atan2Rad);
    return slopeDirAngle;
  }
  else
    return kFloatMissing;
}

// Slope dir laskut eli mihin suuntaan m‰ki on jyrkimmill‰‰n ylˆsp‰in, eli p‰invastainen suunta kuin
// minne vesi valuu.
// Lasketaan vain maalle ja merenphjalle.
// T‰t‰ ei tarvitse optimoida k‰ytt‰m‰‰n kaikkia koneen s‰ikeit‰, koska laskut ovat ns. vakioaikoja,
// ei tarvitse etsi‰ rantaa tms.
// Lasketaan topo parametrin gradienttina k‰ytt‰en seuraavaa aproksimaatiota:
// 1. Jos ollaan tarpeeksi l‰hell‰ oikeaa tai yl‰ reunaa, myˆs ulkopuolella, oletetaan ett‰ ollaan
// tasan reunalla:
// Kaava on (x-suunnassa): (1 * peek(-2, 0) - 4 * peek(-1, 0) + 3 * peek(0, 0)) / (2*deltaX)
// 2. Jos ollaan tarpeeksi l‰hell‰ vasenta tai ala reunaa, myˆs ulkopuolella, oletetaan ett‰ ollaan
// tasan reunalla:
// Kaava on (x-suunnassa): (-1 * peek(2, 0) + 4 * peek(1, 0) - 3 * peek(0, 0)) / (2*deltaX)
// 3. T‰m‰ on tavallinen tapaus, kun ollaan hilan sis‰ll‰:
// Kaava on (x-suunnassa): (peek(1, 0) - peek(-1, 0)) / (2*deltaX)
// Kun tied‰mme x- ja y- komponenttien suuruudet, voimme laske niist‰ muodostuvan vektorin avulla
// suunnan.
void CalcSlopeData(NFmiFastQueryInfo &sourceTopoInfo, NFmiFastQueryInfo &destSlopeDirInfo)
{
  for (sourceTopoInfo.ResetLocation(); sourceTopoInfo.NextLocation();)
  {
    // T‰m‰ on niin nopea, ett‰ se voidaan laskea aina ja kaikkialla,
    // eli ei tarkistusta, onko parametrilla jo arvo.
    destSlopeDirInfo.LocationIndex(sourceTopoInfo.LocationIndex());
    ::PrintProgress(sourceTopoInfo.LocationIndex());
    NFmiPoint gridSizeInKM = ::CalcXYGridSizeInKM(sourceTopoInfo);
    float slopeDirAngle = ::CalcSlopeDirAngle(sourceTopoInfo, gridSizeInKM);
    destSlopeDirInfo.FloatValue(slopeDirAngle);
  }
}

static std::unique_ptr<NFmiFastQueryInfo> CreatePossibleLandSeeMaskInfo(
    NFmiFastQueryInfo &sourceInfo)
{
  if (sourceInfo.Param(kFmiLandSeaMask))
  {
    return std::make_unique<NFmiFastQueryInfo>(sourceInfo);
  }

  return nullptr;
}

void Usage(void)
{
  cout << "calctopodata [options] sourcefile outputfile" << endl
       << endl
       << "Options:" << endl
       << endl
       << "\t-c <copy source params>\tIf source contains calculated parameters should" << endl
       << "\t\tthem be copied or calculated, default is calculate." << endl
       << "\t-s skipLevel <default 0>\tSkip calculation points more the further calculations"
       << "\t\tare from calculation point, default=0, no skipping." << endl
       << endl;
}

void Run(int argc, const char *argv[])
{
  NFmiCmdLine cmdline(argc, argv, "cs!");

  if (cmdline.Status().IsError())
  {
    cerr << "Error: Invalid command line:" << endl << cmdline.Status().ErrorLog().CharPtr() << endl;
    Usage();
    return throw runtime_error("");
  }

  if (cmdline.NumberofParameters() < 2)
  {
    cerr << "Error: two params is must: sourcefile outputfile" << endl;
    Usage();
    return throw runtime_error("");
  }

  bool copySourceParams = false;
  if (cmdline.isOption('c')) copySourceParams = true;
  if (cmdline.isOption('s')) g_SkipLevel = std::stoi(cmdline.OptionValue('s'));

  NFmiQueryData sourceData(cmdline.Parameter(1));
  NFmiFastQueryInfo sourceInfo(&sourceData);
  sourceInfo.First();
  // oletus, lukee sis‰‰n topografia datan, miss‰ vain topo parametri (17)
  // luo uuden querydatan, miss‰ on kaikki halutut parametrit
  std::string outputFile = cmdline.Parameter(2);
  boost::shared_ptr<NFmiQueryData> topoData =
      ::GetDestinationData(sourceInfo, outputFile, copySourceParams);
  if (topoData)
  {
    try
    {
      NFmiStopFunctor stopFunctor;
      NFmiThreadCallBacks threadCallBacks(&stopFunctor);

      topoData->LatLonCache();  // alustetaan latloncache multi-thread koodia varten!!
      NFmiFastQueryInfo topoInfo(topoData.get());
      topoInfo.First();

      NFmiFastQueryInfo dirParamInfo(
          topoInfo);  // joko dir2sea tai dir2land parametri valitaan t‰h‰n
      dirParamInfo.First();
      NFmiFastQueryInfo distParamInfo(
          topoInfo);  // joko dist2sea tai dist2land parametri valitaan t‰h‰n
      distParamInfo.First();

      auto possibleLandSeeMaskInfo = ::CreatePossibleLandSeeMaskInfo(sourceInfo);

      cerr << "Starting topo-calc, target has " << topoInfo.SizeLocations()
           << " grid points (x2, land+sea separat.)." << endl;
      cerr << "Progress: '.' is 1000 points, ':' is 10000, '*' is 100000 and '#' is 1000000"
           << endl;

      // Ensimm‰inen osio, lasketaan dist/dir merelle
      // -------------------------------------------
      topoInfo.Param(kFmiTopoGraf);
      dirParamInfo.Param(kFmiTopoDirectionToSea);
      distParamInfo.Param(kFmiTopoDistanceToSea);
      cerr << "Calculate dist+dir to sea" << endl;
      bool keepGoing = ::DoMultiThreadedCalculations(topoInfo,
                                                     possibleLandSeeMaskInfo.get(),
                                                     distParamInfo,
                                                     dirParamInfo,
                                                     true,
                                                     &threadCallBacks);

      if (keepGoing)
      {
        // Toinen osio, lasketaan dist/dir maalle
        // -------------------------------------------
        dirParamInfo.Param(kFmiTopoDirectionToLand);
        distParamInfo.Param(kFmiTopoDistanceToLand);
        cerr << endl << "Calculate dist+dir to land" << endl;
        keepGoing = ::DoMultiThreadedCalculations(topoInfo,
                                                  possibleLandSeeMaskInfo.get(),
                                                  distParamInfo,
                                                  dirParamInfo,
                                                  false,
                                                  &threadCallBacks);

        if (keepGoing)
        {
          // Kolmas osio, lasketaan slopedir
          // -------------------------------------------
          dirParamInfo.Param(kFmiTopoAzimuth);
          cerr << endl << "Calculate slopedir" << endl;
          ::CalcSlopeData(topoInfo, dirParamInfo);
        }
      }

      // Lopuksi kirjoitetaan data tiedostoon, vaikka keskeytys
      topoData->Write(outputFile);
    }
    catch (...)
    {
      topoData->Write(outputFile);
      throw;
    }
  }
}

int main(int argc, const char *argv[])
{
  try
  {
    NFmiMilliSecondTimer timer;
    Run(argc, argv);
    timer.StopTimer();
    cerr << endl << "Execution lasted: " << timer.EasyTimeDiffStr() << endl;
  }
  catch (exception &e)
  {
    cerr << e.what() << endl;
  }
  catch (...)
  {
    cerr << "Tuntematon poikkeus lopetti ohjelman..." << endl;
  }
}
