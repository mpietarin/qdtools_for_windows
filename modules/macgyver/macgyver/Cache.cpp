#include "Cache.h"

namespace Fmi
{
namespace Cache
{
bool parse_size_t(const std::string& input, std::size_t& result)
{
  typedef boost::spirit::qi::uint_parser<std::size_t, 16, 1, -1> SizeTParser;
  auto begin = input.begin();
  auto end = input.end();

  SizeTParser parser;
  std::size_t res;

  bool ret = boost::spirit::qi::parse(begin, end, parser, res);

  if (ret && (begin == end))
  {
    result = res;
    return true;
  }
  else
  {
    return false;
  }
}

FileCache::FileCache(const fs::path& directory, std::size_t maxSize)
    : itsSize(0), itsMaxSize(maxSize), itsDirectory(directory)
{
  // Check directory validity

  if (!fs::exists(itsDirectory))
  {
    fs::create_directory(itsDirectory);
  }
  else
  {
    if (!fs::is_directory(itsDirectory))
    {
      throw std::runtime_error("File Cache Directory '" + itsDirectory.string() +
                               "' is not a directory");
    }

    // Write a test file to see that we have write permissions
    std::string test("test");
    bool res = writeFile(itsDirectory, "testfile", test);
    if (!res)
    {
      throw std::runtime_error("Unable to write to directory '" + itsDirectory.string() +
                               "', check permissions");
    }

    fs::remove(itsDirectory / "testfile");
  }

  // Check that maximum size is feasible
  auto filesystemInfo = fs::space(itsDirectory);
  if (itsMaxSize > filesystemInfo.capacity)
  {
    std::ostringstream os;
    os << "FileCache maximum size '" << itsMaxSize << "' exceeds filesystem capacity";
    throw std::runtime_error(os.str());
  }

  // Load any available contents
  update();
}

boost::optional<std::string> FileCache::find(std::size_t key)
{
  UpgradeReadLock theLock(itsMutex);
  boost::system::error_code err;

  auto it = itsContentMap.left.find(key);

  if (it == itsContentMap.left.end())
  {
    return boost::optional<std::string>();
  }

  // Found in the map, but somebody may have cleaned the file

  fs::path fullPath = it->second.path;

  if (!fs::exists(fullPath, err))
  {
    // Should we remove the entry here? It will be re-inserted anyways eventually
    return boost::optional<std::string>();
  }

  std::size_t size = it->second.fileSize;

  // Read and return the file

  fs::fstream file(fullPath, fs::fstream::binary | fs::fstream::in);
  if (!file)
  {
    // Report file opening error
    return boost::optional<std::string>();
  }

  std::string ret;
  ret.resize(size);
  file.read(&ret[0], size);  // Should work, c++11 guarantees strings to be contiguous

  // This implements LRU eviction behaviour
  UpgradeWriteLock ugLock(theLock);
  itsContentMap.right.relocate(itsContentMap.right.end(), itsContentMap.project_right(it));

  return boost::optional<std::string>(std::move(ret));
}

bool FileCache::insert(std::size_t key, const std::string& value, bool performCleanup)
{
  WriteLock theLock(itsMutex);
  boost::system::error_code err;

  auto it = itsContentMap.left.find(key);
  if (it != itsContentMap.left.end())
  {
    if (fs::exists(it->second.path, err))
    {
      return true;  // Already in the cache and disk
    }
    else
    {
      // Found in map, but not on the disk. Someone has cleaned it without our knowledge
      // Remove from the map and proceed with insert
      itsSize -= it->second.fileSize;
      itsContentMap.left.erase(it);
    }
  }

  std::pair<std::string, std::string> subDirAndFilename = getFileDirAndName(key);

  std::string subDir = subDirAndFilename.first;
  std::string fileName = subDirAndFilename.second;

  fs::path cacheDir = itsDirectory / subDir;
  fs::path fullPath = cacheDir / fileName;

  bool isValid = checkForDiskSpace(cacheDir, value, performCleanup);

  if (!isValid)
  {
#ifdef MYDEBUG
    std::cout << "Insert: No disk space for key: " << key << std::endl;
#endif
    return false;  // Not possible to cache this value
  }

  isValid = writeFile(cacheDir, fileName, value);

  if (!isValid)
  {
#ifdef MYDEBUG
    std::cout << "Insert: Unable to write key: " << key << std::endl;
#endif
    return false;  // Something went wrong with file write
  }

  std::size_t fileSize = value.size();

  itsContentMap.insert(MapType::value_type(key, FileCacheStruct(fullPath, fileSize)));

  // Successfull insert. Update cache size information
  itsSize += fileSize;

  return true;
}

std::vector<std::size_t> FileCache::getContent()
{
  ReadLock lock(itsMutex);
  std::vector<std::size_t> result;
  result.reserve(itsContentMap.size());
  for (auto it = itsContentMap.right.begin(); it != itsContentMap.right.end(); ++it)
  {
    result.push_back(it->second);
  }
  return result;
}

std::size_t FileCache::getSize()
{
  WriteLock theLock(itsMutex);
  return itsSize;
}

bool FileCache::clean(std::size_t spaceNeeded)
{
  WriteLock theLock(itsMutex);
  return performCleanup(spaceNeeded);
}

bool FileCache::performCleanup(std::size_t space_needed)
{
  boost::system::error_code err;
  while ((itsMaxSize - itsSize) < space_needed)
  {
    if (itsContentMap.empty())
    {
      // Can't clean an empty map or we have deleted everything
      return false;
    }

    fs::path& thisPath = itsContentMap.right.front().first.path;
    std::size_t fileSize = itsContentMap.right.front().first.fileSize;
    fs::remove(thisPath, err);
    if (err)
    {
      // Do something when error. Don't know what though.
      // Also, the file can be already deleted.
    }
    itsContentMap.right.pop_front();
    itsSize -= fileSize;
  }

  return true;
}

void FileCache::update()
{
  fs::recursive_directory_iterator iter(itsDirectory);
  fs::recursive_directory_iterator end;
  boost::system::error_code err;

  std::string current_subdir;

  for (; iter != end; iter++)
  {
    const auto& entry = *iter;

    const auto& status = entry.status(err);

    bool isFile = fs::is_regular_file(status);
    if (isFile)
    {
      const auto& path = entry.path();

      if (path.parent_path() == itsDirectory)
      {
        // Top level file without corresponding subdir. These are not created by us and should be
        // ignored
        continue;
      }

      std::string subDir = path.parent_path().stem().string();

      std::string fileName = path.filename().string();  // Key in the map is filename itself

      std::size_t key;

      bool valid = getKey(subDir, fileName, key);

      if (valid)
      {
        std::size_t fileSize = fs::file_size(path, err);
        if (!err)
        {
          std::size_t expected_size = itsSize + fileSize;
          if (expected_size < itsMaxSize)
          {
            auto res =
                itsContentMap.insert(MapType::value_type(key, FileCacheStruct(path, fileSize)));
            if (res.second) itsSize += fileSize;  // Added new entry, update size information
          }
        }
      }
    }
  }
}

bool FileCache::writeFile(const fs::path& theDir,
                          const std::string& fileName,
                          const std::string& theValue)
{
  // Create subdir if needed

  boost::system::error_code err;

  bool exists = fs::exists(theDir);  // fs::exists with error code argument throws error if
                                     // directory does not exist, instead of returning false

  if (!exists)
  {
    fs::create_directory(theDir);
  }
  else
  {
    bool is_dir = fs::is_directory(theDir, err);
    if (err)
    {
// Filesystem error
#ifdef MYDEBUG
      std::cout << "Wriltefile: Filesystem error: 'is_directory'. Path: " << (theDir).string()
                << std::endl;
      std::cout << err.message() << std::endl;
#endif
      return false;
    }
    if (!is_dir)
    {
// Supposed subdir is not directory, somebody has borked our directory structure
#ifdef MYDEBUG
      std::cout << "Writefile: Subdir not a directory. Path: " << (theDir).string() << std::endl;
#endif
      return false;
    }
  }

  fs::path fullPath = theDir / fileName;
  fs::fstream file(fullPath, fs::fstream::binary | fs::fstream::out);
  if (!file)
  {
// Could not open file
#ifdef MYDEBUG
    std::cout << "WriteFile: Unable to open file. Path: " << (theDir / fileName).string()
              << std::endl;
#endif
    return false;
  }

  std::size_t valueSize = theValue.size();
  const char* theBuffer = theValue.data();

  file.write(theBuffer, valueSize);

  if (!file)
  {
// Write failed
#ifdef MYDEBUG
    std::cout << "WriteFile: Write failed. Path: " << (theDir / fileName).string() << std::endl;
#endif
    return false;
  }

  return true;
}

bool FileCache::checkForDiskSpace(const fs::path& thePath,
                                  const std::string& theValue,
                                  bool doCleanup)

{
  // Get serialization information of the value to be inserted

  std::size_t valueSize = theValue.size();

  // Sanity check for very large inputs

  if (itsMaxSize < valueSize)
  {
    // No enough space
    return false;
  }

  // Check filesystem space
  auto spaceInfo = fs::space(itsDirectory);
  if (spaceInfo.free < valueSize)
  {
    return false;
  }

  // Make sufficient space available

  std::size_t free_space = itsMaxSize - itsSize;

  if (free_space < valueSize)
  {
    // Not enough space to insert, cleanup first
    if (doCleanup)
    {
      bool success = this->performCleanup(valueSize);
      if (!success) return false;  // Something failed during cleanup
    }
    else
    {
      // No cleanup requested, simply return failure
      return false;
    }
  }

  return true;
}

std::pair<std::string, std::string> FileCache::getFileDirAndName(std::size_t hashValue)
{
  std::ostringstream out;
  std::string subDirectory, fileName;

  out << std::hex;

  out << (hashValue & 0xff);

  subDirectory = out.str();

  out.str("");

  out << (hashValue >> 0x8);

  fileName = out.str();

  return std::make_pair(subDirectory, fileName);
}

bool FileCache::getKey(const std::string& directory, const std::string& filename, std::size_t& key)
{
  std::size_t first;
  std::size_t second;
  bool res;

  res = parse_size_t(directory, first);
  if (!res) return false;

  res = parse_size_t(filename, second);
  if (!res) return false;

  key = (second << 0x8) | first;

  return true;
}
}
}
