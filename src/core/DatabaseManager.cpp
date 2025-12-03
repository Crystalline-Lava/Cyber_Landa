#include "DatabaseManager.h"

#include <sstream>
#include <stdexcept>

namespace rove::data {

namespace {
/**
 * @brief Compose a detailed SQLite error message for exception contexts.
 * 中文：为异常上下文拼接 SQLite 错误信息，便于教学演示时清晰说明问题来源。
 *
 * Business logic: converting low-level C strings into std::string keeps exception throwing exception-safe.
 * 中文：将底层 C 字符串转换为 std::string 能保证抛出异常时依旧安全，符合异常安全设计原则。
 *
 * @param prefix Human readable context. 中文：人类可读的描述前缀。
 * @param handle sqlite3 handle for sqlite3_errmsg. 中文：用于调用 sqlite3_errmsg 的句柄。
 * @return Combined message string. 中文：返回组合后的错误消息字符串。
 * @throws None. 中文：不抛出异常。
 */
std::string buildErrorMessage(const std::string& prefix, sqlite3* handle) {
    std::ostringstream oss;
    oss << prefix;
    if (handle != nullptr) {
        oss << " | sqlite: " << sqlite3_errmsg(handle);
    }
    return oss.str();
}
}  // namespace

/**
 * @brief Construct the singleton with a null sqlite handle so RAII can attach later.
 * 中文：构造函数保持 sqlite 句柄为空，等待 initialize() 打开数据库后才交由 RAII 管理。
 *
 * Business logic: delaying actual IO operations avoids unnecessary disk access during program start.
 * 中文：推迟实际 IO 操作可减少程序启动期间的磁盘访问，提高响应速度。
 *
 * @return true 当创建了最外层事务。中文：若当前调用成为最外层事务则返回 true。
 * @throws None. 中文：不抛出异常。
 */
DatabaseManager::DatabaseManager()
    : m_db(nullptr, &sqlite3_close),
      m_databasePath(),
      m_mutex(),
      m_initialized(false),
      m_transactionDepth(0),
      m_transactionLock() {}

/**
 * @brief Destructor closes the connection automatically thanks to std::unique_ptr.
 * 中文：析构函数依赖 std::unique_ptr 自动关闭连接，符合 RAII 原则。
 *
 * Business logic: keeping cleanup centralized means every module can trust DatabaseManager to free
 * resources when the application shuts down. 中文：集中化清理逻辑可确保应用退出时自动释放资源。
 *
 * @return void. 中文：无返回值。
 * @throws None. 中文：不抛出异常。
 */
DatabaseManager::~DatabaseManager() { closeDatabase(); }

/**
 * @brief Retrieve the single global DatabaseManager instance (Singleton pattern).
 * 中文：获取全局唯一的 DatabaseManager 单例，确保所有模块复用同一连接。
 *
 * Business logic: singleton guarantees consistent transaction boundaries and simplifies dependency injection
 * in Qt widgets. 中文：单例保证事务边界一致，简化 Qt 控件的依赖注入。
 *
 * @return Reference to the singleton. 中文：返回单例引用。
 * @throws None. 中文：不抛出异常。
 */
DatabaseManager& DatabaseManager::instance() {
    static DatabaseManager instance;
    return instance;
}

/**
 * @brief Initialize database connection, ensure schema, and seed default data.
 * 中文：初始化数据库连接、确保数据表存在并写入默认数据。
 *
 * Business logic: explicit initialization lets the UI show friendly diagnostics if the path is invalid,
 * and ensures schema creation runs only once. 中文：显式初始化让界面在路径无效时提供提示，并确保建表只执行一次。
 *
 * @param databasePath Path to SQLite file. 中文：SQLite 文件路径。
 * @return void. 中文：无返回值。
 * @throws std::runtime_error When database open or schema bootstrap fails. 中文：打开数据库或建表失败时抛出异常。
 */
void DatabaseManager::initialize(const std::string& databasePath) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    if (m_initialized && m_db != nullptr && databasePath == m_databasePath) {
        // English: Skip redundant open to preserve active connections.
        // 中文：若已连接同一路径则直接返回，保持现有连接和事务。
        return;
    }

    closeDatabase();
    openDatabase(databasePath);
    ensureUserTable();
    ensureTaskTable();
    ensureAchievementTable();
    ensureShopTable();
    ensureInventoryTable();
    ensureLogTable();
    ensureGrowthSnapshotTable();
    m_initialized = true;
}

/**
 * @brief Report whether a valid sqlite3 handle exists.
 * 中文：报告当前是否存在有效 sqlite3 句柄。
 *
 * Business logic: allows UI to gray-out DB-dependent actions when disconnected. 中文：用于界面判断是否禁用依赖数据库的按钮。
 *
 * @return true when connection is alive, false otherwise. 中文：连接有效返回 true，否则 false。
 * @throws None. 中文：不抛出异常。
 */
bool DatabaseManager::isConnected() const noexcept { return m_db != nullptr; }

/**
 * @brief Create the users table if missing and seed the teacher-specified testing account.
 * 中文：若 users 表不存在则创建，并插入教师要求的预置账号。
 *
 * Business logic: centralizing schema creation prevents divergence when multiple developers run migrations
 * on different machines. 中文：集中建表逻辑可避免多开发环境间的数据结构差异。
 *
 * @return void. 中文：无返回值。
 * @throws std::runtime_error On SQL execution failures. 中文：SQL 执行失败时抛出异常。
 */
void DatabaseManager::ensureUserTable() {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    const std::string sql =
        "CREATE TABLE IF NOT EXISTS users ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "username TEXT UNIQUE NOT NULL,"
        "password TEXT NOT NULL,"
        "level INTEGER NOT NULL DEFAULT 1,"
        "currency INTEGER NOT NULL DEFAULT 0,"
        "attributes TEXT NOT NULL DEFAULT '{}'");";

    executeNonQuery(sql);

    const std::string countSql =
        "SELECT COUNT(1) FROM users WHERE username = ? AND password = ?";
    auto stmt = prepareStatement(countSql);
    sqlite3_bind_text(stmt.get(), 1, kPreconfiguredUsername, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt.get(), 2, kPreconfiguredPassword, -1, SQLITE_STATIC);

    int rc = sqlite3_step(stmt.get());
    if (!isSuccessCode(rc)) {
        throw std::runtime_error(buildErrorMessage("Failed to query default account", m_db.get()));
    }

    bool needInsert = sqlite3_column_int(stmt.get(), 0) == 0;
    if (needInsert) {
        const std::string insertSql =
            "INSERT INTO users (username, password, level, currency, attributes) "
            "VALUES (?, ?, 1, 0, '{""initial"":true}')";
        auto insertStmt = prepareStatement(insertSql);
        sqlite3_bind_text(insertStmt.get(), 1, kPreconfiguredUsername, -1, SQLITE_STATIC);
        sqlite3_bind_text(insertStmt.get(), 2, kPreconfiguredPassword, -1, SQLITE_STATIC);
        rc = sqlite3_step(insertStmt.get());
        if (!isSuccessCode(rc)) {
            throw std::runtime_error(buildErrorMessage("Failed to insert default account", m_db.get()));
        }
    }
}

/**
 * @brief 确保任务表存在，集中管理任务系统的持久化结构。
 * 中文：任务表字段涵盖任务类型、难度星级、截止时间、奖励、连胜、宽恕券、进度等信息，
 *       通过集中建表既能满足教师“奖励与难度平衡”的讲解需要，也方便 TaskManager 事务化操作。
 */
void DatabaseManager::ensureTaskTable() {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    const std::string sql =
        "CREATE TABLE IF NOT EXISTS tasks ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "name TEXT NOT NULL,"
        "description TEXT NOT NULL,"
        "type TEXT NOT NULL,"
        "difficulty INTEGER NOT NULL,"
        "deadline TEXT NOT NULL,"
        "completed INTEGER NOT NULL DEFAULT 0,"
        "coin_reward INTEGER NOT NULL DEFAULT 0,"
        "growth_reward INTEGER NOT NULL DEFAULT 0,"
        "attribute_reward TEXT NOT NULL DEFAULT '{}',"
        "bonus_streak INTEGER NOT NULL DEFAULT 0,"
        "custom_settings TEXT NOT NULL DEFAULT '{}',"
        "forgiveness_coupons INTEGER NOT NULL DEFAULT 0,"
        "progress_value INTEGER NOT NULL DEFAULT 0,"
        "progress_goal INTEGER NOT NULL DEFAULT 100");";
    executeNonQuery(sql);
}

