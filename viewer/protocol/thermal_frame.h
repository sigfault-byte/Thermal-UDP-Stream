#ifndef THERMAL_FRAME_H
#define THERMAL_FRAME_H

#include <QByteArray>
#include <QtGlobal>

// This is purely a data struct for the raw dataframe
// no object needed, pure data storage
// once the magic number is verified, the important part will be stored here

struct ThermalFrame
{
    // QT internal 4 byte int
    quint32 frameId = 0;
    quint32 timestampMs = 0;
    quint8 quantizationMode = 1;
    QByteArray pixels;
};

#endif
