// ======================================================================
/*!
 * \file NFmiCommentStripper.h
 * \brief Interface of class NFmiCommentStripper
 */
// ======================================================================

#ifndef NFMICOMMENTSTRIPPER_H
#define NFMICOMMENTSTRIPPER_H

#include "NFmiDef.h"
#include "NFmiDataMatrix.h"
#include <string>
#include <set>

//! Undocumented
class _FMI_DLL NFmiCommentStripper
{
 public:
  virtual ~NFmiCommentStripper(void) {}
  NFmiCommentStripper(bool stripPound = true,
                      bool stripDoubleSlash = true,
                      bool stripSlashAst = true,
                      bool stripSlashAstNested = false,
                      bool stripEndOfLineSpaces = true);

  NFmiCommentStripper(const std::string& theFileName,
                      bool stripPound = false,
                      bool stripDoubleSlash = true,
                      bool stripSlashAst = true,
                      bool stripSlashAstNested = true,
                      bool stripEndOfLineSpaces = true);

  NFmiCommentStripper(const NFmiCommentStripper& theStripper);

  bool ReadFile(const std::string& theFileName);
  bool ReadFileCheckingOptions(const std::string& theFileName,
                               const std::string& theOptionBody,
                               std::set<std::string>& theOptionTexts);
  bool ReadAndStripFile(const std::string& theFileName);
  const std::string GetMessage(void) const { return itsMessage; };
  const std::string& GetString(void) const { return itsString; };
  void SetString(const std::string& theString) { itsString = theString; };
  virtual bool Strip(void);
  virtual bool Strip(const std::string& theString);
  bool StripBlocks(const std::string& theBeginDirective = "/*",
                   const std::string& theEndDirective = "*/");
  bool StripDoubleSlashes(void);  // to endline
  bool StripPounds(void);         // to endline
  bool StripSubStrings(const std::string& theString);

 private:
  bool CollectAndStripNested(const std::string& theBeginDirective,
                             const std::string& theEndDirective);
  bool StripNested(checkedVector<unsigned long> theBeginPositions,
                   checkedVector<unsigned long> theEndPositions);
  bool CollectStringPositions(const std::string& theSearchString,
                              checkedVector<unsigned long>& theResVector);

 protected:
  std::string itsMessage;
  std::string itsString;
  std::string itsFileName;  // for error messages
  bool fStripPound;
  bool fStripNested;
  bool fStripDoubleSlash;
  bool fStripSlashAst;
  bool fStripEndOfLineSpaces;  // Marko Lisäsin falgin, koska oli ongelmia joissain tapauksissa, jos
                               // spacet poistetaan.

};  // class NFmiCommentStripper

#endif  // NFMICOMMENTSTRIPPER_H

// ======================================================================
