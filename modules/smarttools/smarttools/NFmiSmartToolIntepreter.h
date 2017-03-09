#pragma once
//**********************************************************
// C++ Class Name : NFmiSmartToolIntepreter
// ---------------------------------------------------------
//  Author         : pietarin
//  Creation Date  : 8.11. 2010
//
// Aluksi simppeli smarttool-tulkki. Macro voi olla muotoa:
//
// calculationSection1
// IF(mask1)
//   calculationSection2
// ELSEIF(mask2)
//   calculationSection3
// ELSE(mask3)
// {
//   calculationSection4
// }
// calculationSection5
//
// Eli Voi olla laskuja, jotka suoritetaan aina ensin (calculationSection1), sitten tulee
// haluttu määrä IF - ELSEIF - ELSE osioita, joihin jokaiseen liittyy jokin calculationSection.
// ELSEIF:jä voi olla useita, mutta IF ja ELSE osioita voi olla vain yksi kumpaakin
// ja niiden on oltava päissä (IF alussa ja ELSE lopussa).
// Lopuksi vielä on calculationSection, joka suoritetaan aina.
// Kaikki osiot sikä vapaa ehtoisia, ettei mitään tarvitse olla, mutta esim IF, ELSEIF ja ELSE:n
// jälkeen pitää tulla calculationSection.
//**********************************************************

#include <newbase/NFmiParameterName.h>
#include <newbase/NFmiProducerName.h>
#include <newbase/NFmiAreaMask.h>
#include <newbase/NFmiProducer.h>
#include <newbase/NFmiLevelType.h>
#include <newbase/NFmiParamBag.h>
#include <newbase/NFmiDataMatrix.h>

#include <boost/shared_ptr.hpp>
#include <boost/tuple/tuple.hpp>

#include <map>
#include <set>

class NFmiSmartToolCalculationSectionInfo;
class NFmiAreaMaskSectionInfo;
class NFmiAreaMaskInfo;
class NFmiSmartToolCalculationInfo;
class NFmiSmartToolCalculationBlockInfo;
class NFmiProducerSystem;
class NFmiExtraMacroParamData;

class NFmiSmartToolCalculationBlockInfoVector
{
 public:
  typedef checkedVector<boost::shared_ptr<NFmiSmartToolCalculationBlockInfo> >::iterator Iterator;

  NFmiSmartToolCalculationBlockInfoVector(void);
  ~NFmiSmartToolCalculationBlockInfoVector(void);
  void Clear(void);
  void Add(boost::shared_ptr<NFmiSmartToolCalculationBlockInfo> &theBlockInfo);
  void AddModifiedParams(std::set<int> &theModifiedParams);
  Iterator Begin(void) { return itsCalculationBlockInfos.begin(); };
  Iterator End(void) { return itsCalculationBlockInfos.end(); };
  bool Empty(void) const { return itsCalculationBlockInfos.empty(); }
 private:
  // luokka ei omista vektorissa olevia otuksia, Clear pitää kutsua erikseen!!!
  checkedVector<boost::shared_ptr<NFmiSmartToolCalculationBlockInfo> > itsCalculationBlockInfos;
};

class NFmiSmartToolCalculationBlockInfo
{
 public:
  NFmiSmartToolCalculationBlockInfo(void);
  ~NFmiSmartToolCalculationBlockInfo(void);
  void Clear(void);
  void AddModifiedParams(std::set<int> &theModifiedParams);

  // luokka ei omista näitä, Clear pitää kutsua erikseen!!!
  boost::shared_ptr<NFmiSmartToolCalculationSectionInfo> itsFirstCalculationSectionInfo;
  boost::shared_ptr<NFmiAreaMaskSectionInfo> itsIfAreaMaskSectionInfo;
  boost::shared_ptr<NFmiSmartToolCalculationBlockInfoVector> itsIfCalculationBlockInfos;
  boost::shared_ptr<NFmiAreaMaskSectionInfo> itsElseIfAreaMaskSectionInfo;
  boost::shared_ptr<NFmiSmartToolCalculationBlockInfoVector> itsElseIfCalculationBlockInfos;
  bool fElseSectionExist;
  boost::shared_ptr<NFmiSmartToolCalculationBlockInfoVector> itsElseCalculationBlockInfos;
  boost::shared_ptr<NFmiSmartToolCalculationSectionInfo> itsLastCalculationSectionInfo;
};

