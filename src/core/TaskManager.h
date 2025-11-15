#ifndef TASKMANAGER_H
#define TASKMANAGER_H

#include <QObject>
#include <QTimer>

#include <memory>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <vector>

#include "DatabaseManager.h"
#include "Task.h"
#include "UserManager.h"

namespace rove::data {

/**
 * @class TaskManager
 * @brief 任务系统的核心协调者，负责任务缓存、奖励发放、统计与定时重置。
 * 中文：TaskManager 以单例模式运行，内部维护线程安全的任务缓存，使用 QTimer 自动重置每日/每周
 *       任务，并与 UserManager 联动更新学生成长数据，是“任务-成长-成就”联动的中枢。
 */
class TaskManager final {
public:
    /**
     * @brief 获取单例实例，首次调用时注入 DatabaseManager 与 UserManager 依赖。
     */
    static TaskManager& instance(DatabaseManager& database, UserManager& userManager);

    TaskManager(const TaskManager&) = delete;
    TaskManager& operator=(const TaskManager&) = delete;
    TaskManager(TaskManager&&) = delete;
    TaskManager& operator=(TaskManager&&) = delete;

    /**
     * @brief 创建新任务（含自定义任务），返回数据库分配的 ID。
     */
    int createTask(Task task);

    /**
     * @brief 更新任务信息，用于教师调课或学生修改自定义任务。
     */
    void updateTask(const Task& task);

    /**
     * @brief 删除任务，常用于移除已过期的自定义内容。
     */
    void deleteTask(int taskId);

    /**
     * @brief 按 ID 查询任务详情，便于 UI 展示。
     */
    [[nodiscard]] std::optional<Task> taskById(int taskId) const;

    /**
     * @brief 根据任务类型筛选列表，用于面板分栏显示。
     */
    [[nodiscard]] std::vector<Task> tasksByType(Task::TaskType type) const;

    /**
     * @brief 将任务标记为完成并发放奖励，包含连胜与难度加成。
     */
    void markTaskCompleted(int taskId);

    /**
     * @brief 任务失败时调用，可选择消耗宽恕券避免惩罚。
     */
    void failTask(int taskId, bool useForgiveness);

    /**
     * @brief 更新任务进度（完成次数或百分比），并根据目标判定是否直接完成。
     */
    void updateTaskProgress(int taskId, int delta);

    /**
     * @brief 重新从数据库加载任务，适合老师远程修改后刷新。
     */
    void refreshFromDatabase();

    /**
     * @brief 获取当前统计信息，例如各类型累计完成数。
     */
    [[nodiscard]] std::unordered_map<Task::TaskType, int> taskStatistics() const;

    /**
     * @brief 供 QTimer 调用的每日重置逻辑，公共接口方便单元测试。
     */
    void resetDailyTasks();

    /**
     * @brief 供 QTimer 调用的每周重置逻辑。
     */
    void resetWeeklyTasks();

private:
    TaskManager(DatabaseManager& database, UserManager& userManager);

    void configureTimers();
    void hydrateTasksFromRecords(const std::vector<DatabaseManager::TaskRecord>& records);
    Task hydrateTask(const DatabaseManager::TaskRecord& record) const;
    DatabaseManager::TaskRecord toRecord(const Task& task) const;
    std::string serializeAttributes(const User::AttributeSet& set) const;
    User::AttributeSet deserializeAttributes(const std::string& blob) const;
    void resetTasksByPredicate(Task::TaskType type);
    void applyRewardsLocked(Task& task);
    void enforceSemesterDeadlinesLocked();
    User::TaskCategory mapToUserCategory(Task::TaskType type) const;
    double difficultyFactor(const Task& task) const;
    double streakFactor(const Task& task) const;

    DatabaseManager& m_database;
    UserManager& m_userManager;
    std::unordered_map<int, Task> m_tasks;
    std::unordered_map<Task::TaskType, int> m_completionStats;
    std::unique_ptr<QTimer> m_dailyTimer;
    std::unique_ptr<QTimer> m_weeklyTimer;
    QObject m_timerContext;  //!< 专用 QObject 作为 lambda 上下文，确保 QTimer 超时回调安全注销。
    mutable std::mutex m_mutex;
};

}  // namespace rove::data

#endif  // TASKMANAGER_H
