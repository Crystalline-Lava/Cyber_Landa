#ifndef SERENDIPITYENGINE_H
#define SERENDIPITYENGINE_H

#include <QObject>

#include <random>
#include <string>

#include "LogManager.h"

namespace rove::data {

/**
 * @class SerendipityEngine
 * @brief 奇遇事件系统单例，处理每日登录随机增益、限时加成及彩蛋任务生成。
 */
class SerendipityEngine : public QObject {
    Q_OBJECT

public:
    struct ProbabilityConfig {
        double buffChance = 0.1;       // 触发临时增益概率
        double taskChance = 0.05;      // 触发彩蛋任务概率
        double smallRewardChance = 0.2;  // 小额奖励概率
    };

    struct SerendipityResult {
        bool triggered = false;
        std::string description;
        int buffDurationMinutes = 0;
        double rewardMultiplier = 1.0;
        bool spawnedTask = false;
    };

    static SerendipityEngine& instance(DatabaseManager& database, LogManager& logManager, UserManager& userManager);

    SerendipityEngine(const SerendipityEngine&) = delete;
    SerendipityEngine& operator=(const SerendipityEngine&) = delete;
    SerendipityEngine(SerendipityEngine&&) = delete;
    SerendipityEngine& operator=(SerendipityEngine&&) = delete;
    ~SerendipityEngine() override = default;

    /**
     * @brief 每日登录时调用，按概率返回奇遇结果并自动记录日志。
     */
    SerendipityResult triggerDailyLogin();

    /**
     * @brief 更新概率表，方便教师微调平衡。
     */
    void updateProbability(const ProbabilityConfig& config);

    /**
     * @brief 获取当前概率配置。
     */
    [[nodiscard]] ProbabilityConfig probability() const noexcept;

private:
    SerendipityEngine(DatabaseManager& database, LogManager& logManager, UserManager& userManager);

    SerendipityResult rollEvent();
    double random01();

    ProbabilityConfig m_config;
    std::mt19937 m_rng;
    DatabaseManager& m_database;
    LogManager& m_logManager;
    UserManager& m_userManager;
};

}  // namespace rove::data

#endif  // SERENDIPITYENGINE_H
