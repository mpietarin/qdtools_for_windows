// ======================================================================
/*!
 * \file NFmiStringList.cpp
 * \brief Implementation of class NFmiStringList
 */
// ======================================================================
/*!
 * \class NFmiStringList
 *
 * This pointer list is made for the objects of the class NFmiString.
 * It encapsulates the void pointer list and its iterator having pointers to
 * objects of the classes NFmiVoidPtrList and NFmiVoidPtrIterator as attributes
 * of this class. Through casting the data of NFmiVoidPtrList-object classifyed
 * by type void to the pointers of the NFmiString objects the methods of
 * classes NFmiVoidPtrList and NFmiVoidPtrIterator are available.
 *
 * Basically, since the class uses NFmiVoidPtrList, you should not use this
 * class anymore. Just use std::list<std::string>.
 */
// ======================================================================

#include "NFmiStringList.h"
#include "NFmiStatusPositionString.h"

// ----------------------------------------------------------------------
/*!
 * Destructor deletes the string pointers stored internally
 */
// ----------------------------------------------------------------------

NFmiStringList::~NFmiStringList(void) { Destroy(); }
// ----------------------------------------------------------------------
/*!
 * Copy constructor
 *
 * \param theList The object being copied
 */
// ----------------------------------------------------------------------

NFmiStringList::NFmiStringList(const NFmiStringList &theList) : itsList(), itsIndex(0)
{
  const StorageType::const_iterator begin = theList.itsList.begin();
  const StorageType::const_iterator end = theList.itsList.end();
  for (StorageType::const_iterator iter = begin; iter != end; ++iter)
    Add((*iter)->Clone());
}

// ----------------------------------------------------------------------
/*!
 * Takes the pointer to the pointer theItem as input parameter.  The input
 * pointer theItem retrieves the return pointer pointing to the data of
 * the current item. Then calls Next() and returns the bool; true
 * if pointer value of theItem exist, others false.
 *
 * WARNING: This method moves the cursor out of the list when the calling
 * takes place on the last item of the list.
 *
 * \param theItem Undocumented
 * \return Undocumented
 */
// ----------------------------------------------------------------------

bool NFmiStringList::Next(NFmiString **theItem)
{
  *theItem = Current();
  if (*theItem)
  {
    Next();
    return true;
  }
  return false;
}

// ----------------------------------------------------------------------
/*!
 * Moves the iterators itsIter cursor to the next item of the list itsList.
 *
 * \return Undocumented
 */
// ----------------------------------------------------------------------

bool NFmiStringList::Next(void)
{
  if (itsList.size())
  {
    itsIndex = (itsIndex < itsList.size() - 1) ? itsIndex + 1 : 0;
    return (itsIndex != 0);
  }
  return false;
}

//--------------------------------------------------------
/*!
 * Moves the cursor of the iterator itsIter to the previous item
 *
 * \return false, if the original value of itsIndex = 0, others true
 */
//--------------------------------------------------------
bool NFmiStringList::Previous(void)
{
  if (itsIndex > 0)
  {
    itsIndex--;
    return true;
  }
  itsIndex = 0;
  return false;
}

// ----------------------------------------------------------------------
/*!
 * Returns the pointer to the data (NFmiString object) of the current
 * item of the list itsList.
 *
 * \return Undocumented
 */
// ----------------------------------------------------------------------

NFmiString *NFmiStringList::Current(void) const
{
  if (itsIndex < itsList.size())
    return itsList[itsIndex];
  else
    return 0;
}

// ----------------------------------------------------------------------
/*!
 * Resets the iterator itsIter of the list.
 * \param FmiDirection theDirect:
 * - defaul value kForward --> itsIter sets at the first item
 * - kBackward --> itsIter sets at the last item
 *
 * \return false for an empty list, others true
 */
// ----------------------------------------------------------------------

