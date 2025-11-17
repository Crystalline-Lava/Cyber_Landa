#include "InventoryManager.h"

#include <algorithm>
#include <stdexcept>
#include <string>

namespace rove::data {
namespace {
constexpr int kMaxEffectStack = 3;
constexpr int kExpiringSoonHours = 48;
}

InventoryManager& InventoryManager::instance() {
    static InventoryManager instance;
    return instance;
}

InventoryManager::InventoryManager() : m_database(nullptr), m_mutex(), m_effects() {}

void InventoryManager::initialize(DatabaseManager& database) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_database = &database;
    m_effects.clear();
}

void InventoryManager::ensureInitialized() const {
    if (m_database == nullptr) {
        throw std::runtime_error("InventoryManager is not initialized");
    }
}

InventoryItem InventoryManager::createFromShopItem(const ShopItem& item,
                                                   const std::string& owner,
                                                   int quantity,
                                                   const std::string& specialAttributes) {
    std::lock_guard<std::mutex> lock(m_mutex);
    ensureInitialized();
    InventoryItem entry;
    entry.setOwner(owner);
    entry.setItemId(item.id());
    entry.setQuantity(quantity);
    entry.setUsedQuantity(0);
    entry.setStatus(InventoryItem::UsageStatus::Unused);
    const QDateTime now = QDateTime::currentDateTimeUtc();
    entry.setPurchaseTime(now);
    if (item.itemType() == ShopItem::ItemType::Prop && item.effectDurationMinutes() > 0) {
        entry.setExpirationTime(now.addSecs(item.effectDurationMinutes() * 60));
    }
    std::string payload = specialAttributes;
    if (payload.empty()) {
        payload = item.itemType() == ShopItem::ItemType::LuckyBag ? item.serializeLuckyRewards() : std::string("{}");
    }
    entry.setSpecialAttributes(payload);
    if (item.itemType() == ShopItem::ItemType::Physical) {
        entry.setNotes(item.physicalRedeemMethod() + " | " + item.physicalNotes());
    } else {
        entry.setNotes(item.effectDescription());
    }
    auto record = entry.toRecord();
    record.owner = owner;
    record.itemId = item.id();
    record.luckyPayload = entry.specialAttributes();
    const int newId = m_database->insertInventoryRecord(record);
    entry.setId(newId);
    return entry;
}

std::optional<InventoryItem> InventoryManager::findById(int inventoryId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    ensureInitialized();
    auto record = m_database->getInventoryRecordById(inventoryId);
    if (!record.has_value()) {
        return std::nullopt;
    }
    return InventoryItem::fromRecord(*record);
}

std::vector<InventoryItem> InventoryManager::listByOwner(const std::string& owner) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    ensureInitialized();
    auto records = m_database->getInventoryForUser(owner);
    std::vector<InventoryItem> items;
    items.reserve(records.size());
    for (const auto& record : records) {
        items.push_back(InventoryItem::fromRecord(record));
    }
    return items;
}

bool InventoryManager::updateInventory(const InventoryItem& item) {
    std::lock_guard<std::mutex> lock(m_mutex);
    ensureInitialized();
    return m_database->updateInventoryRecord(item.toRecord());
}

bool InventoryManager::removeInventory(int inventoryId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    ensureInitialized();
    return m_database->deleteInventoryRecord(inventoryId);
}

void InventoryManager::cleanupExpiredItems() {
    std::lock_guard<std::mutex> lock(m_mutex);
    ensureInitialized();
    const auto records = m_database->getAllInventoryRecords();
    const QDateTime now = QDateTime::currentDateTimeUtc();
    for (const auto& record : records) {
        InventoryItem item = InventoryItem::fromRecord(record);
        if (item.status() == InventoryItem::UsageStatus::Expired) {
            continue;
        }
        if (item.isExpired(now)) {
            item.setStatus(InventoryItem::UsageStatus::Expired);
            item.setNotes("效果已过期，系统自动回收");
            m_database->updateInventoryRecord(item.toRecord());
        }
    }
    cleanupAllEffectsLocked();
}

