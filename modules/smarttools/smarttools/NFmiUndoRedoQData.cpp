
#include "NFmiUndoRedoQData.h"
#include <newbase/NFmiRawData.h>

NFmiUndoRedoQData::NFmiUndoRedoQData(void)
{
  itsMaxUndoLevelPtr = 0;
  itsMaxRedoLevelPtr = 0;
  itsCurrentUndoLevelPtr = 0;
  itsCurrentRedoLevelPtr = 0;
  itsUndoTable = 0;
  itsUndoTextTable = 0;
  itsUndoRedoHarmonizerBookKeepingData = 0;
}

NFmiUndoRedoQData::~NFmiUndoRedoQData(void)
{
  if (itsUndoTable != 0)
  {
    for (int level = 0; level < (*itsMaxUndoLevelPtr); level++)
    {
      if (itsUndoTable[level])
        delete itsUndoTable[level];
    }
  }
  if (itsUndoTable)
  {
    delete[] itsUndoTable;
    itsUndoTable = 0;
  }
  if (itsUndoTextTable)
  {
    delete[] itsUndoTextTable;
    itsUndoTextTable = 0;
  }

  if (itsMaxUndoLevelPtr)
  {
    delete itsMaxUndoLevelPtr;
    itsMaxUndoLevelPtr = 0;
  }

  if (itsMaxRedoLevelPtr)
  {
    delete itsMaxRedoLevelPtr;
    itsMaxRedoLevelPtr = 0;
  }

  if (itsCurrentUndoLevelPtr)
  {
    delete itsCurrentUndoLevelPtr;
    itsCurrentUndoLevelPtr = 0;
  }

  if (itsCurrentRedoLevelPtr)
  {
    delete itsCurrentRedoLevelPtr;
    itsCurrentRedoLevelPtr = 0;
  }

  if (itsUndoRedoHarmonizerBookKeepingData)
  {
    itsUndoRedoHarmonizerBookKeepingData->clear();
    delete itsUndoRedoHarmonizerBookKeepingData;
    itsUndoRedoHarmonizerBookKeepingData = 0;
  }
}

bool NFmiUndoRedoQData::SnapShotData(
    const std::string &theAction,
    const NFmiHarmonizerBookKeepingData &theHarmonizerBookKeepingData,
    const NFmiRawData &theRawData)
{
  if (itsCurrentUndoLevelPtr == 0)
    return false;
  if (!itsUndoTable || !itsUndoTextTable)
    return false;

  if (*itsMaxUndoLevelPtr <= 0)
    return false;

  if (*itsCurrentUndoLevelPtr == *itsMaxUndoLevelPtr - 1)
    RearrangeUndoTable();

  (*itsCurrentUndoLevelPtr)++;
  theRawData.Backup(itsUndoTable[*itsCurrentUndoLevelPtr]);
  itsUndoRedoHarmonizerBookKeepingData->push_back(
      theHarmonizerBookKeepingData);  // lisätään perään annettu bagi
  (*itsCurrentRedoLevelPtr) = (*itsCurrentUndoLevelPtr);
  (*itsMaxRedoLevelPtr) = (*itsCurrentRedoLevelPtr);

  itsUndoTextTable[*itsCurrentUndoLevelPtr] += theAction;

  return true;
}

void NFmiUndoRedoQData::RearrangeUndoTable(void)
{
  if (itsCurrentUndoLevelPtr == 0)
    return;
  char *undoTmp = itsUndoTable[0];
  std::string undoTmpText = itsUndoTextTable[0];

  for (int i = 0; i < (*itsMaxUndoLevelPtr) - 1; i++)
  {
    itsUndoTable[i] = itsUndoTable[i + 1];
    itsUndoTextTable[i] = itsUndoTextTable[i + 1];
  }

  itsUndoTable[*itsMaxUndoLevelPtr - 1] = undoTmp;
  itsUndoTextTable[*itsMaxUndoLevelPtr - 1] = undoTmpText;

  (*itsCurrentUndoLevelPtr)--;
  (*itsCurrentRedoLevelPtr)--;

  itsUndoRedoHarmonizerBookKeepingData->pop_front();  // otetaan pois alusta yksi parBagi

  return;
}

bool NFmiUndoRedoQData::Undo(void)
{
  if (itsCurrentUndoLevelPtr == 0)
    return false;
  if ((*itsCurrentUndoLevelPtr) < 0)
    return false;
  else
    return true;
}

bool NFmiUndoRedoQData::Redo(void)
{
  if (itsCurrentUndoLevelPtr == 0)
    return false;
  if ((*itsCurrentRedoLevelPtr) == (*itsCurrentUndoLevelPtr) ||
      (*itsCurrentRedoLevelPtr) == ((*itsCurrentUndoLevelPtr) + 1))
    return false;
  else
    return true;
}

