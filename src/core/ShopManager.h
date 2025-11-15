#ifndef SHOPMANAGER_H
#define SHOPMANAGER_H

#include <QRandomGenerator>

#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include "DatabaseManager.h"
#include "InventoryItem.h"
#include "InventoryManager.h"
#include "ShopItem.h"
#include "UserManager.h"

namespace rove::data {

/**
 * @class ShopManager
 * @brief 商城系统分三类商品：
 *        1) Physical 实物奖励：遵循“3 星任务奖励 ≈ 中档实物的 1/5~1/10”定价；
 *        2) Prop 系统道具：根据时效/功能计算兰大币成本，并与任务难度形成闭环；
 *        3) LuckyBag 幸运礼包：存储概率表，使用 QRandomGenerator 保证公平可复现。
 *        该类负责商品 CRUD、购买校验、交易事务、库存落地与礼包抽奖。
 */
class ShopManager final {
public:
    struct PurchaseResult {
        bool success = false;
        std::string message;
        std::vector<InventoryItem> grantedItems;
    };

    static ShopManager& instance();

    void initialize(DatabaseManager& database, UserManager& userManager, InventoryManager& inventoryManager);

    int createItem(ShopItem item);
    bool updateItem(ShopItem item);
    bool removeItem(int itemId);

    std::vector<ShopItem> listItems(bool includeUnavailable = false) const;
    std::optional<ShopItem> findItem(int itemId) const;

    PurchaseResult purchaseItem(int itemId, int quantity);
    bool useInventoryItem(int inventoryId, std::string* message);

private:
    ShopManager();

    void ensureInitialized() const;
    ShopItem applyPricingStrategy(const ShopItem& item) const;
    bool validatePurchase(const ShopItem& item, const User& user, int quantity, std::string& reason) const;

    struct LuckyBagOutcome {
        ShopItem::LuckyBagReward reward;
        std::string payload;
    };

    LuckyBagOutcome rollLuckyBag(const ShopItem& luckyBag);
    void applyLuckyBagReward(const LuckyBagOutcome& outcome, const std::string& username);

    DatabaseManager* m_database;
    UserManager* m_userManager;
    InventoryManager* m_inventoryManager;
    mutable std::mutex m_mutex;
};

}  // namespace rove::data

#endif  // SHOPMANAGER_H
