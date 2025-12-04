// GrowthSystemBridge.h
#ifndef GROWTHSYSTEMBRIDGE_H
#define GROWTHSYSTEMBRIDGE_H

#include <QObject>
#include "GrowthSystem.h"
#include "UserManager.h"
#include "TaskManager.h"
#include "AchievementManager.h"

namespace rove::systems {

/**
 * @class GrowthSystemBridge
 * @brief 成长系统桥接器 - 连接成长系统与现有系统
 * 中文：负责将任务系统、成就系统的信号转发给成长系统
 */
class GrowthSystemBridge : public QObject
{
    Q_OBJECT

public:
    explicit GrowthSystemBridge(GrowthSystem *growthSystem,
                               data::UserManager *userManager,
                               data::TaskManager *taskManager,
                               data::AchievementManager *achievementManager,
                               QObject *parent = nullptr);

    void initialize();

private slots:
    void onUserLoggedIn();
    void onTaskCompleted(int taskId, int taskType, int difficulty);
    void onAchievementUnlocked(int achievementId);
    void onUserLevelChanged(int newLevel);
    void onUserPrideChanged(int newPride);
    void onUserCoinsChanged(int newCoins);

private:
    GrowthSystem *m_growthSystem;
    data::UserManager *m_userManager;
    data::TaskManager *m_taskManager;
    data::AchievementManager *m_achievementManager;
};

} // namespace rove::systems

#endif // GROWTHSYSTEMBRIDGE_H