class NFmiSmartToolIntepreter
{
 public:
  typedef std::map<std::string, FmiProducerName> ProducerMap;
  typedef std::map<std::string, double> ConstantMap;  // esim. MISS 32700 tai PI 3.14159
  void Interpret(const std::string &theMacroText, bool fThisIsMacroParamSkript = false);

  NFmiSmartToolIntepreter(NFmiProducerSystem *theProducerSystem,
                          NFmiProducerSystem *theObservationProducerSystem = 0);
  ~NFmiSmartToolIntepreter(void);

  void Clear(void);
  const std::string &GetMacroText(void) const { return itsMacroText; }
  const std::string &GetStrippedMacroText(void) const { return itsStrippedMacroText; }
  const std::string &IncludeDirectory(void) const { return itsIncludeDirectory; }
  void IncludeDirectory(const std::string &newValue) { itsIncludeDirectory = newValue; }
  checkedVector<NFmiSmartToolCalculationBlockInfo> &SmartToolCalculationBlocks(void)
  {
    return itsSmartToolCalculationBlocks;
  }
  NFmiParamBag ModifiedParams(void);
  NFmiParam GetParamFromString(const std::string &theParamText);
  // kun intepreter on tulkinnut smarttool-tekstin, voidaan kysyä, onko kyseinen makro ns.
  // macroParam-skripti eli sisältääkö se RESULT = ??? tapaista tekstiä
  bool IsInterpretedSkriptMacroParam(void);
  std::unique_ptr<NFmiExtraMacroParamData> GetOwnershipOfExtraMacroParamData();

 private:
  typedef std::map<std::string, FmiParameterName> ParamMap;