void DatabaseManager::ensureAchievementTable() {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    const std::string sql =
        "CREATE TABLE IF NOT EXISTS achievements ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "owner TEXT NOT NULL,"
        "creator TEXT NOT NULL,"
        "name TEXT NOT NULL,"
        "description TEXT NOT NULL,"
        "icon_path TEXT NOT NULL,"
        "display_color TEXT NOT NULL,"
        "type TEXT NOT NULL,"
        "reward_type TEXT NOT NULL,"
        "progress_mode TEXT NOT NULL,"
        "progress_value INTEGER NOT NULL DEFAULT 0,"
        "progress_goal INTEGER NOT NULL DEFAULT 1,"
        "reward_coins INTEGER NOT NULL DEFAULT 0,"
        "reward_attributes TEXT NOT NULL DEFAULT '0,0,0,0,0,0',"
        "reward_items TEXT NOT NULL DEFAULT '',"
        "unlocked INTEGER NOT NULL DEFAULT 0,"
        "completion_time TEXT,"
        "conditions TEXT NOT NULL,"
        "gallery_group TEXT NOT NULL DEFAULT 'default',"
        "created_at TEXT NOT NULL,"
        "special_metadata TEXT NOT NULL DEFAULT ''");";
    executeNonQuery(sql);
}

void DatabaseManager::ensureShopTable() {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    const std::string sql =
        "CREATE TABLE IF NOT EXISTS shop_items ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "name TEXT NOT NULL,"
        "description TEXT NOT NULL,"
        "icon_path TEXT NOT NULL,"
        "item_type TEXT NOT NULL,"
        "price_coins INTEGER NOT NULL,"
        "purchase_limit INTEGER NOT NULL DEFAULT 0,"
        "available INTEGER NOT NULL DEFAULT 1,"
        "effect_description TEXT NOT NULL DEFAULT '',"
        "effect_logic TEXT NOT NULL DEFAULT '',"
        "prop_effect_type TEXT NOT NULL DEFAULT '',"
        "prop_duration_minutes INTEGER NOT NULL DEFAULT 0,"
        "usage_conditions TEXT NOT NULL DEFAULT '',"
        "physical_redeem TEXT NOT NULL DEFAULT '',"
        "physical_notes TEXT NOT NULL DEFAULT '',"
        "lucky_rules TEXT NOT NULL DEFAULT '{}',"
        "level_requirement INTEGER NOT NULL DEFAULT 1);";
    executeNonQuery(sql);
    executeNonQuery("CREATE INDEX IF NOT EXISTS idx_shop_items_type ON shop_items(item_type);");
}

void DatabaseManager::ensureInventoryTable() {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    const std::string sql =
        "CREATE TABLE IF NOT EXISTS user_inventory ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "owner TEXT NOT NULL,"
        "item_id INTEGER NOT NULL,"
        "quantity INTEGER NOT NULL,"
        "used_quantity INTEGER NOT NULL DEFAULT 0,"
        "status TEXT NOT NULL,"
        "purchase_time TEXT NOT NULL,"
        "expiration_time TEXT,"
        "lucky_payload TEXT NOT NULL DEFAULT '{}',"
        "notes TEXT NOT NULL DEFAULT '',"
        "FOREIGN KEY(owner) REFERENCES users(username) ON DELETE CASCADE,"
        "FOREIGN KEY(item_id) REFERENCES shop_items(id) ON DELETE CASCADE);";
    executeNonQuery(sql);
    executeNonQuery("CREATE INDEX IF NOT EXISTS idx_inventory_owner ON user_inventory(owner);");
    executeNonQuery("CREATE INDEX IF NOT EXISTS idx_inventory_item ON user_inventory(item_id);");
}


/**
 * @brief 确保日志表存在并建立必要索引。
 * 中文：记录自动、手动、里程碑与事件日志，为时间线过滤提供结构。
 */
void DatabaseManager::ensureLogTable() {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    const std::string sql =
        "CREATE TABLE IF NOT EXISTS logs (\n"
        "id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
        "timestamp TEXT NOT NULL,\n"
        "type TEXT NOT NULL,\n"
        "content TEXT NOT NULL,\n"
        "related_id INTEGER,\n"
        "attribute_changes TEXT NOT NULL DEFAULT '{}',\n"
        "level_change INTEGER NOT NULL DEFAULT 0,\n"
        "special_event TEXT NOT NULL DEFAULT '',\n"
        "mood TEXT NOT NULL DEFAULT ''"
        " );";
    executeNonQuery(sql);
    executeNonQuery("CREATE INDEX IF NOT EXISTS idx_logs_timestamp ON logs(timestamp);");
    executeNonQuery("CREATE INDEX IF NOT EXISTS idx_logs_type ON logs(type);");
}

/**
 * @brief 确保成长快照表存在，支撑时间轴可视化。
 * 中文：存储等级、成长值与属性，方便绘制折线和雷达图。
 */
void DatabaseManager::ensureGrowthSnapshotTable() {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    const std::string sql =
        "CREATE TABLE IF NOT EXISTS growth_snapshots (\n"
        "id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
        "timestamp TEXT NOT NULL,\n"
        "user_level INTEGER NOT NULL,\n"
        "growth_points INTEGER NOT NULL,\n"
        "execution INTEGER NOT NULL,\n"
        "perseverance INTEGER NOT NULL,\n"
        "decision INTEGER NOT NULL,\n"
        "knowledge INTEGER NOT NULL,\n"
        "social INTEGER NOT NULL,\n"
        "pride INTEGER NOT NULL,\n"
        "achievement_count INTEGER NOT NULL,\n"
        "completed_tasks INTEGER NOT NULL,\n"
        "failed_tasks INTEGER NOT NULL,\n"
        "manual_log_count INTEGER NOT NULL"
        " );";
    executeNonQuery(sql);
    executeNonQuery("CREATE INDEX IF NOT EXISTS idx_growth_snapshots_timestamp ON growth_snapshots(timestamp);");
}

/**
 * @brief Validate credentials specifically against the preconfigured "x"/"1" account.
 * 中文：专门验证预置的用户名 "x" 与密码 "1"。
 *
 * Business logic: teacher demo requires this credential; centralizing validation avoids hard-coded copies.
 * 中文：教师演示只认该账号，集中校验可避免多处硬编码导致维护困难。
 *
 * @param username Supplied username. 中文：输入的用户名。
 * @param password Supplied password. 中文：输入的密码。
 * @return true if credentials match stored row. 中文：匹配成功返回 true。
 * @throws std::runtime_error When SQLite query fails. 中文：查询失败时抛出异常。
 */
bool DatabaseManager::validatePreconfiguredAccount(const std::string& username,
                                                    const std::string& password) const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    if (username != kPreconfiguredUsername || password != kPreconfiguredPassword) {
        return false;
    }

    const std::string sql =
        "SELECT COUNT(1) FROM users WHERE username = ? AND password = ?";
    auto stmt = prepareStatement(sql);
    sqlite3_bind_text(stmt.get(), 1, username.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), 2, password.c_str(), -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmt.get());
    if (!isSuccessCode(rc)) {
        throw std::runtime_error(buildErrorMessage("Failed to validate preconfigured account", m_db.get()));
    }
    return sqlite3_column_int(stmt.get(), 0) > 0;
}

/**
 * @brief Insert a new user record and return the generated row id.
 * 中文：插入新的用户记录，并返回 SQLite 生成的行号。
 *
 * Business logic: returning the row id helps subsequent modules (inventory, quests) link foreign keys.
 * 中文：返回行号可让任务、背包等模块建立外键关联。
 *
 * @param user User data to store (id ignored). 中文：要保存的用户数据（忽略 id）。
 * @return Generated row id. 中文：生成的行号。
 * @throws std::runtime_error When insert fails. 中文：插入失败抛出异常。
 */
int DatabaseManager::createUser(const UserRecord& user) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    const std::string sql =
        "INSERT INTO users (username, password, level, currency, attributes) VALUES (?, ?, ?, ?, ?)";
    auto stmt = prepareStatement(sql);
    sqlite3_bind_text(stmt.get(), 1, user.username.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), 2, user.password.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt.get(), 3, user.level);
    sqlite3_bind_int(stmt.get(), 4, user.currency);
    sqlite3_bind_text(stmt.get(), 5, user.attributes.c_str(), -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmt.get());
    if (rc != SQLITE_DONE) {
        throw std::runtime_error(buildErrorMessage("Failed to create user", m_db.get()));
    }
    return static_cast<int>(sqlite3_last_insert_rowid(m_db.get()));
}

/**
 * @brief Retrieve a user row by username.
 * 中文：根据用户名查询用户记录。
 *
 * Business logic: optional return type clearly communicates the possibility of a missing user.
 * 中文：使用 std::optional 可明确表达“可能不存在”的语义。
 *
 * @param username Target username. 中文：目标用户名。
 * @return Optional user record. 中文：若找到则返回用户记录，否则 std::nullopt。
 * @throws std::runtime_error When SQLite reports an error. 中文：查询出错时抛出异常。
 */
