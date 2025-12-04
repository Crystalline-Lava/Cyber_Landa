#ifndef SHOPINTERFACE_H
#define SHOPINTERFACE_H

#include <QWidget>
#include <QTreeWidget>
#include <QPushButton>
#include <memory>
#include "../core/ShopManager.h"
#include "../core/InventoryManager.h"

namespace Ui {
class ShopInterface;
}

/**
 * @class ShopInterface
 * @brief 商店界面，分类展示商品并提供购买交互。
 * 中文说明：通过树控件按类别显示物品，提供购买按钮并与库存管理器同步更新。
 */
class ShopInterface : public QWidget {
    Q_OBJECT

public:
    ShopInterface(rove::data::ShopManager& shopManager,
                  rove::data::InventoryManager& inventoryManager,
                  QWidget* parent = nullptr);
    ~ShopInterface() override;

signals:
    /**
     * @brief 购买请求发射，供业务层扣币与入库。
     * @param itemId 物品标识。
     */
    void purchaseRequested(int itemId);

public slots:
    /**
     * @brief 重新加载商品列表。
     */
    void reload();

private slots:
    /**
     * @brief 处理购买按钮点击，读取当前选中商品。
     */
    void onPurchaseClicked();

private:
    /**
     * @brief 将商品填充到树控件。
     */
    void populate();

    std::unique_ptr<Ui::ShopInterface> ui;
    rove::data::ShopManager& m_shopManager;
    rove::data::InventoryManager& m_inventoryManager;
    QTreeWidget* m_tree{nullptr};
};

#endif  // SHOPINTERFACE_H

