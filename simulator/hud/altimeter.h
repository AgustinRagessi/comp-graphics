#pragma once
#include "hud_data.h"
#include "hud_manager.h"

class Altimeter {
public:
    void render(const hud::FlightData& data, HudManager& renderer, int width, int height);
};