bool NFmiStringList::Reset(FmiDirection theDirect)
{
  if (theDirect == kBackward)
  {
    if (itsList.size() > 0)
      itsIndex = itsList.size() - 1;
    else
      itsIndex = 0;
  }
  else
    itsIndex = 0;

  return (itsList.size() > 0);
}

// ----------------------------------------------------------------------
/*!
 * Adds the given parameter pointer of NFmiString-object theItem to the end of
 * the list. Note that pointer ownership is taken, no clone is being made.
 * Also, the iterator Reset().
 *
 * \param theItem Undocumented
 */
// ----------------------------------------------------------------------

void NFmiStringList::Add(NFmiString *theItem)
{
  itsList.push_back(theItem);
  Reset();
}

// ----------------------------------------------------------------------
/*!
 * Adds the contents of the list given as a parameter by iterating through theList
 * and calling Add( NFmiString *theItem, bool fCreateCopy = false ) to
 * every item. As result the same data pointer exist in both lists.
 *
 * WARNING: don't delete the list added, please use Flush().
 *
 * \param theList Undocumented
 */
// ----------------------------------------------------------------------

void NFmiStringList::Add(NFmiStringList *theList)
{
  const StorageType::const_iterator begin = theList->itsList.begin();
  const StorageType::const_iterator end = theList->itsList.end();
  for (StorageType::const_iterator iter = begin; iter != end; ++iter)
    Add(*iter);
}

// ----------------------------------------------------------------------
/*!
 * Lisää annetun merkkijonon theStr listaan riveittäin. Rivin pituus annetaan
 * parametrina. Rivit eli listaan lisättävät itemit pyritään jos mahdollista
 * katkaisemaan välilyönnin kohdalta eli sanat pyritään säilyttämään kokonaisina.
 * Kutakin riviä varten luodaan dynaamisesti uusi NFmiString-merkkijono.
 * Esim. jos merkkijonon pituus on pienempi kuin annettu rivin pituus,
 * str-listaan lisätään vain yksi item.
 *
 *
 * \param theStr, listaan lisättävän merkkijonon osoite
 */
// ----------------------------------------------------------------------

void NFmiStringList::Add(NFmiString *theStr, unsigned short theLengthLimitForStrItem)
{
  NFmiString *strLeft = new NFmiString(*theStr);
  NFmiString *tempStr = 0;
  unsigned long lineLenght = theLengthLimitForStrItem;
  unsigned long ind = 0;

  do
  {
    if (strLeft->GetLen() <= lineLenght)  // only on line left
    {
      Add(strLeft);
      return;
    }

    ind = strLeft->SearchLast(reinterpret_cast<const unsigned char *>(" "), lineLenght);
    // if line feed not found, the end of the word which is longer than lineLenght must be splitt
    // into the next line
    if (!ind) ind = lineLenght;
    // the line feed found stays at the end of the line added to the list
    tempStr = new NFmiString(strLeft->GetChars(1, ind));
    Add(tempStr);

    NFmiString strLeft2(strLeft->GetChars(ind + 1, strLeft->GetLen() - ind));
    delete strLeft;
    strLeft = new NFmiString(strLeft2);
  } while (strLeft->GetLen() > 0);  // because of this check have to use "new NFmiString"
  delete strLeft;
}

// ----------------------------------------------------------------------
/*!
 * Deletes all the list items of itsList but remains the list itself.
 *
 * \param fDeleteData Undocumented
 */
// ----------------------------------------------------------------------

void NFmiStringList::Clear(bool fDeleteData)
{
  if (fDeleteData)
    Destroy();
  else
    itsList.clear();
  Reset();
}

// ----------------------------------------------------------------------
/*!
 * Assignment operator
 *
 * \param theList The other object being copied
 * \return The object assigned to
 */
// ----------------------------------------------------------------------

