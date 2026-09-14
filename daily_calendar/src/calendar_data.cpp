#include "calendar_data.h"

// ==========================================
// Curated Stoic Quotes
// ==========================================
static const StoicQuote STOIC_QUOTES[] = {
    {
        "You have power over your mind - not outside events. Realize this, and you will find strength.",
        "Marcus Aurelius",
        "Meditations, XII.3"
    },
    {
        "We suffer more often in imagination than in reality.",
        "Seneca",
        "Letters from a Stoic, XIII"
    },
    {
        "Don't explain your philosophy. Embody it.",
        "Epictetus",
        "Enchiridion, XLVI"
    },
    {
        "Waste no more time arguing what a good man should be. Be one.",
        "Marcus Aurelius",
        "Meditations, X.16"
    },
    {
        "He who fears death will never do anything worthy of a man who is alive.",
        "Seneca",
        "On Peace of Mind"
    },
    {
        "Wealth consists not in having great possessions, but in having few wants.",
        "Epictetus",
        "Fragments"
    },
    {
        "The best revenge is not to be like that.",
        "Marcus Aurelius",
        "Meditations, VI.6"
    },
    {
        "Luck is what happens when preparation meets opportunity.",
        "Seneca",
        "Moral Essays"
    },
    {
        "No man is free who is not master of himself.",
        "Epictetus",
        "Discourses"
    },
    {
        "The impediment to action advances action. What stands in the way becomes the way.",
        "Marcus Aurelius",
        "Meditations, V.20"
    },
    {
        "Difficulties strengthen the mind, as labor does the body.",
        "Seneca",
        "Letters from a Stoic"
    },
    {
        "First say to yourself what you would be; and then do what you have to do.",
        "Epictetus",
        "Discourses, III.23"
    },
    {
        "Dwell on the beauty of life. Watch the stars, and see yourself running with them.",
        "Marcus Aurelius",
        "Meditations, VII.47"
    },
    {
        "Begin at once to live, and count each separate day as a separate life.",
        "Seneca",
        "Letters, CI"
    },
    {
        "If anyone can refute me, show me I'm in error, I will gladly change. I seek the truth.",
        "Marcus Aurelius",
        "Meditations, VI.21"
    },
    {
        "Small-minded people blame others. Average people blame themselves. The wise see all blame as folly.",
        "Epictetus",
        "Enchiridion, V"
    },
    {
        "Life is very short and anxious for those who forget the past, neglect the present, and fear the future.",
        "Seneca",
        "On the Shortness of Life"
    },
    {
        "When you arise in the morning think of what a privilege it is to be alive, to breathe, to enjoy.",
        "Marcus Aurelius",
        "Meditations"
    },
    {
        "Curb your desire - don't set your heart on so many things and you will get what you need.",
        "Epictetus",
        "Discourses, I.4"
    },
    {
        "A gem cannot be polished without friction, nor a man perfected without trials.",
        "Seneca",
        "Moral Letters"
    },
    {
        "Never let the future disturb you. You will meet it, if you have to, with the same weapons of reason.",
        "Marcus Aurelius",
        "Meditations, VII.8"
    },
    {
        "It is not that we have so little time, but that we lose so much.",
        "Seneca",
        "On the Shortness of Life"
    },
    {
        "Man is not worried by real problems so much as by his imagined anxieties about real problems.",
        "Epictetus",
        "Enchiridion"
    },
    {
        "Accept the things to which fate binds you, and love the people with whom fate brings you together.",
        "Marcus Aurelius",
        "Meditations, VI.39"
    },
    {
        "If you want to improve, be content to be thought foolish and stupid.",
        "Epictetus",
        "Enchiridion, XIII"
    },
    {
        "Associate with people who are likely to improve you.",
        "Seneca",
        "Letters, VII"
    },
    {
        "The soul becomes dyed with the color of its thoughts.",
        "Marcus Aurelius",
        "Meditations, V.16"
    },
    {
        "Circumstances don't make the man, they only reveal him to himself.",
        "Epictetus",
        "Discourses, I.24"
    },
    {
        "To be evenminded is the greatest virtue. Wisdom is to speak the truth and act in keeping with nature.",
        "Heraclitus",
        "Fragments"
    },
    {
        "Very little is needed to make a happy life; it is all within yourself, in your way of thinking.",
        "Marcus Aurelius",
        "Meditations, VII.67"
    }
};

