#ifndef INTERNETGAMELOGINDIALOGIMPL_H
#define INTERNETGAMELOGINDIALOGIMPL_H

#include "ui_internetgamelogindialog.h"

class internetGameLoginDialog : public QDialog, private Ui::internetGameLoginDialog {
    Q_OBJECT
public:
    internetGameLoginDialog(QWidget *parent = 0);
};

#endif // INTERNETGAMELOGINDIALOGIMPL_H
