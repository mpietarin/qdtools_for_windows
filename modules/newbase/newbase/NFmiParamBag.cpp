// ======================================================================
/*!
 * \file NFmiParamBag.cpp
 * \brief Implementation of class NFmiParamBag
 */
// ======================================================================
/*!
 * \class NFmiParamBag
 *
 * Undocumented
 *
 */
// ======================================================================

#include "NFmiParamBag.h"
#include <fstream>
#include <cassert>

using namespace std;

// ----------------------------------------------------------------------
/*!
 * Destructor
 */
// ----------------------------------------------------------------------

NFmiParamBag::~NFmiParamBag(void) { Destroy(); }
// ----------------------------------------------------------------------
/*!
 * Void constructor
 */
// ----------------------------------------------------------------------

NFmiParamBag::NFmiParamBag(void)
    : NFmiSize(0), itsParamsVector(0), fIsSubParamUsed(false), itsUsedSubParam(0)
{
}

// ----------------------------------------------------------------------
/*!
 * Constructor
 *
 * \param theParamArray Undocumented
 * \param numberOfParams Undocumented
 */
// ----------------------------------------------------------------------

NFmiParamBag::NFmiParamBag(FmiParameterName* theParamArray, unsigned long numberOfParams)
    : NFmiSize(numberOfParams),
      itsParamsVector(numberOfParams),
      fIsSubParamUsed(false),
      itsUsedSubParam(0)
{
  if (GetSize())
  {
    for (unsigned long i = 0; i < numberOfParams; i++)
    {
      itsParamsVector[i] = NFmiDataIdent(NFmiParam(theParamArray[i]));
    }
    Reset();
  }
}

// ----------------------------------------------------------------------
/*!
 * Constructor
 *
 * \param theParamArray Undocumented
 * \param numberOfParams Undocumented
 */
// ----------------------------------------------------------------------

NFmiParamBag::NFmiParamBag(NFmiDataIdent* theParamArray, unsigned long numberOfParams)
    : NFmiSize(numberOfParams),
      itsParamsVector(numberOfParams),
      fIsSubParamUsed(false),
      itsUsedSubParam(0)
{
  if (GetSize())
  {
    for (unsigned long i = 0; i < numberOfParams; i++)
    {
      itsParamsVector[i] = theParamArray[i];
    }
    Reset();
  }
}

// ----------------------------------------------------------------------
/*!
 * Copy constructor
 *
 * \param theBag The other NFmiParamBag being copied.
 */
// ----------------------------------------------------------------------

NFmiParamBag::NFmiParamBag(const NFmiParamBag& theBag)
    : NFmiSize(theBag.itsSize),
      itsParamsVector(theBag.itsParamsVector),
      fIsSubParamUsed(theBag.fIsSubParamUsed),
      itsUsedSubParam(theBag.itsUsedSubParam)
{
  if (GetSize())
  {
    long currentIndex = theBag.CurrentIndex();
    if (currentIndex >= 0 && currentIndex < long(GetSize()))
      SetCurrentIndex(currentIndex);
    else
      Reset();
  }
}

// ----------------------------------------------------------------------
/*!
 * Assignment operator
 *
 * \param theBag The other NFmiParamBag being copied.
 * \result Reference to the assigned result
 *
 */
// ----------------------------------------------------------------------

NFmiParamBag& NFmiParamBag::operator=(const NFmiParamBag& theBag)
{
  if (this != &theBag)
  {
    Destroy();
    itsSize = theBag.itsSize;
    itsIndex = theBag.itsIndex;
    itsParamsVector.resize(itsSize);
    itsParamsVector = theBag.itsParamsVector;
    fIsSubParamUsed = theBag.fIsSubParamUsed;
  }
  return *this;
}

// ----------------------------------------------------------------------
/*!
 * Equality comparison
 *
 * \param theParam The other NFmiParamBag this is being compared to
 * \result True, if the NFmiParamBag objects are equal.
 *
 */
// ----------------------------------------------------------------------

bool NFmiParamBag::operator==(const NFmiParamBag& theOtherParams) const
{
  bool retVal = false;
  if (GetSize() != theOtherParams.GetSize()) return false;

  for (unsigned long i = 0; i < GetSize(); i++)
  {
    if (!(this->itsParamsVector[i] == theOtherParams.itsParamsVector[i]))
      return false;
    else
      retVal = true;
  }
  return retVal;
}

