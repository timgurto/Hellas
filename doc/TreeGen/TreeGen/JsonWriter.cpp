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

void JsonWriter::addAttribute(const std::string &attributeName, const std::string &value){
    std::string formattedValue = '\"' + value + '\"';
    addAttributeWithFormattedValue(attributeName, formattedValue);
}

void JsonWriter::addAttribute(const std::string &attributeName, double value){
    std::ostringstream formattedValue;
    formattedValue << value;
    addAttributeWithFormattedValue(attributeName, formattedValue.str());
}

void JsonWriter::addAttributeWithFormattedValue(const std::string &attributeName,
                                                const std::string &formattedValue){
    currentEntryIsEmpty = false;
    outputFile << "  " << attributeName << ": " << formattedValue << "," << std::endl;
}
