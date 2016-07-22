#ifndef QUICKMOSAIC_H
#define QUICKMOSAIC_H

#include <QtWidgets/QDialog>
#include "ui_quickmosaic.h"

class QuickMosaic : public QDialog
{
	Q_OBJECT

public:
	QuickMosaic(QWidget *parent = 0);
	~QuickMosaic();

private:
	Ui::QuickMosaicClass ui;
	private	slots :
	void onScanButtonClicked();
	void onStartButtonClicked();
};

#endif // QUICKMOSAIC_H