  bool CheckoutPossibleNextCalculationBlockVector(
      boost::shared_ptr<NFmiSmartToolCalculationBlockInfoVector> &theBlockVector);
  bool CheckoutPossibleNextCalculationBlock(NFmiSmartToolCalculationBlockInfo &theBlock,
                                            bool fFirstLevelCheckout,
                                            int theBlockIndex = -1);
  std::string HandlePossibleUnaryMarkers(const std::string &theCurrentString);
  NFmiLevel GetPossibleLevelInfo(const std::string &theLevelText, NFmiInfoData::Type theDataType);
  NFmiProducer GetPossibleProducerInfo(const std::string &theProducerText);
  bool IsProducerOrig(std::string &theProducerText);
  bool FindParamAndLevelAndSetMaskInfo(const std::string &theVariableText,
                                       const std::string &theLevelText,
                                       NFmiAreaMask::CalculationOperationType theOperType,
                                       NFmiInfoData::Type theDataType,
                                       boost::shared_ptr<NFmiAreaMaskInfo> &theMaskInfo,
                                       int theModelRunIndex);
  bool FindParamAndProducerAndSetMaskInfo(const std::string &theVariableText,
                                          const std::string &theProducerText,
                                          NFmiAreaMask::CalculationOperationType theOperType,
                                          NFmiInfoData::Type theDataType,
                                          boost::shared_ptr<NFmiAreaMaskInfo> &theMaskInfo,
                                          int theModelRunIndex);
  bool FindParamAndLevelAndProducerAndSetMaskInfo(
      const std::string &theVariableText,
      const std::string &theLevelText,
      const std::string &theProducerText,
      NFmiAreaMask::CalculationOperationType theOperType,
      NFmiInfoData::Type theDataType,
      boost::shared_ptr<NFmiAreaMaskInfo> &theMaskInfo,
      int theModelRunIndex);
  bool ExtractParamAndLevel(const std::string &theVariableText,
                            std::string *theParamNameOnly,
                            std::string *theLevelNameOnly);
  bool IsVariableMacroParam(const std::string &theVariableText,
                            boost::shared_ptr<NFmiAreaMaskInfo> &theMaskInfo);
  bool IsVariableDeltaZ(const std::string &theVariableText,
                        boost::shared_ptr<NFmiAreaMaskInfo> &theMaskInfo);
  bool IsVariableExtraInfoCommand(const std::string &theVariableText);
  bool IsVariableRampFunction(const std::string &theVariableText,
                              boost::shared_ptr<NFmiAreaMaskInfo> &theMaskInfo);
  bool FindParamAndSetMaskInfo(const std::string &theVariableText,
                               ParamMap &theParamMap,
                               NFmiAreaMask::CalculationOperationType theOperType,
                               NFmiInfoData::Type theDataType,
                               boost::shared_ptr<NFmiAreaMaskInfo> &theMaskInfo,
                               int theModelRunIndex);
  bool FindParamAndSetMaskInfo(const std::string &theVariableText,
                               ParamMap &theParamMap,
                               NFmiAreaMask::CalculationOperationType theOperType,
                               NFmiInfoData::Type theDataType,
                               boost::shared_ptr<NFmiAreaMaskInfo> &theMaskInfo,
                               const NFmiProducer &theProducer,
                               int theModelRunIndex);
  void InterpretDelimiter(const std::string &theDelimText,
                          boost::shared_ptr<NFmiAreaMaskInfo> &theMaskInfo);
  void InterpretToken(const std::string &theTokenText,
                      boost::shared_ptr<NFmiAreaMaskInfo> &theMaskInfo);
  bool InterpretMaskSection(const std::string &theMaskSectorText,
                            boost::shared_ptr<NFmiAreaMaskSectionInfo> &theAreaMaskSectionInfo);
  bool InterpretMasks(std::string &theMaskSectionText,
                      boost::shared_ptr<NFmiAreaMaskSectionInfo> &theAreaMaskSectionInfo);
  bool InterpretCalculationSection(
      std::string &theCalculationSectionText,
      boost::shared_ptr<NFmiSmartToolCalculationSectionInfo> &theSectionInfo);
  boost::shared_ptr<NFmiSmartToolCalculationInfo> InterpretCalculationLine(
      const std::string &theCalculationLineText);

  std::string::const_iterator ExtractFirstCalculationSection(
      const std::string &theMacroText, std::string::iterator theStartPosition);

