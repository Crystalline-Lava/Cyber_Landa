#include "TaskManager.h"

#include <QDate>
#include <QString>
#include <QtGlobal>

#include <algorithm>
#include <cmath>
#include <sstream>
#include <stdexcept>

namespace rove::data {

namespace {
constexpr int kDayMilliseconds = 24 * 60 * 60 * 1000;
}

TaskManager& TaskManager::instance(DatabaseManager& database, UserManager& userManager) {
    static TaskManager instance(database, userManager);
    return instance;
}

TaskManager::TaskManager(DatabaseManager& database, UserManager& userManager)
    : m_database(database),
      m_userManager(userManager),
      m_tasks(),
      m_completionStats(),
      m_dailyTimer(),
      m_weeklyTimer(),
      m_timerContext(),
      m_mutex() {
    m_dailyTimer = std::make_unique<QTimer>();
    m_weeklyTimer = std::make_unique<QTimer>();
    refreshFromDatabase();
    configureTimers();
}

/**
 * @brief 配置每日/每周 QTimer，自动触发重置逻辑。
 * 中文：Qt 定时器保证在主线程安全执行，避免多线程直接操纵任务缓存，满足“使用 QTimer 实现重置”的要求。
 */
void TaskManager::configureTimers() {
    if (m_dailyTimer) {
        m_dailyTimer->setInterval(kDayMilliseconds);
        m_dailyTimer->setTimerType(Qt::VeryCoarseTimer);
        QObject::connect(m_dailyTimer.get(), &QTimer::timeout, &m_timerContext, [this]() { resetDailyTasks(); });
        m_dailyTimer->start();
    }
    if (m_weeklyTimer) {
        m_weeklyTimer->setInterval(kDayMilliseconds);
        m_weeklyTimer->setTimerType(Qt::VeryCoarseTimer);
        QObject::connect(m_weeklyTimer.get(), &QTimer::timeout, &m_timerContext, [this]() {
            if (QDate::currentDate().dayOfWeek() == 1) {
                resetWeeklyTasks();
            }
        });
        m_weeklyTimer->start();
    }
}

/**
 * @brief 创建任务时，先写入数据库再更新内存缓存，保证 ID 与持久化一致。
 * 中文：此处持有互斥锁，确保在并发环境中不会出现重复插入或 ID 冲突。
 */
int TaskManager::createTask(Task task) {
    std::lock_guard<std::mutex> lock(m_mutex);
    DatabaseManager::TaskRecord record = toRecord(task);
    const int newId = m_database.createTask(record);
    task.setId(newId);
    m_tasks[newId] = task;
    if (task.isCompleted()) {
        m_completionStats[task.type()] += 1;
    }
    return newId;
}

/**
 * @brief 更新任务信息，适用于老师调整难度、奖励或截止时间。
 * 中文：若任务不存在立即抛异常，让 UI 层提示用户防止误操作。
 */
void TaskManager::updateTask(const Task& task) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (task.id() <= 0 || m_tasks.find(task.id()) == m_tasks.end()) {
        throw std::runtime_error("Task not found");
    }
    m_tasks[task.id()] = task;
    m_database.updateTask(toRecord(task));
}

/**
 * @brief 删除任务的统一入口，确保数据库与缓存同步删除。
 */
void TaskManager::deleteTask(int taskId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_database.deleteTask(taskId);
    m_tasks.erase(taskId);
}

/**
 * @brief 提供按 ID 查询的线程安全方法，方便 UI 或脚本读取详情。
 */
std::optional<Task> TaskManager::taskById(int taskId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_tasks.find(taskId);
    if (it == m_tasks.end()) {
        return std::nullopt;
    }
    return it->second;
}

/**
 * @brief 根据类型筛选任务，用于 Daily/Weekly/Semester 分栏展示。
 */
std::vector<Task> TaskManager::tasksByType(Task::TaskType type) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<Task> result;
    for (const auto& [id, task] : m_tasks) {
        if (task.type() == type) {
            result.push_back(task);
        }
    }
    return result;
}

/**
 * @brief 标记任务完成并分发奖励，内部调用 applyRewardsLocked 完成事务保护。
 */
void TaskManager::markTaskCompleted(int taskId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_tasks.find(taskId);
    if (it == m_tasks.end()) {
        throw std::runtime_error("Task not found");
    }
    if (it->second.isCompleted()) {
        return;
    }
    applyRewardsLocked(it->second);
}

/**
 * @brief 任务失败处理逻辑，支持使用“宽恕券”保留连胜。
 * 中文：当学生因客观原因无法完成任务时，老师提供的宽恕券可以避免连胜清零；若仍失败，则重置进度，
 *       并立刻持久化，保证数据一致。
 */
void TaskManager::failTask(int taskId, bool useForgiveness) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_tasks.find(taskId);
    if (it == m_tasks.end()) {
        throw std::runtime_error("Task not found");
    }
    it->second.recordFailure(useForgiveness);
    m_database.updateTask(toRecord(it->second));
}

/**
 * @brief 更新任务进度，达到目标后自动触发完成，体现“进度跟踪与统计”。
 */
