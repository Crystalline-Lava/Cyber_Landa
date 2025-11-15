#ifndef USER_H
#define USER_H

#include <string>

namespace rove::data {

/**
 * @class User
 * @brief Rich domain object describing a student inside the campus simulator.
 * 中文：User 类详细描述校园模拟器中的学生实体，保存等级、成长、属性等关键数据。
 *
 * Design note: the class focuses purely on state + deterministic business rules, while persistence
 * is delegated to UserManager/DatabaseManager. 中文：该类只负责状态和业务计算，持久化交由
 * UserManager 与 DatabaseManager 处理，实现单一职责。
 */
class User {
public:
    /**
     * @struct AttributeSet
     * @brief Holds the six educational attributes required by teachers.
     * 中文：AttributeSet 结构用于保存老师要求的六项教育属性。
     */
    struct AttributeSet {
        int execution = 0;       //!< 行动力（Execution），衡量学生将计划落地的能力。
        int perseverance = 0;    //!< 毅力（Perseverance），衡量坚持完成任务的精神。
        int decision = 0;        //!< 决断力（Decision），衡量学生在压力下做出选择的能力。
        int knowledge = 0;       //!< 知识力（Knowledge），衡量理论学习成果。
        int social = 0;          //!< 社交力（Social），衡量沟通协作能力。
        int pride = 0;           //!< 自豪感（Pride），体现对兰大的认同感。

        /**
         * @brief Calculate total points contained in the set.
         * 中文：计算该属性集合包含的总点数。
         * @return Sum of all fields. 中文：返回六项属性之和。
         */
        [[nodiscard]] int totalPoints() const noexcept;

        /**
         * @brief Add another AttributeSet to this one.
         * 中文：将另一组属性加到当前对象上。
         * @param other Source bonuses. 中文：来源属性加成。
         */
        void add(const AttributeSet& other) noexcept;
    };

    /**
     * @struct ProgressStats
     * @brief Records meta progression such as achievements and task completions.
     * 中文：ProgressStats 记录成就与任务完成等元进度信息。
     */
    struct ProgressStats {
        int achievementsUnlocked = 0;    //!< 累计解锁的成就数量。
        int totalTasksCompleted = 0;     //!< 完成的任务总数。
        int academicTasksCompleted = 0;  //!< 完成的学业类任务数量。
        int socialTasksCompleted = 0;    //!< 完成的社交类任务数量。
        int personalTasksCompleted = 0;  //!< 完成的自我成长类任务数量。
        int attributePointsSpent = 0;    //!< 已经花费的属性点数，用于平衡系统。
    };

    /**
     * @enum TaskCategory
     * @brief Categorizes tasks for statistic tracking.
     * 中文：TaskCategory 用于区分任务类别，方便统计。
     */
    enum class TaskCategory { Academic, Social, Personal };

    /**
     * @brief Default constructor keeps object in safe empty state.
     * 中文：默认构造函数让对象处于安全的空状态。
     */
    User();

    /**
     * @brief Full constructor to populate every field.
     * 中文：完整构造函数，初始化所有字段。
     */
    User(int id,
         std::string username,
         std::string password,
         int level,
         int growthPoints,
         int coins,
         const AttributeSet& attributes,
         const ProgressStats& progress);

    User(const User&) = default;
    User& operator=(const User&) = default;
    User(User&&) noexcept = default;
    User& operator=(User&&) noexcept = default;
    ~User() = default;

    /**
     * @brief Get immutable database identifier.
     * 中文：获取数据库主键 ID。
     * @return Database primary key assigned by SQLite。中文：返回 SQLite 分配的主键。
     * @throws None. 中文：不抛出异常。
     */
    [[nodiscard]] int id() const noexcept;

    /**
     * @brief Access username for UI display.
     * 中文：获取用户名，供 UI 显示。
     * @return Constant reference to username string. 中文：返回用户名常量引用。
     * @throws None. 中文：不抛出异常。
     */
    [[nodiscard]] const std::string& username() const noexcept;

    /**
     * @brief Access stored password (demo only, no hashing yet).
     * 中文：获取存储的密码（演示用，未加密）。
     * @return Password string reference. 中文：返回密码字符串引用。
     * @throws None. 中文：不抛出异常。
     */
    [[nodiscard]] const std::string& password() const noexcept;

    /**
     * @brief Replace password when students update credentials.
     * 中文：学生修改密码时调用，更新密码字段。
     * @param newPassword New plain text password. 中文：新的明文密码。
     * @return void. 中文：无返回值。
     * @throws None. 中文：不抛出异常。
     */
    void setPassword(const std::string& newPassword);

    /**
     * @brief Query current level.
     * 中文：获取当前等级。
     * @return Player level (>=1). 中文：返回等级值（至少为 1）。
     * @throws None. 中文：不抛出异常。
     */
    [[nodiscard]] int level() const noexcept;

    /**
     * @brief Manually set level (mainly for admin tools).
     * 中文：手动设置等级，主要用于教师调试。
     * @param level Desired level. 中文：目标等级。
     * @return void. 中文：无返回值。
     * @throws None (value auto clamped). 中文：不抛异常（会自动校正）。
     */
    void setLevel(int level);

    /**
     * @brief Read accumulated growth points.
     * 中文：读取累计成长值。
     * @return Growth points integer. 中文：返回成长值。
     * @throws None. 中文：不抛出异常。
     */
    [[nodiscard]] int growthPoints() const noexcept;

    /**
     * @brief Directly set growth points and auto update level.
     * 中文：直接设置成长值并自动刷新等级。
     * @param points Total growth points. 中文：成长值总量。
     * @return void. 中文：无返回值。
     * @throws None. 中文：不抛出异常。
     */
    void setGrowthPoints(int points);

