#include "AchievementManager.h"
#include "GrowthSystem.h"
#include "GrowthSystemBridge.h"
#include <QDate>

#include <algorithm>
#include <cmath>
#include <sstream>
#include <stdexcept>

namespace rove::data {
namespace {

std::string serializeConditions(const std::vector<Achievement::Condition>& conditions) {
    std::ostringstream stream;
    bool first = true;
    for (const auto& condition : conditions) {
        if (!first) {
            stream << ';';
        }
        first = false;
        stream << static_cast<int>(condition.type) << ',' << condition.targetValue << ','
               << condition.currentValue << ',' << condition.metadata;
    }
    return stream.str();
}

std::vector<Achievement::Condition> deserializeConditions(const std::string& blob) {
    std::vector<Achievement::Condition> conditions;
    std::istringstream stream(blob);
    std::string segment;
    while (std::getline(stream, segment, ';')) {
        if (segment.empty()) {
            continue;
        }
        std::istringstream part(segment);
        std::string typeToken;
        std::string targetToken;
        std::string currentToken;
        std::string metadataToken;
        std::getline(part, typeToken, ',');
        std::getline(part, targetToken, ',');
        std::getline(part, currentToken, ',');
        std::getline(part, metadataToken);
        Achievement::Condition condition;
        try {
            condition.type = static_cast<Achievement::Condition::ConditionType>(std::stoi(typeToken));
            condition.targetValue = std::max(1, std::stoi(targetToken));
            condition.currentValue = std::max(0, std::stoi(currentToken));
        } catch (...) {
            continue;
        }
        condition.metadata = metadataToken;
        conditions.push_back(condition);
    }
    if (conditions.empty()) {
        Achievement::Condition fallback;
        fallback.targetValue = 1;
        conditions.push_back(fallback);
    }
    return conditions;
}

std::string serializeItems(const std::vector<std::string>& items) {
    std::ostringstream stream;
    bool first = true;
    for (const auto& item : items) {
        if (!first) {
            stream << '|';
        }
        first = false;
        stream << item;
    }
    return stream.str();
}

std::vector<std::string> deserializeItems(const std::string& blob) {
    std::vector<std::string> items;
    std::istringstream stream(blob);
    std::string segment;
    while (std::getline(stream, segment, '|')) {
        if (!segment.empty()) {
            items.push_back(segment);
        }
    }
    return items;
}

std::string serializeAttributes(const User::AttributeSet& set) {
    std::ostringstream stream;
    stream << set.execution << ',' << set.perseverance << ',' << set.decision << ',' << set.knowledge
           << ',' << set.social << ',' << set.pride;
    return stream.str();
}

User::AttributeSet deserializeAttributes(const std::string& blob) {
    User::AttributeSet set;
    std::istringstream stream(blob);
    std::string token;
    int* fields[] = {&set.execution, &set.perseverance, &set.decision, &set.knowledge, &set.social, &set.pride};
    for (int i = 0; i < 6 && std::getline(stream, token, ','); ++i) {
        try {
            *fields[i] = std::stoi(token);
        } catch (...) {
            *fields[i] = 0;
        }
    }
    return set;
}

std::string typeToText(Achievement::Type type) { return type == Achievement::Type::System ? "System" : "Custom"; }

Achievement::Type typeFromText(const std::string& text) {
    return text == "System" ? Achievement::Type::System : Achievement::Type::Custom;
}

std::string rewardTypeToText(Achievement::RewardType type) {
    return type == Achievement::RewardType::WithReward ? "WithReward" : "NoReward";
}

Achievement::RewardType rewardTypeFromText(const std::string& text) {
    return text == "WithReward" ? Achievement::RewardType::WithReward : Achievement::RewardType::NoReward;
}

std::string progressModeToText(Achievement::ProgressMode mode) {
    return mode == Achievement::ProgressMode::Incremental ? "Incremental" : "Milestone";
}

Achievement::ProgressMode progressModeFromText(const std::string& text) {
    return text == "Incremental" ? Achievement::ProgressMode::Incremental : Achievement::ProgressMode::Milestone;
}

}  // namespace

AchievementManager& AchievementManager::instance(DatabaseManager& database,
                                                 UserManager& userManager,
                                                 TaskManager& taskManager) {
    static AchievementManager instance(database, userManager, taskManager);
    return instance;
}

AchievementManager::AchievementManager(DatabaseManager& database,
                                       UserManager& userManager,
                                       TaskManager& taskManager)
    : QObject(nullptr),
      m_database(database),
      m_userManager(userManager),
      m_taskManager(taskManager),
      m_achievements(),
      m_galleryIndex(),
      m_mutex() {
    if (auto* proxy = m_taskManager.signalProxy()) {
        QObject::connect(proxy, &TaskManagerSignalProxy::taskCompleted, this, &AchievementManager::onTaskCompleted);
        QObject::connect(proxy, &TaskManagerSignalProxy::taskProgressed, this, &AchievementManager::onTaskProgressed);
    }
    if (auto* proxy = m_userManager.signalProxy()) {
        QObject::connect(proxy, &UserManagerSignalProxy::levelChanged, this, &AchievementManager::onUserLevelChanged);
        QObject::connect(proxy, &UserManagerSignalProxy::prideChanged, this, &AchievementManager::onPrideChanged);
        QObject::connect(proxy, &UserManagerSignalProxy::coinsChanged, this, &AchievementManager::onCoinsChanged);
    }
}

void AchievementManager::refreshFromDatabase() {
    if (!m_userManager.hasActiveUser()) {
        return;
    }
    const std::string owner = m_userManager.activeUser().username();
    const auto records = m_database.getAchievementsForOwner(owner);
    std::lock_guard<std::mutex> lock(m_mutex);
    m_achievements.clear();
    for (const auto& record : records) {
        Achievement achievement = hydrateAchievement(record);
        m_achievements[achievement.id()] = achievement;
    }
    ensureSystemAchievements();
    rebuildGalleryIndex();
}

std::vector<Achievement> AchievementManager::achievements() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<Achievement> result;
    for (const auto& [id, achievement] : m_achievements) {
        result.push_back(achievement);
    }
    return result;
}

