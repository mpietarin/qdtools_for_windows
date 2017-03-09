
/*!  NFmiDictionaryFunction.h
 * Tämä on Editorin käyttämä sana kirja funktio.
 * Kieli versiot stringeihin tulevat täältä.
 */

#ifndef NFMIDICTIONARYFUNCTION_H
#define NFMIDICTIONARYFUNCTION_H

#include <newbase/NFmiSettings.h>

// HUOM! Tämä on kopio NFmiEditMapGeneralDataDoc-luokan metodista, kun en voinut antaa tänne
// dokumenttia
std::string GetDictionaryString(const char *theMagicWord);

#endif
