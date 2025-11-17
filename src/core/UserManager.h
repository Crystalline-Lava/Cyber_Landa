#ifndef USERMANAGER_H
#define USERMANAGER_H

#include <QObject>

#include <optional>
#include <string>
#include <unordered_map>
#include <memory>

#include "DatabaseManager.h"
#include "User.h"

namespace rove::data {

/**
 * @class UserManager
 * @brief Coordinates session management, progression logic, and persistence.
 * 中文：UserManager 负责会话管理、成长逻辑以及与数据库之间的数据同步。
 *
 * Business reasoning: separating state (User) and IO (DatabaseManager) keeps each class single-
 * purpose and easier to explain to teachers. 中文：通过划分职责让类职责单一，更方便授课讲解。
 */
class UserManager final {
public:
    class SignalProxy : public QObject {
        Q_OBJECT

    public:
        explicit SignalProxy(QObject* parent = nullptr) : QObject(parent) {}

    signals:
        void levelChanged(int newLevel);
        void prideChanged(int newPride);
        void coinsChanged(int newCoins);
    };

    /**
     * @brief Construct manager with dependency injection of DatabaseManager singleton.
     * 中文：通过依赖注入获取 DatabaseManager 单例，便于测试与解耦。
     * @param database Already initialized DatabaseManager. 中文：已初始化的 DatabaseManager。
     * @throws None. 中文：不抛异常。
     */
    explicit UserManager(DatabaseManager& database);

    UserManager(const UserManager&) = delete;
    UserManager& operator=(const UserManager&) = delete;
    UserManager(UserManager&&) = delete;
    UserManager& operator=(UserManager&&) = delete;
    ~UserManager() = default;

    /**
     * @brief Authenticate user and load profile into memory.
     * 中文：验证用户名密码并将用户档案加载到内存。
     * @param username Entered username. 中文：输入的用户名。
     * @param password Entered password. 中文：输入的密码。
     * @return true when authentication success. 中文：验证成功返回 true。
     * @throws std::runtime_error When database query fails. 中文：查询失败时抛异常。
     */
    bool login(const std::string& username, const std::string& password);

    /**
     * @brief Clear session state when学生退出。
     * 中文：学生退出登录时清理会话状态。
     * @return void. 中文：无返回值。
     * @throws None. 中文：不抛异常。
     */
    void logout() noexcept;

    /**
     * @brief Whether a session is currently active.
     * 中文：判断当前是否有激活的会话。
     * @return true if session exists. 中文：有会话返回 true。
     * @throws None. 中文：不抛异常。
     */
    [[nodiscard]] bool hasActiveUser() const noexcept;

    /**
     * @brief Immutable access to active user.
     * 中文：以只读方式访问当前用户。
     * @return Constant reference to active user. 中文：返回当前用户常量引用。
     * @throws std::runtime_error When no session exists. 中文：无会话时抛异常。
     */
    [[nodiscard]] const User& activeUser() const;

    /**
     * @brief Mutable access to active user for UI adjustments.
     * 中文：以可写方式访问当前用户。
     * @return Writable reference. 中文：返回可写引用。
     * @throws std::runtime_error When no session exists. 中文：无会话时抛异常。
     */
    [[nodiscard]] User& activeUser();

    /**
     * @brief Persist current in-memory state back to SQLite.
     * 中文：将内存中的最新状态保存回 SQLite。
     * @return void. 中文：无返回值。
     * @throws std::runtime_error When database update fails. 中文：数据库更新失败抛异常。
     */
    void saveActiveUser();

    /**
     * @brief Apply task rewards including growth, coins, and attribute bonuses.
     * 中文：应用任务奖励（成长、金币、属性），并记录任务统计。
     * @param growthGain Growth points reward. 中文：成长值奖励。
     * @param coinGain Coin reward. 中文：金币奖励。
     * @param attributeBonus Attribute deltas. 中文：属性加成。
     * @param category Task type for statistics. 中文：任务类别。
     * @return void. 中文：无返回值。
     * @throws std::runtime_error When no session or DB fails. 中文：无会话或数据库失败抛异常。
     */
    void applyTaskCompletion(int growthGain,
                             int coinGain,
                             const User::AttributeSet& attributeBonus,
                             User::TaskCategory category);

    /**
     * @brief Register newly unlocked achievement.
     * 中文：登记新的成就解锁事件。
     * @return void. 中文：无返回值。
     * @throws std::runtime_error When no session or DB fails. 中文：无会话/数据库失败抛异常。
     */
    void unlockAchievement();

    /**
     * @brief Handle manual attribute point distribution from UI.
     * 中文：处理 UI 中手动分配属性点的需求。
     * @param distribution Distribution plan. 中文：分配方案。
     * @return void. 中文：无返回值。
     * @throws std::runtime_error When plan invalid/DB fails. 中文：计划非法或数据库失败抛异常。
     */
    void distributeAttributePoints(const User::AttributeSet& distribution);

    /**
     * @brief Reload user data from database (防止脏读)。
     * 中文：从数据库重新加载数据，防止本地状态过期。
     * @return void. 中文：无返回值。
     * @throws std::runtime_error When user missing. 中文：找不到用户时抛异常。
     */
    void refreshFromDatabase();

    /**
     * @brief 暴露信号代理，供成就系统监听等级/自豪感等事件。
     */
    [[nodiscard]] SignalProxy* signalProxy() const noexcept;

private:
    /**
     * @brief Build User object from raw database record.
     * 中文：将数据库记录解析为 User 对象。
     * @param record Row returned by DatabaseManager. 中文：数据库行记录。
     * @return User hydrated domain object. 中文：返回组装好的 User。
     * @throws None (invalid fields default). 中文：字段异常时默认值替换，不抛异常。
     */
    User hydrateUser(const DatabaseManager::UserRecord& record) const;

    /**
     * @brief Serialize growth/attributes/progress into compact string.
     * 中文：将成长值、属性、统计序列化为紧凑字符串。
     * @param user In-memory entity. 中文：需要序列化的用户对象。
     * @return Key-value blob used by DatabaseManager. 中文：返回用于存储的键值串。
     * @throws None. 中文：不抛异常。
     */
    std::string serializeAttributes(const User& user) const;

    /**
     * @brief Parse attribute blob string into key-value map.
     * 中文：把属性字符串解析成键值映射，便于读取。
     * @param blob Compact serialized payload. 中文：紧凑的序列化字符串。
     * @return Map of field names to integers. 中文：返回字段到整数的映射。
     * @throws None (invalid pairs skipped). 中文：遇到非法键值对会跳过，不抛异常。
     */
    std::unordered_map<std::string, int> parseAttributesBlob(const std::string& blob) const;

    /**
     * @brief Persist a given user within a transaction for安全性。
     * 中文：在事务中保存用户，保证一致性。
     * @param user Target user. 中文：要保存的用户。
     * @return void. 中文：无返回值。
     * @throws std::runtime_error When DB operations fail. 中文：数据库失败时抛异常。
     */
    void persistUser(const User& user);

    DatabaseManager& m_database;
    std::optional<User> m_activeUser;  //!< RAII session object. 中文：RAII 管理的会话对象。
    std::unique_ptr<SignalProxy> m_signalProxy;
};

}  // namespace rove::data

#endif  // USERMANAGER_H