// ----------------------------------------------------------------------
/*!
 * Destroy the internal parameter list. This is used as preparation
 * for assignment and in the destructor.
 */
// ----------------------------------------------------------------------

void NFmiParamBag::Destroy(void) { itsParamsVector.clear(); }
// ----------------------------------------------------------------------
/*!
 * \param theBag Undocumented
 * \result Undocumented
 *
 * \todo Change to use either boost::shared_ptr or vectors for interval variables.
 */
// ----------------------------------------------------------------------

const NFmiParamBag NFmiParamBag::Combine(const NFmiParamBag& theBag)
{
  NFmiParamBag combineBag(*this);  // täytetään ensin this-bagillä
  NFmiParamBag otherBag(theBag);   // pakko tehdä kopio, että tätä voidaan loopata
  for (otherBag.Reset(); otherBag.Next();)
    combineBag.Add(*otherBag.Current(), true);
  return combineBag;

  /* // tämä oli ihan kammottavaa koodia, yritän tehdä fiksumman ja nopeamman ja helpompi lukuisen
    if(GetSize() == 0) // Marko, ei toiminut jos this oli tyhjä
          return theBag;

    NFmiParamBag  theCombineBag(theBag);
    NFmiDataIdent * theDataIdentGroup = new NFmiDataIdent[GetSize()+theCombineBag.GetSize()];
    NFmiDataIdent * theDataIdent      = new NFmiDataIdent[GetSize()+theCombineBag.GetSize()];
    bool	 theIsIdentExisting;
    unsigned long theSizeCountGroup=0;
    unsigned long theSizeCount=0;

    Reset();//Alkuperäiset parametrit
    while(Next())
          {
            if(Current()->IsGroup())
                  theDataIdentGroup[theSizeCountGroup++] = *Current();
            else
                  theDataIdent[theSizeCount++] = *Current();
          }

    theCombineBag.Reset();//Lisä  Ryhmä Parametrit
    while(theCombineBag.Next())
          {
            theIsIdentExisting = false;
            if(theCombineBag.Current()->IsGroup())
                  {
                    for(unsigned long i=0; i<theSizeCountGroup && !theIsIdentExisting; i++)
                          {
                            if(theDataIdentGroup[i].GetParam()->GetIdent() ==
    theCombineBag.Current()->GetParam()->GetIdent())
                                  theIsIdentExisting=true;
                          }
                    if(!theIsIdentExisting)
                          theDataIdentGroup[theSizeCountGroup++] = *theCombineBag.Current() ;
                  }
          }


    theCombineBag.Reset();//Lisä  Parametrit
    while(theCombineBag.Next())
          {
            theIsIdentExisting = false;
            if(!theCombineBag.Current()->IsGroup())
                  {
                    for(unsigned long i=0; i<theSizeCount && !theIsIdentExisting; i++)
                          {
                            if(theDataIdent[i].GetParam()->GetIdent() ==
    theCombineBag.Current()->GetParam()->GetIdent())
                                  theIsIdentExisting=true;
                          }
                    if(!theIsIdentExisting)
                          theDataIdent[theSizeCount++] = *theCombineBag.Current() ;
                  }
          }

    //Muutin tästä eteenpäin olevaa koodia siten, että yhdistetyssä paramBagissä
    //on kaikki parametrit yhteen kertaan, paitsi sama parametri on kahteen kertaan,
    //jos toinen on ryhmäparametri ja toinen ei. Ennen paluuarvona olevassa param-
    //bagissä oli ainoastaan yksi ryhmäparametri sellaisten parametrien lisäksi,
    //jotka eivät ole ryhmäparametreja. Ryhmäparametrit ovat parambagissä ennen
    //"ei-ryhmäparametreja". /Mikael 6.9.99

    if (theSizeCount)
          {
            for (unsigned long i = theSizeCountGroup, j=0;
                     i<(theSizeCountGroup+theSizeCount); i++, j++)
                  {
                    theDataIdentGroup[i] = theDataIdent[j];
                  }
          }

    NFmiParamBag theParamBag(theDataIdentGroup, theSizeCountGroup+theSizeCount);

    delete [] theDataIdentGroup;
    delete [] theDataIdent;

    return  theParamBag;
  */
}

// ----------------------------------------------------------------------
/*!
 * \param theParam Undocumented
 * \param fIgnoreSubParam Undcoumented
 * \result Undocumented
 */
// ----------------------------------------------------------------------

