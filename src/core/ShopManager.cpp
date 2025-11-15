#include "ShopManager.h"

#include <algorithm>
#include <sstream>
#include <stdexcept>

#include "InventoryItem.h"
#include "InventoryManager.h"
#include "ShopItem.h"
#include "User.h"

namespace rove::data {
namespace {
constexpr int kThreeStarRewardBaseline = 60;
}

ShopManager& ShopManager::instance() {
    static ShopManager instance;
    return instance;
}

ShopManager::ShopManager() : m_database(nullptr), m_userManager(nullptr), m_inventoryManager(nullptr), m_mutex() {}

void ShopManager::initialize(DatabaseManager& database, UserManager& userManager, InventoryManager& inventoryManager) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_database = &database;
    m_userManager = &userManager;
    m_inventoryManager = &inventoryManager;
}

void ShopManager::ensureInitialized() const {
    if (m_database == nullptr || m_userManager == nullptr || m_inventoryManager == nullptr) {
        throw std::runtime_error("ShopManager 未初始化");
    }
}

int ShopManager::createItem(ShopItem item) {
    std::lock_guard<std::mutex> lock(m_mutex);
    ensureInitialized();
    ShopItem priced = applyPricingStrategy(item);
    auto record = priced.toRecord();
    const int newId = m_database->insertShopItem(record);
    return newId;
}

bool ShopManager::updateItem(ShopItem item) {
    std::lock_guard<std::mutex> lock(m_mutex);
    ensureInitialized();
    ShopItem priced = applyPricingStrategy(item);
    return m_database->updateShopItem(priced.toRecord());
}

bool ShopManager::removeItem(int itemId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    ensureInitialized();
    return m_database->deleteShopItem(itemId);
}

std::vector<ShopItem> ShopManager::listItems(bool includeUnavailable) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    ensureInitialized();
    const auto records = m_database->getAllShopItems();
    std::vector<ShopItem> items;
    for (const auto& record : records) {
        ShopItem item = ShopItem::fromRecord(record);
        if (!includeUnavailable && !item.isAvailable()) {
            continue;
        }
        items.push_back(item);
    }
    return items;
}

std::optional<ShopItem> ShopManager::findItem(int itemId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    ensureInitialized();
    auto record = m_database->getShopItemById(itemId);
    if (!record.has_value()) {
        return std::nullopt;
    }
    return ShopItem::fromRecord(*record);
}

/**
 * 中文说明：交易系统的安全性与回滚
 * - purchaseItem 会在最外层调用 DatabaseManager::beginTransaction，保证“扣币 + 入库”原子性；
 * - UserManager::saveActiveUser 内部有嵌套事务，借助 DatabaseManager 的递归深度控制；
 * - 任一环节失败都会 rollbackTransaction，确保兰大币不会凭空消失或重复扣除。
 */
ShopManager::PurchaseResult ShopManager::purchaseItem(int itemId, int quantity) {
    std::lock_guard<std::mutex> lock(m_mutex);
    ensureInitialized();
    PurchaseResult result;
    if (!m_userManager->hasActiveUser()) {
        result.message = "请先登录后再购买";
        return result;
    }
    auto itemOpt = findItem(itemId);
    if (!itemOpt.has_value()) {
        result.message = "商品不存在";
        return result;
    }
    ShopItem item = applyPricingStrategy(*itemOpt);
    std::string reason;
    const User& user = m_userManager->activeUser();
    if (!validatePurchase(item, user, quantity, reason)) {
        result.message = reason;
        return result;
    }
    const int totalCost = item.priceCoins() * quantity;
    bool transactionStarted = false;
    try {
        transactionStarted = m_database->beginTransaction();
        User& mutableUser = m_userManager->activeUser();
        mutableUser.spendCoins(totalCost);
        m_userManager->saveActiveUser();
        for (int i = 0; i < quantity; ++i) {
            InventoryItem entry = m_inventoryManager->createFromShopItem(item, mutableUser.username(), 1);
            result.grantedItems.push_back(entry);
        }
        m_database->commitTransaction();
        result.success = true;
        std::ostringstream oss;
        oss << "购买 " << item.name() << " x" << quantity << " 成功，花费 " << totalCost << " 兰大币";
        result.message = oss.str();
    } catch (...) {
        if (transactionStarted) {
            try {
                m_database->rollbackTransaction();
            } catch (...) {
            }
        }
        throw;
    }
    return result;
}