bool NFmiUndoRedoQData::UndoData(const NFmiHarmonizerBookKeepingData &theHarmonizerBookKeepingData,
                                 NFmiRawData &theRawData)
{
  if (itsCurrentUndoLevelPtr == 0)
    return false;
  if ((*itsCurrentUndoLevelPtr) < 0)
    return false;
  if ((*itsCurrentUndoLevelPtr) == (*itsCurrentRedoLevelPtr))
  {
    std::string action("");
    SnapShotData(action,
                 theHarmonizerBookKeepingData,
                 theRawData);     // "Ottaa kuvan" undo-toimintoa edeltäneestä tilanteesta,
    (*itsCurrentUndoLevelPtr)--;  // jos siihen halutaankin myöhemmin palata redo:lla.
  }

  theRawData.Undo(itsUndoTable[*itsCurrentUndoLevelPtr]);
  (*itsCurrentUndoLevelPtr)--;
  *itsCurrentRedoLevelPtr = (*itsCurrentUndoLevelPtr) + 2;
  return true;
}

bool NFmiUndoRedoQData::RedoData(NFmiRawData &theRawData)
{
  if (itsCurrentUndoLevelPtr == 0)
    return false;
  if ((*itsCurrentUndoLevelPtr) == (*itsCurrentRedoLevelPtr) ||
      ((*itsCurrentUndoLevelPtr) + 1) == (*itsCurrentRedoLevelPtr))
    return false;
  else
  {
    theRawData.Undo(itsUndoTable[*itsCurrentRedoLevelPtr]);
    (*itsCurrentUndoLevelPtr)++;
    if ((*itsCurrentRedoLevelPtr) + 1 <= (*itsMaxRedoLevelPtr))
      (*itsCurrentRedoLevelPtr)++;
  }
  return true;
}

void NFmiUndoRedoQData::UndoLevel(long theDepth,
                                  const NFmiRawData &theRawData)  // theDepth kuvaa kuinka monta
{                                                                 // Undota voidaan tehdä.
  if (!itsUndoTable && !itsUndoTextTable)  // Muuten taulukon ulkopuolelle viittaaminen voisi
  {                                        // olla mahdollista!
    itsMaxUndoLevelPtr = new long;
    itsMaxRedoLevelPtr = new long;
    itsCurrentUndoLevelPtr = new int;
    itsCurrentRedoLevelPtr = new int;

    if (theDepth <= 0)
      *itsMaxUndoLevelPtr = 0;
    else
    {
      *itsMaxRedoLevelPtr = 0;
      *itsCurrentUndoLevelPtr = -1;
      *itsCurrentRedoLevelPtr = -1;
      *itsMaxUndoLevelPtr = theDepth + 1;  // Redon tekemisen mahdollistamiseksi yksi
      try
      {
        itsUndoTable = new char *[*itsMaxUndoLevelPtr];  // taso lisää.
        itsUndoTextTable = new std::string[*itsMaxUndoLevelPtr];
        for (int level = 0; level < (*itsMaxUndoLevelPtr); level++)
          itsUndoTable[level] = 0;  // nollataan ensin pointteri, että voidaan siivota jälkiä jos
                                    // muistin varaus pettää joskus

        for (int level = 0; level < (*itsMaxUndoLevelPtr); level++)
          itsUndoTable[level] = new char[theRawData.Size() * sizeof(float)];
        itsUndoRedoHarmonizerBookKeepingData = new std::deque<NFmiHarmonizerBookKeepingData>;
      }
      catch (...)
      {  // oletetaan että poikkeus liittyi muistin varaamiseen ja lopetetaan tähän
        // siivotaan talukot tyhjiksi.
        delete itsUndoRedoHarmonizerBookKeepingData;
        itsUndoRedoHarmonizerBookKeepingData = 0;

        for (int level = 0; level < (*itsMaxUndoLevelPtr); level++)
        {
          if (itsUndoTable[level])
            delete[] itsUndoTable[level];
        }
        *itsMaxUndoLevelPtr = 0;
        delete[] itsUndoTable;
        itsUndoTable = 0;
        delete[] itsUndoTextTable;
        itsUndoTextTable = 0;

        throw;  // heitetään lopuksi poikkeus eteenpäin
      }
    }
  }
}

const NFmiHarmonizerBookKeepingData *NFmiUndoRedoQData::CurrentHarmonizerBookKeepingData(void) const
{
  int usedIndex = *itsCurrentUndoLevelPtr + 1;  // tässä pitää olla +1, koska tämä bookkeepin data
                                                // systeemi on poikkeava originaali undo/redo datan
                                                // kanssa
  if (itsUndoRedoHarmonizerBookKeepingData == 0)
    return 0;
  else if (usedIndex >= 0 &&
           usedIndex < static_cast<int>(itsUndoRedoHarmonizerBookKeepingData->size()))
    return &(itsUndoRedoHarmonizerBookKeepingData->operator[](usedIndex));
  else if (usedIndex >= static_cast<int>(itsUndoRedoHarmonizerBookKeepingData->size()))
    throw std::runtime_error("Vika ohjelmassa NFmiSmartInfo::CurrentHarmonizerParams");
  else
    return 0;
}
