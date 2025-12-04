/********************************************************************************
** Form generated from reading UI file 'AchievementGallery.ui'
**
** Created by: Qt User Interface Compiler version 6.9.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_ACHIEVEMENTGALLERY_H
#define UI_ACHIEVEMENTGALLERY_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_AchievementGallery
{
public:
    QVBoxLayout *verticalLayout;
    QScrollArea *scrollArea;
    QWidget *scrollAreaWidgetContents;
    QGridLayout *gridLayout;

    void setupUi(QWidget *AchievementGallery)
    {
        if (AchievementGallery->objectName().isEmpty())
            AchievementGallery->setObjectName("AchievementGallery");
        verticalLayout = new QVBoxLayout(AchievementGallery);
        verticalLayout->setObjectName("verticalLayout");
        scrollArea = new QScrollArea(AchievementGallery);
        scrollArea->setObjectName("scrollArea");
        scrollArea->setWidgetResizable(true);
        scrollAreaWidgetContents = new QWidget();
        scrollAreaWidgetContents->setObjectName("scrollAreaWidgetContents");
        gridLayout = new QGridLayout(scrollAreaWidgetContents);
        gridLayout->setObjectName("gridLayout");
        scrollArea->setWidget(scrollAreaWidgetContents);

        verticalLayout->addWidget(scrollArea);


        retranslateUi(AchievementGallery);

        QMetaObject::connectSlotsByName(AchievementGallery);
    } // setupUi

    void retranslateUi(QWidget *AchievementGallery)
    {
        (void)AchievementGallery;
    } // retranslateUi

};

namespace Ui {
    class AchievementGallery: public Ui_AchievementGallery {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_ACHIEVEMENTGALLERY_H
