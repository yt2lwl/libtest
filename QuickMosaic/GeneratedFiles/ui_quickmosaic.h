/********************************************************************************
** Form generated from reading UI file 'quickmosaic.ui'
**
** Created by: Qt User Interface Compiler version 5.5.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_QUICKMOSAIC_H
#define UI_QUICKMOSAIC_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>

QT_BEGIN_NAMESPACE

class Ui_QuickMosaicClass
{
public:
    QGroupBox *groupBox;
    QLabel *label;
    QLabel *label_2;
    QLineEdit *image_path;
    QComboBox *imageFormat;
    QPushButton *scanButton;
    QPushButton *startButton;

    void setupUi(QDialog *QuickMosaicClass)
    {
        if (QuickMosaicClass->objectName().isEmpty())
            QuickMosaicClass->setObjectName(QStringLiteral("QuickMosaicClass"));
        QuickMosaicClass->resize(692, 454);
        groupBox = new QGroupBox(QuickMosaicClass);
        groupBox->setObjectName(QStringLiteral("groupBox"));
        groupBox->setGeometry(QRect(10, 10, 631, 381));
        label = new QLabel(groupBox);
        label->setObjectName(QStringLiteral("label"));
        label->setGeometry(QRect(20, 40, 81, 18));
        label_2 = new QLabel(groupBox);
        label_2->setObjectName(QStringLiteral("label_2"));
        label_2->setGeometry(QRect(50, 80, 81, 18));
        image_path = new QLineEdit(groupBox);
        image_path->setObjectName(QStringLiteral("image_path"));
        image_path->setGeometry(QRect(110, 34, 401, 31));
        imageFormat = new QComboBox(groupBox);
        imageFormat->setObjectName(QStringLiteral("imageFormat"));
        imageFormat->setGeometry(QRect(110, 80, 99, 24));
        scanButton = new QPushButton(groupBox);
        scanButton->setObjectName(QStringLiteral("scanButton"));
        scanButton->setGeometry(QRect(530, 33, 61, 31));
        startButton = new QPushButton(groupBox);
        startButton->setObjectName(QStringLiteral("startButton"));
        startButton->setGeometry(QRect(480, 130, 112, 34));

        retranslateUi(QuickMosaicClass);

        QMetaObject::connectSlotsByName(QuickMosaicClass);
    } // setupUi

    void retranslateUi(QDialog *QuickMosaicClass)
    {
        QuickMosaicClass->setWindowTitle(QApplication::translate("QuickMosaicClass", "QuickMosaic", 0));
        groupBox->setTitle(QApplication::translate("QuickMosaicClass", "\345\237\272\346\234\254\350\256\276\347\275\256", 0));
        label->setText(QApplication::translate("QuickMosaicClass", "\345\275\261\345\203\217\350\267\257\345\276\204\357\274\232", 0));
        label_2->setText(QApplication::translate("QuickMosaicClass", "\346\240\274\345\274\217\357\274\232", 0));
        imageFormat->clear();
        imageFormat->insertItems(0, QStringList()
         << QApplication::translate("QuickMosaicClass", "*.jpg", 0)
         << QApplication::translate("QuickMosaicClass", "*.tif", 0)
        );
        scanButton->setText(QApplication::translate("QuickMosaicClass", "\346\265\217\350\247\210", 0));
        startButton->setText(QApplication::translate("QuickMosaicClass", "\345\220\257\345\212\250", 0));
    } // retranslateUi

};

namespace Ui {
    class QuickMosaicClass: public Ui_QuickMosaicClass {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_QUICKMOSAIC_H
