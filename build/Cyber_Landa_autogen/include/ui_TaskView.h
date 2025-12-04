/********************************************************************************
** Form generated from reading UI file 'TaskView.ui'
**
** Created by: Qt User Interface Compiler version 6.9.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_TASKVIEW_H
#define UI_TASKVIEW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_TaskView
{
public:
    QVBoxLayout *verticalLayout;
    QComboBox *periodCombo;
    QTreeWidget *taskTree;
    QPushButton *completeBtn;

    void setupUi(QWidget *TaskView)
    {
        if (TaskView->objectName().isEmpty())
            TaskView->setObjectName("TaskView");
        verticalLayout = new QVBoxLayout(TaskView);
        verticalLayout->setObjectName("verticalLayout");
        periodCombo = new QComboBox(TaskView);
        periodCombo->setObjectName("periodCombo");

        verticalLayout->addWidget(periodCombo);

        taskTree = new QTreeWidget(TaskView);
        taskTree->setObjectName("taskTree");

        verticalLayout->addWidget(taskTree);

        completeBtn = new QPushButton(TaskView);
        completeBtn->setObjectName("completeBtn");

        verticalLayout->addWidget(completeBtn);


        retranslateUi(TaskView);

        QMetaObject::connectSlotsByName(TaskView);
    } // setupUi

    void retranslateUi(QWidget *TaskView)
    {
        QTreeWidgetItem *___qtreewidgetitem = taskTree->headerItem();
        ___qtreewidgetitem->setText(2, QCoreApplication::translate("TaskView", "\347\212\266\346\200\201", nullptr));
        ___qtreewidgetitem->setText(1, QCoreApplication::translate("TaskView", "\345\245\226\345\212\261", nullptr));
        ___qtreewidgetitem->setText(0, QCoreApplication::translate("TaskView", "\344\273\273\345\212\241", nullptr));
        completeBtn->setText(QCoreApplication::translate("TaskView", "\346\240\207\350\256\260\345\256\214\346\210\220", nullptr));
        (void)TaskView;
    } // retranslateUi

};

namespace Ui {
    class TaskView: public Ui_TaskView {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_TASKVIEW_H
