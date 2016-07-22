#include "quickmosaic.h"

#include <QMessageBox>
#include <QFileDialog>
#include <QDir>

QuickMosaic::QuickMosaic(QWidget *parent)
	: QDialog(parent)
{
	ui.setupUi(this);
	connect(ui.scanButton, SIGNAL(clicked()), this, SLOT(onScanButtonClicked()));
	connect(ui.startButton, SIGNAL(clicked()), this, SLOT(onStartButtonClicked()));
}

QuickMosaic::~QuickMosaic()
{

}

void QuickMosaic::onScanButtonClicked()
{
	QString dir = QFileDialog::getExistingDirectory(this, "打开影像所在目录...");
	ui.image_path->setText(dir);
}

void QuickMosaic::onStartButtonClicked()
{
	if (ui.image_path->text().isEmpty())
	{
		QMessageBox::information(this, "错误", "未指定输入目录！");
		return;
	}
	QDir dir(ui.image_path->text(),ui.imageFormat->currentText());
	dir.setFilter(QDir::Files);
	if (dir.exists())
	{
		QFileInfoList file_info_list = dir.entryInfoList();
		
	}
	else
	{
		QMessageBox::information(this, "错误", "目录不存在！");
	}
}