NFmiStringList &NFmiStringList::operator=(NFmiStringList &theList)
{
  if (this != &theList)
  {
    Destroy();
    const StorageType::const_iterator begin = theList.itsList.begin();
    const StorageType::const_iterator end = theList.itsList.end();
    for (StorageType::const_iterator iter = begin; iter != end; ++iter)
      Add((*iter)->Clone());
    Reset();
  }
  return *this;
}

// ----------------------------------------------------------------------
/*!
 * Write the object to the given output stream
 *
 * \param file The output stream to write to
 * \return The output stream written to
 */
// ----------------------------------------------------------------------

std::ostream &NFmiStringList::Write(std::ostream &file) const
{
  long items = NumberOfItems();
  file << items << std::endl;

  const StorageType::const_iterator begin = itsList.begin();
  const StorageType::const_iterator end = itsList.end();
  for (StorageType::const_iterator iter = begin; iter != end; ++iter)
  {
    file << (*iter)->ClassId() << " " << *(*iter);
  }
  return file;
}

// ----------------------------------------------------------------------
/*!
 * Read new object contents from the given input stream
 */
// ----------------------------------------------------------------------

std::istream &NFmiStringList::Read(std::istream &file)
{
  int items;
  file >> items;

  unsigned long classId;

  NFmiString *item = 0;
  for (long i = 0; i < items; i++)
  {
    file >> classId;
    if (classId == kNFmiString)
      item = new NFmiString;
    else if (classId == kNFmiStatusString)
      item = new NFmiStatusString;
    else if (classId == kNFmiStatusPositionString)
      item = new NFmiStatusPositionString;
    file >> *item;
    Add(item);
  }
  return file;
}

// ----------------------------------------------------------------------
/*!
 * \param status Undocumented
 * \return Undocumented
 */
// ----------------------------------------------------------------------

bool NFmiStringList::FindWithStatus(long status)
{
  for (itsIndex = 0; itsIndex < itsList.size(); itsIndex++)
  {
    if (Current()->ClassId() == kNFmiStatusString ||
        Current()->ClassId() == kNFmiStatusPositionString)
      if (status == (static_cast<NFmiStatusString *>(Current())->Status())) return true;
  }
  Reset();
  return false;
}

// ----------------------------------------------------------------------
/*!
 * \param str Undocumented
 * \return Undocumented
 */
// ----------------------------------------------------------------------

bool NFmiStringList::Find(const NFmiString &str)
{
  for (itsIndex = 0; itsIndex < itsList.size(); itsIndex++)
  {
    if (str == *Current()) return true;
  }
  return false;
}

// ----------------------------------------------------------------------
/*!
 * \return Undocumented
 */
// ----------------------------------------------------------------------

// 16.5.2001/Marko Poistaa currentin stringin listasta.
// HUOM!!!!! Toimii huonosti, poiston jälkeen lista osoittaa seuraavaa stringiä.
// Ongelma jos käy listaa läpi Next:illa ja poistaa tiettyjä stringeja, tällöin
// poiston jälkeen siirrytään eteenpäin ja Next siirrtää vielä yhden eteenpäin.
// SYY VoidPointerList ja sen rajoitukset ja vaikeudet (en luettele tässä)
// KORJAUS: Toteuta koko juttu STL-vector:ia käyttäen.

bool NFmiStringList::Remove(void)
{
  if (Current())
  {
    StorageType::iterator iter = itsList.begin() + itsIndex;
    delete *iter;
    *iter = 0;
    itsList.erase(iter);
    if (itsIndex > itsList.size()) Reset();
    return true;
  }
  return false;
}

// ----------------------------------------------------------------------
/*!
 * Clear the internal list, deleting the contents
 */
// ----------------------------------------------------------------------

void NFmiStringList::Destroy(void)
{
  const StorageType::iterator begin = itsList.begin();
  const StorageType::iterator end = itsList.end();
  for (StorageType::iterator iter = begin; iter != end; ++iter)
    delete *iter;
  itsList.clear();
  itsIndex = 0;
}

// ======================================================================
