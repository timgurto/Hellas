#include <sstream>
#include <string>

#include "JsonWriter.h"

JsonWriter::JsonWriter(const std::string &collectionName){
    std::string filename = "../web/" + collectionName + ".js";
    outputFile.open(filename.c_str());
    outputFile << collectionName << " = [" << std::endl;
    startEntry();
}

JsonWriter::~JsonWriter(){
    finishEntry();
    outputFile << "];" << std::endl;
}

void JsonWriter::nextEntry(){
    if (currentEntryIsEmpty)
        return;
    finishEntry();
    startEntry();
}

void JsonWriter::startEntry(){
    outputFile << "{" << std::endl;
    currentEntryIsEmpty = true;
}

void JsonWriter::finishEntry(){
    outputFile << "}," << std::endl;
}

void JsonWriter::addArrayAttribute(const std::string &attributeName,
                                   const std::set<std::string> &container){
    if (container.empty())
        return;
    std::ostringstream completeValueString;
    completeValueString << "[";
    for (const auto &value : container)
        completeValueString << formatValue(value) << ",";
    completeValueString << "]";
    addAttributeWithoutFormattingValue(attributeName, completeValueString.str());
}

void JsonWriter::addAttribute(const std::string &attributeName, const std::string &value){
    currentEntryIsEmpty = false;
    addAttributeWithoutFormattingValue(attributeName, formatValue(value));
}

void JsonWriter::addAttributeWithoutFormattingValue(const std::string &attributeName,
                                                    const std::string &formattedValue){
    outputFile << "  " << attributeName << ": " << formattedValue << "," << std::endl;
}

std::string JsonWriter::formatValue(const std::string &value){
    return '\"' + value + '\"';
}
