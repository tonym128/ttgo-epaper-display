#pragma once
#include <Arduino.h>

struct StoicQuote {
    const char* quote;
    const char* author;
    const char* source;
};

struct WordOfTheDay {
    const char* word;
    const char* pronunciation;
    const char* partOfSpeech;
    const char* definition;
};

struct HistoryEvent {
    const char* year;
    const char* title;
    const char* description;
};

class CalendarData {
public:
    static const StoicQuote& getStoicQuote(size_t index);
    static const WordOfTheDay& getWord(size_t index);
    static const HistoryEvent& getHistory(size_t index);

    static size_t getStoicCount();
    static size_t getWordCount();
    static size_t getHistoryCount();
};
