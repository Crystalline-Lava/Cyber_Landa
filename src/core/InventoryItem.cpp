#include "InventoryItem.h"

#include <QString>

namespace rove::data {

InventoryItem::InventoryItem()
    : m_inventoryId(-1),
      m_itemId(-1),
      m_owner(),
      m_purchaseTime(QDateTime::currentDateTimeUtc()),
      m_quantity(0),
      m_usedQuantity(0),
      m_status(UsageStatus::Unused),
      m_expirationTime(),
      m_hasExpiration(false),
      m_specialAttributes("{}"),
      m_notes() {}

int InventoryItem::id() const noexcept { return m_inventoryId; }

void InventoryItem::setId(int id) noexcept { m_inventoryId = id; }

int InventoryItem::itemId() const noexcept { return m_itemId; }

void InventoryItem::setItemId(int itemId) noexcept { m_itemId = itemId; }

const std::string& InventoryItem::owner() const noexcept { return m_owner; }

void InventoryItem::setOwner(std::string owner) { m_owner = std::move(owner); }

const QDateTime& InventoryItem::purchaseTime() const noexcept { return m_purchaseTime; }

void InventoryItem::setPurchaseTime(const QDateTime& time) { m_purchaseTime = time; }

int InventoryItem::quantity() const noexcept { return m_quantity; }

void InventoryItem::setQuantity(int quantity) noexcept { m_quantity = quantity; }

int InventoryItem::usedQuantity() const noexcept { return m_usedQuantity; }

void InventoryItem::setUsedQuantity(int usedQuantity) noexcept { m_usedQuantity = usedQuantity; }

InventoryItem::UsageStatus InventoryItem::status() const noexcept { return m_status; }

void InventoryItem::setStatus(UsageStatus status) noexcept { m_status = status; }

const QDateTime& InventoryItem::expirationTime() const noexcept { return m_expirationTime; }

void InventoryItem::setExpirationTime(const QDateTime& time) {
    m_expirationTime = time;
    m_hasExpiration = time.isValid();
}

const std::string& InventoryItem::specialAttributes() const noexcept { return m_specialAttributes; }

void InventoryItem::setSpecialAttributes(std::string attributes) { m_specialAttributes = std::move(attributes); }

const std::string& InventoryItem::notes() const noexcept { return m_notes; }

void InventoryItem::setNotes(std::string notes) { m_notes = std::move(notes); }

bool InventoryItem::isExpired(const QDateTime& now) const noexcept {
    if (!m_hasExpiration || !m_expirationTime.isValid()) {
        return false;
    }
    return m_expirationTime < now;
}

InventoryItem InventoryItem::fromRecord(const DatabaseManager::InventoryRecord& record) {
    InventoryItem item;
    item.m_inventoryId = record.id;
    item.m_itemId = record.itemId;
    item.m_owner = record.owner;
    item.m_purchaseTime = QDateTime::fromString(QString::fromStdString(record.purchaseTimeIso), Qt::ISODate);
    if (!record.expirationTimeIso.empty()) {
        item.m_expirationTime = QDateTime::fromString(QString::fromStdString(record.expirationTimeIso), Qt::ISODate);
        item.m_hasExpiration = item.m_expirationTime.isValid();
    } else {
        item.m_expirationTime = QDateTime();
        item.m_hasExpiration = false;
    }
    item.m_quantity = record.quantity;
    item.m_usedQuantity = record.usedQuantity;
    item.m_status = statusFromString(record.status);
    item.m_specialAttributes = record.luckyPayload;
    item.m_notes = record.notes;
    return item;
}

DatabaseManager::InventoryRecord InventoryItem::toRecord() const {
    DatabaseManager::InventoryRecord record;
    record.id = m_inventoryId;
    record.itemId = m_itemId;
    record.owner = m_owner;
    record.quantity = m_quantity;
    record.usedQuantity = m_usedQuantity;
    record.status = statusToString(m_status);
    record.purchaseTimeIso = m_purchaseTime.toString(Qt::ISODate).toStdString();
    record.expirationTimeIso =
        m_hasExpiration && m_expirationTime.isValid() ? m_expirationTime.toString(Qt::ISODate).toStdString() : std::string();
    record.luckyPayload = m_specialAttributes;
    record.notes = m_notes;
    return record;
}

std::string InventoryItem::statusToString(UsageStatus status) {
    switch (status) {
        case UsageStatus::Active:
            return "Active";
        case UsageStatus::Consumed:
            return "Consumed";
        case UsageStatus::Expired:
            return "Expired";
        case UsageStatus::Unused:
        default:
            return "Unused";
    }
}

InventoryItem::UsageStatus InventoryItem::statusFromString(const std::string& text) {
    if (text == "Active") {
        return UsageStatus::Active;
    }
    if (text == "Consumed") {
        return UsageStatus::Consumed;
    }
    if (text == "Expired") {
        return UsageStatus::Expired;
    }
    return UsageStatus::Unused;
}

}  // namespace rove::data
