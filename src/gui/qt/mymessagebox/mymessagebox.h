#ifndef MYMESSAGEBOX_H
#define MYMESSAGEBOX_H

#include <QMessageBox>

class MyMessageBox : public QMessageBox
{
	Q_OBJECT
public:
	MyMessageBox(QWidget *parent = 0);

	MyMessageBox(Icon icon, const QString &title, const QString &text,
				 QFlags<QMessageBox::StandardButton> buttons = QMessageBox::NoButton, QWidget *parent = 0,
				 Qt::WindowFlags flags = Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);

	~MyMessageBox();

	static QMessageBox::StandardButton information(QWidget *parent, const QString &title,
			const QString &text, QFlags<QMessageBox::StandardButton> buttons = QMessageBox::Ok, QMessageBox::StandardButton defaultButton = QMessageBox::NoButton);

	static QMessageBox::StandardButton question(QWidget *parent, const QString &title,
			const QString &text, QFlags<QMessageBox::StandardButton> buttons = QMessageBox::Ok, QMessageBox::StandardButton defaultButton = QMessageBox::NoButton);

	static QMessageBox::StandardButton warning(QWidget *parent, const QString &title,
			const QString &text, QFlags<QMessageBox::StandardButton> buttons = QMessageBox::Ok, QMessageBox::StandardButton defaultButton = QMessageBox::NoButton);

	static QMessageBox::StandardButton critical(QWidget *parent, const QString &title,
			const QString &text, QFlags<QMessageBox::StandardButton> buttons = QMessageBox::Ok, QMessageBox::StandardButton defaultButton = QMessageBox::NoButton);

signals:

public slots:

};

#endif // MYMESSAGEBOX_H
