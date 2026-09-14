#pragma once
#include <Arduino.h>
#include "display_mgr.h"

class CalendarManager {
public:
    static void init();
    static void getCurrentItem(CalendarItem& item);
    static void nextItem();
    static void prevItem();
    
    static int getCategory(); // 0=Stoic, 1=Word of the Day, 2=On This Day
    static void setCategory(int cat);

private:
    static int currentCategory;
    static int currentIndex;
};
