#ifndef LOG_MODEL_H
#define LOG_MODEL_H

#include <QAbstractListModel>
#include <QString>
#include <QVector>

struct LogEntry
{
    QString timestampText;
    QString level;
    QString message;
    QString color;
};

class LogModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(
        int count
        READ count
        NOTIFY countChanged
    )

public:
    enum Role
    {
        TimestampTextRole = Qt::UserRole + 1,
        LevelRole,
        MessageRole,
        ColorRole
    };

    explicit LogModel(
        QObject *parent = nullptr
    );

    int rowCount(
        const QModelIndex &parent = QModelIndex()
    ) const override;

    QVariant data(
        const QModelIndex &index,
        int role
    ) const override;

    QHash<int, QByteArray> roleNames() const override;

    int count() const;

    // Append one displayable log line from C++ or a queued Qt message handler.
    Q_INVOKABLE void append(
        const QString &level,
        const QString &message
    );

signals:
    void countChanged();

private:
    QString colorForLevelAndMessage(
        const QString &level,
        const QString &message
    ) const;

    static constexpr int MaximumEntries = 100;

    QVector<LogEntry> m_entries;
};

#endif
