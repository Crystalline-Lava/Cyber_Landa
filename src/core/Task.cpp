#include "Task.h"

#include <algorithm>
#include <stdexcept>

namespace rove::data {

namespace {
constexpr int kMinDifficulty = 1;
constexpr int kMaxDifficulty = 5;
}

Task::Task()
    : m_taskId(-1),
      m_name(),
      m_description(),
      m_type(TaskType::Daily),
      m_difficultyStars(kMinDifficulty),
      m_deadline(QDateTime::currentDateTimeUtc()),
      m_completed(false),
      m_coinReward(0),
      m_growthReward(0),
      m_attributeReward(),
      m_bonusStreak(0),
      m_forgivenessCoupons(0),
      m_customSettings("{}"),
      m_progressValue(0),
      m_progressGoal(100) {}

Task::Task(int id,
           std::string name,
           std::string description,
           TaskType type,
           int difficultyStars,
           const QDateTime& deadline,
           bool completed,
           int coinReward,
           int growthReward,
           User::AttributeSet attributeReward,
           int bonusStreak,
           int forgivenessCoupons,
           std::string customSettings,
           int progressValue,
           int progressGoal)
    : m_taskId(id),
      m_name(std::move(name)),
      m_description(std::move(description)),
      m_type(type),
      m_difficultyStars(std::clamp(difficultyStars, kMinDifficulty, kMaxDifficulty)),
      m_deadline(deadline),
      m_completed(completed),
      m_coinReward(coinReward),
      m_growthReward(growthReward),
      m_attributeReward(attributeReward),
      m_bonusStreak(std::max(0, bonusStreak)),
      m_forgivenessCoupons(std::max(0, forgivenessCoupons)),
      m_customSettings(std::move(customSettings)),
      m_progressValue(std::max(0, progressValue)),
      m_progressGoal(std::max(1, progressGoal)) {}

int Task::id() const noexcept { return m_taskId; }

void Task::setId(int id) noexcept { m_taskId = id; }

const std::string& Task::name() const noexcept { return m_name; }

void Task::setName(std::string name) { m_name = std::move(name); }

const std::string& Task::description() const noexcept { return m_description; }

void Task::setDescription(std::string description) { m_description = std::move(description); }

Task::TaskType Task::type() const noexcept { return m_type; }

void Task::setType(TaskType type) noexcept { m_type = type; }

int Task::difficultyStars() const noexcept { return m_difficultyStars; }

void Task::setDifficultyStars(int stars) { m_difficultyStars = std::clamp(stars, kMinDifficulty, kMaxDifficulty); }

const QDateTime& Task::deadline() const noexcept { return m_deadline; }

void Task::setDeadline(const QDateTime& deadline) { m_deadline = deadline; }

bool Task::isCompleted() const noexcept { return m_completed; }

void Task::setCompleted(bool completed) noexcept { m_completed = completed; }

int Task::coinReward() const noexcept { return m_coinReward; }

void Task::setCoinReward(int coins) { m_coinReward = std::max(0, coins); }

int Task::growthReward() const noexcept { return m_growthReward; }

void Task::setGrowthReward(int growth) { m_growthReward = std::max(0, growth); }

const User::AttributeSet& Task::attributeReward() const noexcept { return m_attributeReward; }

void Task::setAttributeReward(const User::AttributeSet& reward) noexcept { m_attributeReward = reward; }

int Task::bonusStreak() const noexcept { return m_bonusStreak; }

void Task::setBonusStreak(int streak) noexcept { m_bonusStreak = std::max(0, streak); }

void Task::incrementBonusStreak() noexcept { ++m_bonusStreak; }

void Task::resetBonusStreak() noexcept { m_bonusStreak = 0; }

int Task::forgivenessCoupons() const noexcept { return m_forgivenessCoupons; }

void Task::setForgivenessCoupons(int count) noexcept { m_forgivenessCoupons = std::max(0, count); }

const std::string& Task::customSettings() const noexcept { return m_customSettings; }

void Task::setCustomSettings(std::string settings) { m_customSettings = std::move(settings); }

int Task::progressValue() const noexcept { return m_progressValue; }

void Task::setProgressValue(int value) { m_progressValue = std::clamp(value, 0, m_progressGoal); }

int Task::progressGoal() const noexcept { return m_progressGoal; }

void Task::setProgressGoal(int goal) {
    m_progressGoal = std::max(1, goal);
    m_progressValue = std::clamp(m_progressValue, 0, m_progressGoal);
}

bool Task::isExpired(const QDateTime& reference) const noexcept { return reference > m_deadline; }

bool Task::requiresDailyReset() const noexcept { return m_type == TaskType::Daily; }

bool Task::requiresWeeklyReset() const noexcept { return m_type == TaskType::Weekly; }

void Task::resetProgressForNewCycle() {
    m_completed = false;
    m_progressValue = 0;
}

bool Task::recordFailure(bool useForgiveness) {
    if (useForgiveness && m_forgivenessCoupons > 0) {
        --m_forgivenessCoupons;
        return false;
    }
    resetBonusStreak();
    resetProgressForNewCycle();
    return true;
}

std::string Task::typeToString(TaskType type) {
    switch (type) {
        case TaskType::Daily:
            return "Daily";
        case TaskType::Weekly:
            return "Weekly";
        case TaskType::Semester:
            return "Semester";
        case TaskType::Custom:
            return "Custom";
    }
    throw std::runtime_error("Unknown task type");
}

Task::TaskType Task::typeFromString(const std::string& text) {
    if (text == "Daily") {
        return TaskType::Daily;
    }
    if (text == "Weekly") {
        return TaskType::Weekly;
    }
    if (text == "Semester") {
        return TaskType::Semester;
    }
    if (text == "Custom") {
        return TaskType::Custom;
    }
    throw std::runtime_error("Unsupported task type string");
}

}  // namespace rove::data
