#ifndef HOTSPOT_H
#define HOTSPOT_H

// Dumb value type containing the result of hotspot detection.
struct Hotspot
{
    bool valid = false;
    bool aboveRange = false;
    int x = 0;
    int y = 0;
    double temperatureCelsius = 0.0;
};

#endif
