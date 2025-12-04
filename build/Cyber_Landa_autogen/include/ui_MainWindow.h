/********************************************************************************
** Form generated from reading UI file 'MainWindow.ui'
**
** Created by: Qt User Interface Compiler version 6.9.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralwidget;
    QHBoxLayout *horizontalLayout;
    QWidget *navPanel;
    QVBoxLayout *verticalLayout;
    QLabel *profileLabel;
    QLabel *notificationLabel;
    QPushButton *dashboardBtn;
    QPushButton *taskBtn;
    QPushButton *achievementBtn;
    QPushButton *growthBtn;
    QPushButton *shopBtn;
    QPushButton *logBtn;
    QPushButton *customBtn;
    QSpacerItem *verticalSpacer;
    QPushButton *skipTutorialBtn;
    QStackedWidget *stackedWidget;
    QWidget *pageDashboard;
    QWidget *pageTask;
    QWidget *pageAchievement;
    QWidget *pageGrowth;
    QWidget *pageShop;
    QWidget *pageLog;
    QWidget *pageCustom;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->resize(1200, 800);
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName("centralwidget");
        horizontalLayout = new QHBoxLayout(centralwidget);
        horizontalLayout->setObjectName("horizontalLayout");
        navPanel = new QWidget(centralwidget);
        navPanel->setObjectName("navPanel");
        verticalLayout = new QVBoxLayout(navPanel);
        verticalLayout->setObjectName("verticalLayout");
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        profileLabel = new QLabel(navPanel);
        profileLabel->setObjectName("profileLabel");

        verticalLayout->addWidget(profileLabel);

        notificationLabel = new QLabel(navPanel);
        notificationLabel->setObjectName("notificationLabel");
        notificationLabel->setWordWrap(true);

        verticalLayout->addWidget(notificationLabel);

        dashboardBtn = new QPushButton(navPanel);
        dashboardBtn->setObjectName("dashboardBtn");

        verticalLayout->addWidget(dashboardBtn);

        taskBtn = new QPushButton(navPanel);
        taskBtn->setObjectName("taskBtn");

        verticalLayout->addWidget(taskBtn);

        achievementBtn = new QPushButton(navPanel);
        achievementBtn->setObjectName("achievementBtn");

        verticalLayout->addWidget(achievementBtn);

        growthBtn = new QPushButton(navPanel);
        growthBtn->setObjectName("growthBtn");

        verticalLayout->addWidget(growthBtn);

        shopBtn = new QPushButton(navPanel);
        shopBtn->setObjectName("shopBtn");

        verticalLayout->addWidget(shopBtn);

        logBtn = new QPushButton(navPanel);
        logBtn->setObjectName("logBtn");

        verticalLayout->addWidget(logBtn);

        customBtn = new QPushButton(navPanel);
        customBtn->setObjectName("customBtn");

        verticalLayout->addWidget(customBtn);

        verticalSpacer = new QSpacerItem(0, 0, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        verticalLayout->addItem(verticalSpacer);

        skipTutorialBtn = new QPushButton(navPanel);
        skipTutorialBtn->setObjectName("skipTutorialBtn");

        verticalLayout->addWidget(skipTutorialBtn);


        horizontalLayout->addWidget(navPanel);

        stackedWidget = new QStackedWidget(centralwidget);
        stackedWidget->setObjectName("stackedWidget");
        stackedWidget->setMinimumSize(QSize(900, 0));
        pageDashboard = new QWidget();
        pageDashboard->setObjectName("pageDashboard");
        stackedWidget->addWidget(pageDashboard);
        pageTask = new QWidget();
        pageTask->setObjectName("pageTask");
        stackedWidget->addWidget(pageTask);
        pageAchievement = new QWidget();
        pageAchievement->setObjectName("pageAchievement");
        stackedWidget->addWidget(pageAchievement);
        pageGrowth = new QWidget();
        pageGrowth->setObjectName("pageGrowth");
        stackedWidget->addWidget(pageGrowth);
        pageShop = new QWidget();
        pageShop->setObjectName("pageShop");
        stackedWidget->addWidget(pageShop);
        pageLog = new QWidget();
        pageLog->setObjectName("pageLog");
        stackedWidget->addWidget(pageLog);
        pageCustom = new QWidget();
        pageCustom->setObjectName("pageCustom");
        stackedWidget->addWidget(pageCustom);

        horizontalLayout->addWidget(stackedWidget);

        MainWindow->setCentralWidget(centralwidget);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "\345\205\260\345\244\247\346\210\220\351\225\277\346\250\241\346\213\237 - \344\270\273\347\225\214\351\235\242", nullptr));
        profileLabel->setText(QCoreApplication::translate("MainWindow", "\345\255\246\347\224\237\346\241\243\346\241\210", nullptr));
        notificationLabel->setText(QCoreApplication::translate("MainWindow", "\345\256\236\346\227\266\351\200\232\347\237\245\345\214\272\345\237\237", nullptr));
        dashboardBtn->setText(QCoreApplication::translate("MainWindow", "\344\273\252\350\241\250\347\233\230", nullptr));
        taskBtn->setText(QCoreApplication::translate("MainWindow", "\344\273\273\345\212\241", nullptr));
        achievementBtn->setText(QCoreApplication::translate("MainWindow", "\346\210\220\345\260\261", nullptr));
        growthBtn->setText(QCoreApplication::translate("MainWindow", "\346\210\220\351\225\277", nullptr));
        shopBtn->setText(QCoreApplication::translate("MainWindow", "\345\225\206\345\272\227", nullptr));
        logBtn->setText(QCoreApplication::translate("MainWindow", "\346\227\245\345\277\227", nullptr));
        customBtn->setText(QCoreApplication::translate("MainWindow", "\350\207\252\345\256\232\344\271\211", nullptr));
        skipTutorialBtn->setText(QCoreApplication::translate("MainWindow", "\350\267\263\350\277\207\346\225\231\347\250\213", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