std::optional<DatabaseManager::UserRecord> DatabaseManager::getUserByName(
    const std::string& username) const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    const std::string sql =
        "SELECT id, username, password, level, currency, attributes FROM users WHERE username = ?";
    auto stmt = prepareStatement(sql);
    sqlite3_bind_text(stmt.get(), 1, username.c_str(), -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmt.get());
    if (rc == SQLITE_ROW) {
        UserRecord record;
        record.id = sqlite3_column_int(stmt.get(), 0);
        record.username = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 1));
        record.password = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 2));
        record.level = sqlite3_column_int(stmt.get(), 3);
        record.currency = sqlite3_column_int(stmt.get(), 4);
        record.attributes = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 5));
        return record;
    }
    if (rc == SQLITE_DONE) {
        return std::nullopt;
    }
    throw std::runtime_error(buildErrorMessage("Failed to query user", m_db.get()));
}

/**
 * @brief Update the level column for a user.
 * 中文：更新指定用户的等级字段。
 *
 * Business logic: splitting updates by field keeps UI actions granular and simple to audit in logs.
 * 中文：按字段拆分更新，便于界面精细控制并在日志中记录操作。
 *
 * @param username Target username. 中文：目标用户名。
 * @param newLevel Desired level. 中文：新等级。
 * @return true if a row changed. 中文：若有行被更新返回 true。
 * @throws std::runtime_error When SQLite update fails. 中文：更新失败抛出异常。
 */
bool DatabaseManager::updateUserLevel(const std::string& username, int newLevel) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    const std::string sql = "UPDATE users SET level = ? WHERE username = ?";
    auto stmt = prepareStatement(sql);
    sqlite3_bind_int(stmt.get(), 1, newLevel);
    sqlite3_bind_text(stmt.get(), 2, username.c_str(), -1, SQLITE_TRANSIENT);
    int rc = sqlite3_step(stmt.get());
    if (rc != SQLITE_DONE) {
        throw std::runtime_error(buildErrorMessage("Failed to update user level", m_db.get()));
    }
    return sqlite3_changes(m_db.get()) > 0;
}

/**
 * @brief Update the currency column for a user.
 * 中文：更新指定用户的货币字段。
 *
 * Business logic: isolating currency updates simplifies implementing economic audits later.
 * 中文：单独的货币更新接口便于未来实现经济系统审计。
 *
 * @param username Target username. 中文：目标用户名。
 * @param newCurrency Desired currency. 中文：新的货币值。
 * @return true if a row changed. 中文：若有记录被更新返回 true。
 * @throws std::runtime_error When update fails. 中文：更新失败抛出异常。
 */
bool DatabaseManager::updateUserCurrency(const std::string& username, int newCurrency) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    const std::string sql = "UPDATE users SET currency = ? WHERE username = ?";
    auto stmt = prepareStatement(sql);
    sqlite3_bind_int(stmt.get(), 1, newCurrency);
    sqlite3_bind_text(stmt.get(), 2, username.c_str(), -1, SQLITE_TRANSIENT);
    int rc = sqlite3_step(stmt.get());
    if (rc != SQLITE_DONE) {
        throw std::runtime_error(buildErrorMessage("Failed to update user currency", m_db.get()));
    }
    return sqlite3_changes(m_db.get()) > 0;
}

/**
 * @brief Update serialized attribute data for a user.
 * 中文：更新用户的序列化属性数据。
 *
 * Business logic: attributes may store growth stats; keeping this as a blob allows flexible schema evolution.
 * 中文：属性字段可能存储成长数据，使用文本 blob 有助于未来灵活扩展。
 *
 * @param username Target username. 中文：目标用户名。
 * @param newAttributes Serialized attribute string. 中文：新的序列化属性字符串。
 * @return true if a row changed. 中文：若有行被更新返回 true。
 * @throws std::runtime_error When update fails. 中文：更新失败抛出异常。
 */
bool DatabaseManager::updateUserAttributes(const std::string& username,
                                           const std::string& newAttributes) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    const std::string sql = "UPDATE users SET attributes = ? WHERE username = ?";
    auto stmt = prepareStatement(sql);
    sqlite3_bind_text(stmt.get(), 1, newAttributes.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), 2, username.c_str(), -1, SQLITE_TRANSIENT);
    int rc = sqlite3_step(stmt.get());
    if (rc != SQLITE_DONE) {
        throw std::runtime_error(buildErrorMessage("Failed to update user attributes", m_db.get()));
    }
    return sqlite3_changes(m_db.get()) > 0;
}

/**
 * @brief Delete a user row by username.
 * 中文：按用户名删除用户记录。
 *
 * Business logic: this helper allows the admin UI to reset test data quickly without writing SQL manually.
 * 中文：该接口让管理界面可以快速清理测试数据，无需手写 SQL。
 *
 * @param username Target username. 中文：目标用户名。
 * @return true if a row was removed. 中文：若删除了一行返回 true。
 * @throws std::runtime_error When delete fails. 中文：删除失败抛出异常。
 */
bool DatabaseManager::deleteUser(const std::string& username) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    const std::string sql = "DELETE FROM users WHERE username = ?";
    auto stmt = prepareStatement(sql);
    sqlite3_bind_text(stmt.get(), 1, username.c_str(), -1, SQLITE_TRANSIENT);
    int rc = sqlite3_step(stmt.get());
    if (rc != SQLITE_DONE) {
        throw std::runtime_error(buildErrorMessage("Failed to delete user", m_db.get()));
    }
    return sqlite3_changes(m_db.get()) > 0;
}

/**
 * @brief 将 TaskRecord 插入数据库并返回行号。
 * 中文：任务创建流程统一走此接口，便于在 UI 中调用后得到主键用于后续编辑。
 */
int DatabaseManager::createTask(const TaskRecord& task) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    const std::string sql =
        "INSERT INTO tasks (name, description, type, difficulty, deadline, completed, coin_reward, "
        "growth_reward, attribute_reward, bonus_streak, custom_settings, forgiveness_coupons, "
        "progress_value, progress_goal) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
    auto stmt = prepareStatement(sql);
    sqlite3_bind_text(stmt.get(), 1, task.name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), 2, task.description.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), 3, task.type.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt.get(), 4, task.difficulty);
    sqlite3_bind_text(stmt.get(), 5, task.deadlineIso.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt.get(), 6, task.completed ? 1 : 0);
    sqlite3_bind_int(stmt.get(), 7, task.coinReward);
    sqlite3_bind_int(stmt.get(), 8, task.growthReward);
    sqlite3_bind_text(stmt.get(), 9, task.attributeReward.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt.get(), 10, task.bonusStreak);
    sqlite3_bind_text(stmt.get(), 11, task.customSettings.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt.get(), 12, task.forgivenessCoupons);
    sqlite3_bind_int(stmt.get(), 13, task.progressValue);
    sqlite3_bind_int(stmt.get(), 14, task.progressGoal);

    int rc = sqlite3_step(stmt.get());
    if (rc != SQLITE_DONE) {
        throw std::runtime_error(buildErrorMessage("Failed to insert task", m_db.get()));
    }
    return static_cast<int>(sqlite3_last_insert_rowid(m_db.get()));
}

int DatabaseManager::createAchievement(const AchievementRecord& record) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    const std::string sql =
        "INSERT INTO achievements (owner, creator, name, description, icon_path, display_color, type, "
        "reward_type, progress_mode, progress_value, progress_goal, reward_coins, reward_attributes, "
        "reward_items, unlocked, completion_time, conditions, gallery_group, created_at, special_metadata) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
    auto stmt = prepareStatement(sql);
    sqlite3_bind_text(stmt.get(), 1, record.owner.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), 2, record.creator.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), 3, record.name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), 4, record.description.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), 5, record.iconPath.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), 6, record.color.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), 7, record.type.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), 8, record.rewardType.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), 9, record.progressMode.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt.get(), 10, record.progressValue);
    sqlite3_bind_int(stmt.get(), 11, record.progressGoal);
    sqlite3_bind_int(stmt.get(), 12, record.rewardCoins);
    sqlite3_bind_text(stmt.get(), 13, record.rewardAttributes.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), 14, record.rewardItems.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt.get(), 15, record.unlocked ? 1 : 0);
    if (record.completionTime.empty()) {
        sqlite3_bind_null(stmt.get(), 16);
    } else {
        sqlite3_bind_text(stmt.get(), 16, record.completionTime.c_str(), -1, SQLITE_TRANSIENT);
    }
    sqlite3_bind_text(stmt.get(), 17, record.conditions.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), 18, record.galleryGroup.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), 19, record.createdAt.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), 20, record.specialMetadata.c_str(), -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmt.get());
    if (rc != SQLITE_DONE) {
        throw std::runtime_error(buildErrorMessage("Failed to create achievement", m_db.get()));
    }
    return static_cast<int>(sqlite3_last_insert_rowid(m_db.get()));
}

