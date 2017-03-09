// ======================================================================
/*!
 * \file NFmiStatus.cpp
 * \brief Implementation of class NFmiStatus
 */
// ======================================================================

#include "NFmiStatus.h"

// ----------------------------------------------------------------------
/*!
 * Void constructor
 */
// ----------------------------------------------------------------------

NFmiStatus::NFmiStatus(void) : itsErrorLog(), itsWarningLog(), itsMessageLog() {}
// ----------------------------------------------------------------------
/*!
 * Copy constructor
 *
 * \param theStatus The other object being copied
 */
// ----------------------------------------------------------------------

NFmiStatus::NFmiStatus(const NFmiStatus& theStatus)
    : itsErrorLog(theStatus.itsErrorLog),
      itsWarningLog(theStatus.itsWarningLog),
      itsMessageLog(theStatus.itsMessageLog)
{
}

// ----------------------------------------------------------------------
/*!
 * \return Undocumented
 */
// ----------------------------------------------------------------------

bool NFmiStatus::IsWarning(void) const { return itsWarningLog.IsValue(); }
// ----------------------------------------------------------------------
/*!
 * \return Undocumented
 */
// ----------------------------------------------------------------------

bool NFmiStatus::IsError(void) const { return itsErrorLog.IsValue(); }
// ----------------------------------------------------------------------
/*!
 * \return Undocumented
 */
// ----------------------------------------------------------------------

bool NFmiStatus::IsMessage(void) const { return itsMessageLog.IsValue(); }
// ----------------------------------------------------------------------
/*!
 * \param theErrorLog Undocumented
 */
// ----------------------------------------------------------------------

void NFmiStatus::ErrorLog(NFmiString& theErrorLog) { itsErrorLog = theErrorLog; }
// ----------------------------------------------------------------------
/*!
 * \param theWarningLog Undocumented
 */
// ----------------------------------------------------------------------

void NFmiStatus::WarningLog(NFmiString& theWarningLog) { itsWarningLog = theWarningLog; }
// ----------------------------------------------------------------------
/*!
 * \param theMessageLog Undocumented
 */
// ----------------------------------------------------------------------

void NFmiStatus::MessageLog(NFmiString& theMessageLog) { itsMessageLog = theMessageLog; }
// ----------------------------------------------------------------------
/*!
 * \param theErrorLog Undocumented
 */
// ----------------------------------------------------------------------

void NFmiStatus::ErrorLog(const char* theErrorLog) { itsErrorLog = theErrorLog; }
// ----------------------------------------------------------------------
/*!
 * \param theWarningLog Undocumented
 */
// ----------------------------------------------------------------------

void NFmiStatus::WarningLog(const char* theWarningLog) { itsWarningLog = theWarningLog; }
// ----------------------------------------------------------------------
/*!
 * \param theMessageLog Undocumented
 */
// ----------------------------------------------------------------------

void NFmiStatus::MessageLog(const char* theMessageLog) { itsMessageLog = theMessageLog; }
// ----------------------------------------------------------------------
/*!
 * \return Undocumented
 */
// ----------------------------------------------------------------------

const NFmiString NFmiStatus::ErrorLog(void) const { return itsErrorLog; }
// ----------------------------------------------------------------------
/*!
 * \return Undocumented
 */
// ----------------------------------------------------------------------

const NFmiString NFmiStatus::WarningLog(void) const { return itsWarningLog; }
// ----------------------------------------------------------------------
/*!
 * \return Undocumented
 */
// ----------------------------------------------------------------------

const NFmiString NFmiStatus::MessageLog(void) const { return itsMessageLog; }
// ----------------------------------------------------------------------
/*!
 * Assignment operator
 *
 * \param theStatus The other object being copied
 * \return The object assigned to
 */
// ----------------------------------------------------------------------

NFmiStatus& NFmiStatus::operator=(const NFmiStatus& theStatus)
{
  itsMessageLog = theStatus.itsMessageLog;
  itsWarningLog = theStatus.itsWarningLog;
  itsErrorLog = theStatus.itsErrorLog;

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

std::ostream& NFmiStatus::Write(std::ostream& file) const
{
  file << itsErrorLog;
  file << itsWarningLog;
  file << itsMessageLog;
  return file;
}

// ----------------------------------------------------------------------
/*!
 * Read new object contents from the given stream
 *
 * \param file The input stream to read from
 * \return The input stream read from
 */
// ----------------------------------------------------------------------

std::istream& NFmiStatus::Read(std::istream& file)
{
  file >> itsErrorLog;
  file >> itsWarningLog;
  file >> itsMessageLog;
  return file;
}

// ======================================================================
