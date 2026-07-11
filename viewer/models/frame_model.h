#ifndef FRAME_MODEL_H
#define FRAME_MODEL_H

#include <QObject>

class FrameModel : public QObject
{
    Q_OBJECT

    // Expose frameId to QML as a read-only property.
    // QML reads it through frameId() and watches frameIdChanged().
    Q_PROPERTY(
        int frameId
        READ frameId
        NOTIFY frameIdChanged
    )
    Q_PROPERTY(
        int imageRevision
        READ imageRevision
        NOTIFY imageRevisionChanged
    )
    Q_PROPERTY(
        ScaleMode scaleMode
        READ scaleMode
        WRITE setScaleMode
        NOTIFY scaleModeChanged
    )

public:
    // The enum belongs to FrameModel and is exposed to Qt/QML.
        enum class ScaleMode
        {
            Raw,
            Auto
        };
// Register it for qt
        Q_ENUM(ScaleMode)


    explicit FrameModel(QObject *parent = nullptr);

    // Getter used by Qt/QML to read the current frame ID.
    int frameId() const;
    // Update the current frame ID.
    void setFrameId(int frameId);

    // value used by QML to produce a new image URL whenever
    // the thermal image changes.
    int imageRevision() const;
    // Announce that the image provider now contains a new frame.
    void notifyImageUpdated();

    ScaleMode scaleMode() const;
    void setScaleMode(ScaleMode mode);

signals:
    // Emitted whenever frameId changes.
    // QML listens to this signal and updates dependent bindings.
    void frameIdChanged();
    void imageRevisionChanged();
    void scaleModeChanged();

private:
    int m_frameId = 0;
    int m_imageRevision = 0;

    ScaleMode m_scaleMode = ScaleMode::Raw;
};

#endif