/**
 * @brief 根据 TaskRecord 更新数据库行，保持内存缓存与磁盘一致。
 * 中文：所有字段一次性写回，保证教师强调的数据一致性。
 */
bool DatabaseManager::updateTask(const TaskRecord& task) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    const std::string sql =
        "UPDATE tasks SET name = ?, description = ?, type = ?, difficulty = ?, deadline = ?, completed = ?, "
        "coin_reward = ?, growth_reward = ?, attribute_reward = ?, bonus_streak = ?, custom_settings = ?, "
        "forgiveness_coupons = ?, progress_value = ?, progress_goal = ? WHERE id = ?";
    auto stmt = prepareStatement(sql);
    sqlite3_bind_text(stmt.get(), 1, task.name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), 2, task.description.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), 3, task.type.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt.get(), 4, task.difficulty);
    sqlite3_bind_text(stmt.get(), 5, task.deadlineIso.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt.get(), 6, task.completed ? 1 : 0);
    sqlite3_bind_int(stmt.get(), 7, task.coinReward);
    sqlite3_bind_int(stmt.get(), 8, task.growthReward);
    sqlite3_bind_text(stmt.get(), 9, task.attributeReward.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt.get(), 10, task.bonusStreak);
    sqlite3_bind_text(stmt.get(), 11, task.customSettings.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt.get(), 12, task.forgivenessCoupons);
    sqlite3_bind_int(stmt.get(), 13, task.progressValue);
    sqlite3_bind_int(stmt.get(), 14, task.progressGoal);
    sqlite3_bind_int(stmt.get(), 15, task.id);
    int rc = sqlite3_step(stmt.get());
    if (rc != SQLITE_DONE) {
        throw std::runtime_error(buildErrorMessage("Failed to update task", m_db.get()));
    }
    return sqlite3_changes(m_db.get()) > 0;
}

bool DatabaseManager::updateAchievement(const AchievementRecord& record) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    const std::string sql =
        "UPDATE achievements SET owner = ?, creator = ?, name = ?, description = ?, icon_path = ?, "
        "display_color = ?, type = ?, reward_type = ?, progress_mode = ?, progress_value = ?, "
        "progress_goal = ?, reward_coins = ?, reward_attributes = ?, reward_items = ?, unlocked = ?, "
        "completion_time = ?, conditions = ?, gallery_group = ?, created_at = ?, special_metadata = ? "
        "WHERE id = ?";
    auto stmt = prepareStatement(sql);
    sqlite3_bind_text(stmt.get(), 1, record.owner.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), 2, record.creator.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), 3, record.name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), 4, record.description.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), 5, record.iconPath.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), 6, record.color.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), 7, record.type.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), 8, record.rewardType.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), 9, record.progressMode.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt.get(), 10, record.progressValue);
    sqlite3_bind_int(stmt.get(), 11, record.progressGoal);
    sqlite3_bind_int(stmt.get(), 12, record.rewardCoins);
    sqlite3_bind_text(stmt.get(), 13, record.rewardAttributes.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), 14, record.rewardItems.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt.get(), 15, record.unlocked ? 1 : 0);
    if (record.completionTime.empty()) {
        sqlite3_bind_null(stmt.get(), 16);
    } else {
        sqlite3_bind_text(stmt.get(), 16, record.completionTime.c_str(), -1, SQLITE_TRANSIENT);
    }
    sqlite3_bind_text(stmt.get(), 17, record.conditions.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), 18, record.galleryGroup.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), 19, record.createdAt.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), 20, record.specialMetadata.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt.get(), 21, record.id);

    int rc = sqlite3_step(stmt.get());
    if (rc != SQLITE_DONE) {
        throw std::runtime_error(buildErrorMessage("Failed to update achievement", m_db.get()));
    }
    return sqlite3_changes(m_db.get()) > 0;
}

/**
 * @brief 根据任务 ID 删除记录。
 * 中文：提供统一删除入口，便于 TaskManager 在清理自定义任务时使用。
 */
bool DatabaseManager::deleteTask(int taskId) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    const std::string sql = "DELETE FROM tasks WHERE id = ?";
    auto stmt = prepareStatement(sql);
    sqlite3_bind_int(stmt.get(), 1, taskId);
    int rc = sqlite3_step(stmt.get());
    if (rc != SQLITE_DONE) {
        throw std::runtime_error(buildErrorMessage("Failed to delete task", m_db.get()));
    }
    return sqlite3_changes(m_db.get()) > 0;
}

bool DatabaseManager::deleteAchievement(int achievementId) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    const std::string sql = "DELETE FROM achievements WHERE id = ?";
    auto stmt = prepareStatement(sql);
    sqlite3_bind_int(stmt.get(), 1, achievementId);
    int rc = sqlite3_step(stmt.get());
    if (rc != SQLITE_DONE) {
        throw std::runtime_error(buildErrorMessage("Failed to delete achievement", m_db.get()));
    }
    return sqlite3_changes(m_db.get()) > 0;
}

/**
 * @brief 根据主键查询任务。
 * 中文：配合 TaskManager::taskById 进行按需加载。
 */
std::optional<DatabaseManager::TaskRecord> DatabaseManager::getTaskById(int taskId) const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    const std::string sql =
        "SELECT id, name, description, type, difficulty, deadline, completed, coin_reward, growth_reward, "
        "attribute_reward, bonus_streak, custom_settings, forgiveness_coupons, progress_value, progress_goal "
        "FROM tasks WHERE id = ?";
    auto stmt = prepareStatement(sql);
    sqlite3_bind_int(stmt.get(), 1, taskId);
    int rc = sqlite3_step(stmt.get());
    if (rc == SQLITE_ROW) {
        return readTaskRecord(stmt.get());
    }
    if (rc == SQLITE_DONE) {
        return std::nullopt;
    }
    throw std::runtime_error(buildErrorMessage("Failed to query task", m_db.get()));
}

/**
 * @brief 获取全部任务记录，供启动阶段初始化缓存。
 * 中文：一次性读取能减少频繁往返数据库，提高 UI 响应速度。
 */
std::vector<DatabaseManager::TaskRecord> DatabaseManager::getAllTasks() const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    const std::string sql =
        "SELECT id, name, description, type, difficulty, deadline, completed, coin_reward, growth_reward, "
        "attribute_reward, bonus_streak, custom_settings, forgiveness_coupons, progress_value, progress_goal "
        "FROM tasks";
    auto stmt = prepareStatement(sql);
    std::vector<TaskRecord> records;
    while (true) {
        int rc = sqlite3_step(stmt.get());
        if (rc == SQLITE_ROW) {
            records.push_back(readTaskRecord(stmt.get()));
            continue;
        }
        if (rc == SQLITE_DONE) {
            break;
        }
        throw std::runtime_error(buildErrorMessage("Failed to read task list", m_db.get()));
    }
    return records;
}

std::vector<DatabaseManager::AchievementRecord> DatabaseManager::getAchievementsForOwner(
    const std::string& owner) const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    const std::string sql =
        "SELECT id, owner, creator, name, description, icon_path, display_color, type, reward_type, "
        "progress_mode, progress_value, progress_goal, reward_coins, reward_attributes, reward_items, "
        "unlocked, completion_time, conditions, gallery_group, created_at, special_metadata "
        "FROM achievements WHERE owner = ? ORDER BY id";
    auto stmt = prepareStatement(sql);
    sqlite3_bind_text(stmt.get(), 1, owner.c_str(), -1, SQLITE_TRANSIENT);
    std::vector<AchievementRecord> records;
    while (true) {
        int rc = sqlite3_step(stmt.get());
        if (rc == SQLITE_ROW) {
            records.push_back(readAchievementRecord(stmt.get()));
            continue;
        }
        if (rc == SQLITE_DONE) {
            break;
        }
        throw std::runtime_error(buildErrorMessage("Failed to read achievements", m_db.get()));
    }
    return records;
}

int DatabaseManager::insertShopItem(const ShopItemRecord& record) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    const std::string sql =
        "INSERT INTO shop_items (name, description, icon_path, item_type, price_coins, purchase_limit, "
        "available, effect_description, effect_logic, prop_effect_type, prop_duration_minutes, "
        "usage_conditions, physical_redeem, physical_notes, lucky_rules, level_requirement) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
    auto stmt = prepareStatement(sql);
    sqlite3_bind_text(stmt.get(), 1, record.name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), 2, record.description.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), 3, record.iconPath.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), 4, record.itemType.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt.get(), 5, record.priceCoins);
    sqlite3_bind_int(stmt.get(), 6, record.purchaseLimit);
    sqlite3_bind_int(stmt.get(), 7, record.available ? 1 : 0);
    sqlite3_bind_text(stmt.get(), 8, record.effectDescription.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), 9, record.effectLogic.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), 10, record.propEffectType.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt.get(), 11, record.propDurationMinutes);
    sqlite3_bind_text(stmt.get(), 12, record.usageConditions.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), 13, record.physicalRedeem.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), 14, record.physicalNotes.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), 15, record.luckyBagRules.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt.get(), 16, record.levelRequirement);
    int rc = sqlite3_step(stmt.get());
    if (!isSuccessCode(rc)) {
        throw std::runtime_error(buildErrorMessage("Failed to insert shop item", m_db.get()));
    }
    return static_cast<int>(sqlite3_last_insert_rowid(m_db.get()));
}