std::vector<Achievement> AchievementManager::achievementsInGroup(const std::string& group) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<Achievement> result;
    auto it = m_galleryIndex.find(group);
    if (it == m_galleryIndex.end()) {
        return result;
    }
    for (int id : it->second) {
        auto entry = m_achievements.find(id);
        if (entry != m_achievements.end()) {
            result.push_back(entry->second);
        }
    }
    return result;
}

std::optional<Achievement> AchievementManager::achievementById(int id) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_achievements.find(id);
    if (it == m_achievements.end()) {
        return std::nullopt;
    }
    return it->second;
}

int AchievementManager::createCustomAchievement(Achievement achievement) {
    if (!m_userManager.hasActiveUser()) {
        throw std::runtime_error("未登录无法创建成就");
    }
    achievement.setOwner(m_userManager.activeUser().username());
    achievement.setCreator(m_userManager.activeUser().username());
    achievement.setType(Achievement::Type::Custom);
    achievement.setCreatedAt(QDateTime::currentDateTimeUtc());
    if (!validateCustomAchievement(achievement)) {
        throw std::runtime_error("自定义成就校验失败");
    }
    achievement.setConditionBlob(serializeConditions(achievement.conditions()));
    achievement.setRewardItemsBlob(serializeItems(achievement.specialItems()));
    achievement.setProgressValue(0);
    recalculateProgress(achievement);
    const auto record = toRecord(achievement);
    const int newId = m_database.createAchievement(record);
    achievement.setId(newId);
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_achievements[newId] = achievement;
        updateGalleryForAchievement(achievement);
    }
    return newId;
}

