#pragma once
#include <Arduino.h>
#include "config.h"

struct NewsArticle {
    char title[128];
    char url[128];
    char meta[48];        // e.g. "285 pts • 140 cmts" or source domain
    char sourceName[24];  // e.g. "HACKER NEWS", "r/technology", "BBC NEWS"
    int index;
    int total;
};

class NewsManager {
public:
    static void init();
    static bool fetchArticles();
    static bool getCurrentArticle(NewsArticle& outArticle);
    static void nextArticle();
    static void prevArticle();

    static NewsSource getSource();
    static void setSource(NewsSource src);

    static String getRssUrl();
    static void setRssUrl(const String& url);

    static String getSubreddit();
    static void setSubreddit(const String& sub);

    static int getArticleCount();
    static int getCurrentIndex();

    static void saveSettings(NewsSource src, const String& sub, const String& rss);

private:
    static bool fetchHackerNews();
    static bool fetchReddit();
    static bool fetchRssFeed();

    static NewsSource currentSource;
    static String customRssUrl;
    static String subredditName;

    static NewsArticle articles[MAX_ARTICLES];
    static int articleCount;
    static int currentIndex;
};