bool DatabaseManager::updateShopItem(const ShopItemRecord& record) {
    if (record.id < 0) {
        throw std::runtime_error("Invalid shop item id");
    }
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    const std::string sql =
        "UPDATE shop_items SET name = ?, description = ?, icon_path = ?, item_type = ?, price_coins = ?, "
        "purchase_limit = ?, available = ?, effect_description = ?, effect_logic = ?, prop_effect_type = ?, "
        "prop_duration_minutes = ?, usage_conditions = ?, physical_redeem = ?, physical_notes = ?, "
        "lucky_rules = ?, level_requirement = ? WHERE id = ?";
    auto stmt = prepareStatement(sql);
    sqlite3_bind_text(stmt.get(), 1, record.name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), 2, record.description.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), 3, record.iconPath.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), 4, record.itemType.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt.get(), 5, record.priceCoins);
    sqlite3_bind_int(stmt.get(), 6, record.purchaseLimit);
    sqlite3_bind_int(stmt.get(), 7, record.available ? 1 : 0);
    sqlite3_bind_text(stmt.get(), 8, record.effectDescription.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), 9, record.effectLogic.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), 10, record.propEffectType.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt.get(), 11, record.propDurationMinutes);
    sqlite3_bind_text(stmt.get(), 12, record.usageConditions.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), 13, record.physicalRedeem.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), 14, record.physicalNotes.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), 15, record.luckyBagRules.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt.get(), 16, record.levelRequirement);
    sqlite3_bind_int(stmt.get(), 17, record.id);
    int rc = sqlite3_step(stmt.get());
    if (!isSuccessCode(rc)) {
        throw std::runtime_error(buildErrorMessage("Failed to update shop item", m_db.get()));
    }
    return sqlite3_changes(m_db.get()) > 0;
}

bool DatabaseManager::deleteShopItem(int itemId) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    const std::string sql = "DELETE FROM shop_items WHERE id = ?";
    auto stmt = prepareStatement(sql);
    sqlite3_bind_int(stmt.get(), 1, itemId);
    int rc = sqlite3_step(stmt.get());
    if (!isSuccessCode(rc)) {
        throw std::runtime_error(buildErrorMessage("Failed to delete shop item", m_db.get()));
    }
    return sqlite3_changes(m_db.get()) > 0;
}

std::optional<DatabaseManager::ShopItemRecord> DatabaseManager::getShopItemById(int itemId) const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    const std::string sql =
        "SELECT id, name, description, icon_path, item_type, price_coins, purchase_limit, available, "
        "effect_description, effect_logic, prop_effect_type, prop_duration_minutes, usage_conditions, "
        "physical_redeem, physical_notes, lucky_rules, level_requirement FROM shop_items WHERE id = ?";
    auto stmt = prepareStatement(sql);
    sqlite3_bind_int(stmt.get(), 1, itemId);
    int rc = sqlite3_step(stmt.get());
    if (rc == SQLITE_ROW) {
        return readShopItemRecord(stmt.get());
    }
    if (rc == SQLITE_DONE) {
        return std::nullopt;
    }
    throw std::runtime_error(buildErrorMessage("Failed to query shop item", m_db.get()));
}

std::vector<DatabaseManager::ShopItemRecord> DatabaseManager::getAllShopItems() const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    const std::string sql =
        "SELECT id, name, description, icon_path, item_type, price_coins, purchase_limit, available, "
        "effect_description, effect_logic, prop_effect_type, prop_duration_minutes, usage_conditions, "
        "physical_redeem, physical_notes, lucky_rules, level_requirement FROM shop_items ORDER BY id";
    auto stmt = prepareStatement(sql);
    std::vector<ShopItemRecord> items;
    while (true) {
        int rc = sqlite3_step(stmt.get());
        if (rc == SQLITE_ROW) {
            items.push_back(readShopItemRecord(stmt.get()));
            continue;
        }
        if (rc == SQLITE_DONE) {
            break;
        }
        throw std::runtime_error(buildErrorMessage("Failed to read shop items", m_db.get()));
    }
    return items;
}

int DatabaseManager::insertInventoryRecord(const InventoryRecord& record) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    const std::string sql =
        "INSERT INTO user_inventory (owner, item_id, quantity, used_quantity, status, purchase_time, "
        "expiration_time, lucky_payload, notes) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)";
    auto stmt = prepareStatement(sql);
    sqlite3_bind_text(stmt.get(), 1, record.owner.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt.get(), 2, record.itemId);
    sqlite3_bind_int(stmt.get(), 3, record.quantity);
    sqlite3_bind_int(stmt.get(), 4, record.usedQuantity);
    sqlite3_bind_text(stmt.get(), 5, record.status.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), 6, record.purchaseTimeIso.c_str(), -1, SQLITE_TRANSIENT);
    if (record.expirationTimeIso.empty()) {
        sqlite3_bind_null(stmt.get(), 7);
    } else {
        sqlite3_bind_text(stmt.get(), 7, record.expirationTimeIso.c_str(), -1, SQLITE_TRANSIENT);
    }
    sqlite3_bind_text(stmt.get(), 8, record.luckyPayload.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), 9, record.notes.c_str(), -1, SQLITE_TRANSIENT);
    int rc = sqlite3_step(stmt.get());
    if (!isSuccessCode(rc)) {
        throw std::runtime_error(buildErrorMessage("Failed to insert inventory", m_db.get()));
    }
    return static_cast<int>(sqlite3_last_insert_rowid(m_db.get()));
}

bool DatabaseManager::updateInventoryRecord(const InventoryRecord& record) {
    if (record.id < 0) {
        throw std::runtime_error("Invalid inventory id");
    }
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    const std::string sql =
        "UPDATE user_inventory SET owner = ?, item_id = ?, quantity = ?, used_quantity = ?, status = ?, "
        "purchase_time = ?, expiration_time = ?, lucky_payload = ?, notes = ? WHERE id = ?";
    auto stmt = prepareStatement(sql);
    sqlite3_bind_text(stmt.get(), 1, record.owner.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt.get(), 2, record.itemId);
    sqlite3_bind_int(stmt.get(), 3, record.quantity);
    sqlite3_bind_int(stmt.get(), 4, record.usedQuantity);
    sqlite3_bind_text(stmt.get(), 5, record.status.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), 6, record.purchaseTimeIso.c_str(), -1, SQLITE_TRANSIENT);
    if (record.expirationTimeIso.empty()) {
        sqlite3_bind_null(stmt.get(), 7);
    } else {
        sqlite3_bind_text(stmt.get(), 7, record.expirationTimeIso.c_str(), -1, SQLITE_TRANSIENT);
    }
    sqlite3_bind_text(stmt.get(), 8, record.luckyPayload.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), 9, record.notes.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt.get(), 10, record.id);
    int rc = sqlite3_step(stmt.get());
    if (!isSuccessCode(rc)) {
        throw std::runtime_error(buildErrorMessage("Failed to update inventory", m_db.get()));
    }
    return sqlite3_changes(m_db.get()) > 0;
}

bool DatabaseManager::deleteInventoryRecord(int inventoryId) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    const std::string sql = "DELETE FROM user_inventory WHERE id = ?";
    auto stmt = prepareStatement(sql);
    sqlite3_bind_int(stmt.get(), 1, inventoryId);
    int rc = sqlite3_step(stmt.get());
    if (!isSuccessCode(rc)) {
        throw std::runtime_error(buildErrorMessage("Failed to delete inventory", m_db.get()));
    }
    return sqlite3_changes(m_db.get()) > 0;
}

std::optional<DatabaseManager::InventoryRecord> DatabaseManager::getInventoryRecordById(int inventoryId) const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    const std::string sql =
        "SELECT id, item_id, owner, quantity, used_quantity, status, purchase_time, expiration_time, "
        "lucky_payload, notes FROM user_inventory WHERE id = ?";
    auto stmt = prepareStatement(sql);
    sqlite3_bind_int(stmt.get(), 1, inventoryId);
    int rc = sqlite3_step(stmt.get());
    if (rc == SQLITE_ROW) {
        return readInventoryRecord(stmt.get());
    }
    if (rc == SQLITE_DONE) {
        return std::nullopt;
    }
    throw std::runtime_error(buildErrorMessage("Failed to query inventory", m_db.get()));
}