void AchievementManager::updateCustomAchievement(const Achievement& achievement) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_achievements.find(achievement.id());
    if (it == m_achievements.end()) {
        throw std::runtime_error("成就不存在");
    }
    if (it->second.type() == Achievement::Type::System) {
        throw std::runtime_error("系统成就禁止修改");
    }
    Achievement copy = achievement;
    copy.setOwner(m_userManager.activeUser().username());
    copy.setConditionBlob(serializeConditions(copy.conditions()));
    copy.setRewardItemsBlob(serializeItems(copy.specialItems()));
    recalculateProgress(copy);
    m_database.updateAchievement(toRecord(copy));
    m_achievements[copy.id()] = copy;
    rebuildGalleryIndex();
}

void AchievementManager::deleteCustomAchievement(int achievementId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_achievements.find(achievementId);
    if (it == m_achievements.end()) {
        return;
    }
    if (it->second.type() == Achievement::Type::System) {
        throw std::runtime_error("系统成就禁止删除");
    }
    m_database.deleteAchievement(achievementId);
    m_achievements.erase(it);
    rebuildGalleryIndex();
}

void AchievementManager::recordCustomProgress(int achievementId, int delta) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_achievements.find(achievementId);
    if (it == m_achievements.end()) {
        throw std::runtime_error("成就不存在");
    }
    updateConditionCache(it->second, Achievement::Condition::ConditionType::CustomCounter, delta, "");
    if (recalculateProgress(it->second)) {
        m_database.updateAchievement(toRecord(it->second));
        emit achievementProgressChanged(it->second.id(), it->second.progressValue(), it->second.progressGoal());
    }
    evaluateCompletion(it->second);
}

void AchievementManager::onTaskCompleted(int /*taskId*/, int taskType, int /*difficulty*/) {
    const Task::TaskType type = static_cast<Task::TaskType>(taskType);
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& [id, achievement] : m_achievements) {
        updateConditionCache(achievement, Achievement::Condition::ConditionType::CompleteAnyTask, 1, "");
        updateConditionCache(achievement,
                             Achievement::Condition::ConditionType::CompleteTaskType,
                             1,
                             Task::typeToString(type));
        if (recalculateProgress(achievement)) {
            m_database.updateAchievement(toRecord(achievement));
            emit achievementProgressChanged(id, achievement.progressValue(), achievement.progressGoal());
        }
        evaluateCompletion(achievement);
    }
}

void AchievementManager::onTaskProgressed(int /*taskId*/, int currentValue, int goalValue) {
    if (goalValue <= 0) {
        return;
    }
    const int clampedProgress = std::clamp(currentValue, 0, goalValue);
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& [id, achievement] : m_achievements) {
        replaceConditionValue(achievement,
                              Achievement::Condition::ConditionType::CustomCounter,
                              clampedProgress,
                              "task_progress");
        if (recalculateProgress(achievement)) {
            m_database.updateAchievement(toRecord(achievement));
            emit achievementProgressChanged(id, achievement.progressValue(), achievement.progressGoal());
        }
        evaluateCompletion(achievement);
    }
}

void AchievementManager::onUserLevelChanged(int newLevel) {
    std::lock_guard<std::mutex> lock(m_mutex);
    handleUserLevelChangedLocked(newLevel);
}

void AchievementManager::onPrideChanged(int newPride) {
    std::lock_guard<std::mutex> lock(m_mutex);
    handlePrideChangedLocked(newPride);
}

void AchievementManager::onCoinsChanged(int newCoins) {
    std::lock_guard<std::mutex> lock(m_mutex);
    handleCoinsChangedLocked(newCoins);
}

/**
 * @brief 复用的等级变更处理函数，假设调用方已持有 m_mutex。
 * 中文：避免重复加锁导致死锁，因此所有触发等级条件更新的入口（事件回调、奖励发放）
 *       都调用本方法，在同一把锁下完成条件刷新与解锁判定。
 */
