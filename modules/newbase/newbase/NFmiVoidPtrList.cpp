// ======================================================================
/*!
 * \file NFmiVoidPtrList.cpp
 * \brief Implementation of class NFmiVoidPtrList
 * \todo This file should be annihilated
 */
// ======================================================================

#include "NFmiVoidPtrList.h"

// ----------------------------------------------------------------------
/*!
 * Void constructor
 */
// ----------------------------------------------------------------------

NFmiVoidPtrList::NFmiVoidPtrList(void)
    : itsFirstItem(0), itsCurrentItem(0), itsPreviousItem(0), itsNumberOffItems(0)
{
}

// ----------------------------------------------------------------------
/*!
 * Copy constructor
 *
 * \param listItem The other list being copied
 */
// ----------------------------------------------------------------------

NFmiVoidPtrList::NFmiVoidPtrList(const NFmiVoidPtrList &listItem)
    : itsFirstItem(0), itsCurrentItem(0), itsPreviousItem(0), itsNumberOffItems(0)
{
  CopyList(listItem);
}

// ----------------------------------------------------------------------
/*!
 * \param value Undocumented
 */
// ----------------------------------------------------------------------

void NFmiVoidPtrList::Add(NFmiVoidPtrData *value) { AddEnd(value); }
// ----------------------------------------------------------------------
/*!
 * \param value Undocumented
 */
// ----------------------------------------------------------------------

void NFmiVoidPtrList::AddStart(NFmiVoidPtrData *value)
{
  if (value)
  {
    if (itsFirstItem)
      itsFirstItem = new NFmiVoidPtrItem(value, itsFirstItem);
    else
      itsFirstItem = itsCurrentItem = new NFmiVoidPtrItem(value, itsFirstItem);

    ++itsNumberOffItems;
  }
}

// ----------------------------------------------------------------------
/*!
 * \param value Undocumented
 */
// ----------------------------------------------------------------------

void NFmiVoidPtrList::AddEnd(NFmiVoidPtrData *value)
{
  if (value)
  {
    ++itsNumberOffItems;
    if (!itsFirstItem)
    {
      itsFirstItem = itsCurrentItem = new NFmiVoidPtrItem(value);
    }
    else
    {
      while (itsCurrentItem->itsNextItem)
        itsCurrentItem = itsCurrentItem->itsNextItem;
      itsCurrentItem->itsNextItem = new NFmiVoidPtrItem(value);
      itsPreviousItem = itsCurrentItem;
      itsCurrentItem = itsCurrentItem->itsNextItem;
    }
  }
}

// ----------------------------------------------------------------------
/*!
 * \param value Undocumented
 */
// ----------------------------------------------------------------------

void NFmiVoidPtrList::AddBefore(NFmiVoidPtrData *value)
{
  if (value)
  {
    if (!itsCurrentItem)  // empty list
      AddEnd(value);
    else if (!itsPreviousItem)  // added before first item
    {
      itsFirstItem = itsPreviousItem = new NFmiVoidPtrItem(value);
      itsFirstItem->itsNextItem = itsCurrentItem;
      ++itsNumberOffItems;
    }
    else
    {
      itsPreviousItem = itsPreviousItem->itsNextItem = new NFmiVoidPtrItem(value);
      itsPreviousItem->itsNextItem = itsCurrentItem;
      ++itsNumberOffItems;
    }
  }
}

// ----------------------------------------------------------------------
/*!
 * \param removeValue Undocumented
 */
// ----------------------------------------------------------------------

void NFmiVoidPtrList::Remove(NFmiVoidPtrData *removeValue)
{
  NFmiVoidPtrItem *temp = itsFirstItem;
  if (temp->itsValue == removeValue)
  {
    DeleteItem();
    itsNumberOffItems--;
    return;
  }
  NFmiVoidPtrItem *previousItem = 0;

  for (; temp; previousItem = temp, temp = temp->itsNextItem)
    if (temp->itsValue == removeValue)
    {
      previousItem->itsNextItem = temp->itsNextItem;
      delete temp;
      itsNumberOffItems--;
      return;
    }
}

// ----------------------------------------------------------------------
/*!
 * \param doDelete Undocumented, unused
 */
// ----------------------------------------------------------------------

void NFmiVoidPtrList::Clear(
    bool /* doDelete */)  // miksi ei tarkisteta tätä ja jätetä deletoimatta tarvittaessa?
{
  while (itsFirstItem)
  {
    DeleteItem();
  }
  itsNumberOffItems = 0;
  itsCurrentItem = 0;
}

// ----------------------------------------------------------------------
/*!
 *
 */
// ----------------------------------------------------------------------

void NFmiVoidPtrList::DeleteItem(void)
{
  NFmiVoidPtrItem *deleteItem;
  deleteItem = itsFirstItem;
  itsFirstItem = itsFirstItem->itsNextItem;
  if (deleteItem == itsCurrentItem) itsCurrentItem = itsFirstItem;
  delete deleteItem;
}

// ----------------------------------------------------------------------
/*!
 * \param listItem Undocumented
 */
// ----------------------------------------------------------------------