std::vector<DatabaseManager::InventoryRecord> DatabaseManager::getInventoryForUser(const std::string& owner) const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    const std::string sql =
        "SELECT id, item_id, owner, quantity, used_quantity, status, purchase_time, expiration_time, "
        "lucky_payload, notes FROM user_inventory WHERE owner = ? ORDER BY purchase_time DESC";
    auto stmt = prepareStatement(sql);
    sqlite3_bind_text(stmt.get(), 1, owner.c_str(), -1, SQLITE_TRANSIENT);
    std::vector<InventoryRecord> records;
    while (true) {
        int rc = sqlite3_step(stmt.get());
        if (rc == SQLITE_ROW) {
            records.push_back(readInventoryRecord(stmt.get()));
            continue;
        }
        if (rc == SQLITE_DONE) {
            break;
        }
        throw std::runtime_error(buildErrorMessage("Failed to list inventory", m_db.get()));
    }
    return records;
}

std::vector<DatabaseManager::InventoryRecord> DatabaseManager::getAllInventoryRecords() const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    const std::string sql =
        "SELECT id, item_id, owner, quantity, used_quantity, status, purchase_time, expiration_time, "
        "lucky_payload, notes FROM user_inventory";
    auto stmt = prepareStatement(sql);
    std::vector<InventoryRecord> records;
    while (true) {
        int rc = sqlite3_step(stmt.get());
        if (rc == SQLITE_ROW) {
            records.push_back(readInventoryRecord(stmt.get()));
            continue;
        }
        if (rc == SQLITE_DONE) {
            break;
        }
        throw std::runtime_error(buildErrorMessage("Failed to scan inventory", m_db.get()));
    }
    return records;
}

int DatabaseManager::countInventoryByUserAndItem(const std::string& owner, int itemId) const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    const std::string sql =
        "SELECT IFNULL(SUM(quantity), 0) FROM user_inventory WHERE owner = ? AND item_id = ?";
    auto stmt = prepareStatement(sql);
    sqlite3_bind_text(stmt.get(), 1, owner.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt.get(), 2, itemId);
    int rc = sqlite3_step(stmt.get());
    if (!isSuccessCode(rc)) {
        throw std::runtime_error(buildErrorMessage("Failed to count inventory", m_db.get()));
    }
    return sqlite3_column_int(stmt.get(), 0);
}

int DatabaseManager::countCustomRewardAchievements(const std::string& owner,
                                                   const std::string& monthToken) const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    const std::string sql =
        "SELECT COUNT(1) FROM achievements WHERE owner = ? AND type = 'Custom' AND reward_type = 'WithReward' "
        "AND strftime('%Y-%m', created_at) = ?";
    auto stmt = prepareStatement(sql);
    sqlite3_bind_text(stmt.get(), 1, owner.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), 2, monthToken.c_str(), -1, SQLITE_TRANSIENT);
    int rc = sqlite3_step(stmt.get());
    if (!isSuccessCode(rc)) {
        throw std::runtime_error(buildErrorMessage("Failed to count achievements", m_db.get()));
    }
    return sqlite3_column_int(stmt.get(), 0);
}

/**
 * @brief 插入一条日志记录，遵循不可变设计。
 * 中文：所有日志写入后不可更新或删除，仅追加。
 */
int DatabaseManager::insertLogRecord(const LogRecord& record) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    const std::string sql =
        "INSERT INTO logs (timestamp, type, content, related_id, attribute_changes, level_change, special_event, mood) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?)";
    auto stmt = prepareStatement(sql);
    sqlite3_bind_text(stmt.get(), 1, record.timestampIso.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), 2, record.type.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), 3, record.content.c_str(), -1, SQLITE_TRANSIENT);
    if (record.relatedId.has_value()) {
        sqlite3_bind_int(stmt.get(), 4, *record.relatedId);
    } else {
        sqlite3_bind_null(stmt.get(), 4);
    }
    sqlite3_bind_text(stmt.get(), 5, record.attributeChanges.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt.get(), 6, record.levelChange);
    sqlite3_bind_text(stmt.get(), 7, record.specialEvent.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), 8, record.mood.c_str(), -1, SQLITE_TRANSIENT);
    int rc = sqlite3_step(stmt.get());
    if (rc != SQLITE_DONE) {
        throw std::runtime_error(buildErrorMessage("Failed to insert log record", m_db.get()));
    }
    return static_cast<int>(sqlite3_last_insert_rowid(m_db.get()));
}

/**
 * @brief 按过滤条件读取日志，使用时间索引提升性能。
 * 中文：结合类型、时间、心情与关键字进行筛选。
 */
std::vector<DatabaseManager::LogRecord> DatabaseManager::queryLogRecords(const std::optional<std::string>& typeFilter,
                                                                       const std::optional<std::string>& startIso,
                                                                       const std::optional<std::string>& endIso,
                                                                       const std::optional<std::string>& moodFilter,
                                                                       const std::optional<std::string>& keyword) const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    std::string sql = "SELECT id, timestamp, type, content, related_id, attribute_changes, level_change, special_event, mood FROM logs WHERE 1=1";
    std::vector<std::string> params;
    if (typeFilter.has_value()) {
        sql += " AND type = ?";
        params.push_back(*typeFilter);
    }
    if (startIso.has_value()) {
        sql += " AND timestamp >= ?";
        params.push_back(*startIso);
    }
    if (endIso.has_value()) {
        sql += " AND timestamp <= ?";
        params.push_back(*endIso);
    }
    if (moodFilter.has_value()) {
        sql += " AND mood = ?";
        params.push_back(*moodFilter);
    }
    if (keyword.has_value()) {
        sql += " AND content LIKE ?";
        params.push_back(std::string("%") + *keyword + "%");
    }
    sql += " ORDER BY timestamp ASC";
    auto stmt = prepareStatement(sql);
    for (std::size_t i = 0; i < params.size(); ++i) {
        sqlite3_bind_text(stmt.get(), static_cast<int>(i + 1), params[i].c_str(), -1, SQLITE_TRANSIENT);
    }
    std::vector<LogRecord> records;
    while (true) {
        int rc = sqlite3_step(stmt.get());
        if (rc == SQLITE_ROW) {
            records.push_back(readLogRecord(stmt.get()));
            continue;
        }
        if (rc == SQLITE_DONE) {
            break;
        }
        throw std::runtime_error(buildErrorMessage("Failed to query logs", m_db.get()));
    }
    return records;
}

/**
 * @brief 写入成长快照。
 * 中文：在关键事件或定时任务后调用，捕获成长曲线。
 */
int DatabaseManager::insertGrowthSnapshot(const GrowthSnapshotRecord& record) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    const std::string sql =
        "INSERT INTO growth_snapshots (timestamp, user_level, growth_points, execution, perseverance, decision, knowledge, social, pride, achievement_count, completed_tasks, failed_tasks, manual_log_count) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
    auto stmt = prepareStatement(sql);
    sqlite3_bind_text(stmt.get(), 1, record.timestampIso.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt.get(), 2, record.userLevel);
    sqlite3_bind_int(stmt.get(), 3, record.growthPoints);
    sqlite3_bind_int(stmt.get(), 4, record.execution);
    sqlite3_bind_int(stmt.get(), 5, record.perseverance);
    sqlite3_bind_int(stmt.get(), 6, record.decision);
    sqlite3_bind_int(stmt.get(), 7, record.knowledge);
    sqlite3_bind_int(stmt.get(), 8, record.social);
    sqlite3_bind_int(stmt.get(), 9, record.pride);
    sqlite3_bind_int(stmt.get(), 10, record.achievementCount);
    sqlite3_bind_int(stmt.get(), 11, record.completedTasks);
    sqlite3_bind_int(stmt.get(), 12, record.failedTasks);
    sqlite3_bind_int(stmt.get(), 13, record.manualLogCount);
    int rc = sqlite3_step(stmt.get());
    if (rc != SQLITE_DONE) {
        throw std::runtime_error(buildErrorMessage("Failed to insert growth snapshot", m_db.get()));
    }
    return static_cast<int>(sqlite3_last_insert_rowid(m_db.get()));
}

/**
 * @brief 查询成长快照。
 * 中文：可按时间区间提取用于可视化和压缩。
 */
