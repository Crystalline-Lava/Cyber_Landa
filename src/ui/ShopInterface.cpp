#include "ShopInterface.h"
#include "ui_ShopInterface.h"

#include <QMessageBox>

ShopInterface::ShopInterface(rove::data::ShopManager& shopManager,
                             rove::data::InventoryManager& inventoryManager,
                             QWidget* parent)
    : QWidget(parent)
    , ui(std::make_unique<Ui::ShopInterface>())
    , m_shopManager(shopManager)
    , m_inventoryManager(inventoryManager) {
    ui->setupUi(this);
    m_tree = ui->shopTree;
    connect(ui->purchaseBtn, &QPushButton::clicked, this, &ShopInterface::onPurchaseClicked);
    reload();
}

ShopInterface::~ShopInterface() = default;

void ShopInterface::reload() { populate(); }

/**
 * @brief 点击购买后校验并发射事件。
 */
void ShopInterface::onPurchaseClicked() {
    auto* item = m_tree->currentItem();
    if (!item) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请选择商品"));
        return;
    }
    emit purchaseRequested(item->data(0, Qt::UserRole).toInt());
}

/**
 * @brief 填充商品树，按照分类懒加载。
 */
void ShopInterface::populate() {
    m_tree->clear();
    const auto items = m_shopManager.listItems();
    QTreeWidgetItem* physicalRoot = new QTreeWidgetItem(m_tree, QStringList(QStringLiteral("实体")));
    QTreeWidgetItem* propRoot = new QTreeWidgetItem(m_tree, QStringList(QStringLiteral("道具")));
    QTreeWidgetItem* bagRoot = new QTreeWidgetItem(m_tree, QStringList(QStringLiteral("福袋")));
    for (const auto& item : items) {
        QTreeWidgetItem* parent = nullptr;
        if (item.itemType() == rove::data::ShopItem::ItemType::Physical) {
            parent = physicalRoot;
        } else if (item.itemType() == rove::data::ShopItem::ItemType::Prop) {
            parent = propRoot;
        } else {
            parent = bagRoot;
        }
        auto* child = new QTreeWidgetItem(parent);
        child->setText(0, QString::fromStdString(item.name()));
        child->setText(1, QString::number(item.priceCoins()));
        child->setText(2, QString::fromStdString(item.description()));
        child->setData(0, Qt::UserRole, item.id());
    }
    m_tree->expandAll();
}