  void InitCheckOut(void);
  bool IsCaseInsensitiveEqual(const std::string &theStr1, const std::string &theStr2);
  bool IsPossiblyLevelItem(const std::string &theText);
  bool IsPossiblyProducerItem(const std::string &theText, ProducerMap &theMap);
  bool GetProducerFromVariableById(const std::string &theVariableText, NFmiProducer &theProducer);
  bool GetLevelFromVariableById(const std::string &theVariableText,
                                NFmiLevel &theLevel,
                                NFmiInfoData::Type theDataType);
  bool IsWantedStart(const std::string &theText, const std::string &theWantedStart);
  bool GetParamFromVariable(const std::string &theVariableText,
                            ParamMap &theParamMap,
                            NFmiParam &theParam,
                            bool &fUseWildDataType);
  bool GetParamFromVariableById(const std::string &theVariableText, NFmiParam &theParam);
  bool CheckoutPossibleIfClauseSection(
      boost::shared_ptr<NFmiAreaMaskSectionInfo> &theAreaMaskSectionInfo);
  bool CheckoutPossibleElseIfClauseSection(
      boost::shared_ptr<NFmiAreaMaskSectionInfo> &theAreaMaskSectionInfo);
  bool CheckoutPossibleElseClauseSection(void);
  bool CheckoutPossibleNextCalculationSection(
      boost::shared_ptr<NFmiSmartToolCalculationSectionInfo> &theSectionInfo,
      bool &fWasBlockMarksFound);
  bool ExtractPossibleNextCalculationSection(bool &fWasBlockMarksFound);
  bool ExtractPossibleIfClauseSection(void);
  bool ExtractPossibleElseIfClauseSection(void);
  template <typename memfunction>
  bool ExtractPossibleConditionalClauseSection(memfunction conditionalChecker);
  bool IsPossibleCalculationLine(const std::string &theTextLine);
  bool IsPossibleIfConditionLine(const std::string &theTextLine);
  bool IsPossibleElseIfConditionLine(const std::string &theTextLine);
  bool IsPossibleElseConditionLine(const std::string &theTextLine);
  bool FindAnyFromText(const std::string &theText,
                       const checkedVector<std::string> &theSearchedItems);
  bool ConsistOnlyWhiteSpaces(const std::string &theText);
  bool IsVariableBinaryOperator(const std::string &theVariableText,
                                boost::shared_ptr<NFmiAreaMaskInfo> &theMaskInfo);
  NFmiAreaMask::CalculationOperator InterpretCalculationOperator(
      const std::string &theOperatorText);
  void InterpretVariable(const std::string &theVariableText,
                         boost::shared_ptr<NFmiAreaMaskInfo> &theMaskInfo,
                         bool fNewScriptVariable = false);
  bool InterpretVariableCheckTokens(const std::string &theVariableText,
                                    boost::shared_ptr<NFmiAreaMaskInfo> &theMaskInfo,
                                    bool fOrigWanted,
                                    bool fLevelExist,
                                    bool fProducerExist,
                                    const std::string &theParamNameOnly,
                                    const std::string &theLevelNameOnly,
                                    const std::string &theProducerNameOnly,
                                    int theModelRunIndex);
  bool InterpretPossibleScriptVariable(const std::string &theVariableText,
                                       boost::shared_ptr<NFmiAreaMaskInfo> &theMaskInfo,
                                       bool fNewScriptVariable);
  void CheckVariableString(const std::string &theVariableText,
                           std::string &theParamText,
                           bool &fLevelExist,
                           std::string &theLevelText,
                           bool &fProducerExist,
                           std::string &theProducerText,
                           int &theModelRunIndex);
  template <typename mapType>
  bool IsInMap(mapType &theMap, const std::string &theSearchedItem);
  bool IsVariableConstantValue(const std::string &theVariableText,
                               boost::shared_ptr<NFmiAreaMaskInfo> &theMaskInfo);
  std::string ExtractNextLine(std::string &theText,
                              std::string::iterator theStartPos,
                              std::string::iterator *theEndPos);
  bool IsVariableThreeArgumentFunction(const std::string &theVariableText,
                                       boost::shared_ptr<NFmiAreaMaskInfo> &theMaskInfo);
  bool IsVariableFunction(const std::string &theVariableText,
                          boost::shared_ptr<NFmiAreaMaskInfo> &theMaskInfo);
  bool IsVariablePeekFunction(const std::string &theVariableText,
                              boost::shared_ptr<NFmiAreaMaskInfo> &theMaskInfo);
  bool IsVariableMetFunction(const std::string &theVariableText,
                             boost::shared_ptr<NFmiAreaMaskInfo> &theMaskInfo);
  bool IsVariableVertFunction(const std::string &theVariableText,
                              boost::shared_ptr<NFmiAreaMaskInfo> &theMaskInfo);
  bool IsVariableMathFunction(const std::string &theVariableText,
                              boost::shared_ptr<NFmiAreaMaskInfo> &theMaskInfo);
  void SearchUntil(std::string::iterator &theExp_ptr, char *theTempCharPtr, char theSearchedCh);
  bool IsFunctionNameWithUnderScore(const std::string &theVariableText);
  void CheckIfVariableResemblesVerticalFunction(const std::string &theVariableText);
  bool ExtractResolutionInfo();
  bool ExtractCalculationPointInfo();
  bool ExtractObservationRadiusInfo();
  std::string GetWholeNumberFromTokens();

