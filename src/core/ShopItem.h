#ifndef SHOPITEM_H
#define SHOPITEM_H

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <optional>
#include <string>
#include <vector>

#include "DatabaseManager.h"

namespace rove::data {

/**
 * @class ShopItem
 * @brief 商城商品数据模型，完整描述商品分类、定价策略、效果元数据与幸运礼包配置。
 */
class ShopItem final {
public:
    enum class ItemType { Physical, Prop, LuckyBag };

    enum class PropEffectType { None, RestDay, ForgivenessCoupon, DoubleExpCard };

    struct LuckyBagReward {
        enum class RewardType { Coins, ShopItem, Growth };
        RewardType type = RewardType::Coins;
        int amount = 0;
        double probability = 0.0;
        int referenceItemId = -1;
        std::string description;
    };

    ShopItem();

    int id() const noexcept;
    void setId(int id) noexcept;

    const std::string& name() const noexcept;
    void setName(std::string name);

    const std::string& description() const noexcept;
    void setDescription(std::string description);

    const std::string& iconPath() const noexcept;
    void setIconPath(std::string iconPath);

    ItemType itemType() const noexcept;
    void setItemType(ItemType type) noexcept;

    int priceCoins() const noexcept;
    void setPriceCoins(int price) noexcept;

    int purchaseLimit() const noexcept;
    void setPurchaseLimit(int limit) noexcept;

    bool isAvailable() const noexcept;
    void setAvailable(bool available) noexcept;

    const std::string& effectDescription() const noexcept;
    void setEffectDescription(std::string description);

    const std::string& effectLogic() const noexcept;
    void setEffectLogic(std::string logic);

    PropEffectType propEffectType() const noexcept;
    void setPropEffectType(PropEffectType type) noexcept;

    int effectDurationMinutes() const noexcept;
    void setEffectDurationMinutes(int minutes) noexcept;

    const std::string& usageConditions() const noexcept;
    void setUsageConditions(std::string conditions);

    const std::string& physicalRedeemMethod() const noexcept;
    void setPhysicalRedeemMethod(std::string redeem);

    const std::string& physicalNotes() const noexcept;
    void setPhysicalNotes(std::string notes);

    const std::vector<LuckyBagReward>& luckyRewards() const noexcept;
    void setLuckyRewards(const std::vector<LuckyBagReward>& rewards);

    int levelRequirement() const noexcept;
    void setLevelRequirement(int level) noexcept;

    std::string serializeLuckyRewards() const;
    void deserializeLuckyRewards(const std::string& json);

    static ShopItem fromRecord(const DatabaseManager::ShopItemRecord& record);
    DatabaseManager::ShopItemRecord toRecord() const;

    static std::string itemTypeToString(ItemType type);
    static ItemType itemTypeFromString(const std::string& text);
    static std::string propEffectToString(PropEffectType type);
    static PropEffectType propEffectFromString(const std::string& text);
    static std::string rewardTypeToString(LuckyBagReward::RewardType type);
    static LuckyBagReward::RewardType rewardTypeFromString(const std::string& text);

private:
    int m_id;
    std::string m_name;
    std::string m_description;
    std::string m_iconPath;
    ItemType m_itemType;
    int m_price;
    int m_purchaseLimit;
    bool m_available;
    std::string m_effectDescription;
    std::string m_effectLogic;
    PropEffectType m_propEffectType;
    int m_effectDurationMinutes;
    std::string m_usageConditions;
    std::string m_physicalRedeemMethod;
    std::string m_physicalNotes;
    std::vector<LuckyBagReward> m_luckyRewards;
    int m_levelRequirement;
};

}  // namespace rove::data

#endif  // SHOPITEM_H
