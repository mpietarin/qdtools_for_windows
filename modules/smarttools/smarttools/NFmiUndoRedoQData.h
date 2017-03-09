#pragma once

#include <deque>
#include "NFmiHarmonizerBookKeepingData.h"

class NFmiRawData;

class NFmiUndoRedoQData
{
 public:
  NFmiUndoRedoQData(void);
  ~NFmiUndoRedoQData(void);

  bool SnapShotData(const std::string& theAction,
                    const NFmiHarmonizerBookKeepingData& theHarmonizerBookKeepingData,
                    const NFmiRawData& theRawData);
  void RearrangeUndoTable(void);
  bool Undo(void);
  bool Redo(void);
  bool UndoData(const NFmiHarmonizerBookKeepingData& theHarmonizerBookKeepingData,
                NFmiRawData& theRawData);
  bool RedoData(NFmiRawData& theRawData);
  void UndoLevel(long theDepth, const NFmiRawData& theRawData);
  const NFmiHarmonizerBookKeepingData* CurrentHarmonizerBookKeepingData(void) const;

 private:
  long* itsMaxUndoLevelPtr;
  long* itsMaxRedoLevelPtr;
  int* itsCurrentUndoLevelPtr;
  int* itsCurrentRedoLevelPtr;

  char** itsUndoTable;
  std::string* itsUndoTextTable;
  std::deque<NFmiHarmonizerBookKeepingData>*
      itsUndoRedoHarmonizerBookKeepingData;  // tämän elin kaari seuraa tiiviisti itsUndoTable:a
  // tähän talletetaan harmonisaatiossa 'likaantuvat' parametrit ja ajat ja koska
  // editorissa on undo/redo toiminto, pitää myös tämän olla synkassa datan kanssa
};