void TaskManager::updateTaskProgress(int taskId, int delta) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_tasks.find(taskId);
    if (it == m_tasks.end()) {
        throw std::runtime_error("Task not found");
    }
    const int newValue = std::clamp(it->second.progressValue() + delta, 0, it->second.progressGoal());
    it->second.setProgressValue(newValue);
    if (newValue >= it->second.progressGoal()) {
        applyRewardsLocked(it->second);
        return;
    }
    m_database.updateTask(toRecord(it->second));
}

/**
 * @brief 从数据库重新加载任务，便于多端协作或教师远程干预后保持一致性。
 */
void TaskManager::refreshFromDatabase() {
    auto records = m_database.getAllTasks();
    std::lock_guard<std::mutex> lock(m_mutex);
    hydrateTasksFromRecords(records);
}

/**
 * @brief 返回当前统计信息副本，让 UI 可以在不加锁的情况下展示完成度。
 */
std::unordered_map<Task::TaskType, int> TaskManager::taskStatistics() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_completionStats;
}

/**
 * @brief 每日重置逻辑：对所有日常任务重置进度、必要时清空连胜，并再次检查学期任务是否过期。
 */
void TaskManager::resetDailyTasks() {
    std::lock_guard<std::mutex> lock(m_mutex);
    resetTasksByPredicate(Task::TaskType::Daily);
    enforceSemesterDeadlinesLocked();
}

/**
 * @brief 每周重置逻辑，只在周一触发，保持周任务节奏感。
 */
void TaskManager::resetWeeklyTasks() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (QDate::currentDate().dayOfWeek() != 1) {
        return;
    }
    resetTasksByPredicate(Task::TaskType::Weekly);
    enforceSemesterDeadlinesLocked();
}

/**
 * @brief 将数据库记录灌入内存缓存，同时重建统计数据。
 * 中文：应用启动时一次性装载，既保证 UI 快速响应，也避免频繁访问磁盘。
 */
void TaskManager::hydrateTasksFromRecords(const std::vector<DatabaseManager::TaskRecord>& records) {
    m_tasks.clear();
    m_completionStats.clear();
    for (const auto& record : records) {
        Task task = hydrateTask(record);
        if (task.isCompleted()) {
            m_completionStats[task.type()] += 1;
        }
        m_tasks.emplace(task.id(), task);
    }
}

/**
 * @brief 将 TaskRecord 还原为领域对象，包含截止时间、奖励等字段。
 */
Task TaskManager::hydrateTask(const DatabaseManager::TaskRecord& record) const {
    QDateTime deadline = QDateTime::fromString(QString::fromStdString(record.deadlineIso), Qt::ISODate);
    if (!deadline.isValid()) {
        deadline = QDateTime::currentDateTimeUtc();
    }
    return Task(record.id,
                record.name,
                record.description,
                Task::typeFromString(record.type),
                record.difficulty,
                deadline,
                record.completed,
                record.coinReward,
                record.growthReward,
                deserializeAttributes(record.attributeReward),
                record.bonusStreak,
                record.forgivenessCoupons,
                record.customSettings,
                record.progressValue,
                record.progressGoal);
}

/**
 * @brief 将领域对象序列化为 TaskRecord，以便写入 SQLite。
 */
DatabaseManager::TaskRecord TaskManager::toRecord(const Task& task) const {
    DatabaseManager::TaskRecord record;
    record.id = task.id();
    record.name = task.name();
    record.description = task.description();
    record.type = Task::typeToString(task.type());
    record.difficulty = task.difficultyStars();
    record.deadlineIso = task.deadline().toString(Qt::ISODate).toStdString();
    record.completed = task.isCompleted();
    record.coinReward = task.coinReward();
    record.growthReward = task.growthReward();
    record.attributeReward = serializeAttributes(task.attributeReward());
    record.bonusStreak = task.bonusStreak();
    record.customSettings = task.customSettings();
    record.forgivenessCoupons = task.forgivenessCoupons();
    record.progressValue = task.progressValue();
    record.progressGoal = task.progressGoal();
    return record;
}

/**
 * @brief 将属性奖励压缩为 key=value 串，方便存入 TEXT 字段。
 */
std::string TaskManager::serializeAttributes(const User::AttributeSet& set) const {
    std::ostringstream stream;
    stream << "execution=" << set.execution << ';'
           << "perseverance=" << set.perseverance << ';'
           << "decision=" << set.decision << ';'
           << "knowledge=" << set.knowledge << ';'
           << "social=" << set.social << ';'
           << "pride=" << set.pride;
    return stream.str();
}

/**
 * @brief 将 TEXT 字段还原为 AttributeSet，异常时使用 0 兜底，保证健壮性。
 */
User::AttributeSet TaskManager::deserializeAttributes(const std::string& blob) const {
    User::AttributeSet attributes;
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
        int value = 0;
        try {
            value = std::stoi(valuePart);
        } catch (...) {
            value = 0;
        }
        if (key == "execution") {
            attributes.execution = value;
        } else if (key == "perseverance") {
            attributes.perseverance = value;
        } else if (key == "decision") {
            attributes.decision = value;
        } else if (key == "knowledge") {
            attributes.knowledge = value;
        } else if (key == "social") {
            attributes.social = value;
        } else if (key == "pride") {
            attributes.pride = value;
        }
    }
    return attributes;
}

