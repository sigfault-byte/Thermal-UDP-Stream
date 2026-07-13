#ifndef HOTSPOT_DETECTOR_H
#define HOTSPOT_DETECTOR_H

#include "hotspot.h"
#include "hotspot_settings.h"

#include <QByteArray>

class HotspotDetector
{
public:
    static Hotspot detect(
        const QByteArray &pixels,
        const HotspotSettings &settings
    );
};


#endif
