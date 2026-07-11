#include "frame_model.h"

FrameModel::FrameModel(QObject *parent)
    : QObject(parent)
{
}

int FrameModel::frameId() const
{
    return m_frameId;
}

void FrameModel::setFrameId(int frameId)
{
    // Do nothing if the value did not actually change.
    if (m_frameId == frameId)
        return;

    // Store the new frame ID.
    m_frameId = frameId;

    // Tell Qt that the frameId property changed.
    emit frameIdChanged();
}