/**
 * @brief 针对给定类型执行重置策略，包含进度归零和连胜校验。
 */
void TaskManager::resetTasksByPredicate(Task::TaskType type) {
    for (auto& [id, task] : m_tasks) {
        if (task.type() != type) {
            continue;
        }
        if (!task.isCompleted()) {
            task.resetBonusStreak();
        }
        task.resetProgressForNewCycle();
        m_database.updateTask(toRecord(task));
    }
}

/**
 * @brief 奖励发放核心：难度+连胜加权 -> UserManager -> 持久化。
 * 中文：该函数负责“任务完成”数据流：任务 -> 计算奖励 -> UserManager 更新成长/属性/成就 -> Database 更新任务状态。
 *       期间还会根据类型追加教育意义的属性奖励，并在连胜达到阈值时解锁成就，实现“多系统联动”。
 *       全过程放在事务内，任何异常都会回滚，保证成长值、金币与任务状态始终一致。
 */
void TaskManager::applyRewardsLocked(Task& task) {
    bool transactionStarted = false;
    try {
        m_database.beginTransaction();
        transactionStarted = true;

        const double rewardFactor = difficultyFactor(task) * streakFactor(task);
        const int finalCoins = std::max(0, static_cast<int>(std::round(task.coinReward() * rewardFactor)));
        const int finalGrowth = std::max(0, static_cast<int>(std::round(task.growthReward() * rewardFactor)));
        User::AttributeSet finalAttributes = task.attributeReward();

        if (task.type() == Task::TaskType::Daily) {
            finalAttributes.execution += 1;  // 每日任务强调执行力。
        } else if (task.type() == Task::TaskType::Weekly) {
            finalAttributes.social += task.difficultyStars();  // 每周项目强调团队协作。
        } else if (task.type() == Task::TaskType::Semester) {
            finalAttributes.knowledge += task.difficultyStars() * 2;
            finalAttributes.perseverance += task.difficultyStars();
        }

        m_userManager.applyTaskCompletion(finalGrowth, finalCoins, finalAttributes, mapToUserCategory(task.type()));
        if (task.type() == Task::TaskType::Weekly && (task.bonusStreak() + 1) % 4 == 0) {
            m_userManager.unlockAchievement();
        }

        task.setCompleted(true);
        task.incrementBonusStreak();
        task.setProgressValue(task.progressGoal());
        m_database.updateTask(toRecord(task));
        m_completionStats[task.type()] += 1;
        if (m_completionStats[task.type()] % 10 == 0) {
            m_userManager.unlockAchievement();
        }

        m_database.commitTransaction();
    } catch (...) {
        if (transactionStarted) {
            try {
                m_database.rollbackTransaction();
            } catch (...) {
            }
        }
        throw;
    }
}

/**
 * @brief 检查学期任务的截止时间，过期则按失败处理。
 */
void TaskManager::enforceSemesterDeadlinesLocked() {
    const QDateTime now = QDateTime::currentDateTimeUtc();
    for (auto& [id, task] : m_tasks) {
        if (task.type() != Task::TaskType::Semester || task.isCompleted()) {
            continue;
        }
        if (task.isExpired(now)) {
            task.recordFailure(false);
            m_database.updateTask(toRecord(task));
        }
    }
}

/**
 * @brief 将任务类型映射到 UserManager 统计用的类别，保持跨系统一致。
 */
User::TaskCategory TaskManager::mapToUserCategory(Task::TaskType type) const {
    switch (type) {
        case Task::TaskType::Daily:
            return User::TaskCategory::Academic;
        case Task::TaskType::Weekly:
            return User::TaskCategory::Social;
        case Task::TaskType::Semester:
            return User::TaskCategory::Academic;
        case Task::TaskType::Custom:
            return User::TaskCategory::Personal;
    }
    throw std::runtime_error("Unsupported mapping");
}

/**
 * @brief 难度-奖励平衡公式：1+(星级-1)*0.15，并对长周期任务附加加成。
 * 中文：学期任务投入时间长，因此额外 +0.35；每周任务强调团队协作，+0.1。这样既满足“难度越大奖励越多”的教学要求，
 *       又避免奖励增长过快导致失衡。
 */
double TaskManager::difficultyFactor(const Task& task) const {
    double factor = 1.0 + (static_cast<double>(task.difficultyStars()) - 1.0) * 0.15;
    if (task.type() == Task::TaskType::Semester) {
        factor += 0.35;
    } else if (task.type() == Task::TaskType::Weekly) {
        factor += 0.1;
    }
    return factor;
}

/**
 * @brief 连续完成奖励公式：每次连胜额外 +5% 奖励，鼓励学生保持学习节奏。
 */
double TaskManager::streakFactor(const Task& task) const {
    return 1.0 + static_cast<double>(task.bonusStreak()) * 0.05;
}

}  // namespace rove::data
