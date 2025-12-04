/********************************************************************************
** Form generated from reading UI file 'ShopInterface.ui'
**
** Created by: Qt User Interface Compiler version 6.9.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_SHOPINTERFACE_H
#define UI_SHOPINTERFACE_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_ShopInterface
{
public:
    QVBoxLayout *verticalLayout;
    QTreeWidget *shopTree;
    QPushButton *purchaseBtn;

    void setupUi(QWidget *ShopInterface)
    {
        if (ShopInterface->objectName().isEmpty())
            ShopInterface->setObjectName("ShopInterface");
        verticalLayout = new QVBoxLayout(ShopInterface);
        verticalLayout->setObjectName("verticalLayout");
        shopTree = new QTreeWidget(ShopInterface);
        shopTree->setObjectName("shopTree");

        verticalLayout->addWidget(shopTree);

        purchaseBtn = new QPushButton(ShopInterface);
        purchaseBtn->setObjectName("purchaseBtn");

        verticalLayout->addWidget(purchaseBtn);


        retranslateUi(ShopInterface);

        QMetaObject::connectSlotsByName(ShopInterface);
    } // setupUi

    void retranslateUi(QWidget *ShopInterface)
    {
        QTreeWidgetItem *___qtreewidgetitem = shopTree->headerItem();
        ___qtreewidgetitem->setText(2, QCoreApplication::translate("ShopInterface", "\346\217\217\350\277\260", nullptr));
        ___qtreewidgetitem->setText(1, QCoreApplication::translate("ShopInterface", "\344\273\267\346\240\274", nullptr));
        ___qtreewidgetitem->setText(0, QCoreApplication::translate("ShopInterface", "\347\211\251\345\223\201", nullptr));
        purchaseBtn->setText(QCoreApplication::translate("ShopInterface", "\350\264\255\344\271\260\351\200\211\344\270\255", nullptr));
        (void)ShopInterface;
    } // retranslateUi

};

namespace Ui {
    class ShopInterface: public Ui_ShopInterface {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_SHOPINTERFACE_H
