#ifndef JSON_WRITER_H
#define JSON_WRITER_H

#include <fstream>

class JsonWriter{
public:
    JsonWriter(const std::string &collectionName);
    ~JsonWriter();
    void nextEntry();
    void addAttribute(const std::string &attributeName, const std::string &value);
    void addAttribute(const std::string &attributeName, double value);

private:
    std::ofstream outputFile;
    bool currentEntryIsEmpty;

    void startEntry();
    void finishEntry();
    void addAttributeWithFormattedValue(const std::string &attributeName,
                                        const std::string &formattedValue);
};

#endif
