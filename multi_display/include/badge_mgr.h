#pragma once
#include <Arduino.h>
#include "display_mgr.h"

class BadgeManager {
public:
    static void init();
    static void load();
    static void save();

    static BadgeConfig& getConfig();
    static void updateConfig(const BadgeConfig& cfg);
    static void cycleSubMode();

private:
    static BadgeConfig currentConfig;
};
