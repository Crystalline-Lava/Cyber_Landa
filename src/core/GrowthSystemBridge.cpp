#include "GrowthSystemBridge.h"
#include "Task.h"
#include "Achievement.h"
#include <QDebug>

namespace rove::systems {

GrowthSystemBridge::GrowthSystemBridge(GrowthSystem *growthSystem,
                                      data::UserManager *userManager,
                                      data::TaskManager *taskManager,
                                      data::AchievementManager *achievementManager,
                                      QObject *parent)
    : QObject(parent),
      m_growthSystem(growthSystem),
      m_userManager(userManager),
      m_taskManager(taskManager),
      m_achievementManager(achievementManager)
{
}

void GrowthSystemBridge::initialize()
{
    if (!m_userManager || !m_taskManager || !m_achievementManager) {
        qWarning() << "GrowthSystemBridge: 缺少必要的管理器";
        return;
    }
    
    // 连接用户登录信号
    // 注意：UserManager 没有直接的登录信号，需要检查实际信号
    // 这里假设有一个信号 userLoggedIn()
    
    // 连接任务完成信号
    if (auto proxy = m_taskManager->signalProxy()) {
        connect(proxy, &data::TaskManagerSignalProxy::taskCompleted,
                this, &GrowthSystemBridge::onTaskCompleted);
    }
    
    // 连接成就解锁信号
    connect(m_achievementManager, &data::AchievementManager::achievementUnlocked,
            this, &GrowthSystemBridge::onAchievementUnlocked);
    
    // 连接用户属性变化信号
    if (auto userProxy = m_userManager->signalProxy()) {
        connect(userProxy, &data::UserManagerSignalProxy::levelChanged,
                this, &GrowthSystemBridge::onUserLevelChanged);
        connect(userProxy, &data::UserManagerSignalProxy::prideChanged,
                this, &GrowthSystemBridge::onUserPrideChanged);
        connect(userProxy, &data::UserManagerSignalProxy::coinsChanged,
                this, &GrowthSystemBridge::onUserCoinsChanged);
    }
}

void GrowthSystemBridge::onUserLoggedIn()
{
    if (!m_growthSystem) return;
    
    // 同步用户数据到成长系统
    if (m_userManager->hasActiveUser()) {
        const data::User& user = m_userManager->activeUser();
        m_growthSystem->syncWithUser(user);
    }
}

void GrowthSystemBridge::onTaskCompleted(int taskId, int taskType, int difficulty)
{
    if (!m_growthSystem) return;
    
    // 构建任务数据
    QJsonObject taskData;
    taskData["type"] = data::Task::typeToString(static_cast<data::Task::TaskType>(taskType));
    taskData["difficulty"] = difficulty;
    
    // 获取任务详情（可能需要从 TaskManager 获取）
    auto taskOpt = m_taskManager->taskById(taskId);
    if (taskOpt.has_value()) {
        const auto& task = taskOpt.value();
        taskData["coinReward"] = task.coinReward();
        taskData["growthReward"] = task.growthReward();
    }
    
    // 通知成长系统
    m_growthSystem->onTaskCompleted(taskData);
}

void GrowthSystemBridge::onAchievementUnlocked(int achievementId)
{
    if (!m_growthSystem || !m_achievementManager) return;
    
    // 获取成就详情
    auto achievementOpt = m_achievementManager->achievementById(achievementId);
    if (!achievementOpt.has_value()) return;
    
    const auto& achievement = achievementOpt.value();
    
    // 构建成就数据
    QJsonObject achievementData;
    achievementData["name"] = QString::fromStdString(achievement.name());
    achievementData["rewardType"] = QString::fromStdString(
        achievement.rewardType() == data::Achievement::RewardType::WithReward ? 
        "WithReward" : "NoReward");
    
    // 根据成就类型设置稀有度
    if (achievement.name().find("稀有") != std::string::npos ||
        achievement.name().find("隐藏") != std::string::npos) {
        achievementData["rarity"] = 3;
    } else if (achievement.name().find("高级") != std::string::npos) {
        achievementData["rarity"] = 2;
    } else {
        achievementData["rarity"] = 1;
    }
    
    // 判断是否为里程碑成就
    if (achievement.progressMode() == data::Achievement::ProgressMode::Milestone) {
        achievementData["isMilestone"] = true;
    } else {
        achievementData["isMilestone"] = false;
    }
    
    // 设置类别
    achievementData["category"] = QString::fromStdString(achievement.galleryGroup());
    
    // 通知成长系统
    m_growthSystem->onAchievementUnlocked(achievementData);
}

void GrowthSystemBridge::onUserLevelChanged(int newLevel)
{
    if (!m_growthSystem) return;
    
    // 这里不需要特别处理，成长系统应该从 User 同步数据
    // 保持成长系统与用户数据同步
    if (m_userManager->hasActiveUser()) {
        const data::User& user = m_userManager->activeUser();
        m_growthSystem->syncWithUser(user);
    }
}

void GrowthSystemBridge::onUserPrideChanged(int newPride)
{
    if (!m_growthSystem) return;
    
    // 同步自豪感变化
    if (m_userManager->hasActiveUser()) {
        const data::User& user = m_userManager->activeUser();
        m_growthSystem->syncWithUser(user);
    }
}

void GrowthSystemBridge::onUserCoinsChanged(int newCoins)
{
    if (!m_growthSystem) return;
    
    // 同步金币变化
    if (m_userManager->hasActiveUser()) {
        const data::User& user = m_userManager->activeUser();
        m_growthSystem->syncWithUser(user);
    }
}

} // namespace rove::systems