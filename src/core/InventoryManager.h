#ifndef INVENTORYMANAGER_H
#define INVENTORYMANAGER_H

#include <QDateTime>

#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "DatabaseManager.h"
#include "InventoryItem.h"
#include "ShopItem.h"

namespace rove::data {

/**
 * @class InventoryManager
 * @brief 库存管理器采用哈希表缓存 + SQLite 记录的混合结构：
 *        - 哈希表（m_effects）保存道具效果堆栈，便于 O(1) 查询 RestDay/原谅券/双倍经验状态；
 *        - SQLite 表 user_inventory 提供持久化与线程安全的行级锁保证；
 *        - std::mutex 保护所有复合读写，确保 UI 线程与后台清理线程并发安全。
 */
class InventoryManager final {
public:
    struct InventoryStatistics {
        int total = 0;
        int props = 0;
        int physical = 0;
        int luckyBags = 0;
        int expiringSoon = 0;
    };

    static InventoryManager& instance();

    void initialize(DatabaseManager& database);

    InventoryItem createFromShopItem(const ShopItem& item,
                                     const std::string& owner,
                                     int quantity,
                                     const std::string& specialAttributes = std::string());

    std::optional<InventoryItem> findById(int inventoryId) const;
    std::vector<InventoryItem> listByOwner(const std::string& owner) const;
    bool updateInventory(const InventoryItem& item);
    bool removeInventory(int inventoryId);

    void cleanupExpiredItems();
    InventoryStatistics statisticsForOwner(const std::string& owner) const;
    int countPurchasesForItem(const std::string& owner, int itemId) const;

    bool applyPropEffect(const ShopItem& item,
                         InventoryItem& entry,
                         const std::string& username,
                         std::string* message);
    bool markPhysicalRedeemed(InventoryItem& entry, const std::string& notes);
    bool markLuckyBagOpened(InventoryItem& entry, const std::string& payload);

    bool consumeEffectToken(const std::string& username, ShopItem::PropEffectType type);
    bool hasEffectToken(const std::string& username, ShopItem::PropEffectType type) const;
    double doubleExpMultiplier(const std::string& username) const;

private:
    InventoryManager();

    void ensureInitialized() const;
    void cleanupExpiredEffectsLocked(const std::string& username) const;
    void cleanupAllEffectsLocked() const;
    void registerEffectLocked(const std::string& username,
                              ShopItem::PropEffectType type,
                              int durationMinutes,
                              int stackDelta = 1);

    struct ActiveEffect {
        ShopItem::PropEffectType type;
        int stack;
        QDateTime expiresAt;
    };

    DatabaseManager* m_database;
    mutable std::mutex m_mutex;
    mutable std::unordered_map<std::string, std::vector<ActiveEffect>> m_effects;
};

}  // namespace rove::data

#endif  // INVENTORYMANAGER_H
