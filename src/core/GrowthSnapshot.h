#ifndef GROWTHSNAPSHOT_H
#define GROWTHSNAPSHOT_H

#include <QDateTime>

#include <string>

#include "User.h"

namespace rove::data {

/**
 * @class GrowthSnapshot
 * @brief 成长快照模型，记录某一时刻的等级、成长值和六维属性，用于时间线与可视化。
 */
class GrowthSnapshot {
public:
    /**
     * @brief 默认构造，便于序列化或占位。
     */
    GrowthSnapshot();

    /**
     * @brief 完整构造函数，填充全部字段。
     */
    GrowthSnapshot(int id,
                   const QDateTime& timestamp,
                   int level,
                   int growthPoints,
                   const User::AttributeSet& attributes,
                   int achievementCount,
                   int completedTasks,
                   int failedTasks,
                   int manualLogCount);

    [[nodiscard]] int id() const noexcept;
    void setId(int id) noexcept;

    [[nodiscard]] const QDateTime& timestamp() const noexcept;
    void setTimestamp(const QDateTime& timestamp) noexcept;

    [[nodiscard]] int level() const noexcept;
    void setLevel(int level) noexcept;

    [[nodiscard]] int growthPoints() const noexcept;
    void setGrowthPoints(int points) noexcept;

    [[nodiscard]] const User::AttributeSet& attributes() const noexcept;
    void setAttributes(const User::AttributeSet& attributes) noexcept;

    [[nodiscard]] int achievementCount() const noexcept;
    void setAchievementCount(int count) noexcept;

    [[nodiscard]] int completedTasks() const noexcept;
    void setCompletedTasks(int count) noexcept;

    [[nodiscard]] int failedTasks() const noexcept;
    void setFailedTasks(int count) noexcept;

    [[nodiscard]] int manualLogCount() const noexcept;
    void setManualLogCount(int count) noexcept;

private:
    int m_id;
    QDateTime m_timestamp;
    int m_level;
    int m_growthPoints;
    User::AttributeSet m_attributes;
    int m_achievementCount;
    int m_completedTasks;
    int m_failedTasks;
    int m_manualLogCount;
};

}  // namespace rove::data

#endif  // GROWTHSNAPSHOT_H