void NFmiVoidPtrList::CopyList(const NFmiVoidPtrList &listItem)
{
  NFmiVoidPtrItem *temp = listItem.itsFirstItem;
  for (; temp; temp = temp->itsNextItem)
  {
    AddEnd(temp->itsValue);
  }
}

// ----------------------------------------------------------------------
/*!
 * \param listItem Undocumented
 */
// ----------------------------------------------------------------------

void NFmiVoidPtrList::operator+=(const NFmiVoidPtrList &listItem) { CopyList(listItem); }
// IIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIII

//                            Iterator                          II

// IIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIII

// ----------------------------------------------------------------------
/*!
 * Constructor
 *
 * \param listItem Undocumented
 */
// ----------------------------------------------------------------------

NFmiVoidPtrIterator::NFmiVoidPtrIterator(NFmiVoidPtrList *listItem)
    : itsIndex(0), itsCurrentItem(listItem->itsFirstItem), itsPreviousItem(0), itsListItem(listItem)
{
}

// ----------------------------------------------------------------------
/*!
 * Constructor
 *
 * \param listItem Undocumented
 */
// ----------------------------------------------------------------------

NFmiVoidPtrIterator::NFmiVoidPtrIterator(NFmiVoidPtrList &listItem)
    : itsIndex(0), itsCurrentItem(listItem.itsFirstItem), itsPreviousItem(0), itsListItem(&listItem)
{
}

// ----------------------------------------------------------------------
/*!
 *
 */
// ----------------------------------------------------------------------

void NFmiVoidPtrIterator::Reset(void)
{
  itsListItem->itsPreviousItem = itsPreviousItem = 0;
  itsCurrentItem = itsListItem->itsFirstItem;
  itsIndex = 0;
}

// ----------------------------------------------------------------------
/*!
 * \return Undocumented
 */
// ----------------------------------------------------------------------

void *NFmiVoidPtrIterator::Next(void)
{
  NFmiVoidPtrItem *temp = itsCurrentItem;
  if (temp)
  {
    itsListItem->itsPreviousItem = itsPreviousItem;
    itsListItem->itsCurrentItem = temp;
    itsPreviousItem = itsCurrentItem;
    itsCurrentItem = itsCurrentItem->itsNextItem;
    itsIndex++;
    return temp->itsValue->itsDataValue;
  }
  return 0;
}

// ----------------------------------------------------------------------
/*!
 *
 */
// ----------------------------------------------------------------------

void NFmiVoidPtrIterator::NextPreviousPtr(void)
{
  if (itsPreviousItem)
    itsPreviousItem = itsPreviousItem->itsNextItem;
  else
    itsPreviousItem = itsListItem->itsFirstItem;
}

// ----------------------------------------------------------------------
/*!
 * \return Undocumented
 */
// ----------------------------------------------------------------------

NFmiVoidPtrData *NFmiVoidPtrIterator::NextPtr(void)
{
  NFmiVoidPtrItem *temp = itsCurrentItem;
  if (temp)
  {
    itsPreviousItem = itsCurrentItem;
    itsCurrentItem = itsCurrentItem->itsNextItem;
    itsIndex++;
    return temp->itsValue;
  }

  return 0;
}

// ----------------------------------------------------------------------
/*!
 * \return Undocumented
 */
// ----------------------------------------------------------------------

NFmiVoidPtrData *NFmiVoidPtrIterator::CurrentPtr(void) { return itsCurrentItem->itsValue; }
// ----------------------------------------------------------------------
/*!
 * \param theItem Undocumented
 * \return Undocumented
 */
// ----------------------------------------------------------------------

bool NFmiVoidPtrIterator::Next(void *&theItem)
{
  theItem = Next();
  return *&theItem ? true : false;
}

// ----------------------------------------------------------------------
/*!
 * \param theItem Undocumented
 * \return Undocumented
 */
// ----------------------------------------------------------------------

bool NFmiVoidPtrIterator::NextPtr(NFmiVoidPtrData *&theItem)
{
  theItem = NextPtr();
  return *&theItem ? true : false;
}

// ----------------------------------------------------------------------
/*!
 * \return Undocumented
 */
// ----------------------------------------------------------------------
void *NFmiVoidPtrIterator::Current(void)
{
  return itsCurrentItem ? itsCurrentItem->itsValue->itsDataValue : 0;
}

// ----------------------------------------------------------------------
/*!
 * \return Undocumented
 */
// ----------------------------------------------------------------------

long NFmiVoidPtrIterator::Index(void) const { return CheckIndex(itsIndex) ? itsIndex : kNotInList; }
// ----------------------------------------------------------------------
/*!
 * \param theNewValue Undocumented
 * \return Undocumented
 */
// ----------------------------------------------------------------------

bool NFmiVoidPtrIterator::Index(long theNewValue)
{
  if (!CheckIndex(theNewValue)) return false;
  Reset();
  do
  {
    if (theNewValue == itsIndex)
    {
      return true;
    }
  } while (Next());
  return false;
}

// ----------------------------------------------------------------------
/*!
 * \param theValue Undocumented
 * \return Undocumented
 */
// ----------------------------------------------------------------------

bool NFmiVoidPtrIterator::CheckIndex(long theValue) const
{
  return theValue >= 0 && theValue < itsListItem->itsNumberOffItems;
}

// ======================================================================
