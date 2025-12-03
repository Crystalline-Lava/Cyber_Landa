#ifndef LOGMANAGER_H
#define LOGMANAGER_H

#include <QObject>

#include <optional>
#include <set>
#include <string>
#include <vector>

#include "AchievementManager.h"
#include "DatabaseManager.h"
#include "GrowthSnapshot.h"
#include "LogEntry.h"
#include "TaskManager.h"
#include "UserManager.h"

namespace rove::data {

/**
 * @class LogManager
 * @brief 日志管理器单例，负责自动与手动日志写入、过滤查询以及成长快照采集。
 * 中文：LogManager 与任务、成就、用户系统联动，自动捕获事件并写入不可变日志，同时定期采集成长快照供可视化使用。
 */
class LogManager : public QObject {
    Q_OBJECT

public:
    /**
     * @brief 获取单例实例，首次调用时注入依赖。
     */
    static LogManager& instance(DatabaseManager& database,
                                UserManager& userManager,
                                AchievementManager& achievementManager,
                                TaskManager& taskManager);

    LogManager(const LogManager&) = delete;
    LogManager& operator=(const LogManager&) = delete;
    LogManager(LogManager&&) = delete;
    LogManager& operator=(LogManager&&) = delete;
    ~LogManager() override = default;

    /**
     * @brief 记录自动日志，例如任务完成、升级、解锁成就。
     */
    int recordAutoLog(LogEntry::LogType type,
                      const std::string& content,
                      const std::optional<int>& relatedId,
                      const std::vector<LogEntry::AttributeChange>& attributeChanges,
                      int levelChange,
                      const std::string& specialEvent);

    /**
     * @brief 记录手动日志（平凡事迹），带有心情表情。
     */
    int recordManualLog(const std::string& content, LogEntry::MoodTag mood);

    /**
     * @brief 添加里程碑日志，用于重大节点标记。
     */
    int recordMilestone(const std::string& content, const std::optional<int>& relatedAchievementId);

    /**
     * @brief 按过滤条件检索日志，支持时间区间、类型、心情和关键词。
     */
    [[nodiscard]] std::vector<LogEntry> filterLogs(const std::optional<LogEntry::LogType>& type,
                                                  const std::optional<QDateTime>& start,
                                                  const std::optional<QDateTime>& end,
                                                  const std::optional<LogEntry::MoodTag>& mood,
                                                  const std::optional<std::string>& keyword,
                                                  bool includeForgiven = false) const;

    /**
     * @brief 采集当前成长快照并写入数据库。
     */
    GrowthSnapshot captureSnapshot();

    /**
     * @brief 查询指定时间段的成长快照，内部自动压缩冗余节点。
     */
    [[nodiscard]] std::vector<GrowthSnapshot> querySnapshots(const std::optional<QDateTime>& start,
                                                            const std::optional<QDateTime>& end) const;

    /**
     * @brief 标记一条日志为“宽恕”隐藏，用于负面记录美化折线图。
     */
    void forgiveLog(int logId);

signals:
    void logInserted(const LogEntry& entry);
    void snapshotCaptured(const GrowthSnapshot& snapshot);

private:
    LogManager(DatabaseManager& database,
               UserManager& userManager,
               AchievementManager& achievementManager,
               TaskManager& taskManager);

    void bindSystemEvents();
    int persistLog(const LogEntry& entry);
    LogEntry fromRecord(const DatabaseManager::LogRecord& record) const;
    std::string serializeAttributeChanges(const std::vector<LogEntry::AttributeChange>& changes) const;
    std::vector<LogEntry::AttributeChange> deserializeAttributeChanges(const std::string& text) const;
    static std::string serializeMood(const std::optional<LogEntry::MoodTag>& mood);
    static std::optional<LogEntry::MoodTag> deserializeMood(const std::string& text);
    static std::optional<std::string> toIso(const std::optional<QDateTime>& time);

    DatabaseManager& m_database;
    UserManager& m_userManager;
    AchievementManager& m_achievementManager;
    TaskManager& m_taskManager;
    std::set<int> m_forgivenLogIds;
};

}  // namespace rove::data

#endif  // LOGMANAGER_H
