#ifndef GROWTHSYSTEM_H
#define GROWTHSYSTEM_H

#include "User.h"
#include <QObject>
#include <QMap>
#include <QJsonObject>
#include <QJsonArray>

namespace rove::systems {

/**
 * @class GrowthSystem
 * @brief 成长系统 - 负责等级、属性、成长值与任务/成就奖励的联动
 * 中文：与现有 User、TaskManager、AchievementManager 完全集成，提供统一的成长体验
 */
class GrowthSystem : public QObject
{
    Q_OBJECT

public:
    // 属性枚举 - 与 User::AttributeSet 对应
    enum class Attribute {
        Execution = 0,      // 执行力
        Perseverance,       // 毅力值
        Decision,           // 决策力
        Knowledge,          // 学识值
        Social,             // 社交力
        Pride,              // 自豪感
        AttributeCount
    };

    explicit GrowthSystem(QObject *parent = nullptr);
    
    // 核心等级系统
    int getLevel() const { return m_currentLevel; }
    int getExperience() const { return m_currentExp; }
    int getExpToNextLevel() const;
    void addExperience(int exp, const QString &source = "");
    
    // 属性系统
    int getAttribute(Attribute attr) const;
    void addAttribute(Attribute attr, int value);
    void addAttributes(const QMap<Attribute, int> &attributes);
    QMap<Attribute, int> getAllAttributes() const;
    
    // 货币系统
    int getCoins() const { return m_currentCoins; }
    void addCoins(int coins);
    bool spendCoins(int coins);
    
    // 等级特权
    bool isFeatureUnlocked(const QString &feature) const;
    QList<QString> getUnlockedFeatures() const;
    
    // 数据持久化
    QJsonObject toJson() const;
    bool fromJson(const QJsonObject &json);
    
    // 供任务系统调用的接口
    void onTaskCompleted(const QJsonObject &taskData);
    
    // 供成就系统调用的接口
    void onAchievementUnlocked(const QJsonObject &achievementData);
    
    // 供用户系统同步的接口
    void syncWithUser(const data::User &user);
    data::User::AttributeSet toUserAttributeSet() const;

signals:
    void levelChanged(int newLevel, int oldLevel);
    void experienceChanged(int currentExp, int expToNextLevel);
    void attributeChanged(GrowthSystem::Attribute attr, int newValue);
    void coinsChanged(int newAmount, int delta);
    void featureUnlocked(const QString &feature);

private:
    void levelUp();
    void checkLevelUpFeatures();
    void applyAchievementReward(const QJsonObject &achievementData);
    Attribute stringToAttribute(const QString &attrStr) const;
    QString attributeToString(Attribute attr) const;
    
    // 核心数据
    int m_currentLevel;
    int m_currentExp;
    int m_currentCoins;
    
    // 属性系统
    QMap<Attribute, int> m_attributes;
    
    // 特权系统
    QMap<QString, int> m_featureUnlockLevels;
    QList<QString> m_unlockedFeatures;
    
    // 统计信息
    int m_totalExpGained;
    int m_totalCoinsGained;
    QDateTime m_firstLoginDate;
    QDateTime m_lastLoginDate;
    
    // 经验需求配置
    static const QMap<int, int> EXP_REQUIREMENTS;
};

} // namespace rove::systems

#endif // GROWTHSYSTEM_H