    /**
     * @brief Add extra growth points when activities conclude.
     * 中文：活动奖励时增加成长值。
     * @param delta Points to add. 中文：要增加的点数。
     * @return void. 中文：无返回值。
     * @throws None. 中文：不抛异常。
     */
    void addGrowthPoints(int delta);

    /**
     * @brief Query coin balance (校园币)。
     * 中文：查询当前校园币数量。
     * @return Coin amount. 中文：返回校园币数量。
     * @throws None. 中文：不抛异常。
     */
    [[nodiscard]] int coins() const noexcept;

    /**
     * @brief Reward coins after tasks or events.
     * 中文：任务或活动后增加校园币。
     * @param amount Coins to add. 中文：要增加的币数。
     * @return void. 中文：无返回值。
     * @throws None. 中文：不抛异常。
     */
    void addCoins(int amount);

    /**
     * @brief Spend coins for store interactions.
     * 中文：在商城等场景下扣除校园币。
     * @param amount Coins to spend. 中文：要消费的币数。
     * @return void. 中文：无返回值。
     * @throws std::runtime_error When balance insufficient. 中文：余额不足时抛异常。
     */
    void spendCoins(int amount);

    /**
     * @brief Read-only access to attribute set.
     * 中文：只读方式获取属性集合。
     * @return const reference to AttributeSet. 中文：返回属性集合常量引用。
     * @throws None. 中文：不抛异常。
     */
    [[nodiscard]] const AttributeSet& attributes() const noexcept;

    /**
     * @brief Replace attribute set (用于导入存档)。
     * 中文：用于导入存档的属性整体设置。
     * @param attributes Serialized values. 中文：新的属性值。
     * @return void. 中文：无返回值。
     * @throws None. 中文：不抛异常。
     */
    void setAttributes(const AttributeSet& attributes);

    /**
     * @brief Apply bonuses from tasks or classes.
     * 中文：应用任务或课程提供的属性加成。
     * @param bonus Attribute deltas. 中文：属性增量。
     * @return void. 中文：无返回值。
     * @throws None. 中文：不抛异常。
     */
    void applyAttributeBonus(const AttributeSet& bonus);

    /**
     * @brief Allocate points from growth-based budget.
     * 中文：根据成长获得的点数手动分配属性。
     * @param distribution Distribution plan. 中文：分配方案。
     * @return void. 中文：无返回值。
     * @throws std::runtime_error When plan exceeds available points. 中文：超额时抛异常。
     */
    void distributeAttributes(const AttributeSet& distribution);

    /**
     * @brief Access progress structure (const).
     * 中文：只读获取进度统计。
     * @return Progress stats reference. 中文：返回进度统计常量引用。
     * @throws None. 中文：不抛异常。
     */
    [[nodiscard]] const ProgressStats& progress() const noexcept;

    /**
     * @brief Mutable access to progress stats for management logic.
     * 中文：提供可写引用，方便管理器做复杂更新。
     * @return Progress stats reference. 中文：返回进度统计可写引用。
     * @throws None. 中文：不抛异常。
     */
    [[nodiscard]] ProgressStats& progress() noexcept;

    /**
     * @brief Record a finished task with category statistics.
     * 中文：记录已完成的任务并按照类别计数。
     * @param category Task type. 中文：任务类别。
     * @return void. 中文：无返回值。
     * @throws None. 中文：不抛异常。
     */
    void recordTaskCompletion(TaskCategory category);

    /**
     * @brief Increase achievement counter。
     * 中文：成就解锁后递增计数。
     * @return void. 中文：无返回值。
     * @throws None. 中文：不抛异常。
     */
    void recordAchievementUnlock();

    /**
     * @brief Compute remaining attribute budget。
     * 中文：计算剩余可用属性点，便于 UI 显示。
     * @return Available points. 中文：可分配点数。
     * @throws None. 中文：不抛异常。
     */
    [[nodiscard]] int computeAvailableAttributePoints() const noexcept;

    /**
     * @brief Describe attribute formula to students.
     * 中文：返回属性摘要字符串，方便 UI 展示。
     * @return Human-readable summary string. 中文：可读性强的属性摘要。
     * @throws None. 中文：不抛异常。
     */
    [[nodiscard]] std::string buildAttributeSummary() const;

    /**
     * @brief Convert growth points into level per teacher formula.
     * 中文：按照老师给出的公式将成长值转换为等级。
     * @param growthPoints Input growth points. 中文：成长值输入。
     * @return Computed level >=1. 中文：返回计算后的等级。
     * @throws None. 中文：不抛异常。
     */
    static int computeLevelFromGrowth(int growthPoints) noexcept;

private:
    /**
     * @brief Update level based on current growth points.
     * 中文：根据最新成长值重新计算等级。
     * @return void. 中文：无返回值。
     */
    void recalculateLevel();

    /**
     * @brief Clamp attributes to safe range.
     * 中文：将属性限制在安全范围内，防止溢出。
     * @return void. 中文：无返回值。
     */
    void clampAttributes();

    /**
     * @brief Helper enforcing attribute bounds.
     * 中文：限制单项属性上下限的辅助函数。
     * @param value Raw attribute. 中文：原始属性值。
     * @return Clamped value. 中文：裁剪后的属性值。
     */
    static int clampAttributeValue(int value) noexcept;

    int m_id = -1;
    std::string m_username;
    std::string m_password;
    int m_level = 1;
    int m_growthPoints = 0;
    int m_coins = 0;
    AttributeSet m_attributes;
    ProgressStats m_progress;
};

}  // namespace rove::data

#endif  // USER_H