InventoryManager::InventoryStatistics InventoryManager::statisticsForOwner(const std::string& owner) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    ensureInitialized();
    InventoryStatistics stats;
    const auto records = m_database->getInventoryForUser(owner);
    const QDateTime now = QDateTime::currentDateTimeUtc();
    for (const auto& record : records) {
        const InventoryItem item = InventoryItem::fromRecord(record);
        ++stats.total;
        if (item.expirationTime().isValid() && item.expirationTime() > now &&
            item.expirationTime() < now.addSecs(kExpiringSoonHours * 3600)) {
            ++stats.expiringSoon;
        }
        auto shopRecord = m_database->getShopItemById(item.itemId());
        if (!shopRecord.has_value()) {
            continue;
        }
        switch (ShopItem::itemTypeFromString(shopRecord->itemType)) {
            case ShopItem::ItemType::Physical:
                ++stats.physical;
                break;
            case ShopItem::ItemType::Prop:
                ++stats.props;
                break;
            case ShopItem::ItemType::LuckyBag:
                ++stats.luckyBags;
                break;
        }
    }
    return stats;
}

int InventoryManager::countPurchasesForItem(const std::string& owner, int itemId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    ensureInitialized();
    return m_database->countInventoryByUserAndItem(owner, itemId);
}

/**
 * 中文说明：道具效果技术实现
 * 1. RestDay/原谅券：使用 registerEffectLocked 在 m_effects 哈希表中累积堆栈，并记录到期时间；
 *    其他系统可通过 hasEffectToken/consumeEffectToken 以 O(1) 查询/消费，保证“跳过任务”与“失败清零”逻辑可靠。
 * 2. DoubleExpCard：堆栈代表倍率-1，多个卡片会延长 expiresAt 并叠加 stack，doubleExpMultiplier 会返回 1+stack 的实时倍率。
 * 3. 所有效果写入 user_inventory.special_attributes，方便重新登录后恢复 UI 状态；
 * 4. 所有操作均在 mutex 保护下执行，确保多线程安全。
 */
bool InventoryManager::applyPropEffect(const ShopItem& item,
                                       InventoryItem& entry,
                                       const std::string& username,
                                       std::string* message) {
    std::lock_guard<std::mutex> lock(m_mutex);
    ensureInitialized();
    cleanupExpiredEffectsLocked(username);
    std::string feedback;
    switch (item.propEffectType()) {
        case ShopItem::PropEffectType::RestDay:
            registerEffectLocked(username, ShopItem::PropEffectType::RestDay, item.effectDurationMinutes());
            feedback = "已登记一张休息日卡，可在时效内跳过一次每日任务";
            break;
        case ShopItem::PropEffectType::ForgivenessCoupon:
            registerEffectLocked(username, ShopItem::PropEffectType::ForgivenessCoupon, item.effectDurationMinutes());
            feedback = "已存入原谅券，下一次任务失败会被清零记录";
            break;
        case ShopItem::PropEffectType::DoubleExpCard:
            registerEffectLocked(username, ShopItem::PropEffectType::DoubleExpCard, item.effectDurationMinutes());
            feedback = "已激活双倍成长 buff";
            break;
        case ShopItem::PropEffectType::None:
            feedback = "该道具无实际效果";
            break;
    }
    entry.setStatus(InventoryItem::UsageStatus::Consumed);
    entry.setUsedQuantity(entry.quantity());
    entry.setSpecialAttributes("{\"effect\":\"" + ShopItem::propEffectToString(item.propEffectType()) + "\"}");
    m_database->updateInventoryRecord(entry.toRecord());
    if (message != nullptr) {
        *message = feedback;
    }
    return true;
}

bool InventoryManager::markPhysicalRedeemed(InventoryItem& entry, const std::string& notes) {
    std::lock_guard<std::mutex> lock(m_mutex);
    ensureInitialized();
    entry.setStatus(InventoryItem::UsageStatus::Consumed);
    entry.setUsedQuantity(entry.quantity());
    entry.setNotes(notes);
    return m_database->updateInventoryRecord(entry.toRecord());
}

