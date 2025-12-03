#include "SerendipityEngine.h"

#include <stdexcept>

namespace rove::data {

SerendipityEngine& SerendipityEngine::instance(DatabaseManager& database,
                                               LogManager& logManager,
                                               UserManager& userManager) {
    static SerendipityEngine instance(database, logManager, userManager);
    return instance;
}

SerendipityEngine::SerendipityEngine(DatabaseManager& database, LogManager& logManager, UserManager& userManager)
    : m_config(),
      m_rng(std::random_device{}()),
      m_database(database),
      m_logManager(logManager),
      m_userManager(userManager) {}

SerendipityEngine::SerendipityResult SerendipityEngine::triggerDailyLogin() {
    SerendipityResult result = rollEvent();
    if (result.triggered) {
        std::string content = "奇遇事件：" + result.description;
        m_logManager.recordAutoLog(LogEntry::LogType::Event, content, std::nullopt, {}, 0, "Serendipity");
    }
    return result;
}

void SerendipityEngine::updateProbability(const ProbabilityConfig& config) {
    const auto validateRange = [](double value, const char* name) {
        if (value < 0.0 || value > 1.0) {
            throw std::invalid_argument(std::string(name) + " probability must be within [0,1]");
        }
    };
    validateRange(config.buffChance, "buffChance");
    validateRange(config.taskChance, "taskChance");
    validateRange(config.smallRewardChance, "smallRewardChance");
    const double total = config.buffChance + config.taskChance + config.smallRewardChance;
    if (total > 1.0) {
        throw std::invalid_argument("Total probability cannot exceed 1.0");
    }
    m_config = config;
}

SerendipityEngine::ProbabilityConfig SerendipityEngine::probability() const noexcept { return m_config; }

SerendipityEngine::SerendipityResult SerendipityEngine::rollEvent() {
    SerendipityResult result;
    constexpr int kBuffDurationOneDayMinutes = 1440;
    const double roll = random01();
    const double taskThreshold = m_config.buffChance + m_config.taskChance;
    const double rewardThreshold = taskThreshold + m_config.smallRewardChance;

    if (roll < m_config.buffChance) {
        result.triggered = true;
        result.buffDurationMinutes = kBuffDurationOneDayMinutes;  // 当天有效
        result.rewardMultiplier = 1.2;
        result.description = "今日任务奖励 +20%";
    } else if (roll < taskThreshold) {
        result.triggered = true;
        result.spawnedTask = true;
        result.description = "获得彩蛋任务：校园探索";
    } else if (roll < rewardThreshold) {
        result.triggered = true;
        result.description = "获得微小祝福，成长值 +5";
        if (m_userManager.hasActiveUser()) {
            auto& user = m_userManager.activeUser();
            user.addGrowthPoints(5);
        }
    } else {
        result.description = "今日平静如常";
    }
    return result;
}

double SerendipityEngine::random01() {
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    return dist(m_rng);
}

}  // namespace rove::data