void AchievementManager::handleUserLevelChangedLocked(int newLevel) {
    for (auto& [id, achievement] : m_achievements) {
        replaceConditionValue(achievement,
                              Achievement::Condition::ConditionType::ReachLevel,
                              newLevel,
                              "");
        if (recalculateProgress(achievement)) {
            m_database.updateAchievement(toRecord(achievement));
            emit achievementProgressChanged(id, achievement.progressValue(), achievement.progressGoal());
        }
        evaluateCompletion(achievement);
    }
}

/**
 * @brief 自豪感变更的共享处理逻辑，调用时需确保互斥锁已锁定。
 */
void AchievementManager::handlePrideChangedLocked(int newPride) {
    for (auto& [id, achievement] : m_achievements) {
        replaceConditionValue(achievement,
                              Achievement::Condition::ConditionType::ReachPride,
                              newPride,
                              "");
        if (recalculateProgress(achievement)) {
            m_database.updateAchievement(toRecord(achievement));
            emit achievementProgressChanged(id, achievement.progressValue(), achievement.progressGoal());
        }
        evaluateCompletion(achievement);
    }
}

/**
 * @brief 兰州币余额变动的共享处理逻辑，调用前同样需要持有互斥锁。
 */
void AchievementManager::handleCoinsChangedLocked(int newCoins) {
    for (auto& [id, achievement] : m_achievements) {
        replaceConditionValue(achievement,
                              Achievement::Condition::ConditionType::ReachCoins,
                              newCoins,
                              "");
        if (recalculateProgress(achievement)) {
            m_database.updateAchievement(toRecord(achievement));
            emit achievementProgressChanged(id, achievement.progressValue(), achievement.progressGoal());
        }
        evaluateCompletion(achievement);
    }
}

void AchievementManager::ensureSystemAchievements() {
    const std::string owner = m_userManager.activeUser().username();
    const auto templates = buildSystemTemplates(owner);
    for (auto templ : templates) {
        bool exists = false;
        for (const auto& [id, achievement] : m_achievements) {
            if (achievement.type() == Achievement::Type::System && achievement.name() == templ.name()) {
                exists = true;
                break;
            }
        }
        if (!exists) {
            templ.setConditionBlob(serializeConditions(templ.conditions()));
            templ.setRewardItemsBlob(serializeItems(templ.specialItems()));
            const int newId = m_database.createAchievement(toRecord(templ));
            templ.setId(newId);
            m_achievements[newId] = templ;
        }
    }
}

