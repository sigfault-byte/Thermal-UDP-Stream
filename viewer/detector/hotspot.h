#ifndef HOTSPOT_H
#define HOTSPOT_H

// Dumb value type containing the result of hotspot detection.
// struct Hotspot
// {
//     bool valid = false;
//     bool aboveRange = false;
//     int x = 0;
//     int y = 0;
//     double temperatureCelsius = 0.0;
// };
//
struct Hotspot
{
    bool valid = false;
    bool peakAboveRange = false;

    //peak pixel coord
    int peakX = 0;
    int peakY = 0;

    // temp recorded at the peak pixel
    double peakTemperatureCelsius = 0.0;

    // center of the circle
    double centerX = 0.0;
    double centerY = 0.0;
    // its radius
    double radiusPixels = 0.0;

    // Number of hot pixels captured by the candidate circle.
    int hotPixelCount = 0;
    // Total number of frame pixels contained in the candidate circle.
    int totalPixelCount = 0;

    // score assigned to the selected circle by the detector
    double score = 0.0;
};

#endif
