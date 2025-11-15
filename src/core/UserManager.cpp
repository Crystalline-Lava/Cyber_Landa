#include "UserManager.h"

#include <sstream>
#include <stdexcept>

namespace rove::data {
namespace {
// Key names reused during serialization to keep schema human-readable.
constexpr const char* kGrowthKey = "growth";
constexpr const char* kExecutionKey = "execution";
constexpr const char* kPerseveranceKey = "perseverance";
constexpr const char* kDecisionKey = "decision";
constexpr const char* kKnowledgeKey = "knowledge";
constexpr const char* kSocialKey = "social";
constexpr const char* kPrideKey = "pride";
constexpr const char* kAchievementsKey = "achievements";
constexpr const char* kTasksTotalKey = "tasks_total";
constexpr const char* kTasksAcademicKey = "tasks_academic";
constexpr const char* kTasksSocialKey = "tasks_social";
constexpr const char* kTasksPersonalKey = "tasks_personal";
constexpr const char* kAttributeSpentKey = "attribute_spent";
}  // namespace

/**
 * @brief Constructor simply stores DatabaseManager reference.
 * 中文：构造函数仅保存 DatabaseManager 引用，实现依赖注入。
 * 说明：Manager 本身不拥有数据库，遵循 RAII，避免重复关闭句柄。
 */
UserManager::UserManager(DatabaseManager& database)
    : m_database(database), m_activeUser(), m_signalProxy(std::make_unique<SignalProxy>()) {}

/**
 * @brief Validate credentials and populate in-memory session.
 * 中文：校验账号密码，并构建内存中的会话对象。
 * 逻辑：查询 -> 校验密码 -> hydrateUser。若任一步骤失败则返回 false，确保登录流程可预测。
 */
bool UserManager::login(const std::string& username, const std::string& password) {
    auto record = m_database.getUserByName(username);
    if (!record.has_value()) {
        return false;  // 中文：用户名不存在。
    }
    if (record->password != password) {
        return false;  // 中文：密码不匹配。
    }
    m_activeUser = hydrateUser(*record);
    return true;
}

/**
 * @brief Clear optional session to free memory and展示退出操作。
 * 中文：清空可选对象，释放内存并展示退出操作。
 */
void UserManager::logout() noexcept { m_activeUser.reset(); }

/**
 * @brief Check session existence for UI enabling/disabling.
 * 中文：检查会话是否存在，供界面启用/禁用按钮使用。
 */
bool UserManager::hasActiveUser() const noexcept { return m_activeUser.has_value(); }

const User& UserManager::activeUser() const {
    if (!m_activeUser.has_value()) {
        throw std::runtime_error("No active user session");
    }
    return *m_activeUser;
}

User& UserManager::activeUser() {
    if (!m_activeUser.has_value()) {
        throw std::runtime_error("No active user session");
    }
    return *m_activeUser;
}

/**
 * @brief Public wrapper ensuring session存在后调用事务保存。
 * 中文：会话存在时调用事务保存，统一错误信息。
 */
void UserManager::saveActiveUser() {
    if (!m_activeUser.has_value()) {
        throw std::runtime_error("Cannot save without active user");
    }
    persistUser(*m_activeUser);
}

/**
 * @brief Apply growth/coin/attribute rewards from任务逻辑。
 * 中文：将任务的成长、金币、属性奖励同步到用户对象。
 * 细节：每个增量都先更新内存，再一次性持久化，减少数据库写操作次数。
 */
void UserManager::applyTaskCompletion(int growthGain,
                                      int coinGain,
                                      const User::AttributeSet& attributeBonus,
                                      User::TaskCategory category) {
    User& user = activeUser();
    const int previousLevel = user.level();
    const int previousCoins = user.coins();
    const int previousPride = user.attributes().pride;
    if (growthGain > 0) {
        user.addGrowthPoints(growthGain);
    }
    if (coinGain > 0) {
        user.addCoins(coinGain);
    }
    user.applyAttributeBonus(attributeBonus);
    user.recordTaskCompletion(category);
    persistUser(user);
    if (m_signalProxy) {
        if (user.level() != previousLevel) {
            emit m_signalProxy->levelChanged(user.level());
        }
        if (user.coins() != previousCoins) {
            emit m_signalProxy->coinsChanged(user.coins());
        }
        if (user.attributes().pride != previousPride) {
            emit m_signalProxy->prideChanged(user.attributes().pride);
        }
    }
}

/**
 * @brief Increment achievement counter + save for展示荣誉墙。
 * 中文：递增成就计数并立即保存，方便荣誉墙实时刷新。
 */
void UserManager::unlockAchievement() {
    User& user = activeUser();
    user.recordAchievementUnlock();
    persistUser(user);
}

/**
 * @brief Forward UI分配到 User::distributeAttributes 并保存。
 * 中文：把 UI 分配结果交给 User 进行校验，然后保存，保证逻辑集中。
 */
void UserManager::distributeAttributePoints(const User::AttributeSet& distribution) {
    User& user = activeUser();
    const int previousPride = user.attributes().pride;
    user.distributeAttributes(distribution);
    persistUser(user);
    if (m_signalProxy && user.attributes().pride != previousPride) {
        emit m_signalProxy->prideChanged(user.attributes().pride);
    }
}

/**
 * @brief Re-sync active user with authoritative database row.
 * 中文：将当前用户与数据库权威记录重新同步。
 * 用途：当老师使用外部工具修改数据库时，学生客户端可刷新以避免脏数据。
 */
void UserManager::refreshFromDatabase() {
    if (!m_activeUser.has_value()) {
        return;  // 中文：无会话无需刷新。
    }
    auto record = m_database.getUserByName(m_activeUser->username());
    if (!record.has_value()) {
        throw std::runtime_error("Active user missing from database");
    }
    m_activeUser = hydrateUser(*record);
}

UserManager::SignalProxy* UserManager::signalProxy() const noexcept { return m_signalProxy.get(); }

/**
 * @brief Convert DatabaseManager::UserRecord -> domain object with attributes/stats.
 * 中文：将数据库记录转换为包含属性与统计信息的领域对象。
 */
User UserManager::hydrateUser(const DatabaseManager::UserRecord& record) const {
    const auto rawMap = parseAttributesBlob(record.attributes);
    auto valueFor = [&rawMap](const std::string& key) -> int {
        auto it = rawMap.find(key);
        if (it == rawMap.end()) {
            return 0;
        }
        return it->second;
    };

    User::AttributeSet attributes;
    attributes.execution = valueFor(kExecutionKey);
    attributes.perseverance = valueFor(kPerseveranceKey);
    attributes.decision = valueFor(kDecisionKey);
    attributes.knowledge = valueFor(kKnowledgeKey);
    attributes.social = valueFor(kSocialKey);
    attributes.pride = valueFor(kPrideKey);

    User::ProgressStats stats;
    stats.achievementsUnlocked = valueFor(kAchievementsKey);
    stats.totalTasksCompleted = valueFor(kTasksTotalKey);
    stats.academicTasksCompleted = valueFor(kTasksAcademicKey);
    stats.socialTasksCompleted = valueFor(kTasksSocialKey);
    stats.personalTasksCompleted = valueFor(kTasksPersonalKey);
    stats.attributePointsSpent = valueFor(kAttributeSpentKey);

    const int growthPoints = valueFor(kGrowthKey);

    return User(record.id,
                record.username,
                record.password,
                record.level,
                growthPoints,
                record.currency,
                attributes,
                stats);
}

/**
 * @brief Flatten User state into key=value pairs for SQLite column.
 * 中文：把用户状态压平成 key=value; 字符串存入 SQLite 字段。
 * 这样无需增加额外列，减少迁移成本，便于课堂说明。
 */
std::string UserManager::serializeAttributes(const User& user) const {
    std::ostringstream stream;
    stream << kGrowthKey << '=' << user.growthPoints() << ';'
           << kExecutionKey << '=' << user.attributes().execution << ';'
           << kPerseveranceKey << '=' << user.attributes().perseverance << ';'
           << kDecisionKey << '=' << user.attributes().decision << ';'
           << kKnowledgeKey << '=' << user.attributes().knowledge << ';'
           << kSocialKey << '=' << user.attributes().social << ';'
           << kPrideKey << '=' << user.attributes().pride << ';'
           << kAchievementsKey << '=' << user.progress().achievementsUnlocked << ';'
           << kTasksTotalKey << '=' << user.progress().totalTasksCompleted << ';'
           << kTasksAcademicKey << '=' << user.progress().academicTasksCompleted << ';'
           << kTasksSocialKey << '=' << user.progress().socialTasksCompleted << ';'
           << kTasksPersonalKey << '=' << user.progress().personalTasksCompleted << ';'
           << kAttributeSpentKey << '=' << user.progress().attributePointsSpent;
    return stream.str();
}

/**
 * @brief Split attribute blob into map with异常兜底。
 * 中文：将属性串拆分为映射，并为异常情况提供兜底值。
 */
std::unordered_map<std::string, int> UserManager::parseAttributesBlob(const std::string& blob) const {
    std::unordered_map<std::string, int> values;
    std::istringstream stream(blob);
    std::string segment;
    while (std::getline(stream, segment, ';')) {
        if (segment.empty()) {
            continue;
        }
        const auto separator = segment.find('=');
        if (separator == std::string::npos) {
            continue;
        }
        const std::string key = segment.substr(0, separator);
        const std::string valuePart = segment.substr(separator + 1);
        try {
            values[key] = std::stoi(valuePart);
        } catch (...) {
            values[key] = 0;  // 中文：遇到非法值时回退到 0，保证健壮性。
        }
    }
    return values;
}

/**
 * @brief Persist user data using explicit transaction for ACID 保证。
 * 中文：使用显式事务写入用户数据，保证 ACID 特性。
 * 步骤：BEGIN -> 三条更新 -> COMMIT，出错则 ROLLBACK，完全符合老师对事务安全的要求。
 */
void UserManager::persistUser(const User& user) {
    bool transactionStarted = false;
    try {
        transactionStarted = m_database.beginTransaction();
        m_database.updateUserLevel(user.username(), user.level());
        m_database.updateUserCurrency(user.username(), user.coins());
        m_database.updateUserAttributes(user.username(), serializeAttributes(user));
        m_database.commitTransaction();
    } catch (...) {
        if (transactionStarted) {
            try {
                m_database.rollbackTransaction();
            } catch (...) {
                // 中文：回滚失败只能记录，保持异常向上抛出。
            }
        }
        throw;
    }
}

}  // namespace rove::data
