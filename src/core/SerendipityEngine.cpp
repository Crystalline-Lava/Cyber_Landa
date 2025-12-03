#include "SerendipityEngine.h"

#include <chrono>

namespace rove::data {

SerendipityEngine& SerendipityEngine::instance(DatabaseManager& database,
                                               LogManager& logManager,
                                               UserManager& userManager) {
    static SerendipityEngine instance(database, logManager, userManager);
    return instance;
}

SerendipityEngine::SerendipityEngine(DatabaseManager& database, LogManager& logManager, UserManager& userManager)
    : m_config(),
      m_rng(static_cast<unsigned int>(std::chrono::system_clock::now().time_since_epoch().count())),
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

void SerendipityEngine::updateProbability(const ProbabilityConfig& config) { m_config = config; }

SerendipityEngine::ProbabilityConfig SerendipityEngine::probability() const noexcept { return m_config; }

SerendipityEngine::SerendipityResult SerendipityEngine::rollEvent() {
    SerendipityResult result;
    double roll = random01();
    if (roll < m_config.buffChance) {
        result.triggered = true;
        result.buffDurationMinutes = 1440;  // 当天有效
        result.rewardMultiplier = 1.2;
        result.description = "今日任务奖励 +20%";
        return result;
    }
    roll = random01();
    if (roll < m_config.taskChance) {
        result.triggered = true;
        result.spawnedTask = true;
        result.description = "获得彩蛋任务：校园探索";
        return result;
    }
    roll = random01();
    if (roll < m_config.smallRewardChance) {
        result.triggered = true;
        result.description = "获得微小祝福，成长值 +5";
        if (m_userManager.hasActiveUser()) {
            auto& user = m_userManager.activeUser();
            user.addGrowthPoints(5);
        }
        return result;
    }
    result.description = "今日平静如常";
    return result;
}

double SerendipityEngine::random01() {
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    return dist(m_rng);
}

}  // namespace rove::data
