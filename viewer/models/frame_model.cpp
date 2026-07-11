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

int FrameModel::imageRevision() const
{
    // not a frame or timestamp -> this is for QML to use a different image url
    return m_imageRevision;
}

void FrameModel::notifyImageUpdated()
{
    ++m_imageRevision;
    emit imageRevisionChanged();
}
