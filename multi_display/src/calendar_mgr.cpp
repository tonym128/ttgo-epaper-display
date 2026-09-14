#include "calendar_mgr.h"
#include <Preferences.h>

int CalendarManager::currentCategory = 0; // 0=Stoic, 1=Word, 2=History
int CalendarManager::currentIndex = 0;

static Preferences prefs;

struct StoicEntry {
    const char* author;
    const char* quote;
    const char* theme;
};

static const StoicEntry STOIC_QUOTES[] = {
    {
        "Marcus Aurelius",
        "You have power over your mind - not outside events. Realize this, and you will find strength.",
        "Inner Citadel"
    },
    {
        "Seneca",
        "We suffer more often in imagination than in reality. True tranquility lies in the present.",
        "On Peace of Mind"
    },
    {
        "Epictetus",
        "Wealth consists not in having great possessions, but in having few wants.",
        "Freedom & Contentment"
    },
    {
        "Marcus Aurelius",
        "Waste no more time arguing about what a good man should be. Be one.",
        "Virtue in Action"
    },
    {
        "Seneca",
        "Luck is what happens when preparation meets opportunity.",
        "Preparation"
    },
    {
        "Epictetus",
        "It's not what happens to you, but how you react to it that matters.",
        "Perspective"
    },
    {
        "Marcus Aurelius",
        "The best revenge is not to be like that which caused the injury.",
        "Equanimity"
    }
};

struct VocabEntry {
    const char* word;
    const char* partOfSpeech;
    const char* definition;
    const char* example;
};

static const VocabEntry VOCAB_WORDS[] = {
    {
        "Ataraxia",
        "noun • [at-uh-rak-see-uh]",
        "A state of serene calmness, lucid tranquility, and unwavering peace of mind.",
        "\"Cultivating ataraxia amid chaos was his daily practice.\""
    },
    {
        "Amor Fati",
        "noun • [ah-mor fah-tee]",
        "A love of fate; accepting all that happens in life as necessary and good.",
        "\"He welcomed every challenge with unwavering amor fati.\""
    },
    {
        "Eudaimonia",
        "noun • [yoo-dye-moh-nee-uh]",
        "A state of living well, human flourishing, and ethical prosperity.",
        "\"Virtuous living is the direct path toward eudaimonia.\""
    },
    {
        "Equanimity",
        "noun • [ee-kwuh-nim-i-tee]",
        "Mental calmness, composure, and evenness of temper, especially in a difficult situation.",
        "\"She faced the stormy crisis with calm equanimity.\""
    }
};

struct HistoryEntry {
    const char* year;
    const char* eventTitle;
    const char* description;
};

static const HistoryEntry HISTORY_EVENTS[] = {
    {
        "1977",
        "Voyager 1 Launched",
        "NASA launches Voyager 1 on its historic grand tour of the solar system and beyond into interstellar space."
    },
    {
        "1969",
        "Apollo 11 Moon Landing",
        "Neil Armstrong steps onto the lunar surface: 'That's one small step for man, one giant leap for mankind.'"
    },
    {
        "1879",
        "Incandescent Light Bulb",
        "Thomas Edison successfully tests the electric incandescent light bulb, transforming modern civilization."
    },
    {
        "1928",
        "Discovery of Penicillin",
        "Alexander Fleming discovers penicillin, opening the age of life-saving antibiotics."
    }
};

void CalendarManager::init() {
    prefs.begin("calendar", false);
    currentCategory = prefs.getInt("cat", 0);
    currentIndex = prefs.getInt("idx", 0);
    prefs.end();
}

void CalendarManager::getCurrentItem(CalendarItem& item) {
    memset(&item, 0, sizeof(item));

    // Simulated day info based on index
    int dayNum = (currentIndex % 30) + 1;
    item.dayNumber = dayNum;

    const char* days[] = {"Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"};
    strncpy(item.dayName, days[currentIndex % 7], sizeof(item.dayName) - 1);
    snprintf(item.dateStr, sizeof(item.dateStr), "Day %d", dayNum);

    if (currentCategory == 0) {
        // Stoic
        int num = sizeof(STOIC_QUOTES) / sizeof(STOIC_QUOTES[0]);
        const StoicEntry& e = STOIC_QUOTES[currentIndex % num];
        strncpy(item.title, "DAILY STOIC", sizeof(item.title) - 1);
        strncpy(item.subtitle, e.author, sizeof(item.subtitle) - 1);
        strncpy(item.body, e.quote, sizeof(item.body) - 1);
        snprintf(item.footer, sizeof(item.footer), "Theme: %s", e.theme);
    } else if (currentCategory == 1) {
        // Vocab
        int num = sizeof(VOCAB_WORDS) / sizeof(VOCAB_WORDS[0]);
        const VocabEntry& v = VOCAB_WORDS[currentIndex % num];
        strncpy(item.title, "WORD OF DAY", sizeof(item.title) - 1);
        strncpy(item.subtitle, v.word, sizeof(item.subtitle) - 1);
        char bodyBuf[160];
        snprintf(bodyBuf, sizeof(bodyBuf), "%s\n%s\n%s", v.partOfSpeech, v.definition, v.example);
        strncpy(item.body, bodyBuf, sizeof(item.body) - 1);
        strncpy(item.footer, "Expand Your Lexicon", sizeof(item.footer) - 1);
    } else {
        // History
        int num = sizeof(HISTORY_EVENTS) / sizeof(HISTORY_EVENTS[0]);
        const HistoryEntry& h = HISTORY_EVENTS[currentIndex % num];
        strncpy(item.title, "THIS DAY IN HISTORY", sizeof(item.title) - 1);
        char subBuf[48];
        snprintf(subBuf, sizeof(subBuf), "%s - %s", h.year, h.eventTitle);
        strncpy(item.subtitle, subBuf, sizeof(item.subtitle) - 1);
        strncpy(item.body, h.description, sizeof(item.body) - 1);
        strncpy(item.footer, "Historic Milestones", sizeof(item.footer) - 1);
    }
}

void CalendarManager::nextItem() {
    currentIndex++;
    prefs.begin("calendar", false);
    prefs.putInt("idx", currentIndex);
    prefs.end();
}

void CalendarManager::prevItem() {
    if (currentIndex > 0) currentIndex--;
    prefs.begin("calendar", false);
    prefs.putInt("idx", currentIndex);
    prefs.end();
}

int CalendarManager::getCategory() { return currentCategory; }

void CalendarManager::setCategory(int cat) {
    currentCategory = cat % 3;
    prefs.begin("calendar", false);
    prefs.putInt("cat", currentCategory);
    prefs.end();
}