std::vector<Achievement> AchievementManager::buildSystemTemplates(const std::string& owner) const {
    std::vector<Achievement> templates;

    Achievement newbie;
    newbie.setOwner(owner);
    newbie.setCreator("system");
    newbie.setType(Achievement::Type::System);
    newbie.setRewardType(Achievement::RewardType::WithReward);
    newbie.setProgressMode(Achievement::ProgressMode::Milestone);
    newbie.setName("初入兰大");
    newbie.setDescription("首次达到 3 级，证明已融入校园节奏");
    newbie.setIconPath(":/icons/rookie.png");
    newbie.setDisplayColor(QColor("#4CAF50"));
    newbie.setGalleryGroup("新生礼遇");
    Achievement::Condition levelCond;
    levelCond.type = Achievement::Condition::ConditionType::ReachLevel;
    levelCond.targetValue = 3;
    newbie.setConditions({levelCond});
    newbie.setRewardCoins(50);
    User::AttributeSet levelReward;
    levelReward.pride = 2;
    newbie.setRewardAttributes(levelReward);
    templates.push_back(newbie);

    Achievement pride;
    pride.setOwner(owner);
    pride.setCreator("system");
    pride.setType(Achievement::Type::System);
    pride.setRewardType(Achievement::RewardType::WithReward);
    pride.setProgressMode(Achievement::ProgressMode::Incremental);
    pride.setName("西北自豪");
    pride.setDescription("自豪感达到 20，成为兰大形象大使");
    pride.setIconPath(":/icons/pride.png");
    pride.setDisplayColor(QColor("#FFC107"));
    pride.setGalleryGroup("精神成长");
    Achievement::Condition prideCond;
    prideCond.type = Achievement::Condition::ConditionType::ReachPride;
    prideCond.targetValue = 20;
    pride.setConditions({prideCond});
    User::AttributeSet prideReward;
    prideReward.pride = 5;
    pride.setRewardAttributes(prideReward);
    pride.setRewardCoins(80);
    pride.setSpecialItems({"校史徽章"});
    templates.push_back(pride);

    Achievement taskHunter;
    taskHunter.setOwner(owner);
    taskHunter.setCreator("system");
    taskHunter.setType(Achievement::Type::System);
    taskHunter.setRewardType(Achievement::RewardType::NoReward);
    taskHunter.setProgressMode(Achievement::ProgressMode::Incremental);
    taskHunter.setName("任务达人");
    taskHunter.setDescription("累计完成 10 个任务");
    taskHunter.setIconPath(":/icons/tasks.png");
    taskHunter.setDisplayColor(QColor("#03A9F4"));
    taskHunter.setGalleryGroup("勤奋实践");
    Achievement::Condition taskCond;
    taskCond.type = Achievement::Condition::ConditionType::CompleteAnyTask;
    taskCond.targetValue = 10;
    taskHunter.setConditions({taskCond});
    templates.push_back(taskHunter);

    Achievement weeklyStar;
    weeklyStar.setOwner(owner);
    weeklyStar.setCreator("system");
    weeklyStar.setType(Achievement::Type::System);
    weeklyStar.setRewardType(Achievement::RewardType::WithReward);
    weeklyStar.setProgressMode(Achievement::ProgressMode::Incremental);
    weeklyStar.setName("周计划达人");
    weeklyStar.setDescription("完成 5 个周任务");
    weeklyStar.setIconPath(":/icons/weekly.png");
    weeklyStar.setDisplayColor(QColor("#9C27B0"));
    weeklyStar.setGalleryGroup("勤奋实践");
    Achievement::Condition weeklyCond;
    weeklyCond.type = Achievement::Condition::ConditionType::CompleteTaskType;
    weeklyCond.targetValue = 5;
    weeklyCond.metadata = Task::typeToString(Task::TaskType::Weekly);
    weeklyStar.setConditions({weeklyCond});
    User::AttributeSet weeklyReward;
    weeklyReward.execution = 1;
    weeklyReward.pride = 1;
    weeklyStar.setRewardAttributes(weeklyReward);
    weeklyStar.setRewardCoins(40);
    templates.push_back(weeklyStar);

    return templates;
}

Achievement AchievementManager::hydrateAchievement(const DatabaseManager::AchievementRecord& record) const {
    Achievement achievement;
    achievement.setId(record.id);
    achievement.setOwner(record.owner);
    achievement.setCreator(record.creator);
    achievement.setName(record.name);
    achievement.setDescription(record.description);
    achievement.setIconPath(record.iconPath);
    achievement.setDisplayColor(QColor(record.color.c_str()));
    achievement.setType(typeFromText(record.type));
    achievement.setRewardType(rewardTypeFromText(record.rewardType));
    achievement.setProgressMode(progressModeFromText(record.progressMode));
    achievement.setConditions(deserializeConditions(record.conditions));
    achievement.setConditionBlob(record.conditions);
    achievement.setProgressValue(record.progressValue);
    achievement.setProgressGoal(record.progressGoal);
    achievement.setRewardCoins(record.rewardCoins);
    achievement.setRewardAttributes(deserializeAttributes(record.rewardAttributes));
    achievement.setSpecialItems(deserializeItems(record.rewardItems));
    achievement.setRewardItemsBlob(record.rewardItems);
    achievement.setUnlocked(record.unlocked);
    if (!record.completionTime.empty()) {
        achievement.setCompletedAt(QDateTime::fromString(QString::fromStdString(record.completionTime), Qt::ISODate));
    }
    if (!record.createdAt.empty()) {
        achievement.setCreatedAt(QDateTime::fromString(QString::fromStdString(record.createdAt), Qt::ISODate));
    }
    achievement.setGalleryGroup(record.galleryGroup);
    achievement.setSpecialMetadata(record.specialMetadata);
    return achievement;
}

