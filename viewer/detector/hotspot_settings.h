#ifndef HOTSPOT_SETTINGS_H
#define HOTSPOT_SETTINGS_H

#include "protocol/thermal_quantization.h"

struct HotspotSettings
{

    // actual temperature
    double temperatureToleranceCelsius = 1;
    // encoded temprature
    //
    int temperatureToleranceEncoded() const
    {
        return ThermalQuantization::encodeTemperatureDifference(
            temperatureToleranceCelsius
        );
    }


    // penalty for cold / missing pixels
    double coldPixelPenalty = 1.0;

    // maximum candidate circle radius measured in sensor pixels.
    // for 12 = 12^2*3 = 432 ~ 56% of the whole 768 bpixels
    int maximumRadiusPixels = 12;
};

#endif
