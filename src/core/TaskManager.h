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

class TaskManagerSignalProxy : public QObject {
    Q_OBJECT

public:
    explicit TaskManagerSignalProxy(QObject* parent = nullptr) : QObject(parent) {}

signals:
    void taskCompleted(int taskId, int taskType, int difficultyStars);
    void taskProgressed(int taskId, int currentValue, int goalValue);
};

class TaskManager final {
public:
    static TaskManager& instance(DatabaseManager& database, UserManager& userManager);

    TaskManager(const TaskManager&) = delete;
    TaskManager& operator=(const TaskManager&) = delete;
    TaskManager(TaskManager&&) = delete;
    TaskManager& operator=(TaskManager&&) = delete;

    int createTask(Task task);
    void updateTask(const Task& task);
    void deleteTask(int taskId);
    [[nodiscard]] std::optional<Task> taskById(int taskId) const;
    [[nodiscard]] std::vector<Task> tasksByType(Task::TaskType type) const;
    void markTaskCompleted(int taskId);
    void failTask(int taskId, bool useForgiveness);
    void updateTaskProgress(int taskId, int delta);
    void refreshFromDatabase();
    [[nodiscard]] std::unordered_map<Task::TaskType, int> taskStatistics() const;
    void resetDailyTasks();
    void resetWeeklyTasks();
    [[nodiscard]] TaskManagerSignalProxy* signalProxy() const noexcept;

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
    QObject m_timerContext;
    std::unique_ptr<TaskManagerSignalProxy> m_signalProxy;
    mutable std::mutex m_mutex;
};

}  // namespace rove::data

#endif  // TASKMANAGER_H
