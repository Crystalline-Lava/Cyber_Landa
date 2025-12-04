#include "GrowthSystem.h"
#include <QDebug>
#include <QDateTime>
#include "User.h"
#include "Task.h"
#include "Achievement.h"

namespace rove::systems {

// 经验需求表
const QMap<int, int> GrowthSystem::EXP_REQUIREMENTS = {
    {1, 100}, {2, 250}, {3, 450}, {4, 700}, {5, 1000},
    {6, 1350}, {7, 1750}, {8, 2200}, {9, 2700}, {10, 3250},
    {11, 3850}, {12, 4500}, {13, 5200}, {14, 5950}, {15, 6750},
    {16, 7600}, {17, 8500}, {18, 9450}, {19, 10450}, {20, 11500}
};

GrowthSystem::GrowthSystem(QObject *parent) : QObject(parent)
{
    m_currentLevel = 1;
    m_currentExp = 0;
    m_currentCoins = 0;
    m_totalExpGained = 0;
    m_totalCoinsGained = 0;
    m_firstLoginDate = QDateTime::currentDateTime();
    m_lastLoginDate = QDateTime::currentDateTime();
    
    // 初始化属性
    for (int i = 0; i < static_cast<int>(Attribute::AttributeCount); ++i) {
        m_attributes[static_cast<Attribute>(i)] = 0;
    }
    
    // 初始化功能解锁等级
    m_featureUnlockLevels = {
        {"custom_task_advanced", 5},
        {"custom_achievement_advanced", 8},
        {"shop_discount", 10},
        {"double_exp_weekend", 12},
        {"premium_backgrounds", 15},
        {"achievement_analyze", 18}
    };
}

int GrowthSystem::getExpToNextLevel() const
{
    if (m_currentLevel >= EXP_REQUIREMENTS.size()) {
        return 0; // 满级
    }
    return EXP_REQUIREMENTS.value(m_currentLevel + 1, 0);
}

void GrowthSystem::addExperience(int exp, const QString &source)
{
    if (exp <= 0) return;
    
    int oldLevel = m_currentLevel;
    m_currentExp += exp;
    m_totalExpGained += exp;
    
    // 检查升级
    while (m_currentLevel < EXP_REQUIREMENTS.size() && 
           m_currentExp >= EXP_REQUIREMENTS.value(m_currentLevel + 1)) {
        levelUp();
    }
    
    emit experienceChanged(m_currentExp, getExpToNextLevel());
    
    if (m_currentLevel > oldLevel) {
        emit levelChanged(m_currentLevel, oldLevel);
        qDebug() << "升级！当前等级:" << m_currentLevel << "来源:" << source;
    } else {
        qDebug() << "获得经验:" << exp << "来源:" << source;
    }
}

void GrowthSystem::levelUp()
{
    m_currentLevel++;
    
    // 检查新解锁的功能
    checkLevelUpFeatures();
}

void GrowthSystem::checkLevelUpFeatures()
{
    for (auto it = m_featureUnlockLevels.constBegin(); it != m_featureUnlockLevels.constEnd(); ++it) {
        if (m_currentLevel >= it.value() && !m_unlockedFeatures.contains(it.key())) {
            m_unlockedFeatures.append(it.key());
            emit featureUnlocked(it.key());
            qDebug() << "解锁功能:" << it.key();
        }
    }
}

int GrowthSystem::getAttribute(Attribute attr) const
{
    return m_attributes.value(attr, 0);
}

void GrowthSystem::addAttribute(Attribute attr, int value)
{
    if (value == 0) return;
    
    int oldValue = m_attributes.value(attr, 0);
    m_attributes[attr] = oldValue + value;
    
    emit attributeChanged(attr, m_attributes[attr]);
    qDebug() << "属性增加:" << attributeToString(attr) << "值:" << value << "新值:" << m_attributes[attr];
}

void GrowthSystem::addAttributes(const QMap<Attribute, int> &attributes)
{
    for (auto it = attributes.constBegin(); it != attributes.constEnd(); ++it) {
        addAttribute(it.key(), it.value());
    }
}

QMap<GrowthSystem::Attribute, int> GrowthSystem::getAllAttributes() const
{
    return m_attributes;
}

void GrowthSystem::addCoins(int coins)
{
    if (coins <= 0) return;
    
    int oldCoins = m_currentCoins;
    m_currentCoins += coins;
    m_totalCoinsGained += coins;
    
    emit coinsChanged(m_currentCoins, coins);
    qDebug() << "获得金币:" << coins << "总额:" << m_currentCoins;
}

bool GrowthSystem::spendCoins(int coins)
{
    if (coins <= 0 || m_currentCoins < coins) {
        return false;
    }
    
    int oldCoins = m_currentCoins;
    m_currentCoins -= coins;
    
    emit coinsChanged(m_currentCoins, -coins);
    qDebug() << "花费金币:" << coins << "剩余:" << m_currentCoins;
    return true;
}

bool GrowthSystem::isFeatureUnlocked(const QString &feature) const
{
    return m_unlockedFeatures.contains(feature);
}

QList<QString> GrowthSystem::getUnlockedFeatures() const
{
    return m_unlockedFeatures;
}

QJsonObject GrowthSystem::toJson() const
{
    QJsonObject json;
    json["level"] = m_currentLevel;
    json["exp"] = m_currentExp;
    json["coins"] = m_currentCoins;
    json["totalExpGained"] = m_totalExpGained;
    json["totalCoinsGained"] = m_totalCoinsGained;
    json["firstLoginDate"] = m_firstLoginDate.toString(Qt::ISODate);
    json["lastLoginDate"] = m_lastLoginDate.toString(Qt::ISODate);
    
    // 保存属性
    QJsonObject attrsJson;
    for (auto it = m_attributes.constBegin(); it != m_attributes.constEnd(); ++it) {
        attrsJson[attributeToString(it.key())] = it.value();
    }
    json["attributes"] = attrsJson;
    
    // 保存解锁的功能
    QJsonArray featuresArray;
    for (const QString &feature : m_unlockedFeatures) {
        featuresArray.append(feature);
    }
    json["unlockedFeatures"] = featuresArray;
    
    return json;
}

bool GrowthSystem::fromJson(const QJsonObject &json)
{
    if (json.isEmpty()) return false;
    
    m_currentLevel = json["level"].toInt(1);
    m_currentExp = json["exp"].toInt(0);
    m_currentCoins = json["coins"].toInt(0);
    m_totalExpGained = json["totalExpGained"].toInt(0);
    m_totalCoinsGained = json["totalCoinsGained"].toInt(0);
    m_firstLoginDate = QDateTime::fromString(json["firstLoginDate"].toString(), Qt::ISODate);
    m_lastLoginDate = QDateTime::fromString(json["lastLoginDate"].toString(), Qt::ISODate);
    
    // 加载属性
    QJsonObject attrsJson = json["attributes"].toObject();
    for (auto it = attrsJson.constBegin(); it != attrsJson.constEnd(); ++it) {
        Attribute attr = stringToAttribute(it.key());
        if (attr != Attribute::AttributeCount) {
            m_attributes[attr] = it.value().toInt();
        }
    }
    
    // 加载解锁的功能
    m_unlockedFeatures.clear();
    QJsonArray featuresArray = json["unlockedFeatures"].toArray();
    for (const QJsonValue &value : featuresArray) {
        m_unlockedFeatures.append(value.toString());
    }
    
    return true;
}

// 任务系统调用接口
void GrowthSystem::onTaskCompleted(const QJsonObject &taskData)
{
    QString taskType = taskData["type"].toString();
    int difficulty = taskData["difficulty"].toInt();
    int coinReward = taskData["coinReward"].toInt();
    int growthReward = taskData["growthReward"].toInt();
    
    // 基础经验奖励
    int baseExp = growthReward > 0 ? growthReward : difficulty * 25;
    
    // 金币奖励
    if (coinReward > 0) {
        addCoins(coinReward);
    }
    
    // 根据任务类型给予属性奖励
    if (taskType == "Daily") {
        addAttribute(Attribute::Execution, 1);
        addAttribute(Attribute::Perseverance, 1);
    } else if (taskType == "Weekly") {
        addAttribute(Attribute::Execution, 2);
        addAttribute(Attribute::Decision, 1);
    } else if (taskType == "Semester") {
        addAttribute(Attribute::Execution, 3);
        addAttribute(Attribute::Perseverance, 3);
        addAttribute(Attribute::Decision, 2);
    } else if (taskType == "Custom") {
        addAttribute(Attribute::Execution, difficulty);
    }
    
    // 经验奖励
    addExperience(baseExp, QString("任务:%1").arg(taskType));
    
    // 处理连续完成奖励
    if (taskData.contains("continuous_days")) {
        int continuousDays = taskData["continuous_days"].toInt();
        if (continuousDays >= 7) {
            addAttribute(Attribute::Perseverance, 2);
        }
    }
}

// 成就系统调用接口
void GrowthSystem::onAchievementUnlocked(const QJsonObject &achievementData)
{
    QString achievementType = achievementData["rewardType"].toString();
    int rarity = achievementData["rarity"].toInt(1);
    bool isMilestone = achievementData["isMilestone"].toBool();
    
    // 基础经验奖励
    int expReward = rarity * 50;
    addExperience(expReward, "成就解锁");
    
    // 自豪感奖励
    addAttribute(Attribute::Pride, rarity * 2);
    
    // 根据成就类别给予额外属性
    QString category = achievementData["category"].toString();
    if (category == "learning") {
        addAttribute(Attribute::Knowledge, rarity);
    } else if (category == "social") {
        addAttribute(Attribute::Social, rarity);
    } else if (category == "sports") {
        addAttribute(Attribute::Perseverance, rarity);
    } else if (category == "creativity") {
        addAttribute(Attribute::Decision, rarity);
    } else if (category == "milestone") {
        addAttribute(Attribute::Execution, rarity);
    }
    
    // 里程碑成就额外奖励
    if (isMilestone) {
        addExperience(expReward * 2, "里程碑成就");
        addAttribute(Attribute::Pride, 5);
    }
    
    qDebug() << "成就奖励发放 - 类别:" << category << "稀有度:" << rarity;
}

// 与用户系统同步
void GrowthSystem::syncWithUser(const data::User &user)
{
    m_currentLevel = user.level();
    m_currentExp = user.growthPoints();
    m_currentCoins = user.coins();
    
    // 同步属性
    const data::User::AttributeSet &userAttrs = user.attributes();
    m_attributes[Attribute::Execution] = userAttrs.execution;
    m_attributes[Attribute::Perseverance] = userAttrs.perseverance;
    m_attributes[Attribute::Decision] = userAttrs.decision;
    m_attributes[Attribute::Knowledge] = userAttrs.knowledge;
    m_attributes[Attribute::Social] = userAttrs.social;
    m_attributes[Attribute::Pride] = userAttrs.pride;
}

data::User::AttributeSet GrowthSystem::toUserAttributeSet() const
{
    data::User::AttributeSet attrs;
    attrs.execution = m_attributes.value(Attribute::Execution, 0);
    attrs.perseverance = m_attributes.value(Attribute::Perseverance, 0);
    attrs.decision = m_attributes.value(Attribute::Decision, 0);
    attrs.knowledge = m_attributes.value(Attribute::Knowledge, 0);
    attrs.social = m_attributes.value(Attribute::Social, 0);
    attrs.pride = m_attributes.value(Attribute::Pride, 0);
    return attrs;
}

// 工具函数
GrowthSystem::Attribute GrowthSystem::stringToAttribute(const QString &attrStr) const
{
    static const QMap<QString, Attribute> attributeMap = {
        {"execution", Attribute::Execution},
        {"perseverance", Attribute::Perseverance},
        {"decision", Attribute::Decision},
        {"knowledge", Attribute::Knowledge},
        {"social", Attribute::Social},
        {"pride", Attribute::Pride}
    };
    
    QString key = attrStr.toLower();
    return attributeMap.value(key, Attribute::AttributeCount);
}

QString GrowthSystem::attributeToString(Attribute attr) const
{
    static const QMap<Attribute, QString> attributeMap = {
        {Attribute::Execution, "execution"},
        {Attribute::Perseverance, "perseverance"},
        {Attribute::Decision, "decision"},
        {Attribute::Knowledge, "knowledge"},
        {Attribute::Social, "social"},
        {Attribute::Pride, "pride"}
    };
    
    return attributeMap.value(attr, "unknown");
}

} // namespace rove::systems