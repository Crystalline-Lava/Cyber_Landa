#include "Achievement.h"

namespace rove::data {

Achievement::Achievement()
    : m_id(-1),
      m_owner(),
      m_creator(),
      m_name(),
      m_description(),
      m_iconPath(),
      m_displayColor(Qt::white),
      m_type(Type::System),
      m_rewardType(RewardType::NoReward),
      m_progressMode(ProgressMode::Milestone),
      m_conditions(),
      m_progressValue(0),
      m_progressGoal(1),
      m_rewardCoins(0),
      m_rewardAttributes(),
      m_specialItems(),
      m_unlocked(false),
      m_completedAt(),
      m_createdAt(QDateTime::currentDateTimeUtc()),
      m_galleryGroup("default"),
      m_conditionBlob(),
      m_rewardItemsBlob(),
      m_colorText("#FFFFFF"),
      m_specialMetadata() {}

int Achievement::id() const noexcept { return m_id; }
void Achievement::setId(int id) noexcept { m_id = id; }

const std::string& Achievement::owner() const noexcept { return m_owner; }
void Achievement::setOwner(std::string owner) { m_owner = std::move(owner); }

const std::string& Achievement::creator() const noexcept { return m_creator; }
void Achievement::setCreator(std::string creator) { m_creator = std::move(creator); }

const std::string& Achievement::name() const noexcept { return m_name; }
void Achievement::setName(std::string name) { m_name = std::move(name); }

const std::string& Achievement::description() const noexcept { return m_description; }
void Achievement::setDescription(std::string description) { m_description = std::move(description); }

const std::string& Achievement::iconPath() const noexcept { return m_iconPath; }
void Achievement::setIconPath(std::string iconPath) { m_iconPath = std::move(iconPath); }

const QColor& Achievement::displayColor() const noexcept { return m_displayColor; }
void Achievement::setDisplayColor(const QColor& color) {
    m_displayColor = color;
    m_colorText = color.name(QColor::HexRgb).toStdString();
}

Achievement::Type Achievement::type() const noexcept { return m_type; }
void Achievement::setType(Type type) noexcept { m_type = type; }

Achievement::RewardType Achievement::rewardType() const noexcept { return m_rewardType; }
void Achievement::setRewardType(RewardType type) noexcept { m_rewardType = type; }

Achievement::ProgressMode Achievement::progressMode() const noexcept { return m_progressMode; }
void Achievement::setProgressMode(ProgressMode mode) noexcept { m_progressMode = mode; }

const std::vector<Achievement::Condition>& Achievement::conditions() const noexcept { return m_conditions; }
std::vector<Achievement::Condition>& Achievement::conditions() { return m_conditions; }
void Achievement::setConditions(std::vector<Condition> conditions) { m_conditions = std::move(conditions); }

int Achievement::progressValue() const noexcept { return m_progressValue; }
void Achievement::setProgressValue(int value) { m_progressValue = value; }

int Achievement::progressGoal() const noexcept { return m_progressGoal; }
void Achievement::setProgressGoal(int goal) { m_progressGoal = goal; }

int Achievement::rewardCoins() const noexcept { return m_rewardCoins; }
void Achievement::setRewardCoins(int coins) { m_rewardCoins = coins; }

const User::AttributeSet& Achievement::rewardAttributes() const noexcept { return m_rewardAttributes; }
void Achievement::setRewardAttributes(const User::AttributeSet& attributes) { m_rewardAttributes = attributes; }

const std::vector<std::string>& Achievement::specialItems() const noexcept { return m_specialItems; }
void Achievement::setSpecialItems(std::vector<std::string> items) { m_specialItems = std::move(items); }

bool Achievement::unlocked() const noexcept { return m_unlocked; }
void Achievement::setUnlocked(bool unlocked) noexcept { m_unlocked = unlocked; }

const QDateTime& Achievement::completedAt() const noexcept { return m_completedAt; }
void Achievement::setCompletedAt(const QDateTime& timestamp) { m_completedAt = timestamp; }

const QDateTime& Achievement::createdAt() const noexcept { return m_createdAt; }
void Achievement::setCreatedAt(const QDateTime& timestamp) { m_createdAt = timestamp; }

const std::string& Achievement::galleryGroup() const noexcept { return m_galleryGroup; }
void Achievement::setGalleryGroup(std::string group) { m_galleryGroup = std::move(group); }

const std::string& Achievement::conditionBlob() const noexcept { return m_conditionBlob; }
void Achievement::setConditionBlob(std::string blob) { m_conditionBlob = std::move(blob); }

const std::string& Achievement::rewardItemsBlob() const noexcept { return m_rewardItemsBlob; }
void Achievement::setRewardItemsBlob(std::string blob) { m_rewardItemsBlob = std::move(blob); }

const std::string& Achievement::colorText() const noexcept { return m_colorText; }

const std::string& Achievement::specialMetadata() const noexcept { return m_specialMetadata; }
void Achievement::setSpecialMetadata(std::string metadata) { m_specialMetadata = std::move(metadata); }

bool Achievement::isProgressBased() const noexcept { return m_progressMode == ProgressMode::Incremental; }

int Achievement::progressPercent() const noexcept {
    if (m_progressGoal <= 0) {
        return 100;
    }
    return static_cast<int>((static_cast<double>(m_progressValue) / static_cast<double>(m_progressGoal)) * 100.0);
}

}  // namespace rove::data