bool NFmiParamBag::SetCurrent(FmiParameterName theParam, bool fIgnoreSubParam)
{
  Reset();
  while (Next())
  {
    if (theParam == CurrentParam()) return true;
  }
  if (!fIgnoreSubParam)
  {
    Reset();
    while (Next(false))
    {
      if (fIsSubParamUsed)
        if (theParam == static_cast<FmiParameterName>(itsUsedSubParam->GetParamIdent()))
          return true;
    }
  }

  return false;
}

// ----------------------------------------------------------------------
/*!
 * \param theParam Undocumented
 * \param fIgnoreSubParam Undocumented
 * \result Undocumented
 */
// ----------------------------------------------------------------------

bool NFmiParamBag::Current(const NFmiDataIdent& theParam, bool fIgnoreSubParam)
{
  for (Reset(); Next();)
  {
    NFmiDataIdent& id = *Current();
    if (theParam == id)  // Current())
      return true;
  }
  if (!fIgnoreSubParam) return FindSubParam(theParam);
  return false;
}

// ----------------------------------------------------------------------
/*!
 * \param theParam Undocumented
 * \param fIgnoreSubParam Undocumented
 * \result Undocumented
 */
// ----------------------------------------------------------------------

bool NFmiParamBag::Current(const NFmiParam& theParam, bool fIgnoreSubParam)
{
  for (Reset(); Next();)
  {
    NFmiParam* param = Current()->GetParam();
    assert(param);
    if (theParam == *param)  // Current())
      return true;
  }
  if (!fIgnoreSubParam) return FindSubParam(theParam);

  return false;
}

// ----------------------------------------------------------------------
/*!
 * \param fIgnoreSubParam Undocumented
 * \result Undocumented
 */
// ----------------------------------------------------------------------

NFmiDataIdent* NFmiParamBag::Current(bool fIgnoreSubParam) const
{
  if (!fIgnoreSubParam)
    if (fIsSubParamUsed) return itsUsedSubParam;
  if (GetSize() > 0 && itsIndex >= 0 && itsIndex < static_cast<long>(GetSize()))
    return const_cast<NFmiDataIdent*>(&itsParamsVector[itsIndex]);
  else
  {
    static NFmiDataIdent dummy;
    return &dummy;
  }
  //  return GetSize() ? const_cast<NFmiDataIdent*>(&itsParamsVector[itsIndex]) : 0;
}

// ----------------------------------------------------------------------
/*!
 * \param fIgnoreSubParam Undocumented
 * \result Undocumented
 */
// ----------------------------------------------------------------------

NFmiDataIdent* NFmiParamBag::EditParam(bool fIgnoreSubParam)
{
  if (!fIgnoreSubParam)
    if (fIsSubParamUsed) return itsUsedSubParam;
  return GetSize() ? const_cast<NFmiDataIdent*>(&itsParamsVector[itsIndex]) : 0;
}

// ----------------------------------------------------------------------
/*!
 * \param fIgnoreSubParam Undocumented
 * \result Undocumented
 */
// ----------------------------------------------------------------------

bool NFmiParamBag::NextActive(bool fIgnoreSubParam)
{
  while (Next(fIgnoreSubParam))
    if (Current(fIgnoreSubParam)->IsActive()) return true;

  return false;
}

// ----------------------------------------------------------------------
/*!
 * \result Undocumented
 */
// ----------------------------------------------------------------------

bool NFmiParamBag::NextData(void)
{
  while (Next())
    if (Current()->IsDataParam()) return true;

  return false;
}

// ----------------------------------------------------------------------
/*!
 * \param theParam Undocumented
 * \param isActive Undocumented
 * \param fIgnoreSubParam Undocumented
 * \result Undocumented
 */
// ----------------------------------------------------------------------

bool NFmiParamBag::SetActive(const NFmiParam& theParam, bool isActive, bool fIgnoreSubParam)
{
  for (Reset(); Next(fIgnoreSubParam);)
    if (*Current(fIgnoreSubParam)->GetParam() == theParam)
    {
      Current(fIgnoreSubParam)->SetActive(isActive);
      return true;
    }
  return false;
}

// ----------------------------------------------------------------------
/*!
 * \param theIndex Undocumented
 * \param isActive Undocumented
 * \param fIgnoreSubParam Undocumented
 * \result Undocumented
 */
// ----------------------------------------------------------------------