bool ShopManager::useInventoryItem(int inventoryId, std::string* message) {
    std::lock_guard<std::mutex> lock(m_mutex);
    ensureInitialized();
    if (!m_userManager->hasActiveUser()) {
        if (message != nullptr) {
            *message = "请先登录";
        }
        return false;
    }
    auto entryOpt = m_inventoryManager->findById(inventoryId);
    if (!entryOpt.has_value()) {
        if (message != nullptr) {
            *message = "道具不存在";
        }
        return false;
    }
    InventoryItem entry = *entryOpt;
    if (entry.owner() != m_userManager->activeUser().username()) {
        if (message != nullptr) {
            *message = "无权使用他人道具";
        }
        return false;
    }
    auto itemOpt = findItem(entry.itemId());
    if (!itemOpt.has_value()) {
        if (message != nullptr) {
            *message = "对应商品缺失";
        }
        return false;
    }
    ShopItem item = *itemOpt;
    bool transactionStarted = false;
    try {
        transactionStarted = m_database->beginTransaction();
        std::string feedback;
        if (entry.isExpired()) {
            entry.setStatus(InventoryItem::UsageStatus::Expired);
            m_inventoryManager->updateInventory(entry);
            feedback = "道具已过期";
        } else {
            switch (item.itemType()) {
                case ShopItem::ItemType::Physical: {
                    std::string notes = "学生确认兑换：" + item.physicalRedeemMethod();
                    m_inventoryManager->markPhysicalRedeemed(entry, notes);
                    feedback = "已登记实物奖励，请在备注中查看兑换方式";
                    break;
                }
                case ShopItem::ItemType::Prop: {
                    m_inventoryManager->applyPropEffect(item, entry, entry.owner(), &feedback);
                    break;
                }
                case ShopItem::ItemType::LuckyBag: {
                    LuckyBagOutcome outcome = rollLuckyBag(item);
                    applyLuckyBagReward(outcome, entry.owner());
                    m_inventoryManager->markLuckyBagOpened(entry, outcome.payload);
                    feedback = "已开启幸运包：" + outcome.payload;
                    break;
                }
            }
        }
        m_database->commitTransaction();
        if (message != nullptr) {
            *message = feedback;
        }
    } catch (...) {
        if (transactionStarted) {
            try {
                m_database->rollbackTransaction();
            } catch (...) {
            }
        }
        throw;
    }
    return true;
}

/**
 * 中文说明：兰大币平衡算法
 * - Physical：将价格限制在 [5,10] 倍 3 星任务奖励区间，避免学生过早获得过多实物奖励；
 * - Prop：以 30 分钟为一个计费单元，乘以道具权重（双倍经验 1.5，休息日/原谅券 1.0），保证“时间越长越贵”；
 * - LuckyBag：计算奖励期望值 expected，再额外乘以 1.2 作为“抽卡税”，既防止通胀又保留惊喜感。
 */
ShopItem ShopManager::applyPricingStrategy(const ShopItem& item) const {
    ShopItem priced = item;
    if (priced.priceCoins() <= 0) {
        priced.setPriceCoins(kThreeStarRewardBaseline);
    }
    switch (priced.itemType()) {
        case ShopItem::ItemType::Physical: {
            const int minPrice = kThreeStarRewardBaseline * 5;
            const int maxPrice = kThreeStarRewardBaseline * 10;
            int clamped = std::max(minPrice, std::min(priced.priceCoins(), maxPrice));
            priced.setPriceCoins(clamped);
            break;
        }
        case ShopItem::ItemType::Prop: {
            const int duration = std::max(priced.effectDurationMinutes(), 30);
            double weight = 1.0;
            if (priced.propEffectType() == ShopItem::PropEffectType::DoubleExpCard) {
                weight = 1.5;
            }
            const int unitCount = (duration + 29) / 30;
            int price = static_cast<int>(kThreeStarRewardBaseline * weight * unitCount);
            priced.setPriceCoins(std::max(price, kThreeStarRewardBaseline / 2));
            break;
        }
        case ShopItem::ItemType::LuckyBag: {
            double expected = 0.0;
            if (priced.luckyRewards().empty()) {
                expected = kThreeStarRewardBaseline;
            } else {
                for (const auto& reward : priced.luckyRewards()) {
                    switch (reward.type) {
                        case ShopItem::LuckyBagReward::RewardType::Coins:
                            expected += reward.amount * reward.probability;
                            break;
                        case ShopItem::LuckyBagReward::RewardType::Growth:
                            expected += reward.amount * reward.probability;
                            break;
                        case ShopItem::LuckyBagReward::RewardType::ShopItem:
                            expected += kThreeStarRewardBaseline * reward.probability;
                            break;
                    }
                }
            }
            int price = static_cast<int>(std::max(expected * 1.2, static_cast<double>(kThreeStarRewardBaseline)));
            priced.setPriceCoins(price);
            break;
        }
    }
    return priced;
}