  NFmiProducerSystem *itsProducerSystem;               // ei omista
  std::string itsCheckOutSectionText;                  // esim. if-sectionin koko teksti
  std::string::iterator itsCheckOutTextStartPosition;  // sen hetkinen tekstiosan alkupiste
  std::string::iterator itsCheckOutTextEndPosition;    // sen hetkinen tekstiosan alkupiste
  bool fContinueCurrentSectionCheckOut;  // jatketaanko sen hetkisen sectionin tutkimista, vai onko
                                         // selvää, ettei esim. else-sectionia ole olemassa

  void SetMacroTexts(const std::string &theMacroText);
  std::string itsMacroText;
  std::string itsStrippedMacroText;
  std::string itsIncludeDirectory;  // mistä ladataan mahd. include filet

  checkedVector<NFmiSmartToolCalculationBlockInfo> itsSmartToolCalculationBlocks;
  int itsMaxCalculationSectionCount;
  std::unique_ptr<NFmiExtraMacroParamData> itsExtraMacroParamData;

  static void InitTokens(NFmiProducerSystem *theProducerSystem,
                         NFmiProducerSystem *theObservationProducerSystem);
  static void InitProducerTokens(NFmiProducerSystem *theProducerSystem);
  static bool fTokensInitialized;
  static ParamMap itsTokenParameterNamesAndIds;
  static ProducerMap itsTokenProducerNamesAndIds;
  static ConstantMap itsTokenConstants;
  static checkedVector<std::string> itsTokenConditionalCommands;
  static checkedVector<std::string> itsTokenIfCommands;
  static checkedVector<std::string> itsTokenElseIfCommands;
  static checkedVector<std::string> itsTokenElseCommands;
  static checkedVector<std::string> itsTokenCalculationBlockMarkers;
  static checkedVector<std::string> itsTokenMaskBlockMarkers;
  static checkedVector<std::string> itsTokenRampUpFunctions;
  static checkedVector<std::string> itsTokenRampDownFunctions;
  static checkedVector<std::string> itsTokenDoubleRampFunctions;
  static checkedVector<std::string> itsTokenRampFunctions;
  static checkedVector<std::string> itsTokenMacroParamIdentifiers;  // tänne listataan result jne.
                                                                    // sanat joita käytetään
                                                                    // makrojen visualisoinnissa
  static checkedVector<std::string> itsTokenDeltaZIdentifiers;  // tänne listataan deltaz 'funktiot'

  typedef std::map<std::string, FmiMaskOperation> MaskOperMap;
  static MaskOperMap itsTokenMaskOperations;
  typedef std::map<std::string, NFmiAreaMask::CalculationOperator> CalcOperMap;
  static CalcOperMap itsCalculationOperations;
  typedef std::map<std::string, NFmiAreaMask::BinaryOperator> BinaOperMap;
  static BinaOperMap itsBinaryOperator;
  static ParamMap itsTokenStaticParameterNamesAndIds;
  static ParamMap itsTokenCalculatedParameterNamesAndIds;  // mm. lat, lon ja elevAngle

  typedef std::map<std::string, NFmiAreaMask::FunctionType> FunctionMap;
  static FunctionMap itsTokenFunctions;
  static FunctionMap itsTokenThreeArgumentFunctions;
  static FunctionMap itsExtraInfoCommands;  // Tänne mm. resolution- ja calculationpoint -jutut