bool NFmiParamBag::SetActive(unsigned long theIndex, bool isActive, bool fIgnoreSubParam)
{
  if (!fIgnoreSubParam)
    if (fIsSubParamUsed)
    {
      itsUsedSubParam->SetActive(isActive);
      return true;
    }

  unsigned long oldIndex = itsIndex;
  if (Param(theIndex))
  {
    Current()->SetActive(isActive);
    Param(oldIndex);
    return true;
  }
  return false;
}

// ----------------------------------------------------------------------
/*!
 * \param isActive Undocumented
 * \param fIgnoreSubParam Undocumented
 * \result Undocumented
 */
// ----------------------------------------------------------------------

bool NFmiParamBag::SetCurrentActive(bool isActive, bool fIgnoreSubParam)
{
  if (Current(fIgnoreSubParam))
  {
    Current(fIgnoreSubParam)->SetActive(isActive);
    return true;
  }
  return false;
}

// ----------------------------------------------------------------------
/*!
 * \result Undocumented
 */
// ----------------------------------------------------------------------

unsigned long NFmiParamBag::SizeOfActive(void)
{
  NFmiParamBag tmpBag(*this);
  tmpBag.Reset();
  unsigned long sizeOfActive = 0;
  while (tmpBag.Next(false))
  {
    if (tmpBag.Current(false)->IsActive()) sizeOfActive++;
  }
  return sizeOfActive;
}

// ----------------------------------------------------------------------
/*!
 * Write the NFmiParamBag to the given stream.
 *
 * \param file The output stream to write to
 * \result The output stream written to
 */
// ----------------------------------------------------------------------

ostream& NFmiParamBag::Write(ostream& file) const
{
  NFmiSize::Write(file);

  for (unsigned long i = 0; i < GetSize(); i++)
    file << itsParamsVector[i];

  return file;
}

// ----------------------------------------------------------------------
/*!
 * Read new contents to this NFmiParamBag from the given stream.
 * All old contents are destroyed in the process.
 *
 * \param file The input stream to read from
 * \result The input stream read from
 */
// ----------------------------------------------------------------------

istream& NFmiParamBag::Read(istream& file)
{
  Destroy();

  NFmiSize::Read(file);

  if (GetSize())
  {
    itsParamsVector.resize(itsSize);

    if (GetSize())
    {
      for (unsigned long i = 0; i < GetSize(); i++)
      {
        file >> itsParamsVector[i];
      }
      Reset();
    }
  }
  return file;
}

enum BinaryOperator
{
  kNoValue = 0,
  kAnd,
  kOr,
  kXor,
  kNot
};

// ----------------------------------------------------------------------
/*!
 * Asettaa parambagin kaikkien parametrien aktiviteetit samanlaisiksi kuin
 * parametrina annetun parambagin parametreilla on (jos löytyy samoja).
 * Käy asettamassa myös kaikki aliparametrit kohdalleen.
 * Jos parametrit ovata ristiriitaisia, asetetaan epäselvät tilanteet fDefaultActivity: mukaisesti.
 *
 * \param theParams The other NFmiParamBag being partially copied
 * \param fDefaultActivity Undocumented
 * \result Undocumented
 */
// ----------------------------------------------------------------------

bool NFmiParamBag::SetActivities(NFmiParamBag& theParams,
                                 bool fDefaultActivity,
                                 bool fUseDefaultActivity,
                                 int theBinaryOperation)
{
  if (fUseDefaultActivity)
    SetActivities(fDefaultActivity);  // asetetaan aluksi aktiviteetit defaultin mukaisesti

  for (theParams.Reset(); theParams.Next(false);)
  {
    if (this->SetCurrent(theParams.Current(false)->GetParamIdent(), false))
    {
      switch (theBinaryOperation)
      {
        case kNoValue:  // eli asetetaan this:in aktiviteetti niin kuin se on theParamsissa
          this->Current(false)->SetActive(theParams.Current(false)->IsActive());
          break;
        case kAnd:  // eli jos molemmissa aktiviteetti oli true, laitetaan true, muuten false
          this->Current(false)
              ->SetActive(this->Current(false)->IsActive() && theParams.Current(false)->IsActive());
          break;
        case kOr:  // eli jos kummassa tahansa on true, laitetaan true, muuten false (itse haluan
          // käyttää tätä siten, että theParams:issa on aktiivisina ne mitkä haluan lisäksi
          // aktivoida, mutta muita en halua deaktivoida)
          this->Current(false)
              ->SetActive(this->Current(false)->IsActive() || theParams.Current(false)->IsActive());
          break;
        case kXor:  // jos toinen on false ja toinen true, tulkee true, muuten false
          this->Current(false)
              ->SetActive(this->Current(false)->IsActive() ^ theParams.Current(false)->IsActive());
          break;
        case kNot:  // eli asetetaan this:in aktiviteetti päinvastoin kuin se on theParamsissa
          this->Current(false)->SetActive(!theParams.Current(false)->IsActive());
          break;
      }
    }
  }
  return true;
}