std::vector<DatabaseManager::GrowthSnapshotRecord> DatabaseManager::queryGrowthSnapshots(const std::optional<std::string>& startIso,
                                                                                     const std::optional<std::string>& endIso) const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    std::string sql = "SELECT id, timestamp, user_level, growth_points, execution, perseverance, decision, knowledge, social, pride, achievement_count, completed_tasks, failed_tasks, manual_log_count FROM growth_snapshots WHERE 1=1";
    std::vector<std::string> params;
    if (startIso.has_value()) {
        sql += " AND timestamp >= ?";
        params.push_back(*startIso);
    }
    if (endIso.has_value()) {
        sql += " AND timestamp <= ?";
        params.push_back(*endIso);
    }
    sql += " ORDER BY timestamp ASC";
    auto stmt = prepareStatement(sql);
    for (std::size_t i = 0; i < params.size(); ++i) {
        sqlite3_bind_text(stmt.get(), static_cast<int>(i + 1), params[i].c_str(), -1, SQLITE_TRANSIENT);
    }
    std::vector<GrowthSnapshotRecord> records;
    while (true) {
        int rc = sqlite3_step(stmt.get());
        if (rc == SQLITE_ROW) {
            records.push_back(readGrowthSnapshotRecord(stmt.get()));
            continue;
        }
        if (rc == SQLITE_DONE) {
            break;
        }
        throw std::runtime_error(buildErrorMessage("Failed to query growth snapshots", m_db.get()));
    }
    return records;
}

/**
 * @brief Begin an explicit transaction to guard multiple operations.
 * 中文：开启显式事务以保护多条操作的原子性。
 *
 * Business logic: gameplay events often update level, currency, and logs simultaneously; using BEGIN ensures
 * either all succeed or all fail. 中文：成长事件常需同时更新等级、货币与日志，事务可保证要么全部成功要么全部失败。
 *
 * @return void. 中文：无返回值。
 * @throws std::runtime_error When SQLite cannot start the transaction. 中文：开启事务失败抛出异常。
 */
bool DatabaseManager::beginTransaction() {
    if (!m_transactionLock.owns_lock()) {
        m_transactionLock = std::unique_lock<std::recursive_mutex>(m_mutex);
        executeNonQuery("BEGIN TRANSACTION;");
        m_transactionDepth = 1;
        return true;
    }
    ++m_transactionDepth;
    return false;
}

/**
 * @brief Commit the active transaction.
 * 中文：提交当前事务。
 *
 * Business logic: committing here keeps ownership of transactional lifecycle inside DatabaseManager, so other
 * modules don't have to send raw SQL strings. 中文：在此提交可让其他模块无需直接写 SQL 即可控制事务生命周期。
 *
 * @return void. 中文：无返回值。
 * @throws std::runtime_error When commit fails. 中文：提交失败抛出异常。
 */
void DatabaseManager::commitTransaction() {
    if (!m_transactionLock.owns_lock()) {
        throw std::runtime_error("No active transaction to commit");
    }
    if (m_transactionDepth == 0) {
        throw std::runtime_error("Transaction depth mismatch on commit");
    }
    if (m_transactionDepth == 1) {
        executeNonQuery("COMMIT;");
        m_transactionDepth = 0;
        m_transactionLock.unlock();
        return;
    }
    --m_transactionDepth;
}

/**
 * @brief Roll back the active transaction, undoing uncommitted changes.
 * 中文：回滚当前事务，撤销尚未提交的修改。
 *
 * Business logic: by exposing rollback we can safely abort when gameplay validation fails mid-way.
 * 中文：暴露回滚功能后，当业务校验失败时可安全终止。
 *
 * @return void. 中文：无返回值。
 * @throws std::runtime_error When rollback fails. 中文：回滚失败抛出异常。
 */
void DatabaseManager::rollbackTransaction() {
    if (!m_transactionLock.owns_lock()) {
        return;  // 中文：若当前没有事务则无需处理，防止双重回滚导致崩溃。
    }
    executeNonQuery("ROLLBACK;");
    m_transactionDepth = 0;
    m_transactionLock.unlock();
}

/**
 * @brief Expose raw sqlite3 handle for rare advanced scenarios (e.g., analytics queries).
 * 中文：在极少数高级场景下（统计分析等）暴露底层 sqlite3 句柄。
 *
 * Business logic: providing a controlled escape hatch avoids duplicate singleton implementations.
 * 中文：提供受控的“后门”避免重复实现单例逻辑。
 *
 * @return sqlite3 pointer or nullptr. 中文：返回 sqlite3 指针或 nullptr。
 * @throws None. 中文：不抛出异常。
 */
sqlite3* DatabaseManager::rawHandle() const noexcept { return m_db.get(); }

/**
 * @brief Open database file and register its handle in std::unique_ptr for RAII.
 * 中文：打开数据库文件并交由 std::unique_ptr 托管，确保异常时自动释放。
 *
 * Business logic: isolating open logic lets us reuse it whenever we support runtime database switching.
 * 中文：独立的打开逻辑方便未来支持运行时切换数据库。
 *
 * @param path Filesystem path to SQLite file. 中文：SQLite 文件路径。
 * @return void. 中文：无返回值。
 * @throws std::runtime_error When sqlite3_open fails. 中文：打开失败抛出异常。
 */
void DatabaseManager::openDatabase(const std::string& path) {
    sqlite3* rawHandle = nullptr;
    int rc = sqlite3_open(path.c_str(), &rawHandle);
    if (rc != SQLITE_OK) {
        std::string message = buildErrorMessage("Failed to open database", rawHandle);
        if (rawHandle != nullptr) {
            sqlite3_close(rawHandle);
        }
        throw std::runtime_error(message);
    }
    m_db.reset(rawHandle);
    m_databasePath = path;
}

/**
 * @brief Close the sqlite handle safely.
 * 中文：安全关闭 sqlite 句柄。
 *
 * Business logic: resetting unique_ptr keeps destructor idempotent, so repeated shutdown attempts are safe.
 * 中文：重置 unique_ptr 让析构函数可多次调用而不会崩溃。
 *
 * @return void. 中文：无返回值。
 * @throws None. 中文：不抛出异常。
 */
void DatabaseManager::closeDatabase() noexcept {
    if (m_db != nullptr) {
        m_db.reset();
    }
    m_initialized = false;
}

/**
 * @brief Execute SQL commands that do not return result sets (DDL / DML without rows).
 * 中文：执行不返回结果集的 SQL 语句（如 DDL 或无结果集的 DML）。
 *
 * Business logic: encapsulating error handling here keeps transaction helpers simple and consistent.
 * 中文：封装错误处理可让事务相关方法保持简洁并风格统一。
 *
 * @param sql Command string. 中文：要执行的 SQL 字符串。
 * @return void. 中文：无返回值。
 * @throws std::runtime_error When sqlite3_exec reports an error. 中文：执行失败抛出异常。
 */
void DatabaseManager::executeNonQuery(const std::string& sql) {
    char* errorMessage = nullptr;
    int rc = sqlite3_exec(m_db.get(), sql.c_str(), nullptr, nullptr, &errorMessage);
    if (rc != SQLITE_OK) {
        std::string message = sql;
        if (errorMessage != nullptr) {
            message += " | sqlite: ";
            message += errorMessage;
            sqlite3_free(errorMessage);
        }
        throw std::runtime_error(buildErrorMessage(message, m_db.get()));
    }
}

/**
 * @brief Prepare a sqlite3_stmt and wrap it with std::unique_ptr for automatic finalization.
 * 中文：准备 sqlite3_stmt，并用 std::unique_ptr 在作用域结束时自动调用 sqlite3_finalize。
 *
 * Business logic: RAII statements prevent leaks even when exceptions propagate to the Qt event loop.
 * 中文：RAII 语句即使在异常回到 Qt 事件循环时也不会泄露句柄。
 *
 * @param sql SQL statement text. 中文：SQL 语句文本。
 * @return Managed StatementHandle. 中文：返回自动管理的 StatementHandle。
 * @throws std::runtime_error When database is not initialized or prepare fails. 中文：数据库未初始化或准备失败时抛出异常。
 */
DatabaseManager::StatementHandle DatabaseManager::prepareStatement(const std::string& sql) const {
    if (!m_db) {
        throw std::runtime_error("Database is not initialized");
    }
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(m_db.get(), sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        throw std::runtime_error(buildErrorMessage("Failed to prepare statement", m_db.get()));
    }
    return StatementHandle(stmt, &sqlite3_finalize);
}

/**
 * @brief Helper to detect SQLite success codes.
 * 中文：用于统一判断 SQLite 返回码是否代表成功的辅助函数。
 *
 * Business logic: centralizing the comparison avoids typo-prone magic numbers across the file.
 * 中文：集中判断可避免全文件散落的魔法数字。
 *
 * @param sqliteResult SQLite return code. 中文：SQLite 返回码。
 * @return true if code represents success. 中文：若表示成功则返回 true。
 * @throws None. 中文：不抛出异常。
 */
bool DatabaseManager::isSuccessCode(int sqliteResult) {
    return sqliteResult == SQLITE_DONE || sqliteResult == SQLITE_ROW || sqliteResult == SQLITE_OK;
}