  typedef boost::tuple<NFmiAreaMask::FunctionType,
                       NFmiAreaMask::MetFunctionDirection,
                       int,
                       std::string>
      MetFunctionMapValue;  // MetFunktioihin talletetaan 'taika'-sanan lisäksi Funktio tyyppi ja
  // funktion argumenttien lukumäärä ja funktion oikea 'muoto' stringinä,
  // joka pitää sisällään esim. grad-funktion tapauksessa "grad(param)"
  typedef std::map<std::string, MetFunctionMapValue> MetFunctionMap;  // 'Meteorologiset' funktiot.
                                                                      // Näillä funktioilla
                                                                      // käsitellään
                                                                      // queryData-olioita eli
                                                                      // pyydetään erilaisia
  // arvoja siitä (esim. advektiota Adv(T_Hir)).
  static MetFunctionMap itsTokenMetFunctions;

  typedef boost::tuple<NFmiAreaMask::FunctionType, NFmiAreaMask::FunctionType, int, std::string>
      VertFunctionMapValue;  // Vert(ikaali)Funktioihin talletetaan 'taika'-sanan
  // lisäksi Funktio tyyppi1 (avg,max,find, jne.) ja tyyppi2 (vertikaali rajojen muoto, eli ala- ja
  // ylärajojen määritykset,
  // joilla voi olla arvoja VertP = Pressure, VertZ = height, VertFL = flight-level tai VertHyb =
  // hybridi levelit)
  // ja funktion argumenttien lukumäärä (int) ja funktion oikea 'muoto' stringinä,
  // joka pitää sisällään esim. grad-funktion tapauksessa "grad(param)"
  typedef std::map<std::string, VertFunctionMapValue> VertFunctionMap;  // Vertikaali funktiot.
                                                                        // Näillä funktioilla
                                                                        // käsitellään
                                                                        // queryData-olioita eli
                                                                        // pyydetään erilaisia
  // arvoja siitä (esim. vertp_max(WS_hir, p1, p2) hakee hirlamin maksimi tuulen nopeuden p1 ja p2
  // painepintojen väliltä).
  static VertFunctionMap itsTokenVertFunctions;

  typedef std::map<std::string, NFmiAreaMask::CalculationOperationType> PeekFunctionMap;
  static PeekFunctionMap itsTokenPeekFunctions;

  typedef std::map<std::string, NFmiAreaMask::MathFunctionType> MathFunctionMap;
  static MathFunctionMap itsMathFunctions;

  typedef std::map<std::string, FmiLevelType> ResolutionLevelTypesMap;
  static ResolutionLevelTypesMap itsResolutionLevelTypes;

  typedef std::map<std::string, int> ScriptVariableMap;
  ScriptVariableMap itsTokenScriptVariableNames;  // skriptissä varatut muuttujat (var x = ?)
                                                  // talletetaan tänne, että voidaan tarkistaa
                                                  // niiden olemassa olo
  int itsScriptVariableParamIdCounter;  // pitää keksia muutujille id, joten tehdää juokseva counter

  // normaali ja macroParam sijoituksia halutaan seurata, että ei tapahdu vahinkoja eli niitä olisi
  // sekaisin, jolloin seuramukset ovat vahingollisia
  bool fNormalAssigmentFound;  // loytyykö skriptistä normaaleja sijoituksia esim. T = ???
  bool fMacroParamFound;  // loytyykö skriptistä ns. macroParameri sijoituksia eli RESULT = ?????
  bool fMacroParamSkriptInProgress;  // Tieto siitä tulkitaanko macroParam-skriptiä vai tavallista
  // skriptiä. Poikkeus heitetään jos macrpParam-skripti päällä,
  // mutta tehdään tavallinen sijoitus

  // GetToken ja IsDelim otettu H. Schilbertin  C++: the Complete Refeference third ed.
  // jouduin muuttamaan niitä vähän sopimaan tähän ympäristöön.
  bool GetToken(void);
  bool IsDelim(char c);
  // Ed. funktiot käyttävät seuraavia muuttujia:
  enum types
  {
    NOTYPE = 0,
    DELIMITER = 1,
    VARIABLE,
    NUMBER
  };
  std::string::iterator exp_ptr;  // points to the expression
  std::string::iterator exp_end;
  char token[128];  // holds current token
  types tok_type;   // holds token's type
};
