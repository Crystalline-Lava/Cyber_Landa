/********************************************************************************
** Form generated from reading UI file 'CustomizationPanel.ui'
**
** Created by: Qt User Interface Compiler version 6.9.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_CUSTOMIZATIONPANEL_H
#define UI_CUSTOMIZATIONPANEL_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_CustomizationPanel
{
public:
    QVBoxLayout *verticalLayout;
    QTabWidget *tabWidget;
    QWidget *tabTask;
    QFormLayout *formLayoutTask;
    QLabel *labelTaskTitle;
    QLineEdit *taskTitleEdit;
    QLabel *labelTaskDesc;
    QTextEdit *taskDescEdit;
    QLabel *labelTaskReward;
    QLineEdit *taskRewardEdit;
    QPushButton *createTaskBtn;
    QWidget *tabAchievement;
    QFormLayout *formLayoutAchievement;
    QLabel *labelAchName;
    QLineEdit *achNameEdit;
    QLabel *labelAchCond;
    QTextEdit *achCondEdit;
    QPushButton *createAchievementBtn;
    QWidget *tabSerendipity;
    QFormLayout *formLayoutSerendipity;
    QLabel *labelSerName;
    QLineEdit *serNameEdit;
    QLabel *labelSerWeight;
    QLineEdit *serWeightEdit;
    QPushButton *createSerendipityBtn;

    void setupUi(QWidget *CustomizationPanel)
    {
        if (CustomizationPanel->objectName().isEmpty())
            CustomizationPanel->setObjectName("CustomizationPanel");
        verticalLayout = new QVBoxLayout(CustomizationPanel);
        verticalLayout->setObjectName("verticalLayout");
        tabWidget = new QTabWidget(CustomizationPanel);
        tabWidget->setObjectName("tabWidget");
        tabTask = new QWidget();
        tabTask->setObjectName("tabTask");
        formLayoutTask = new QFormLayout(tabTask);
        formLayoutTask->setObjectName("formLayoutTask");
        labelTaskTitle = new QLabel(tabTask);
        labelTaskTitle->setObjectName("labelTaskTitle");

        formLayoutTask->setWidget(0, QFormLayout::ItemRole::LabelRole, labelTaskTitle);

        taskTitleEdit = new QLineEdit(tabTask);
        taskTitleEdit->setObjectName("taskTitleEdit");

        formLayoutTask->setWidget(0, QFormLayout::ItemRole::FieldRole, taskTitleEdit);

        labelTaskDesc = new QLabel(tabTask);
        labelTaskDesc->setObjectName("labelTaskDesc");

        formLayoutTask->setWidget(1, QFormLayout::ItemRole::LabelRole, labelTaskDesc);

        taskDescEdit = new QTextEdit(tabTask);
        taskDescEdit->setObjectName("taskDescEdit");

        formLayoutTask->setWidget(1, QFormLayout::ItemRole::FieldRole, taskDescEdit);

        labelTaskReward = new QLabel(tabTask);
        labelTaskReward->setObjectName("labelTaskReward");

        formLayoutTask->setWidget(2, QFormLayout::ItemRole::LabelRole, labelTaskReward);

        taskRewardEdit = new QLineEdit(tabTask);
        taskRewardEdit->setObjectName("taskRewardEdit");

        formLayoutTask->setWidget(2, QFormLayout::ItemRole::FieldRole, taskRewardEdit);

        createTaskBtn = new QPushButton(tabTask);
        createTaskBtn->setObjectName("createTaskBtn");

        formLayoutTask->setWidget(3, QFormLayout::ItemRole::FieldRole, createTaskBtn);

        tabWidget->addTab(tabTask, QString());
        tabAchievement = new QWidget();
        tabAchievement->setObjectName("tabAchievement");
        formLayoutAchievement = new QFormLayout(tabAchievement);
        formLayoutAchievement->setObjectName("formLayoutAchievement");
        labelAchName = new QLabel(tabAchievement);
        labelAchName->setObjectName("labelAchName");

        formLayoutAchievement->setWidget(0, QFormLayout::ItemRole::LabelRole, labelAchName);

        achNameEdit = new QLineEdit(tabAchievement);
        achNameEdit->setObjectName("achNameEdit");

        formLayoutAchievement->setWidget(0, QFormLayout::ItemRole::FieldRole, achNameEdit);

        labelAchCond = new QLabel(tabAchievement);
        labelAchCond->setObjectName("labelAchCond");

        formLayoutAchievement->setWidget(1, QFormLayout::ItemRole::LabelRole, labelAchCond);

        achCondEdit = new QTextEdit(tabAchievement);
        achCondEdit->setObjectName("achCondEdit");

        formLayoutAchievement->setWidget(1, QFormLayout::ItemRole::FieldRole, achCondEdit);

        createAchievementBtn = new QPushButton(tabAchievement);
        createAchievementBtn->setObjectName("createAchievementBtn");

        formLayoutAchievement->setWidget(2, QFormLayout::ItemRole::FieldRole, createAchievementBtn);

        tabWidget->addTab(tabAchievement, QString());
        tabSerendipity = new QWidget();
        tabSerendipity->setObjectName("tabSerendipity");
        formLayoutSerendipity = new QFormLayout(tabSerendipity);
        formLayoutSerendipity->setObjectName("formLayoutSerendipity");
        labelSerName = new QLabel(tabSerendipity);
        labelSerName->setObjectName("labelSerName");

        formLayoutSerendipity->setWidget(0, QFormLayout::ItemRole::LabelRole, labelSerName);

        serNameEdit = new QLineEdit(tabSerendipity);
        serNameEdit->setObjectName("serNameEdit");

        formLayoutSerendipity->setWidget(0, QFormLayout::ItemRole::FieldRole, serNameEdit);

        labelSerWeight = new QLabel(tabSerendipity);
        labelSerWeight->setObjectName("labelSerWeight");

        formLayoutSerendipity->setWidget(1, QFormLayout::ItemRole::LabelRole, labelSerWeight);

        serWeightEdit = new QLineEdit(tabSerendipity);
        serWeightEdit->setObjectName("serWeightEdit");

        formLayoutSerendipity->setWidget(1, QFormLayout::ItemRole::FieldRole, serWeightEdit);

        createSerendipityBtn = new QPushButton(tabSerendipity);
        createSerendipityBtn->setObjectName("createSerendipityBtn");

        formLayoutSerendipity->setWidget(2, QFormLayout::ItemRole::FieldRole, createSerendipityBtn);

        tabWidget->addTab(tabSerendipity, QString());

        verticalLayout->addWidget(tabWidget);


        retranslateUi(CustomizationPanel);

        QMetaObject::connectSlotsByName(CustomizationPanel);
    } // setupUi

    void retranslateUi(QWidget *CustomizationPanel)
    {
        labelTaskTitle->setText(QCoreApplication::translate("CustomizationPanel", "\346\240\207\351\242\230", nullptr));
        labelTaskDesc->setText(QCoreApplication::translate("CustomizationPanel", "\346\217\217\350\277\260", nullptr));
        labelTaskReward->setText(QCoreApplication::translate("CustomizationPanel", "\345\245\226\345\212\261", nullptr));
        createTaskBtn->setText(QCoreApplication::translate("CustomizationPanel", "\345\210\233\345\273\272\344\273\273\345\212\241", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(tabTask), QCoreApplication::translate("CustomizationPanel", "\350\207\252\345\256\232\344\271\211\344\273\273\345\212\241", nullptr));
        labelAchName->setText(QCoreApplication::translate("CustomizationPanel", "\345\220\215\347\247\260", nullptr));
        labelAchCond->setText(QCoreApplication::translate("CustomizationPanel", "\346\235\241\344\273\266", nullptr));
        createAchievementBtn->setText(QCoreApplication::translate("CustomizationPanel", "\345\210\233\345\273\272\346\210\220\345\260\261", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(tabAchievement), QCoreApplication::translate("CustomizationPanel", "\350\207\252\345\256\232\344\271\211\346\210\220\345\260\261", nullptr));
        labelSerName->setText(QCoreApplication::translate("CustomizationPanel", "\345\220\215\347\247\260", nullptr));
        labelSerWeight->setText(QCoreApplication::translate("CustomizationPanel", "\346\235\203\351\207\215", nullptr));
        createSerendipityBtn->setText(QCoreApplication::translate("CustomizationPanel", "\345\210\233\345\273\272\345\245\207\351\201\207", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(tabSerendipity), QCoreApplication::translate("CustomizationPanel", "\350\207\252\345\256\232\344\271\211\345\245\207\351\201\207", nullptr));
        (void)CustomizationPanel;
    } // retranslateUi

};

namespace Ui {
    class CustomizationPanel: public Ui_CustomizationPanel {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_CUSTOMIZATIONPANEL_H
