// ======================================================================
/*!
 * \file NFmiRawData.h
 * \brief Interface of class NFmiRawData
 */
// ======================================================================

#ifndef NFMIRAWDATA_H
#define NFMIRAWDATA_H

#include "NFmiGlobals.h"
#include <string>
#include <iosfwd>
#include <string>

class _FMI_DLL NFmiRawData
{
 private:
  class Pimple;
  Pimple* itsPimple;

  NFmiRawData& operator=(const NFmiRawData& other);

 public:
  ~NFmiRawData();

  NFmiRawData();
  bool Init(size_t size);
  bool Init(size_t size,
            const std::string& theHeader,
            const std::string& theFilename,
            bool fInitialize);

  NFmiRawData(const NFmiRawData& other);

  // no mmap

  NFmiRawData(std::istream& file, size_t size, bool endianswap);

  // possible mmap
  NFmiRawData(const std::string& filename, std::istream& file, size_t size);

  size_t Size() const;
  float GetValue(size_t index) const;
  bool SetValue(size_t index, float value);

  void SetBinaryStorage(bool flag) const;
  bool IsBinaryStorageUsed() const;

  std::ostream& Write(std::ostream& file) const;

  void Backup(char* ptr) const;
  void Undo(char* ptr);

  // Advice memory mapping
  bool Advise(FmiAdvice theAdvice);

};  // class NFmiRawData

#endif  // NFMIRAWDATA_H

// ======================================================================
