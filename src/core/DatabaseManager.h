#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <cstddef>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include <sqlite3.h>

namespace rove::data {

/**
 * @class DatabaseManager
 * @brief Core SQLite access layer implemented as a thread-safe singleton.
 * 中文：DatabaseManager 是线程安全的单例类，负责集中管理所有 SQLite 操作。
 *
 * Business logic: centralizing connection management prevents resource duplication and enforces
 * consistent transactional behavior across UI modules. 中文：集中管理连接可避免资源重复并保证各界面模块的事务行为一致。
 */
class DatabaseManager final {
public:
    /**
     * @brief Obtain singleton instance (lazy initialized per C++ standard).
     * 中文：获取延迟初始化的单例实例。
     *
     * @return Reference to DatabaseManager. 中文：返回 DatabaseManager 引用。
     * @throws None. 中文：不抛出异常。
     */
    static DatabaseManager& instance();

    /**
     * @brief Initialize connection and ensure schema exists.
     * 中文：初始化数据库连接并确保数据表存在。
     *
     * Business logic: explicit initialization lets the caller specify which SQLite file to use.
     * 中文：显式初始化让调用者能够指定要使用的 SQLite 文件。
     *
     * @param databasePath Path to database file. 中文：数据库文件路径。
     * @return void. 中文：无返回值。
     * @throws std::runtime_error On failure to open DB or create tables. 中文：打开或建表失败抛出异常。
     */
    void initialize(const std::string& databasePath);

    /**
     * @brief Determine whether a valid SQLite handle is present.
     * 中文：判断当前是否存在有效的 SQLite 句柄。
     *
     * @return true if connected, false otherwise. 中文：连接存在返回 true，否则 false。
     * @throws None. 中文：不抛出异常。
     */
    [[nodiscard]] bool isConnected() const noexcept;

    /**
     * @brief Create user table if missing and seed default account.
     * 中文：若用户表不存在则创建，并插入默认账号。
     *
     * @return void. 中文：无返回值。
     * @throws std::runtime_error When DDL/DML execution fails. 中文：执行建表或插入失败时抛出异常。
     */
    void ensureUserTable();

    /**
     * @brief Validate teacher-specified account (username x, password 1).
     * 中文：验证教师指定的账号（用户名 x，密码 1）。
     *
     * @param username User-supplied username. 中文：用户输入的用户名。
     * @param password User-supplied password. 中文：用户输入的密码。
     * @return true if credentials match stored row. 中文：匹配成功返回 true。
     * @throws std::runtime_error On query errors. 中文：查询失败抛出异常。
     */
    [[nodiscard]] bool validatePreconfiguredAccount(const std::string& username,
                                                    const std::string& password) const;

    /**
     * @struct UserRecord
     * @brief In-memory representation of a row in users table.
     * 中文：UserRecord 用于描述 users 表的一行数据。
     */
    struct UserRecord {
        int id = -1;                //!< Primary key. 中文：主键。
        std::string username;       //!< Username. 中文：用户名。
        std::string password;       //!< Plain password (demo). 中文：演示用密码。
        int level = 1;              //!< Player level. 中文：等级。
        int currency = 0;           //!< Currency amount. 中文：货币。
        std::string attributes;     //!< Serialized attributes. 中文：序列化属性。
    };

    /**
     * @struct TaskRecord
     * @brief 用于描述 tasks 表中的一条任务记录，方便 TaskManager 进行序列化与反序列化。
     */
    struct TaskRecord {
        int id = -1;                    //!< 主键 ID。
        std::string name;               //!< 任务名称。
        std::string description;        //!< 任务描述。
        std::string type;               //!< 任务类型（Daily/Weekly/Semester/Custom）。
        int difficulty = 1;             //!< 难度星级。
        std::string deadlineIso;        //!< 截止时间，ISO8601 字符串。
        bool completed = false;         //!< 完成状态。
        int coinReward = 0;             //!< 基础兰州币奖励。
        int growthReward = 0;           //!< 成长值奖励。
        std::string attributeReward;    //!< 属性奖励序列化结果。
        int bonusStreak = 0;            //!< 连续完成次数。
        std::string customSettings;     //!< 自定义配置（JSON 或键值对）。
        int forgivenessCoupons = 0;     //!< 宽恕券数量，用于处理失败。
        int progressValue = 0;          //!< 当前进度。
        int progressGoal = 100;         //!< 进度目标。
    };

    struct AchievementRecord {
        int id = -1;
        std::string owner;
        std::string creator;
        std::string name;
        std::string description;
        std::string iconPath;
        std::string color;
        std::string type;
        std::string rewardType;
        std::string progressMode;
        int progressValue = 0;
        int progressGoal = 1;
        int rewardCoins = 0;
        std::string rewardAttributes;
        std::string rewardItems;
        bool unlocked = false;
        std::string completionTime;
        std::string conditions;
        std::string galleryGroup;
        std::string createdAt;
        std::string specialMetadata;
    };

    /**
     * @brief Insert a new user and return row id.
     * 中文：插入新用户并返回行号。
     *
     * @param user User data to persist. 中文：需要保存的用户数据。
     * @return New row id. 中文：新生成的行号。
     * @throws std::runtime_error When insert fails. 中文：插入失败抛出异常。
     */
    int createUser(const UserRecord& user);

    /**
     * @brief Fetch a user by username.
     * 中文：根据用户名查询用户。
     *
     * @param username Target username. 中文：目标用户名。
     * @return Optional UserRecord if found. 中文：若找到则返回记录，否则返回空。
     * @throws std::runtime_error On query errors. 中文：查询失败抛出异常。
     */
    [[nodiscard]] std::optional<UserRecord> getUserByName(const std::string& username) const;

