#ifndef TASK_H
#define TASK_H

#include <QDateTime>

#include <string>

#include "User.h"

namespace rove::data {

/**
 * @class Task
 * @brief 任务数据模型，集中描述“任务卡片”在校园成长系统中的全部属性。
 * 中文：Task 负责封装任务的标识、奖励、难度以及教育意义相关的数据，方便 UI、任务管理器、
 *       数据库之间以统一的结构交互。
 */
class Task final {
public:
    /**
     * @enum TaskType
     * @brief 任务类型区分每日、每周、学期以及自定义，便于应用不同重置策略。
     */
    enum class TaskType { Daily, Weekly, Semester, Custom };

    /**
     * @brief 默认构造保持安全初始状态，便于在读取数据库前占位。
     */
    Task();

    /**
     * @brief 带参构造一次性填充全部关键字段，常用于从数据库反序列化。
     */
    Task(int id,
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
         int progressGoal);

    /**
     * @brief 以下 getter/setter 让上层模块以 const/非 const 方式安全访问字段。
     */
    [[nodiscard]] int id() const noexcept;
    void setId(int id) noexcept;

    [[nodiscard]] const std::string& name() const noexcept;
    void setName(std::string name);

    [[nodiscard]] const std::string& description() const noexcept;
    void setDescription(std::string description);

    [[nodiscard]] TaskType type() const noexcept;
    void setType(TaskType type) noexcept;

    [[nodiscard]] int difficultyStars() const noexcept;
    void setDifficultyStars(int stars);

    [[nodiscard]] const QDateTime& deadline() const noexcept;
    void setDeadline(const QDateTime& deadline);

    [[nodiscard]] bool isCompleted() const noexcept;
    void setCompleted(bool completed) noexcept;

    [[nodiscard]] int coinReward() const noexcept;
    void setCoinReward(int coins);

    [[nodiscard]] int growthReward() const noexcept;
    void setGrowthReward(int growth);

    [[nodiscard]] const User::AttributeSet& attributeReward() const noexcept;
    void setAttributeReward(const User::AttributeSet& reward) noexcept;

    [[nodiscard]] int bonusStreak() const noexcept;
    void setBonusStreak(int streak) noexcept;
    void incrementBonusStreak() noexcept;
    void resetBonusStreak() noexcept;

    [[nodiscard]] int forgivenessCoupons() const noexcept;
    void setForgivenessCoupons(int count) noexcept;

    [[nodiscard]] const std::string& customSettings() const noexcept;
    void setCustomSettings(std::string settings);

    [[nodiscard]] int progressValue() const noexcept;
    void setProgressValue(int value);

    [[nodiscard]] int progressGoal() const noexcept;
    void setProgressGoal(int goal);

    /**
     * @brief 任务是否因为截止时间而失效，供 TaskManager 判断是否需要失败处理。
     */
    [[nodiscard]] bool isExpired(const QDateTime& reference) const noexcept;

    /**
     * @brief 每日任务是否需要重置，用于定时器回调。
     */
    [[nodiscard]] bool requiresDailyReset() const noexcept;

    /**
     * @brief 每周任务是否需要重置。
     */
    [[nodiscard]] bool requiresWeeklyReset() const noexcept;

    /**
     * @brief 针对新周期重置进度与完成标记。
     */
    void resetProgressForNewCycle();

    /**
     * @brief 记录一次失败，必要时消耗宽恕券；返回是否重置连胜。
     */
    bool recordFailure(bool useForgiveness);

    /**
     * @brief 将 TaskType 转换为数据库存储的字符串。
     */
    static std::string typeToString(TaskType type);

    /**
     * @brief 将数据库中的字符串还原为 TaskType。
     */
    static TaskType typeFromString(const std::string& text);

private:
    int m_taskId;
    std::string m_name;
    std::string m_description;
    TaskType m_type;
    int m_difficultyStars;
    QDateTime m_deadline;
    bool m_completed;
    int m_coinReward;
    int m_growthReward;
    User::AttributeSet m_attributeReward;
    int m_bonusStreak;
    int m_forgivenessCoupons;
    std::string m_customSettings;
    int m_progressValue;
    int m_progressGoal;
};

}  // namespace rove::data

#endif  // TASK_H
