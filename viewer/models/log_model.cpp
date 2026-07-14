#include "log_model.h"

#include <QDateTime>

LogModel::LogModel(
    QObject *parent
)
    : QAbstractListModel(parent)
{
}

int LogModel::rowCount(
    const QModelIndex &parent
) const
{
    if (parent.isValid())
    {
        return 0;
    }

    return m_entries.size();
}

QVariant LogModel::data(
    const QModelIndex &index,
    int role
) const
{
    if (
        !index.isValid()
        || index.row() < 0
        || index.row() >= m_entries.size()
    )
    {
        return {};
    }

    const LogEntry &entry =
        m_entries[index.row()];

    switch (role)
    {
        case TimestampTextRole:
            return entry.timestampText;

        case LevelRole:
            return entry.level;

        case MessageRole:
            return entry.message;

        case ColorRole:
            return entry.color;

        default:
            return {};
    }
}

QHash<int, QByteArray> LogModel::roleNames() const
{
    return {
        {
            TimestampTextRole,
            "timestampText"
        },
        {
            LevelRole,
            "level"
        },
        {
            MessageRole,
            "message"
        },
        {
            ColorRole,
            "entryColor"
        }
    };
}

int LogModel::count() const
{
    return m_entries.size();
}

void LogModel::append(
    const QString &level,
    const QString &message
)
{
    if (m_entries.size() >= MaximumEntries)
    {
        beginRemoveRows(
            QModelIndex(),
            0,
            0
        );

        m_entries.removeFirst();

        endRemoveRows();
    }

    const int newRow =
        m_entries.size();

    LogEntry entry;
    entry.timestampText =
        QDateTime::currentDateTime().toString("HH:mm:ss");
    entry.level =
        level;
    entry.message =
        message;
    entry.color =
        colorForLevelAndMessage(
            level,
            message
        );

    beginInsertRows(
        QModelIndex(),
        newRow,
        newRow
    );

    m_entries.append(
        entry
    );

    endInsertRows();

    emit countChanged();
}

QString LogModel::colorForLevelAndMessage(
    const QString &level,
    const QString &message
) const
{
    if (level == "warning" || level == "critical" || level == "fatal")
    {
        return "#ff8a8a";
    }

    if (
        message.contains("command", Qt::CaseInsensitive)
        || message.contains("acknowledged", Qt::CaseInsensitive)
        || message.contains("accepted", Qt::CaseInsensitive)
        || message.contains("discovered", Qt::CaseInsensitive)
        || message.contains("prepared", Qt::CaseInsensitive)
    )
    {
        return "#8fd19e";
    }

    return "#d8d8df";
}
