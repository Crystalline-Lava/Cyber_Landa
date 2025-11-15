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
 * @return void. 中文：无返回值。
 * @throws None. 中文：不抛出异常。
 */
DatabaseManager::DatabaseManager()
    : m_db(nullptr, &sqlite3_close), m_databasePath(), m_mutex(), m_initialized(false) {}

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
 * @brief Begin an explicit transaction to guard multiple operations.
 * 中文：开启显式事务以保护多条操作的原子性。
 *
 * Business logic: gameplay events often update level, currency, and logs simultaneously; using BEGIN ensures
 * either all succeed or all fail. 中文：成长事件常需同时更新等级、货币与日志，事务可保证要么全部成功要么全部失败。
 *
 * @return void. 中文：无返回值。
 * @throws std::runtime_error When SQLite cannot start the transaction. 中文：开启事务失败抛出异常。
 */
void DatabaseManager::beginTransaction() {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    executeNonQuery("BEGIN TRANSACTION;");
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
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    executeNonQuery("COMMIT;");
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
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    executeNonQuery("ROLLBACK;");
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

}  // namespace rove::data
