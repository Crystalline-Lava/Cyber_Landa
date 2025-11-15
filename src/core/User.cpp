#include "User.h"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <stdexcept>

namespace rove::data {
namespace {
constexpr int kAttributeMin = 0;
constexpr int kAttributeMax = 999;  // Upper bound to keep balance. 中文：上限防止属性爆炸。
}  // namespace

// ------------------------- AttributeSet helpers -------------------------
int User::AttributeSet::totalPoints() const noexcept {
    return execution + perseverance + decision + knowledge + social + pride;
}

void User::AttributeSet::add(const AttributeSet& other) noexcept {
    execution += other.execution;
    perseverance += other.perseverance;
    decision += other.decision;
    knowledge += other.knowledge;
    social += other.social;
    pride += other.pride;
}

// ----------------------------- User methods -----------------------------
User::User() = default;

User::User(int id,
           std::string username,
           std::string password,
           int level,
           int growthPoints,
           int coins,
           const AttributeSet& attributes,
           const ProgressStats& progress)
    : m_id(id),
      m_username(std::move(username)),
      m_password(std::move(password)),
      m_level(level),
      m_growthPoints(growthPoints),
      m_coins(coins),
      m_attributes(attributes),
      m_progress(progress) {
    clampAttributes();
}

int User::id() const noexcept { return m_id; }

const std::string& User::username() const noexcept { return m_username; }

const std::string& User::password() const noexcept { return m_password; }

void User::setPassword(const std::string& newPassword) { m_password = newPassword; }

int User::level() const noexcept { return m_level; }

void User::setLevel(int level) {
    m_level = std::max(1, level);
}

int User::growthPoints() const noexcept { return m_growthPoints; }

void User::setGrowthPoints(int points) {
    m_growthPoints = std::max(0, points);
    recalculateLevel();
}

/**
 * @brief Increase growth points using additive reward model.
 * 中文：通过累加奖励模型提升成长值。
 * 业务逻辑：成长值直接驱动等级计算，因此每次增长后立即重算等级，保持 UI 与数据库一致。
 */
void User::addGrowthPoints(int delta) {
    if (delta <= 0) {
        return;  // 中文：非正增量不做处理，防止误操作。
    }
    m_growthPoints += delta;
    recalculateLevel();
}

int User::coins() const noexcept { return m_coins; }

void User::addCoins(int amount) {
    if (amount <= 0) {
        return;
    }
    m_coins += amount;
}

void User::spendCoins(int amount) {
    if (amount <= 0) {
        return;
    }
    if (amount > m_coins) {
        throw std::runtime_error("Insufficient coins to spend");
    }
    m_coins -= amount;
}

const User::AttributeSet& User::attributes() const noexcept { return m_attributes; }

void User::setAttributes(const AttributeSet& attributes) {
    m_attributes = attributes;
    clampAttributes();
}

void User::applyAttributeBonus(const AttributeSet& bonus) {
    m_attributes.add(bonus);
    clampAttributes();
}

/**
 * @brief Allocate attribute points while respecting balancing rules.
 * 中文：在遵守平衡规则的前提下分配属性点。
 * 设计说明：available=成长值/50，体现 "学习-成长-属性" 的教育闭环；花费记录方便教学演示。
 */
void User::distributeAttributes(const AttributeSet& distribution) {
    const int available = computeAvailableAttributePoints();
    const int requested = distribution.totalPoints();
    if (requested > available) {
        throw std::runtime_error("Distribution exceeds available attribute points");
    }
    applyAttributeBonus(distribution);
    m_progress.attributePointsSpent += requested;
}

const User::ProgressStats& User::progress() const noexcept { return m_progress; }

User::ProgressStats& User::progress() noexcept { return m_progress; }

void User::recordTaskCompletion(TaskCategory category) {
    ++m_progress.totalTasksCompleted;
    switch (category) {
        case TaskCategory::Academic:
            ++m_progress.academicTasksCompleted;
            break;
        case TaskCategory::Social:
            ++m_progress.socialTasksCompleted;
            break;
        case TaskCategory::Personal:
            ++m_progress.personalTasksCompleted;
            break;
    }
}

void User::recordAchievementUnlock() { ++m_progress.achievementsUnlocked; }

/**
 * @brief Convert total growth into distributable budget.
 * 中文：将总成长值转换成可分配的属性预算。
 * 教学说明：1 点属性 = 50 成长值，鼓励学生持续积累；spent 记录保证不会超支。
 */
int User::computeAvailableAttributePoints() const noexcept {
    // 中文：每获得 50 成长值奖励 1 点可分配属性，减去已花费点数确保平衡。
    const int earned = m_growthPoints / 50;
    return std::max(0, earned - m_progress.attributePointsSpent);
}

/**
 * @brief Prepare bilingual-friendly attribute summary.
 * 中文：生成便于双语讲解的属性摘要。
 * 说明：该字符串可直接展示在 Qt UI，老师无需额外拼接文本。
 */
std::string User::buildAttributeSummary() const {
    std::ostringstream stream;
    stream << "Execution:" << m_attributes.execution << ', '
           << "Perseverance:" << m_attributes.perseverance << ', '
           << "Decision:" << m_attributes.decision << ', '
           << "Knowledge:" << m_attributes.knowledge << ', '
           << "Social:" << m_attributes.social << ', '
           << "Pride:" << m_attributes.pride;
    return stream.str();
}

/**
 * @brief Implement level curve = 1 + sqrt(growth/100).
 * 中文：实现等级曲线公式：Level = 1 + sqrt(成长值 / 100)。
 * 平衡说明：平方根带来前期升级快、后期渐缓的体验，符合校园成长节奏。
 */
int User::computeLevelFromGrowth(int growthPoints) noexcept {
    if (growthPoints <= 0) {
        return 1;
    }
    const double normalized = static_cast<double>(growthPoints) / 100.0;
    const int computed = 1 + static_cast<int>(std::sqrt(normalized));
    return std::max(1, computed);
}

void User::recalculateLevel() { m_level = computeLevelFromGrowth(m_growthPoints); }

void User::clampAttributes() {
    m_attributes.execution = clampAttributeValue(m_attributes.execution);
    m_attributes.perseverance = clampAttributeValue(m_attributes.perseverance);
    m_attributes.decision = clampAttributeValue(m_attributes.decision);
    m_attributes.knowledge = clampAttributeValue(m_attributes.knowledge);
    m_attributes.social = clampAttributeValue(m_attributes.social);
    m_attributes.pride = clampAttributeValue(m_attributes.pride);
}

int User::clampAttributeValue(int value) noexcept {
    return std::clamp(value, kAttributeMin, kAttributeMax);
}

}  // namespace rove::data