// ----------------------------------------------------------------------
/*!
 *  Asettaa parambagin kaikkien parametrien aktiviteetit halutuksi.
 *  Käy asettamassa myös kaikki aliparametrit kohdalleen.
 *
 * \param newState Undocumented
 */
// ----------------------------------------------------------------------

void NFmiParamBag::SetActivities(bool newState)
{
  for (Reset(); Next(false);)
  {
    Current(false)->SetActive(newState);
  }
}

// ----------------------------------------------------------------------
/*!
 * Vaihtaa kaikille parametreille tuottajan
 *
 * \param newProducer The new producer to be set for the parameters
 */
// ----------------------------------------------------------------------

void NFmiParamBag::SetProducer(const NFmiProducer& newProducer)
{
  for (Reset(); Next();)
    Current()->SetProducer(newProducer);  // asettaa myös aliparametrit!
}

// ----------------------------------------------------------------------
/*!
 * \param theIndex Undocumented
 * \param fIgnoreSubParam Undocumented
 * \return Undocumented
 */
// ----------------------------------------------------------------------

NFmiDataIdent* NFmiParamBag::Param(unsigned long theIndex, bool fIgnoreSubParam) const
{
  if (!fIgnoreSubParam)
    if (fIsSubParamUsed) return itsUsedSubParam;

  unsigned long size = GetSize();
  if (size && theIndex < size) return const_cast<NFmiDataIdent*>(&itsParamsVector[theIndex]);
  return 0;
}

// ----------------------------------------------------------------------
/*!
 * \param theIndex Undocumented
 * \param fIgnoreSubParam Undocumented
 * \return Undocumented
 */
// ----------------------------------------------------------------------

NFmiDataIdent* NFmiParamBag::EditParam(unsigned long theIndex, bool fIgnoreSubParam)
{
  if (!fIgnoreSubParam)
    if (fIsSubParamUsed) return itsUsedSubParam;

  unsigned long size = GetSize();
  if (size && theIndex < size) return const_cast<NFmiDataIdent*>(&itsParamsVector[theIndex]);
  return 0;
}

// ----------------------------------------------------------------------
/*!
 * \return Undocumented
 */
// ----------------------------------------------------------------------

bool NFmiParamBag::Next(void) { return Next(true); }
// ----------------------------------------------------------------------
/*!
 * \param fIgnoreSubParam Undocumented
 * \return Undocumented
 *
 * \todo Make subparameters work correctly.
 */
// ----------------------------------------------------------------------

bool NFmiParamBag::Next(bool fIgnoreSubParam)
{
  if (fIgnoreSubParam)
    return NFmiSize::Next();
  else
  {
    if (itsIndex != -1)
    {
      if (Current()->HasDataParams())
        if (Current()->NextDataParam())
        {
          itsUsedSubParam = &Current()->CurrentDataParam();
          fIsSubParamUsed = true;
          return true;
        }
    }

    if (NFmiSize::Next())
    {
      if (Current()->HasDataParams())
      {
        Current()->ResetDataParams();
        if (Current()->NextDataParam())
        {
          itsUsedSubParam = &Current()->CurrentDataParam();
          fIsSubParamUsed = true;
          return true;
        }
      }
      else
      {
        fIsSubParamUsed = false;
        itsUsedSubParam = 0;
        return true;
      }
    }
  }
  return false;
}

// ----------------------------------------------------------------------
/*!
 * \return Undocumented
 */
// ----------------------------------------------------------------------

bool NFmiParamBag::Previous(void) { return Previous(true); }
// ----------------------------------------------------------------------
/*!
 * \param fIgnoreSubParam Undocumented
 * \return Undocumented
 *
 * \todo Make subparameters work correctly.
 */
// ----------------------------------------------------------------------

