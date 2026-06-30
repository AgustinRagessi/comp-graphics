#pragma once
#include "hud_data.h"
#include "hud_manager.h"

class AirspeedIndicator {
public:
    void render(const hud::FlightData& data, HudManager& renderer, int width, int height);
};