#ifndef INTERNETGAMELOGINDIALOGIMPL_H
#define INTERNETGAMELOGINDIALOGIMPL_H

#include "ui_internetgamelogindialog.h"

class ConfigFile;

class internetGameLoginDialogImpl : public QDialog, private Ui::internetGameLoginDialog {
    Q_OBJECT
	
public:
    internetGameLoginDialogImpl(QWidget *parent = 0, ConfigFile *c =0);
	
	void exec();
	void accept();
	
private:
	
	ConfigFile *myConfig;
};

#endif // INTERNETGAMELOGINDIALOGIMPL_H
