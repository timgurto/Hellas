#include "JsonWriter.h"

#include <sstream>
#include <string>

JsonWriter::JsonWriter(std::string collectionName) {
  std::string filename = "../web/" + collectionName + ".js";
  outputFile.open(filename.c_str());
  outputFile << collectionName << " = [" << std::endl;
  startEntry();
}

JsonWriter::~JsonWriter() {
  finishEntry();
  outputFile << "];" << std::endl;
}

void JsonWriter::nextEntry() {
  if (currentEntryIsEmpty) return;
  finishEntry();
  startEntry();
}

void JsonWriter::startEntry() {
  outputFile << "{" << std::endl;
  currentEntryIsEmpty = true;
}

void JsonWriter::finishEntry() { outputFile << "}," << std::endl; }

void JsonWriter::addArrayAttribute(const std::string &attributeName,
                                   const std::set<std::string> &container,
                                   bool alreadyFormatted) {
  if (container.empty()) return;
  std::ostringstream completeValueString;
  completeValueString << "[";
  for (const auto &value : container) {
    std::string formattedValue = alreadyFormatted ? value : formatValue(value);
    completeValueString << formattedValue << ",";
  }
  completeValueString << "]";
  addAttributeWithoutFormattingValue(attributeName, completeValueString.str());
}

void JsonWriter::addAttribute(const std::string &attributeName,
                              const std::string &value, bool alreadyFormatted) {
  currentEntryIsEmpty = false;
  std::string formattedValue = alreadyFormatted ? value : formatValue(value);
  addAttributeWithoutFormattingValue(attributeName, formattedValue);
}

void JsonWriter::addAttributeWithoutFormattingValue(
    const std::string &attributeName, const std::string &formattedValue) {
  outputFile << "  " << attributeName << ": " << formattedValue << ","
             << std::endl;
}

std::string JsonWriter::formatValue(const std::string &value) {
  return '\"' + value + '\"';
}
