/********************************************************************************
** Form generated from reading UI file 'LogBrowser.ui'
**
** Created by: Qt User Interface Compiler version 6.9.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_LOGBROWSER_H
#define UI_LOGBROWSER_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_LogBrowser
{
public:
    QVBoxLayout *verticalLayout;
    QComboBox *filterCombo;
    QTableWidget *logTable;

    void setupUi(QWidget *LogBrowser)
    {
        if (LogBrowser->objectName().isEmpty())
            LogBrowser->setObjectName("LogBrowser");
        verticalLayout = new QVBoxLayout(LogBrowser);
        verticalLayout->setObjectName("verticalLayout");
        filterCombo = new QComboBox(LogBrowser);
        filterCombo->setObjectName("filterCombo");

        verticalLayout->addWidget(filterCombo);

        logTable = new QTableWidget(LogBrowser);
        if (logTable->columnCount() < 3)
            logTable->setColumnCount(3);
        QTableWidgetItem *__qtablewidgetitem = new QTableWidgetItem();
        logTable->setHorizontalHeaderItem(0, __qtablewidgetitem);
        QTableWidgetItem *__qtablewidgetitem1 = new QTableWidgetItem();
        logTable->setHorizontalHeaderItem(1, __qtablewidgetitem1);
        QTableWidgetItem *__qtablewidgetitem2 = new QTableWidgetItem();
        logTable->setHorizontalHeaderItem(2, __qtablewidgetitem2);
        logTable->setObjectName("logTable");

        verticalLayout->addWidget(logTable);


        retranslateUi(LogBrowser);

        QMetaObject::connectSlotsByName(LogBrowser);
    } // setupUi

    void retranslateUi(QWidget *LogBrowser)
    {
        QTableWidgetItem *___qtablewidgetitem = logTable->horizontalHeaderItem(0);
        ___qtablewidgetitem->setText(QCoreApplication::translate("LogBrowser", "\346\227\266\351\227\264", nullptr));
        QTableWidgetItem *___qtablewidgetitem1 = logTable->horizontalHeaderItem(1);
        ___qtablewidgetitem1->setText(QCoreApplication::translate("LogBrowser", "\347\261\273\345\236\213", nullptr));
        QTableWidgetItem *___qtablewidgetitem2 = logTable->horizontalHeaderItem(2);
        ___qtablewidgetitem2->setText(QCoreApplication::translate("LogBrowser", "\350\257\246\346\203\205", nullptr));
        (void)LogBrowser;
    } // retranslateUi

};

namespace Ui {
    class LogBrowser: public Ui_LogBrowser {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_LOGBROWSER_H