// ==========================================
// Curated Word of the Day
// ==========================================
static const WordOfTheDay WORDS_OF_THE_DAY[] = {
    {
        "Ataraxia",
        "[at-uh-RAK-see-uh]",
        "noun",
        "A state of serene calmness, equanimity, and untroubled peace of mind."
    },
    {
        "Eudaimonia",
        "[yoo-dye-MOH-nee-uh]",
        "noun",
        "Flourishing or living well; the highest human good achieved through virtue."
    },
    {
        "Petrichor",
        "[PET-ri-kor]",
        "noun",
        "The distinctive, pleasant earthy scent produced when rain falls on dry soil."
    },
    {
        "Apotheosis",
        "[uh-poth-ee-OH-sis]",
        "noun",
        "The highest point in the development of something; a culmination or elevation."
    },
    {
        "Sonder",
        "[SAHN-der]",
        "noun",
        "The profound realization that each random passerby lives a life as vivid as your own."
    },
    {
        "Apricity",
        "[uh-PRIS-ih-tee]",
        "noun",
        "The warmth of the winter sun on your face."
    },
    {
        "Amor Fati",
        "[ah-mor FAH-tee]",
        "phrase",
        "Love of one's fate; seeing everything that happens, good and bad, as necessary and good."
    },
    {
        "Meraki",
        "[may-RAH-kee]",
        "adjective",
        "Doing something with total soul, creativity, and love; putting yourself into your work."
    },
    {
        "Serendipity",
        "[ser-un-DIP-ih-tee]",
        "noun",
        "The occurrence of finding valuable or agreeable things not sought for by chance."
    },
    {
        "Kenopsia",
        "[ken-AHP-see-uh]",
        "noun",
        "The eerie, poignant atmosphere of a place that is usually bustling, but is now abandoned."
    },
    {
        "Metanoia",
        "[met-uh-NOY-uh]",
        "noun",
        "A transformative change of heart; a fundamental conversion of one's mind and life."
    },
    {
        "Hiraeth",
        "[HEER-eyeth]",
        "noun",
        "A homesickness or deep yearning for a home or past time to which you cannot return."
    },
    {
        "Susurrus",
        "[soo-SUR-us]",
        "noun",
        "A whispering or rustling sound, like gentle wind through autumn leaves."
    },
    {
        "Equanimity",
        "[ek-wuh-NIM-ih-tee]",
        "noun",
        "Mental calmness, composure, and evenness of temper, especially in a difficult situation."
    },
    {
        "Defenestration",
        "[dee-fen-uh-STRAY-shun]",
        "noun",
        "The action of throwing someone or something out of a window."
    },
    {
        "Velleity",
        "[veh-LEE-ih-tee]",
        "noun",
        "A mere wish or inclination, not strong enough to lead to the slightest effort or action."
    },
    {
        "Komorebi",
        "[koh-moh-REH-bee]",
        "noun",
        "Sunlight filtering through the leaves of trees onto the ground."
    },
    {
        "Phronesis",
        "[froh-NEE-sis]",
        "noun",
        "Practical wisdom; the ability to make righteous and prudent ethical choices in daily life."
    },
    {
        "Numinous",
        "[NOO-min-us]",
        "adjective",
        "Having a strong religious or spiritual quality; indicating or suggesting the presence of a divinity."
    },
    {
        "Fernweh",
        "[FEIRN-vey]",
        "noun",
        "An intense longing to travel and explore far-off places; distant-sickness."
    }
};

// ==========================================
// Curated On This Day in History
// ==========================================
static const HistoryEvent HISTORY_EVENTS[] = {
    {
        "1969",
        "First Moonwalk",
        "Apollo 11 astronaut Neil Armstrong stepped onto the lunar surface, declaring: 'One small step for man, one giant leap for mankind.'"
    },
    {
        "1977",
        "Voyager 1 Launched",
        "NASA launched Voyager 1 on a mission to explore the outer Solar System. It is now the farthest human-made object from Earth."
    },
    {
        "1989",
        "Fall of the Berlin Wall",
        "East Germany announced that citizens were free to cross border checkpoints, marking the symbolic end of the Cold War."
    },
    {
        "1928",
        "Penicillin Discovered",
        "Alexander Fleming noticed mold preventing bacterial growth in his lab, ushering in the era of modern life-saving antibiotics."
    },
    {
        "1903",
        "First Powered Flight",
        "Orville and Wilbur Wright achieved the first controlled, sustained flight of a powered heavier-than-air aircraft at Kitty Hawk."
    },
    {
        "1990",
        "World Wide Web Proposed",
        "Tim Berners-Lee published the formal proposal for the World Wide Web, creating the foundation of the modern internet."
    },
    {
        "1879",
        "Incandescent Bulb",
        "Thomas Edison successfully tested the first practical, long-lasting electric incandescent light bulb at Menlo Park."
    },
    {
        "1947",
        "Sound Barrier Broken",
        "Chuck Yeager piloted the Bell X-1 experimental rocket aircraft past Mach 1.05, becoming the first human to fly supersonic."
    },
    {
        "1957",
        "Sputnik 1 Orbit",
        "The Soviet Union launched Sputnik 1 into low Earth orbit, inaugurating the Space Age and the Space Race."
    },
    {
        "1610",
        "Moons of Jupiter",
        "Galileo Galilei observed the four largest moons of Jupiter through his telescope, proving not everything orbits Earth."
    }
};

const StoicQuote& CalendarData::getStoicQuote(size_t index) {
    size_t count = sizeof(STOIC_QUOTES) / sizeof(STOIC_QUOTES[0]);
    return STOIC_QUOTES[index % count];
}

const WordOfTheDay& CalendarData::getWord(size_t index) {
    size_t count = sizeof(WORDS_OF_THE_DAY) / sizeof(WORDS_OF_THE_DAY[0]);
    return WORDS_OF_THE_DAY[index % count];
}

const HistoryEvent& CalendarData::getHistory(size_t index) {
    size_t count = sizeof(HISTORY_EVENTS) / sizeof(HISTORY_EVENTS[0]);
    return HISTORY_EVENTS[index % count];
}

size_t CalendarData::getStoicCount() {
    return sizeof(STOIC_QUOTES) / sizeof(STOIC_QUOTES[0]);
}

size_t CalendarData::getWordCount() {
    return sizeof(WORDS_OF_THE_DAY) / sizeof(WORDS_OF_THE_DAY[0]);
}

size_t CalendarData::getHistoryCount() {
    return sizeof(HISTORY_EVENTS) / sizeof(HISTORY_EVENTS[0]);
}