DatabaseManager::AchievementRecord AchievementManager::toRecord(const Achievement& achievement) const {
    DatabaseManager::AchievementRecord record;
    record.id = achievement.id();
    record.owner = achievement.owner();
    record.creator = achievement.creator();
    record.name = achievement.name();
    record.description = achievement.description();
    record.iconPath = achievement.iconPath();
    record.color = achievement.colorText();
    record.type = typeToText(achievement.type());
    record.rewardType = rewardTypeToText(achievement.rewardType());
    record.progressMode = progressModeToText(achievement.progressMode());
    record.conditions = achievement.conditionBlob().empty() ? serializeConditions(achievement.conditions())
                                                          : achievement.conditionBlob();
    record.progressValue = achievement.progressValue();
    record.progressGoal = achievement.progressGoal();
    record.rewardCoins = achievement.rewardCoins();
    record.rewardAttributes = serializeAttributes(achievement.rewardAttributes());
    record.rewardItems = achievement.rewardItemsBlob().empty() ? serializeItems(achievement.specialItems())
                                                              : achievement.rewardItemsBlob();
    record.unlocked = achievement.unlocked();
    record.completionTime = achievement.completedAt().isValid()
                                ? achievement.completedAt().toString(Qt::ISODate).toStdString()
                                : std::string();
    record.galleryGroup = achievement.galleryGroup();
    record.createdAt = achievement.createdAt().isValid()
                           ? achievement.createdAt().toString(Qt::ISODate).toStdString()
                           : QDateTime::currentDateTimeUtc().toString(Qt::ISODate).toStdString();
    record.specialMetadata = achievement.specialMetadata();
    return record;
}

bool AchievementManager::recalculateProgress(Achievement& achievement) {
    int totalGoal = 0;
    int totalProgress = 0;
    for (const auto& condition : achievement.conditions()) {
        totalGoal += condition.targetValue;
        totalProgress += std::min(condition.targetValue, condition.currentValue);
    }
    totalGoal = std::max(1, totalGoal);
    const int before = achievement.progressValue();
    achievement.setProgressGoal(totalGoal);
    achievement.setProgressValue(std::min(totalGoal, totalProgress));
    return before != achievement.progressValue();
}

// AchievementManager.cpp 中修改 evaluateCompletion 函数
void AchievementManager::evaluateCompletion(Achievement& achievement)
{
    if (achievement.unlocked()) {
        return;
    }
    if (achievement.progressValue() < achievement.progressGoal()) {
        return;
    }
    achievement.setUnlocked(true);
    achievement.setCompletedAt(QDateTime::currentDateTimeUtc());
    
    // 准备成就数据供成长系统使用
    QJsonObject achievementData;
    achievementData["name"] = achievement.name();
    achievementData["rewardType"] = Achievement::rewardTypeToText(achievement.rewardType());
    achievementData["rarity"] = 1; // 根据成就稀有度设置
    achievementData["category"] = "general"; // 根据成就类型设置
    achievementData["isMilestone"] = achievement.progressMode() == Achievement::ProgressMode::Milestone;
    
    // 通知成长系统成就解锁
    if (m_growthSystem) {
        m_growthSystem->onAchievementUnlocked(achievementData);
    }
    
    // 原有奖励逻辑...
    grantRewards(achievement);
    m_database.updateAchievement(toRecord(achievement));
    emit achievementUnlocked(achievement.id());
}