DatabaseManager::TaskRecord DatabaseManager::readTaskRecord(sqlite3_stmt* statement) const {
    TaskRecord record;
    record.id = sqlite3_column_int(statement, 0);
    record.name = reinterpret_cast<const char*>(sqlite3_column_text(statement, 1));
    record.description = reinterpret_cast<const char*>(sqlite3_column_text(statement, 2));
    record.type = reinterpret_cast<const char*>(sqlite3_column_text(statement, 3));
    record.difficulty = sqlite3_column_int(statement, 4);
    record.deadlineIso = reinterpret_cast<const char*>(sqlite3_column_text(statement, 5));
    record.completed = sqlite3_column_int(statement, 6) != 0;
    record.coinReward = sqlite3_column_int(statement, 7);
    record.growthReward = sqlite3_column_int(statement, 8);
    record.attributeReward = reinterpret_cast<const char*>(sqlite3_column_text(statement, 9));
    record.bonusStreak = sqlite3_column_int(statement, 10);
    record.customSettings = reinterpret_cast<const char*>(sqlite3_column_text(statement, 11));
    record.forgivenessCoupons = sqlite3_column_int(statement, 12);
    record.progressValue = sqlite3_column_int(statement, 13);
    record.progressGoal = sqlite3_column_int(statement, 14);
    return record;
}

DatabaseManager::AchievementRecord DatabaseManager::readAchievementRecord(sqlite3_stmt* statement) const {
    AchievementRecord record;
    record.id = sqlite3_column_int(statement, 0);
    record.owner = reinterpret_cast<const char*>(sqlite3_column_text(statement, 1));
    record.creator = reinterpret_cast<const char*>(sqlite3_column_text(statement, 2));
    record.name = reinterpret_cast<const char*>(sqlite3_column_text(statement, 3));
    record.description = reinterpret_cast<const char*>(sqlite3_column_text(statement, 4));
    record.iconPath = reinterpret_cast<const char*>(sqlite3_column_text(statement, 5));
    record.color = reinterpret_cast<const char*>(sqlite3_column_text(statement, 6));
    record.type = reinterpret_cast<const char*>(sqlite3_column_text(statement, 7));
    record.rewardType = reinterpret_cast<const char*>(sqlite3_column_text(statement, 8));
    record.progressMode = reinterpret_cast<const char*>(sqlite3_column_text(statement, 9));
    record.progressValue = sqlite3_column_int(statement, 10);
    record.progressGoal = sqlite3_column_int(statement, 11);
    record.rewardCoins = sqlite3_column_int(statement, 12);
    record.rewardAttributes = reinterpret_cast<const char*>(sqlite3_column_text(statement, 13));
    record.rewardItems = reinterpret_cast<const char*>(sqlite3_column_text(statement, 14));
    record.unlocked = sqlite3_column_int(statement, 15) != 0;
    if (sqlite3_column_type(statement, 16) != SQLITE_NULL) {
        record.completionTime = reinterpret_cast<const char*>(sqlite3_column_text(statement, 16));
    }
    record.conditions = reinterpret_cast<const char*>(sqlite3_column_text(statement, 17));
    record.galleryGroup = reinterpret_cast<const char*>(sqlite3_column_text(statement, 18));
    record.createdAt = reinterpret_cast<const char*>(sqlite3_column_text(statement, 19));
    record.specialMetadata = reinterpret_cast<const char*>(sqlite3_column_text(statement, 20));
    return record;
}

DatabaseManager::ShopItemRecord DatabaseManager::readShopItemRecord(sqlite3_stmt* statement) const {
    ShopItemRecord record;
    record.id = sqlite3_column_int(statement, 0);
    auto readText = [](sqlite3_stmt* stmt, int column) -> std::string {
        const unsigned char* text = sqlite3_column_text(stmt, column);
        if (text == nullptr) {
            return {};
        }
        return reinterpret_cast<const char*>(text);
    };
    record.name = readText(statement, 1);
    record.description = readText(statement, 2);
    record.iconPath = readText(statement, 3);
    record.itemType = readText(statement, 4);
    record.priceCoins = sqlite3_column_int(statement, 5);
    record.purchaseLimit = sqlite3_column_int(statement, 6);
    record.available = sqlite3_column_int(statement, 7) != 0;
    record.effectDescription = readText(statement, 8);
    record.effectLogic = readText(statement, 9);
    record.propEffectType = readText(statement, 10);
    record.propDurationMinutes = sqlite3_column_int(statement, 11);
    record.usageConditions = readText(statement, 12);
    record.physicalRedeem = readText(statement, 13);
    record.physicalNotes = readText(statement, 14);
    record.luckyBagRules = readText(statement, 15);
    record.levelRequirement = sqlite3_column_int(statement, 16);
    return record;
}

DatabaseManager::InventoryRecord DatabaseManager::readInventoryRecord(sqlite3_stmt* statement) const {
    InventoryRecord record;
    record.id = sqlite3_column_int(statement, 0);
    record.itemId = sqlite3_column_int(statement, 1);
    const unsigned char* ownerText = sqlite3_column_text(statement, 2);
    record.owner = ownerText == nullptr ? std::string() : reinterpret_cast<const char*>(ownerText);
    record.quantity = sqlite3_column_int(statement, 3);
    record.usedQuantity = sqlite3_column_int(statement, 4);
    const unsigned char* statusText = sqlite3_column_text(statement, 5);
    record.status = statusText == nullptr ? std::string() : reinterpret_cast<const char*>(statusText);
    const unsigned char* purchaseText = sqlite3_column_text(statement, 6);
    record.purchaseTimeIso = purchaseText == nullptr ? std::string() : reinterpret_cast<const char*>(purchaseText);
    const unsigned char* expirationText = sqlite3_column_text(statement, 7);
    record.expirationTimeIso = expirationText == nullptr ? std::string() : reinterpret_cast<const char*>(expirationText);
    const unsigned char* luckyText = sqlite3_column_text(statement, 8);
    record.luckyPayload = luckyText == nullptr ? std::string() : reinterpret_cast<const char*>(luckyText);
    const unsigned char* notesText = sqlite3_column_text(statement, 9);
    record.notes = notesText == nullptr ? std::string() : reinterpret_cast<const char*>(notesText);
    return record;

/**
 * @brief 反序列化 SQLite 行为内存日志记录。
 */
DatabaseManager::LogRecord DatabaseManager::readLogRecord(sqlite3_stmt* statement) const {
    LogRecord record;
    record.id = sqlite3_column_int(statement, 0);
    const unsigned char* ts = sqlite3_column_text(statement, 1);
    record.timestampIso = ts == nullptr ? std::string() : reinterpret_cast<const char*>(ts);
    const unsigned char* typeText = sqlite3_column_text(statement, 2);
    record.type = typeText == nullptr ? std::string() : reinterpret_cast<const char*>(typeText);
    const unsigned char* contentText = sqlite3_column_text(statement, 3);
    record.content = contentText == nullptr ? std::string() : reinterpret_cast<const char*>(contentText);
    if (sqlite3_column_type(statement, 4) != SQLITE_NULL) {
        record.relatedId = sqlite3_column_int(statement, 4);
    }
    const unsigned char* attrText = sqlite3_column_text(statement, 5);
    record.attributeChanges = attrText == nullptr ? std::string() : reinterpret_cast<const char*>(attrText);
    record.levelChange = sqlite3_column_int(statement, 6);
    const unsigned char* specialText = sqlite3_column_text(statement, 7);
    record.specialEvent = specialText == nullptr ? std::string() : reinterpret_cast<const char*>(specialText);
    const unsigned char* moodText = sqlite3_column_text(statement, 8);
    record.mood = moodText == nullptr ? std::string() : reinterpret_cast<const char*>(moodText);
    return record;
}

/**
 * @brief 反序列化成长快照行。
 */
DatabaseManager::GrowthSnapshotRecord DatabaseManager::readGrowthSnapshotRecord(sqlite3_stmt* statement) const {
    GrowthSnapshotRecord record;
    record.id = sqlite3_column_int(statement, 0);
    const unsigned char* ts = sqlite3_column_text(statement, 1);
    record.timestampIso = ts == nullptr ? std::string() : reinterpret_cast<const char*>(ts);
    record.userLevel = sqlite3_column_int(statement, 2);
    record.growthPoints = sqlite3_column_int(statement, 3);
    record.execution = sqlite3_column_int(statement, 4);
    record.perseverance = sqlite3_column_int(statement, 5);
    record.decision = sqlite3_column_int(statement, 6);
    record.knowledge = sqlite3_column_int(statement, 7);
    record.social = sqlite3_column_int(statement, 8);
    record.pride = sqlite3_column_int(statement, 9);
    record.achievementCount = sqlite3_column_int(statement, 10);
    record.completedTasks = sqlite3_column_int(statement, 11);
    record.failedTasks = sqlite3_column_int(statement, 12);
    record.manualLogCount = sqlite3_column_int(statement, 13);
    return record;
}
}

}  // namespace rove::data