bool NFmiParamBag::Previous(bool fIgnoreSubParam)
{
  if (fIgnoreSubParam)
    return NFmiSize::Previous();
  else
  {
    if (Current()->HasDataParams())
      if (Current()->PreviousDataParam())
      {
        itsUsedSubParam = &Current()->CurrentDataParam();
        fIsSubParamUsed = true;
        return true;
      }

    if (NFmiSize::Previous())
    {
      if (Current()->HasDataParams())
      {
        itsUsedSubParam = &Current()->CurrentDataParam();
        fIsSubParamUsed = true;
        return Current()->ResetLastDataParams();
      }
      else
      {
        fIsSubParamUsed = false;
        itsUsedSubParam = 0;
        return true;
      }
    }
  }
  return false;
}

// ----------------------------------------------------------------------
/*!
 * \param fIgnoreSubParam Undocumented
 * \return Undocumented
 */
// ----------------------------------------------------------------------

bool NFmiParamBag::IsActive(bool fIgnoreSubParam) const
{
  if (!fIgnoreSubParam)
    if (fIsSubParamUsed) return itsUsedSubParam->IsActive();

  unsigned long size = GetSize();
  if (size > 0 && CurrentIndex() < static_cast<long>(size))
    return itsParamsVector[CurrentIndex()].IsActive();
  return 0;
}

// ----------------------------------------------------------------------
/*!
 * \param index Undocumented
 * \param fIgnoreSubParam Undocumented
 * \return Undocumented
 */
// ----------------------------------------------------------------------

bool NFmiParamBag::IsActive(unsigned long index, bool fIgnoreSubParam) const
{
  if (!fIgnoreSubParam)
    if (fIsSubParamUsed) return itsUsedSubParam->IsActive();

  unsigned long size = GetSize();
  if (size && index < size) return itsParamsVector[index].IsActive();
  return 0;
}

// ----------------------------------------------------------------------
/*!
 * Katsoo, löytyykö minkään parametrin aliparametreistä annettua parametriä. Jos löytyy
 * palauttaa true
 *
 * \param theParam The parameter being searched from the bag.
 * \return True, if the given parameter was found among the subparameters
 */
// ----------------------------------------------------------------------

bool NFmiParamBag::FindSubParam(const NFmiParam& theParam)
{
  fIsSubParamUsed = false;
  for (Reset(); Next();)
  {
    NFmiDataIdent* param = Current();
    if (param->IsDataParam(FmiParameterName(theParam.GetIdent())))
    {
      itsUsedSubParam = &Current()->CurrentDataParam();
      fIsSubParamUsed = true;
      return true;
    }
  }
  return false;
}

// ----------------------------------------------------------------------
/*!
 * \param theDataIdent Undocumented
 * \return Undocumented
 */
// ----------------------------------------------------------------------

bool NFmiParamBag::FindSubParam(const NFmiDataIdent& theDataIdent)
{
  fIsSubParamUsed = false;
  for (Reset(); Next();)
  {
    NFmiDataIdent* param = Current();
    if (param->IsDataParam(theDataIdent))
    {
      itsUsedSubParam = &Current()->CurrentDataParam();
      fIsSubParamUsed = true;
      return true;
    }
  }
  return false;
}

// ----------------------------------------------------------------------
/*!
 * \param directionToIter Undocumented
 */
// ----------------------------------------------------------------------

void NFmiParamBag::Reset(FmiDirection directionToIter)
{
  NFmiSize::Reset(directionToIter);
  itsUsedSubParam = 0;
  fIsSubParamUsed = false;
}

// ----------------------------------------------------------------------
/*!
 * \param theParam The parameter to add to the bag
 * \return Always true
 */
// ----------------------------------------------------------------------

bool NFmiParamBag::Add(const NFmiDataIdent& theParam, bool fCheckNoDuplicateParams)
{
  if (fCheckNoDuplicateParams)
  {
    if (Current(*theParam.GetParam()))  // huom! tarkistaa vain NFmiParam-osion, ei tuottajaa
      return false;
  }
  int size = GetSize() + 1;
  SetSize(size);
  itsParamsVector.push_back(theParam);

  return true;
}

/*!
 * Remove the current parameter (not a subparameter) from the bag
 *
 * \return True if remove succeeded, false if there was no current parameter.
 */

bool NFmiParamBag::Remove(void)
{
  if (itsIndex < 0 || static_cast<unsigned long>(itsIndex) >= itsSize)
    return false;
  else
  {
    checkedVector<NFmiDataIdent>::iterator it = itsParamsVector.begin();
    it += itsIndex;
    itsParamsVector.erase(it);
    SetSize(itsParamsVector.size());
  }
  return true;
}

// ======================================================================
