#ifndef INVENTORYITEM_H
#define INVENTORYITEM_H

#include <QDateTime>

#include <string>

#include "DatabaseManager.h"

namespace rove::data {

/**
 * @class InventoryItem
 * @brief 库存条目模型，负责记录用户持有的商品、使用状态与幸运礼包展开信息。
 */
class InventoryItem final {
public:
    enum class UsageStatus { Unused, Active, Consumed, Expired };

    InventoryItem();

    int id() const noexcept;
    void setId(int id) noexcept;

    int itemId() const noexcept;
    void setItemId(int itemId) noexcept;

    const std::string& owner() const noexcept;
    void setOwner(std::string owner);

    const QDateTime& purchaseTime() const noexcept;
    void setPurchaseTime(const QDateTime& time);

    int quantity() const noexcept;
    void setQuantity(int quantity) noexcept;

    int usedQuantity() const noexcept;
    void setUsedQuantity(int usedQuantity) noexcept;

    UsageStatus status() const noexcept;
    void setStatus(UsageStatus status) noexcept;

    const QDateTime& expirationTime() const noexcept;
    void setExpirationTime(const QDateTime& time);

    const std::string& specialAttributes() const noexcept;
    void setSpecialAttributes(std::string attributes);

    const std::string& notes() const noexcept;
    void setNotes(std::string notes);

    bool isExpired(const QDateTime& now = QDateTime::currentDateTimeUtc()) const noexcept;

    static InventoryItem fromRecord(const DatabaseManager::InventoryRecord& record);
    DatabaseManager::InventoryRecord toRecord() const;

    static std::string statusToString(UsageStatus status);
    static UsageStatus statusFromString(const std::string& text);

private:
    int m_inventoryId;
    int m_itemId;
    std::string m_owner;
    QDateTime m_purchaseTime;
    int m_quantity;
    int m_usedQuantity;
    UsageStatus m_status;
    QDateTime m_expirationTime;
    bool m_hasExpiration;
    std::string m_specialAttributes;
    std::string m_notes;
};

}  // namespace rove::data

#endif  // INVENTORYITEM_H
