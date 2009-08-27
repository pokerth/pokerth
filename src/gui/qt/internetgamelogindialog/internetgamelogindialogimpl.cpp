#include "internetgamelogindialogimpl.h"
#include "configfile.h"
#include <QtCore>

internetGameLoginDialogImpl::internetGameLoginDialogImpl(QWidget *parent, ConfigFile *c) :
    QDialog(parent), myConfig(c){
    setupUi(this);
}

void internetGameLoginDialogImpl::exec() {

	lineEdit_username->setText(QString::fromUtf8(myConfig->readConfigString("InternetLoginUserName").c_str()));
//	lineEdit_password->
	qDebug() << QString::fromUtf8(myConfig->readConfigString("InternetLoginPasswordMd5").c_str());
	
	QDialog::exec();
}

void internetGameLoginDialogImpl::accept() {

	myConfig->writeConfigString("InternetLoginUserName", lineEdit_username->text().toUtf8().constData());
	
	QByteArray myBytes = QCryptographicHash::hash(lineEdit_password->text().toUtf8(), QCryptographicHash::Md5).toHex();
	std::string myPwMd5String (myBytes.constData(), myBytes.size());
	myConfig->writeConfigString("InternetLoginPasswordMd5", myPwMd5String);
	
	myConfig->writeBuffer();
	
	QDialog::accept();
} 
