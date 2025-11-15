#ifndef ACHIEVEMENT_H
#define ACHIEVEMENT_H

#include <QColor>
#include <QDateTime>

#include <string>
#include <vector>

#include "User.h"

namespace rove::data {

/**
 * @class Achievement
 * @brief 成就数据模型：封装展示信息、条件、奖励与进度，用于成就画廊与解锁逻辑。
 */
class Achievement final {
public:
    /**
     * @enum Type
     * @brief 区分系统成就与自定义成就，控制是否允许编辑。
     */
    enum class Type { System, Custom };

    /**
     * @enum RewardType
     * @brief 区分是否附带奖励，便于执行每月奖励数量限制。
     */
    enum class RewardType { WithReward, NoReward };

    /**
     * @enum ProgressMode
     * @brief 指示是一次性里程碑还是需要累计进度的成长型成就。
     */
    enum class ProgressMode { Milestone, Incremental };

    /**
     * @struct Condition
     * @brief 单条完成条件，支持多条件并列判断。
     */
    struct Condition {
        /**
         * @enum ConditionType
         * @brief 支持的触发来源：任务、等级、自豪感、金币或自定义计数。
         */
        enum class ConditionType {
            CompleteAnyTask,
            CompleteTaskType,
            ReachLevel,
            ReachPride,
            ReachCoins,
            CustomCounter
        };

        ConditionType type = ConditionType::CompleteAnyTask;
        int targetValue = 1;
        int currentValue = 0;
        std::string metadata;
    };

    Achievement();

    int id() const noexcept;
    void setId(int id) noexcept;

    const std::string& owner() const noexcept;
    void setOwner(std::string owner);

    const std::string& creator() const noexcept;
    void setCreator(std::string creator);

    const std::string& name() const noexcept;
    void setName(std::string name);

    const std::string& description() const noexcept;
    void setDescription(std::string description);

    const std::string& iconPath() const noexcept;
    void setIconPath(std::string iconPath);

    const QColor& displayColor() const noexcept;
    void setDisplayColor(const QColor& color);

    Type type() const noexcept;
    void setType(Type type) noexcept;

    RewardType rewardType() const noexcept;
    void setRewardType(RewardType type) noexcept;

    ProgressMode progressMode() const noexcept;
    void setProgressMode(ProgressMode mode) noexcept;

    const std::vector<Condition>& conditions() const noexcept;
    std::vector<Condition>& conditions();
    void setConditions(std::vector<Condition> conditions);

    int progressValue() const noexcept;
    void setProgressValue(int value);

    int progressGoal() const noexcept;
    void setProgressGoal(int goal);

    int rewardCoins() const noexcept;
    void setRewardCoins(int coins);

    const User::AttributeSet& rewardAttributes() const noexcept;
    void setRewardAttributes(const User::AttributeSet& attributes);

    const std::vector<std::string>& specialItems() const noexcept;
    void setSpecialItems(std::vector<std::string> items);

    bool unlocked() const noexcept;
    void setUnlocked(bool unlocked) noexcept;

    const QDateTime& completedAt() const noexcept;
    void setCompletedAt(const QDateTime& timestamp);

    const QDateTime& createdAt() const noexcept;
    void setCreatedAt(const QDateTime& timestamp);

    const std::string& galleryGroup() const noexcept;
    void setGalleryGroup(std::string group);

    const std::string& conditionBlob() const noexcept;
    void setConditionBlob(std::string blob);

    const std::string& rewardItemsBlob() const noexcept;
    void setRewardItemsBlob(std::string blob);

    const std::string& colorText() const noexcept;

    const std::string& specialMetadata() const noexcept;
    void setSpecialMetadata(std::string metadata);

    bool isProgressBased() const noexcept;
    int progressPercent() const noexcept;

private:
    int m_id;
    std::string m_owner;
    std::string m_creator;
    std::string m_name;
    std::string m_description;
    std::string m_iconPath;
    QColor m_displayColor;
    Type m_type;
    RewardType m_rewardType;
    ProgressMode m_progressMode;
    std::vector<Condition> m_conditions;
    int m_progressValue;
    int m_progressGoal;
    int m_rewardCoins;
    User::AttributeSet m_rewardAttributes;
    std::vector<std::string> m_specialItems;
    bool m_unlocked;
    QDateTime m_completedAt;
    QDateTime m_createdAt;
    std::string m_galleryGroup;
    std::string m_conditionBlob;
    std::string m_rewardItemsBlob;
    std::string m_colorText;
    std::string m_specialMetadata;
};

}  // namespace rove::data

#endif  // ACHIEVEMENT_H