/**
 * @brief 成就奖励发放：强调“自豪感”是特殊属性，直接影响成长荣誉。
 * 中文：老师要求通过成就鼓励学生的“兰大自豪感”，因此当奖励包含 pride 时，会同步触发 prideChanged
 *       信号，既驱动属性加点，也让荣誉墙实时更新。
 */
void AchievementManager::grantRewards(Achievement& achievement) {
    if (!m_userManager.hasActiveUser()) {
        return;
    }
    if (achievement.rewardType() == Achievement::RewardType::NoReward) {
        m_userManager.unlockAchievement();
        return;
    }
    User& user = m_userManager.activeUser();
    const int beforeLevel = user.level();
    const int previousCoins = user.coins();
    user.addCoins(achievement.rewardCoins());
    const int previousPride = user.attributes().pride;
    user.applyAttributeBonus(achievement.rewardAttributes());
    m_userManager.unlockAchievement();
    if (beforeLevel != user.level()) {
        handleUserLevelChangedLocked(user.level());
    }
    if (previousPride != user.attributes().pride) {
        handlePrideChangedLocked(user.attributes().pride);
    }
    if (user.coins() != previousCoins) {
        handleCoinsChangedLocked(user.coins());
    }
}

bool AchievementManager::validateCustomAchievement(const Achievement& achievement) const {
    if (achievement.conditions().empty()) {
        return false;
    }
    for (const auto& condition : achievement.conditions()) {
        if (condition.targetValue <= 0) {
            return false;
        }
    }
    if (achievement.rewardType() == Achievement::RewardType::WithReward) {
        const int used = countRewardAchievementsThisMonth(achievement.owner());
        if (used >= 2) {
            throw std::runtime_error("本月奖励型自定义成就已达上限");
        }
    }
    return true;
}

void AchievementManager::rebuildGalleryIndex() {
    m_galleryIndex.clear();
    for (const auto& [id, achievement] : m_achievements) {
        updateGalleryForAchievement(achievement);
    }
}

void AchievementManager::updateGalleryForAchievement(const Achievement& achievement) {
    auto& slot = m_galleryIndex[achievement.galleryGroup()];
    if (std::find(slot.begin(), slot.end(), achievement.id()) == slot.end()) {
        slot.push_back(achievement.id());
    }
}

int AchievementManager::countRewardAchievementsThisMonth(const std::string& owner) const {
    const QString monthToken = QDate::currentDate().toString("yyyy-MM");
    return m_database.countCustomRewardAchievements(owner, monthToken.toStdString());
}

void AchievementManager::updateConditionCache(Achievement& achievement,
                                              Achievement::Condition::ConditionType type,
                                              int delta,
                                              const std::string& metadata) {
    if (delta == 0) {
        return;
    }
    for (auto& condition : achievement.conditions()) {
        if (condition.type != type) {
            continue;
        }
        if (!metadata.empty() && !condition.metadata.empty() && metadata != condition.metadata) {
            continue;
        }
        condition.currentValue = std::clamp(condition.currentValue + delta, 0, condition.targetValue);
    }
    achievement.setConditionBlob(serializeConditions(achievement.conditions()));
}

void AchievementManager::replaceConditionValue(Achievement& achievement,
                                               Achievement::Condition::ConditionType type,
                                               int value,
                                               const std::string& metadata) {
    for (auto& condition : achievement.conditions()) {
        if (condition.type != type) {
            continue;
        }
        if (!metadata.empty() && !condition.metadata.empty() && metadata != condition.metadata) {
            continue;
        }
        condition.currentValue = std::clamp(value, 0, condition.targetValue);
    }
    achievement.setConditionBlob(serializeConditions(achievement.conditions()));
}

}  // namespace rove::data

