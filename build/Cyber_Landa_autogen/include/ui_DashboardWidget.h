/********************************************************************************
** Form generated from reading UI file 'DashboardWidget.ui'
**
** Created by: Qt User Interface Compiler version 6.9.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_DASHBOARDWIDGET_H
#define UI_DASHBOARDWIDGET_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QLabel>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_DashboardWidget
{
public:
    QVBoxLayout *verticalLayout;
    QLabel *statsLabel;
    QWidget *chartContainer;
    QVBoxLayout *chartLayout;
    QListWidget *activityList;

    void setupUi(QWidget *DashboardWidget)
    {
        if (DashboardWidget->objectName().isEmpty())
            DashboardWidget->setObjectName("DashboardWidget");
        verticalLayout = new QVBoxLayout(DashboardWidget);
        verticalLayout->setObjectName("verticalLayout");
        statsLabel = new QLabel(DashboardWidget);
        statsLabel->setObjectName("statsLabel");

        verticalLayout->addWidget(statsLabel);

        chartContainer = new QWidget(DashboardWidget);
        chartContainer->setObjectName("chartContainer");
        chartLayout = new QVBoxLayout(chartContainer);
        chartLayout->setObjectName("chartLayout");
        chartLayout->setContentsMargins(0, 0, 0, 0);

        verticalLayout->addWidget(chartContainer);

        activityList = new QListWidget(DashboardWidget);
        activityList->setObjectName("activityList");

        verticalLayout->addWidget(activityList);


        retranslateUi(DashboardWidget);

        QMetaObject::connectSlotsByName(DashboardWidget);
    } // setupUi

    void retranslateUi(QWidget *DashboardWidget)
    {
        statsLabel->setText(QCoreApplication::translate("DashboardWidget", "\347\255\211\347\272\247/\351\207\221\345\270\201", nullptr));
        (void)DashboardWidget;
    } // retranslateUi

};

namespace Ui {
    class DashboardWidget: public Ui_DashboardWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_DASHBOARDWIDGET_H
