/********************************************************************************
** Form generated from reading UI file 'GrowthDashboard.ui'
**
** Created by: Qt User Interface Compiler version 6.9.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_GROWTHDASHBOARD_H
#define UI_GROWTHDASHBOARD_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_GrowthDashboard
{
public:
    QHBoxLayout *horizontalLayout;
    QWidget *radarContainer;
    QVBoxLayout *radarLayout;
    QWidget *timelineContainer;
    QVBoxLayout *timelineLayout;

    void setupUi(QWidget *GrowthDashboard)
    {
        if (GrowthDashboard->objectName().isEmpty())
            GrowthDashboard->setObjectName("GrowthDashboard");
        horizontalLayout = new QHBoxLayout(GrowthDashboard);
        horizontalLayout->setObjectName("horizontalLayout");
        radarContainer = new QWidget(GrowthDashboard);
        radarContainer->setObjectName("radarContainer");
        radarLayout = new QVBoxLayout(radarContainer);
        radarLayout->setObjectName("radarLayout");
        radarLayout->setContentsMargins(0, 0, 0, 0);

        horizontalLayout->addWidget(radarContainer);

        timelineContainer = new QWidget(GrowthDashboard);
        timelineContainer->setObjectName("timelineContainer");
        timelineLayout = new QVBoxLayout(timelineContainer);
        timelineLayout->setObjectName("timelineLayout");
        timelineLayout->setContentsMargins(0, 0, 0, 0);

        horizontalLayout->addWidget(timelineContainer);


        retranslateUi(GrowthDashboard);

        QMetaObject::connectSlotsByName(GrowthDashboard);
    } // setupUi

    void retranslateUi(QWidget *GrowthDashboard)
    {
        (void)GrowthDashboard;
    } // retranslateUi

};

namespace Ui {
    class GrowthDashboard: public Ui_GrowthDashboard {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_GROWTHDASHBOARD_H