bool InventoryManager::markLuckyBagOpened(InventoryItem& entry, const std::string& payload) {
    std::lock_guard<std::mutex> lock(m_mutex);
    ensureInitialized();
    entry.setStatus(InventoryItem::UsageStatus::Consumed);
    entry.setUsedQuantity(entry.quantity());
    entry.setSpecialAttributes(payload);
    return m_database->updateInventoryRecord(entry.toRecord());
}

bool InventoryManager::consumeEffectToken(const std::string& username, ShopItem::PropEffectType type) {
    std::lock_guard<std::mutex> lock(m_mutex);
    ensureInitialized();
    cleanupExpiredEffectsLocked(username);
    auto it = m_effects.find(username);
    if (it == m_effects.end()) {
        return false;
    }
    auto& bucket = it->second;
    for (auto& effect : bucket) {
        if (effect.type == type && effect.stack > 0) {
            --effect.stack;
            if (effect.stack == 0) {
                effect.expiresAt = QDateTime::currentDateTimeUtc();
            }
            return true;
        }
    }
    return false;
}

bool InventoryManager::hasEffectToken(const std::string& username, ShopItem::PropEffectType type) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    ensureInitialized();
    auto it = m_effects.find(username);
    if (it == m_effects.end()) {
        return false;
    }
    const QDateTime now = QDateTime::currentDateTimeUtc();
    for (const auto& effect : it->second) {
        if (effect.type == type && effect.expiresAt > now && effect.stack > 0) {
            return true;
        }
    }
    return false;
}

double InventoryManager::doubleExpMultiplier(const std::string& username) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    ensureInitialized();
    auto it = m_effects.find(username);
    if (it == m_effects.end()) {
        return 1.0;
    }
    const QDateTime now = QDateTime::currentDateTimeUtc();
    for (const auto& effect : it->second) {
        if (effect.type == ShopItem::PropEffectType::DoubleExpCard && effect.expiresAt > now) {
            return 1.0 + effect.stack;
        }
    }
    return 1.0;
}

void InventoryManager::cleanupExpiredEffectsLocked(const std::string& username) const {
    auto it = m_effects.find(username);
    if (it == m_effects.end()) {
        return;
    }
    const QDateTime now = QDateTime::currentDateTimeUtc();
    auto& bucket = it->second;
    bucket.erase(
        std::remove_if(bucket.begin(), bucket.end(), [&now](const ActiveEffect& effect) { return effect.expiresAt <= now; }),
        bucket.end());
    if (bucket.empty()) {
        m_effects.erase(it);
    }
}

void InventoryManager::cleanupAllEffectsLocked() const {
    const QDateTime now = QDateTime::currentDateTimeUtc();
    for (auto it = m_effects.begin(); it != m_effects.end();) {
        auto& bucket = it->second;
        bucket.erase(std::remove_if(bucket.begin(),
                                    bucket.end(),
                                    [&now](const ActiveEffect& effect) { return effect.expiresAt <= now; }),
                     bucket.end());
        if (bucket.empty()) {
            it = m_effects.erase(it);
        } else {
            ++it;
        }
    }
}

void InventoryManager::registerEffectLocked(const std::string& username,
                                            ShopItem::PropEffectType type,
                                            int durationMinutes,
                                            int stackDelta) {
    cleanupExpiredEffectsLocked(username);
    auto& bucket = m_effects[username];
    const QDateTime now = QDateTime::currentDateTimeUtc();
    auto it = std::find_if(bucket.begin(), bucket.end(), [type](const ActiveEffect& effect) { return effect.type == type; });
    const int duration = durationMinutes > 0 ? durationMinutes : 1440;
    if (it == bucket.end()) {
        ActiveEffect effect{type, std::min(stackDelta, kMaxEffectStack), now.addSecs(duration * 60)};
        bucket.push_back(effect);
    } else {
        it->stack = std::min(it->stack + stackDelta, kMaxEffectStack);
        it->expiresAt = it->expiresAt.addSecs(duration * 60);
    }
}

}  // namespace rove::data
