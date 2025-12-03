#include "LogManager.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <algorithm>
#include <sstream>

namespace rove::data {

LogManager& LogManager::instance(DatabaseManager& database,
                                 UserManager& userManager,
                                 AchievementManager& achievementManager,
                                 TaskManager& taskManager) {
    static LogManager instance(database, userManager, achievementManager, taskManager);
    return instance;
}

LogManager::LogManager(DatabaseManager& database,
                       UserManager& userManager,
                       AchievementManager& achievementManager,
                       TaskManager& taskManager)
    : m_database(database),
      m_userManager(userManager),
      m_achievementManager(achievementManager),
      m_taskManager(taskManager),
      m_forgivenLogIds() {
    bindSystemEvents();
}

void LogManager::bindSystemEvents() {
    QObject::connect(m_taskManager.signalProxy(), &TaskManager::SignalProxy::taskCompleted, this,
                     [this](int taskId, int, int difficulty) {
                         std::ostringstream oss;
                         oss << "任务完成：" << taskId << "，难度" << difficulty << "星";
                         recordAutoLog(LogEntry::LogType::Auto, oss.str(), taskId, {}, 0, "TaskCompleted");
                     });
    QObject::connect(&m_achievementManager, &AchievementManager::achievementUnlocked, this,
                     [this](int achievementId) {
                         std::ostringstream oss;
                         oss << "解锁成就：" << achievementId;
                         recordAutoLog(LogEntry::LogType::Milestone, oss.str(), achievementId, {}, 0, "AchievementUnlocked");
                     });
    QObject::connect(m_userManager.signalProxy(), &UserManager::SignalProxy::levelChanged, this,
                     [this](int newLevel) {
                         std::ostringstream oss;
                         oss << "等级提升至 " << newLevel;
                         recordAutoLog(LogEntry::LogType::Event, oss.str(), std::nullopt, {}, 1, "LevelUp");
                     });
}

int LogManager::recordAutoLog(LogEntry::LogType type,
                              const std::string& content,
                              const std::optional<int>& relatedId,
                              const std::vector<LogEntry::AttributeChange>& attributeChanges,
                              int levelChange,
                              const std::string& specialEvent) {
    LogEntry entry(-1, QDateTime::currentDateTime(), type, content, relatedId, attributeChanges, levelChange,
                   specialEvent, std::nullopt);
    int id = persistLog(entry);
    return id;
}

int LogManager::recordManualLog(const std::string& content, LogEntry::MoodTag mood) {
    LogEntry entry(-1, QDateTime::currentDateTime(), LogEntry::LogType::Manual, content, std::nullopt, {}, 0,
                   "Manual", mood);
    return persistLog(entry);
}

int LogManager::recordMilestone(const std::string& content, const std::optional<int>& relatedAchievementId) {
    LogEntry entry(-1, QDateTime::currentDateTime(), LogEntry::LogType::Milestone, content, relatedAchievementId,
                   {}, 0, "Milestone", std::nullopt);
    return persistLog(entry);
}

std::vector<LogEntry> LogManager::filterLogs(const std::optional<LogEntry::LogType>& type,
                                             const std::optional<QDateTime>& start,
                                             const std::optional<QDateTime>& end,
                                             const std::optional<LogEntry::MoodTag>& mood,
                                             const std::optional<std::string>& keyword,
                                             bool includeForgiven) const {
    auto records = m_database.queryLogRecords(
        type ? std::make_optional(LogEntry::typeToString(*type)) : std::nullopt, toIso(start), toIso(end),
        mood ? std::make_optional(serializeMood(mood)) : std::nullopt, keyword);

    std::vector<LogEntry> result;
    result.reserve(records.size());
    for (const auto& record : records) {
        if (!includeForgiven && m_forgivenLogIds.count(record.id) > 0) {
            continue;
        }
        result.push_back(fromRecord(record));
    }
    return result;
}

GrowthSnapshot LogManager::captureSnapshot() {
    if (!m_userManager.hasActiveUser()) {
        return GrowthSnapshot();
    }
    const User& user = m_userManager.activeUser();
    const auto& stats = user.progress();
    GrowthSnapshot snapshot(-1, QDateTime::currentDateTime(), user.level(), user.growthPoints(), user.attributes(),
                            stats.achievementsUnlocked, stats.totalTasksCompleted, stats.personalTasksCompleted,
                            static_cast<int>(filterLogs(LogEntry::LogType::Manual, std::nullopt, std::nullopt,
                                                       std::nullopt, std::nullopt, true)
                                            .size()));
    DatabaseManager::GrowthSnapshotRecord record{};
    record.timestampIso = snapshot.timestamp().toString(Qt::ISODate).toStdString();
    record.userLevel = snapshot.level();
    record.growthPoints = snapshot.growthPoints();
    record.execution = snapshot.attributes().execution;
    record.perseverance = snapshot.attributes().perseverance;
    record.decision = snapshot.attributes().decision;
    record.knowledge = snapshot.attributes().knowledge;
    record.social = snapshot.attributes().social;
    record.pride = snapshot.attributes().pride;
    record.achievementCount = snapshot.achievementCount();
    record.completedTasks = snapshot.completedTasks();
    record.failedTasks = snapshot.failedTasks();
    record.manualLogCount = snapshot.manualLogCount();
    int id = m_database.insertGrowthSnapshot(record);
    snapshot.setId(id);
    emit snapshotCaptured(snapshot);
    return snapshot;
}

std::vector<GrowthSnapshot> LogManager::querySnapshots(const std::optional<QDateTime>& start,
                                                       const std::optional<QDateTime>& end) const {
    auto records = m_database.queryGrowthSnapshots(toIso(start), toIso(end));
    std::vector<GrowthSnapshot> snapshots;
    snapshots.reserve(records.size());
    for (const auto& record : records) {
        GrowthSnapshot snapshot(record.id, QDateTime::fromString(QString::fromStdString(record.timestampIso), Qt::ISODate),
                                record.userLevel, record.growthPoints,
                                User::AttributeSet{record.execution, record.perseverance, record.decision, record.knowledge,
                                                   record.social, record.pride},
                                record.achievementCount, record.completedTasks, record.failedTasks, record.manualLogCount);
        snapshots.push_back(snapshot);
    }
    if (snapshots.size() > 200) {
        std::vector<GrowthSnapshot> compressed;
        const std::size_t step = snapshots.size() / 200 + 1;
        for (std::size_t i = 0; i < snapshots.size(); i += step) {
            compressed.push_back(snapshots[i]);
        }
        snapshots.swap(compressed);
    }
    return snapshots;
}

void LogManager::forgiveLog(int logId) { m_forgivenLogIds.insert(logId); }

int LogManager::persistLog(const LogEntry& entry) {
    DatabaseManager::LogRecord record{};
    record.timestampIso = entry.timestamp().toString(Qt::ISODate).toStdString();
    record.type = LogEntry::typeToString(entry.type());
    record.content = entry.content();
    record.relatedId = entry.relatedId();
    record.attributeChanges = serializeAttributeChanges(entry.attributeChanges());
    record.levelChange = entry.levelChange();
    record.specialEvent = entry.specialEvent();
    record.mood = serializeMood(entry.mood());
    int id = m_database.insertLogRecord(record);
    LogEntry persisted = entry;
    persisted.setId(id);
    emit logInserted(persisted);
    return id;
}

LogEntry LogManager::fromRecord(const DatabaseManager::LogRecord& record) const {
    LogEntry entry(record.id,
                   QDateTime::fromString(QString::fromStdString(record.timestampIso), Qt::ISODate),
                   LogEntry::typeFromString(record.type), record.content, record.relatedId,
                   deserializeAttributeChanges(record.attributeChanges), record.levelChange, record.specialEvent,
                   deserializeMood(record.mood));
    return entry;
}

std::string LogManager::serializeAttributeChanges(const std::vector<LogEntry::AttributeChange>& changes) const {
    QJsonArray array;
    for (const auto& change : changes) {
        QJsonObject obj;
        obj.insert("name", QString::fromStdString(change.name));
        obj.insert("delta", change.delta);
        array.append(obj);
    }
    return QString(QJsonDocument(array).toJson(QJsonDocument::Compact)).toStdString();
}

std::vector<LogEntry::AttributeChange> LogManager::deserializeAttributeChanges(const std::string& text) const {
    std::vector<LogEntry::AttributeChange> changes;
    auto json = QJsonDocument::fromJson(QString::fromStdString(text).toUtf8());
    if (!json.isArray()) {
        return changes;
    }
    for (const auto& value : json.array()) {
        if (!value.isObject()) {
            continue;
        }
        auto obj = value.toObject();
        LogEntry::AttributeChange change;
        change.name = obj.value("name").toString().toStdString();
        change.delta = obj.value("delta").toInt();
        changes.push_back(change);
    }
    return changes;
}

std::string LogManager::serializeMood(const std::optional<LogEntry::MoodTag>& mood) {
    if (!mood.has_value()) {
        return {};
    }
    return LogEntry::moodToEmoji(*mood);
}

std::optional<LogEntry::MoodTag> LogManager::deserializeMood(const std::string& text) {
    if (text == "😊") {
        return LogEntry::MoodTag::Happy;
    }
    if (text == "😐") {
        return LogEntry::MoodTag::Neutral;
    }
    if (text == "😔") {
        return LogEntry::MoodTag::Sad;
    }
    return std::nullopt;
}

std::optional<std::string> LogManager::toIso(const std::optional<QDateTime>& time) {
    if (!time.has_value()) {
        return std::nullopt;
    }
    return time->toString(Qt::ISODate).toStdString();
}

}  // namespace rove::data
