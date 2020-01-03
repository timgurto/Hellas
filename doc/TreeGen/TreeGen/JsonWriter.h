#ifndef JSON_WRITER_H
#define JSON_WRITER_H

#include <fstream>
#include <set>

class JsonWriter {
 public:
  JsonWriter(std::string collectionName);
  ~JsonWriter();
  void nextEntry();
  void addAttribute(const std::string &attributeName, const std::string &value,
                    bool alreadyFormatted = false);
  void addArrayAttribute(const std::string &attributeName,
                         const std::set<std::string> &container,
                         bool alreadyFormatted = false);

 private:
  std::ofstream outputFile;
  bool currentEntryIsEmpty;

  void startEntry();
  void finishEntry();
  std::string formatValue(const std::string &value);
  void addAttributeWithoutFormattingValue(const std::string &attributeName,
                                          const std::string &formattedValue);
};

#endif