    /**
     * @brief Update level column for specified user.
     * 中文：更新指定用户的等级。
     *
     * @param username Target username. 中文：目标用户名。
     * @param newLevel New level value. 中文：新的等级值。
     * @return true if a row changed. 中文：若成功更新返回 true。
     * @throws std::runtime_error On update errors. 中文：更新失败抛出异常。
     */
    bool updateUserLevel(const std::string& username, int newLevel);

    /**
     * @brief Update currency column for specified user.
     * 中文：更新指定用户的货币数值。
     *
     * @param username Target username. 中文：目标用户名。
     * @param newCurrency New currency amount. 中文：新的货币值。
     * @return true if a row changed. 中文：若成功更新返回 true。
     * @throws std::runtime_error On update errors. 中文：更新失败抛出异常。
     */
    bool updateUserCurrency(const std::string& username, int newCurrency);

    /**
     * @brief Update serialized attributes for specified user.
     * 中文：更新指定用户的序列化属性。
     *
     * @param username Target username. 中文：目标用户名。
     * @param newAttributes Serialized attribute string. 中文：新的属性字符串。
     * @return true if a row changed. 中文：若成功更新返回 true。
     * @throws std::runtime_error On update errors. 中文：更新失败抛出异常。
     */
    bool updateUserAttributes(const std::string& username, const std::string& newAttributes);

    /**
     * @brief Delete user by username.
     * 中文：根据用户名删除用户。
     *
     * @param username Target username. 中文：目标用户名。
     * @return true if a row was removed. 中文：若删除成功返回 true。
     * @throws std::runtime_error On delete errors. 中文：删除失败抛出异常。
     */
    bool deleteUser(const std::string& username);

    /**
     * @brief 确保任务表存在，若不存在则创建。
     */
    void ensureTaskTable();

    /**
     * @brief 确保成就表存在，覆盖成就、进度与画廊信息。
     */
    void ensureAchievementTable();

    /**
     * @brief 新建任务记录并返回行号。
     */
    int createTask(const TaskRecord& task);

    /**
     * @brief 新建成就记录（系统或自定义）。
     */
    int createAchievement(const AchievementRecord& record);

    /**
     * @brief 更新任务记录。
     */
    bool updateTask(const TaskRecord& task);

    /**
     * @brief 更新成就记录。
     */
    bool updateAchievement(const AchievementRecord& record);

    /**
     * @brief 根据 ID 删除任务。
     */
    bool deleteTask(int taskId);

    /**
     * @brief 根据 ID 删除成就。
     */
    bool deleteAchievement(int achievementId);

    /**
     * @brief 根据 ID 查询任务。
     */
    [[nodiscard]] std::optional<TaskRecord> getTaskById(int taskId) const;

    /**
     * @brief 查询指定月份内学生创建的奖励型自定义成就数量。
     */
    [[nodiscard]] int countCustomRewardAchievements(const std::string& owner,
                                                    const std::string& monthToken) const;

    /**
     * @brief 获取所有任务记录，用于初始化内存缓存。
     */
    [[nodiscard]] std::vector<TaskRecord> getAllTasks() const;

    /**
     * @brief 根据学生用户名获取其全部成就记录。
     */
    [[nodiscard]] std::vector<AchievementRecord> getAchievementsForOwner(const std::string& owner) const;

    /**
     * @brief Begin explicit transaction.
     * 中文：开启显式事务。
     *
     * @return true 当本次调用启动了新的顶层事务。中文：若当前调用创建了新的最外层事务则返回 true。
     * @throws std::runtime_error On begin failure. 中文：开启事务失败抛出异常。
     */
    bool beginTransaction();

    /**
     * @brief Commit current transaction.
     * 中文：提交当前事务。
     *
     * @return void. 中文：无返回值。
     * @throws std::runtime_error On commit failure. 中文：提交失败抛出异常。
     */
    void commitTransaction();

    /**
     * @brief Roll back current transaction.
     * 中文：回滚当前事务。
     *
     * @return void. 中文：无返回值。
     * @throws std::runtime_error On rollback failure. 中文：回滚失败抛出异常。
     */
    void rollbackTransaction();

    /**
     * @brief Access raw sqlite3 handle.
     * 中文：访问底层 sqlite3 句柄。
     *
     * @return sqlite3 pointer or nullptr. 中文：返回 sqlite3 指针或 nullptr。
     * @throws None. 中文：不抛出异常。
     */
    [[nodiscard]] sqlite3* rawHandle() const noexcept;

private:
    DatabaseManager();
    ~DatabaseManager();

    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;
    DatabaseManager(DatabaseManager&&) = delete;
    DatabaseManager& operator=(DatabaseManager&&) = delete;

    using DatabaseHandle = std::unique_ptr<sqlite3, decltype(&sqlite3_close)>;
    using StatementHandle = std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)>;

    void openDatabase(const std::string& path);
    void closeDatabase() noexcept;
    void executeNonQuery(const std::string& sql);
    [[nodiscard]] StatementHandle prepareStatement(const std::string& sql) const;
    [[nodiscard]] static bool isSuccessCode(int sqliteResult);
    [[nodiscard]] TaskRecord readTaskRecord(sqlite3_stmt* statement) const;
    [[nodiscard]] AchievementRecord readAchievementRecord(sqlite3_stmt* statement) const;

    DatabaseHandle m_db;
    std::string m_databasePath;
    mutable std::recursive_mutex m_mutex;
    bool m_initialized;
    std::size_t m_transactionDepth;
    std::unique_lock<std::recursive_mutex> m_transactionLock;

    inline static const char* kPreconfiguredUsername = "x";
    inline static const char* kPreconfiguredPassword = "1";
};

}  // namespace rove::data

#endif  // DATABASEMANAGER_H
