#include "ShopItem.h"

#include <QByteArray>
#include <QString>

#include <algorithm>

namespace rove::data {

ShopItem::ShopItem()
    : m_id(-1),
      m_name(),
      m_description(),
      m_iconPath(),
      m_itemType(ItemType::Physical),
      m_price(0),
      m_purchaseLimit(0),
      m_available(true),
      m_effectDescription(),
      m_effectLogic(),
      m_propEffectType(PropEffectType::None),
      m_effectDurationMinutes(0),
      m_usageConditions(),
      m_physicalRedeemMethod(),
      m_physicalNotes(),
      m_luckyRewards(),
      m_levelRequirement(1) {}

int ShopItem::id() const noexcept { return m_id; }

void ShopItem::setId(int id) noexcept { m_id = id; }

const std::string& ShopItem::name() const noexcept { return m_name; }

void ShopItem::setName(std::string name) { m_name = std::move(name); }

const std::string& ShopItem::description() const noexcept { return m_description; }

void ShopItem::setDescription(std::string description) { m_description = std::move(description); }

const std::string& ShopItem::iconPath() const noexcept { return m_iconPath; }

void ShopItem::setIconPath(std::string iconPath) { m_iconPath = std::move(iconPath); }

ShopItem::ItemType ShopItem::itemType() const noexcept { return m_itemType; }

void ShopItem::setItemType(ItemType type) noexcept { m_itemType = type; }

int ShopItem::priceCoins() const noexcept { return m_price; }

void ShopItem::setPriceCoins(int price) noexcept { m_price = price; }

int ShopItem::purchaseLimit() const noexcept { return m_purchaseLimit; }

void ShopItem::setPurchaseLimit(int limit) noexcept { m_purchaseLimit = limit; }

bool ShopItem::isAvailable() const noexcept { return m_available; }

void ShopItem::setAvailable(bool available) noexcept { m_available = available; }

const std::string& ShopItem::effectDescription() const noexcept { return m_effectDescription; }

void ShopItem::setEffectDescription(std::string description) { m_effectDescription = std::move(description); }

const std::string& ShopItem::effectLogic() const noexcept { return m_effectLogic; }

void ShopItem::setEffectLogic(std::string logic) { m_effectLogic = std::move(logic); }

ShopItem::PropEffectType ShopItem::propEffectType() const noexcept { return m_propEffectType; }

void ShopItem::setPropEffectType(PropEffectType type) noexcept { m_propEffectType = type; }

int ShopItem::effectDurationMinutes() const noexcept { return m_effectDurationMinutes; }

void ShopItem::setEffectDurationMinutes(int minutes) noexcept { m_effectDurationMinutes = minutes; }

const std::string& ShopItem::usageConditions() const noexcept { return m_usageConditions; }

void ShopItem::setUsageConditions(std::string conditions) { m_usageConditions = std::move(conditions); }

const std::string& ShopItem::physicalRedeemMethod() const noexcept { return m_physicalRedeemMethod; }

void ShopItem::setPhysicalRedeemMethod(std::string redeem) { m_physicalRedeemMethod = std::move(redeem); }

const std::string& ShopItem::physicalNotes() const noexcept { return m_physicalNotes; }

void ShopItem::setPhysicalNotes(std::string notes) { m_physicalNotes = std::move(notes); }

const std::vector<ShopItem::LuckyBagReward>& ShopItem::luckyRewards() const noexcept { return m_luckyRewards; }

void ShopItem::setLuckyRewards(const std::vector<LuckyBagReward>& rewards) { m_luckyRewards = rewards; }

int ShopItem::levelRequirement() const noexcept { return m_levelRequirement; }

void ShopItem::setLevelRequirement(int level) noexcept { m_levelRequirement = level; }

std::string ShopItem::serializeLuckyRewards() const {
    QJsonArray entries;
    for (const auto& reward : m_luckyRewards) {
        QJsonObject obj;
        obj.insert("type", QString::fromStdString(rewardTypeToString(reward.type)));
        obj.insert("amount", reward.amount);
        obj.insert("probability", reward.probability);
        obj.insert("reference", reward.referenceItemId);
        obj.insert("description", QString::fromStdString(reward.description));
        entries.append(obj);
    }
    QJsonObject root;
    root.insert("entries", entries);
    QJsonDocument doc(root);
    return doc.toJson(QJsonDocument::Compact).toStdString();
}

void ShopItem::deserializeLuckyRewards(const std::string& json) {
    m_luckyRewards.clear();
    if (json.empty()) {
        return;
    }
    const QJsonDocument doc = QJsonDocument::fromJson(QByteArray::fromStdString(json));
    if (!doc.isObject()) {
        return;
    }
    const QJsonArray entries = doc.object().value("entries").toArray();
    for (const auto& value : entries) {
        if (!value.isObject()) {
            continue;
        }
        QJsonObject obj = value.toObject();
        LuckyBagReward reward;
        reward.type = rewardTypeFromString(obj.value("type").toString().toStdString());
        reward.amount = obj.value("amount").toInt();
        reward.probability = obj.value("probability").toDouble();
        reward.referenceItemId = obj.value("reference").toInt(-1);
        reward.description = obj.value("description").toString().toStdString();
        m_luckyRewards.push_back(reward);
    }
}

ShopItem ShopItem::fromRecord(const DatabaseManager::ShopItemRecord& record) {
    ShopItem item;
    item.m_id = record.id;
    item.m_name = record.name;
    item.m_description = record.description;
    item.m_iconPath = record.iconPath;
    item.m_itemType = itemTypeFromString(record.itemType);
    item.m_price = record.priceCoins;
    item.m_purchaseLimit = record.purchaseLimit;
    item.m_available = record.available;
    item.m_effectDescription = record.effectDescription;
    item.m_effectLogic = record.effectLogic;
    item.m_propEffectType = propEffectFromString(record.propEffectType);
    item.m_effectDurationMinutes = record.propDurationMinutes;
    item.m_usageConditions = record.usageConditions;
    item.m_physicalRedeemMethod = record.physicalRedeem;
    item.m_physicalNotes = record.physicalNotes;
    item.m_levelRequirement = record.levelRequirement;
    item.deserializeLuckyRewards(record.luckyBagRules);
    return item;
}

DatabaseManager::ShopItemRecord ShopItem::toRecord() const {
    DatabaseManager::ShopItemRecord record;
    record.id = m_id;
    record.name = m_name;
    record.description = m_description;
    record.iconPath = m_iconPath;
    record.itemType = itemTypeToString(m_itemType);
    record.priceCoins = m_price;
    record.purchaseLimit = m_purchaseLimit;
    record.available = m_available;
    record.effectDescription = m_effectDescription;
    record.effectLogic = m_effectLogic;
    record.propEffectType = propEffectToString(m_propEffectType);
    record.propDurationMinutes = m_effectDurationMinutes;
    record.usageConditions = m_usageConditions;
    record.physicalRedeem = m_physicalRedeemMethod;
    record.physicalNotes = m_physicalNotes;
    record.luckyBagRules = serializeLuckyRewards();
    record.levelRequirement = m_levelRequirement;
    return record;
}

std::string ShopItem::itemTypeToString(ItemType type) {
    switch (type) {
        case ItemType::Physical:
            return "Physical";
        case ItemType::Prop:
            return "Prop";
        case ItemType::LuckyBag:
            return "LuckyBag";
    }
    return "Physical";
}

ShopItem::ItemType ShopItem::itemTypeFromString(const std::string& text) {
    if (text == "Prop") {
        return ItemType::Prop;
    }
    if (text == "LuckyBag") {
        return ItemType::LuckyBag;
    }
    return ItemType::Physical;
}

std::string ShopItem::propEffectToString(PropEffectType type) {
    switch (type) {
        case PropEffectType::RestDay:
            return "RestDay";
        case PropEffectType::ForgivenessCoupon:
            return "ForgivenessCoupon";
        case PropEffectType::DoubleExpCard:
            return "DoubleExpCard";
        case PropEffectType::None:
        default:
            return "None";
    }
}

ShopItem::PropEffectType ShopItem::propEffectFromString(const std::string& text) {
    if (text == "RestDay") {
        return PropEffectType::RestDay;
    }
    if (text == "ForgivenessCoupon") {
        return PropEffectType::ForgivenessCoupon;
    }
    if (text == "DoubleExpCard") {
        return PropEffectType::DoubleExpCard;
    }
    return PropEffectType::None;
}

std::string ShopItem::rewardTypeToString(LuckyBagReward::RewardType type) {
    switch (type) {
        case LuckyBagReward::RewardType::Coins:
            return "Coins";
        case LuckyBagReward::RewardType::ShopItem:
            return "ShopItem";
        case LuckyBagReward::RewardType::Growth:
            return "Growth";
    }
    return "Coins";
}

ShopItem::LuckyBagReward::RewardType ShopItem::rewardTypeFromString(const std::string& text) {
    if (text == "ShopItem") {
        return LuckyBagReward::RewardType::ShopItem;
    }
    if (text == "Growth") {
        return LuckyBagReward::RewardType::Growth;
    }
    return LuckyBagReward::RewardType::Coins;
}

}  // namespace rove::data
