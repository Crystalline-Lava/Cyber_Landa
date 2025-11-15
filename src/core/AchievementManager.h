#ifndef ACHIEVEMENTMANAGER_H
#define ACHIEVEMENTMANAGER_H

#include <QObject>

#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "Achievement.h"
#include "DatabaseManager.h"
#include "TaskManager.h"
#include "UserManager.h"

namespace rove::data {

/**
 * @class AchievementManager
 * @brief 成就系统单例，负责以下事项：
 * 1. 成就分类：系统成就模板 + 学生自定义成就（含奖励型与非奖励型）。
 * 2. 条件检测：通过 Qt 信号槽监听 TaskManager 与 UserManager 的事件，实现事件驱动的完成判定。
 * 3. 进度追踪：多条件累计进度 -> 统一换算为 progressValue/progressGoal，供 UI 展示进度条。
 * 4. 成就画廊：按照 galleryGroup 构建索引，界面可以直接按分组展示。
 * 5. 奖励派发：解锁时发放兰州币、属性点及“自豪感”特殊加成，记录获得的纪念物品。
 * 6. 自定义限制：每名学生每月仅允许创建 2 个带奖励的自定义成就，纯展示型不受限。
 */
class AchievementManager : public QObject {
    Q_OBJECT

public:
    static AchievementManager& instance(DatabaseManager& database,
                                        UserManager& userManager,
                                        TaskManager& taskManager);

    AchievementManager(const AchievementManager&) = delete;
    AchievementManager& operator=(const AchievementManager&) = delete;
    AchievementManager(AchievementManager&&) = delete;
    AchievementManager& operator=(AchievementManager&&) = delete;

    void refreshFromDatabase();
    [[nodiscard]] std::vector<Achievement> achievements() const;
    [[nodiscard]] std::vector<Achievement> achievementsInGroup(const std::string& group) const;
    [[nodiscard]] std::optional<Achievement> achievementById(int id) const;

    int createCustomAchievement(Achievement achievement);
    void updateCustomAchievement(const Achievement& achievement);
    void deleteCustomAchievement(int achievementId);
    void recordCustomProgress(int achievementId, int delta);

signals:
    void achievementUnlocked(int achievementId);
    void achievementProgressChanged(int achievementId, int currentValue, int goalValue);

private slots:
    void onTaskCompleted(int taskId, int taskType, int difficulty);
    void onTaskProgressed(int taskId, int currentValue, int goalValue);
    void onUserLevelChanged(int newLevel);
    void onPrideChanged(int newPride);
    void onCoinsChanged(int newCoins);

private:
    explicit AchievementManager(DatabaseManager& database,
                                UserManager& userManager,
                                TaskManager& taskManager);

    void ensureSystemAchievements();
    [[nodiscard]] std::vector<Achievement> buildSystemTemplates(const std::string& owner) const;
    Achievement hydrateAchievement(const DatabaseManager::AchievementRecord& record) const;
    DatabaseManager::AchievementRecord toRecord(const Achievement& achievement) const;
    bool recalculateProgress(Achievement& achievement);
    void evaluateCompletion(Achievement& achievement);
    void grantRewards(Achievement& achievement);
    bool validateCustomAchievement(const Achievement& achievement) const;
    void rebuildGalleryIndex();
    void updateGalleryForAchievement(const Achievement& achievement);
    int countRewardAchievementsThisMonth(const std::string& owner) const;
    void updateConditionCache(Achievement& achievement,
                              Achievement::Condition::ConditionType type,
                              int delta,
                              const std::string& metadata);
    void replaceConditionValue(Achievement& achievement,
                               Achievement::Condition::ConditionType type,
                               int value,
                               const std::string& metadata);

    DatabaseManager& m_database;
    UserManager& m_userManager;
    TaskManager& m_taskManager;
    std::unordered_map<int, Achievement> m_achievements;
    std::unordered_map<std::string, std::vector<int>> m_galleryIndex;
    mutable std::mutex m_mutex;
};

}  // namespace rove::data

#endif  // ACHIEVEMENTMANAGER_H