/**
 * 中文说明：与用户系统的货币/等级集成校验
 * - validatePurchase 使用 UserManager::activeUser 提供的等级与币值；
 * - 先检查等级是否满足商品 levelRequirement，再计算 totalCost 是否超出 coins；
 * - 通过 InventoryManager::countPurchasesForItem 实现限购，防止刷道具。
 */
bool ShopManager::validatePurchase(const ShopItem& item, const User& user, int quantity, std::string& reason) const {
    if (quantity <= 0) {
        reason = "数量必须大于 0";
        return false;
    }
    if (!item.isAvailable()) {
        reason = "商品暂未上架";
        return false;
    }
    if (user.level() < item.levelRequirement()) {
        reason = "等级不足，无法购买";
        return false;
    }
    const int totalCost = item.priceCoins() * quantity;
    if (user.coins() < totalCost) {
        reason = "兰大币余额不足";
        return false;
    }
    if (item.purchaseLimit() > 0) {
        const int purchased = m_inventoryManager->countPurchasesForItem(user.username(), item.id());
        if (purchased + quantity > item.purchaseLimit()) {
            reason = "已达到限购次数";
            return false;
        }
    }
    return true;
}

/**
 * 中文说明：幸运礼包随机算法
 * - 先遍历 luckyRewards，累加概率形成累积分布；
 * - 使用 QRandomGenerator::global()->generateDouble() 生成 [0,1) 随机数，再映射到累积分布；
 * - 若概率和不足 1，则自动归一化到最后一档，保证每次必有奖励；
 * - outcome.payload 记录 JSON 字符串，方便 UI 展示和库存追踪。
 */
ShopManager::LuckyBagOutcome ShopManager::rollLuckyBag(const ShopItem& luckyBag) {
    LuckyBagOutcome outcome;
    const auto& rewards = luckyBag.luckyRewards();
    if (rewards.empty()) {
        outcome.reward.type = ShopItem::LuckyBagReward::RewardType::Coins;
        outcome.reward.amount = kThreeStarRewardBaseline;
        outcome.reward.probability = 1.0;
    } else {
        double totalProbability = 0.0;
        for (const auto& reward : rewards) {
            totalProbability += reward.probability;
        }
        const double randomValue = QRandomGenerator::global()->generateDouble();
        const double target = totalProbability > 0.0 ? randomValue * totalProbability : randomValue;
        double cursor = 0.0;
        for (const auto& reward : rewards) {
            cursor += reward.probability;
            if (target <= cursor) {
                outcome.reward = reward;
                break;
            }
        }
        if (outcome.reward.probability <= 0.0) {
            outcome.reward = rewards.back();
        }
    }
    std::ostringstream oss;
    oss << "{\"type\":\"" << ShopItem::rewardTypeToString(outcome.reward.type) << "\",";
    oss << "\"amount\":" << outcome.reward.amount << ",";
    oss << "\"desc\":\"" << outcome.reward.description << "\"}";
    outcome.payload = oss.str();
    return outcome;
}

void ShopManager::applyLuckyBagReward(const LuckyBagOutcome& outcome, const std::string& username) {
    switch (outcome.reward.type) {
        case ShopItem::LuckyBagReward::RewardType::Coins: {
            User& user = m_userManager->activeUser();
            user.addCoins(outcome.reward.amount);
            m_userManager->saveActiveUser();
            break;
        }
        case ShopItem::LuckyBagReward::RewardType::Growth: {
            User& user = m_userManager->activeUser();
            user.addGrowthPoints(outcome.reward.amount);
            m_userManager->saveActiveUser();
            break;
        }
        case ShopItem::LuckyBagReward::RewardType::ShopItem: {
            if (outcome.reward.referenceItemId > 0) {
                auto referenced = findItem(outcome.reward.referenceItemId);
                if (referenced.has_value()) {
                    m_inventoryManager->createFromShopItem(*referenced, username, 1);
                }
            }
            break;
        }
    }
}

}  // namespace rove::data
