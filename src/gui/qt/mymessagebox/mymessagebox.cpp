#include "mymessagebox.h"
#include <QMessageBox>

MyMessageBox::MyMessageBox(QWidget *parent)
{
    QMessageBox box(parent);
    box.setWindowModality(Qt::WindowModal);
    box.setWindowFlags(Qt::ToolTip);
    box.setStyleSheet("QDialog{ border: 1px solid black; border-style: solid; border-radius: 4px; }");
    box.exec();
}

MyMessageBox::MyMessageBox(Icon icon, const QString &title, const QString &text, QFlags<QMessageBox::StandardButton> buttons, QWidget *parent, Qt::WindowFlags flags)
{
    QMessageBox box(icon, title, text, buttons, parent, flags);
    box.setWindowModality(Qt::WindowModal);
    box.setWindowFlags(Qt::ToolTip);
    box.setStyleSheet("QDialog{ border: 1px solid black; border-style: solid; border-radius: 4px; }");
    box.exec();
}

MyMessageBox::~MyMessageBox()
{

}

QMessageBox::StandardButton MyMessageBox::information(QWidget *parent, const QString &title, const QString &text, QFlags<QMessageBox::StandardButton> buttons, QMessageBox::StandardButton defaultButton)
{
    QMessageBox box(QMessageBox::Information, title, text, buttons, parent);
    box.setWindowModality(Qt::WindowModal);
    box.setWindowFlags(Qt::ToolTip);
    box.setStyleSheet("QDialog{ border: 1px solid black; border-style: solid; border-radius: 4px; }");
    box.setDefaultButton(defaultButton);
    return static_cast<QMessageBox::StandardButton>(box.exec());
}

QMessageBox::StandardButton MyMessageBox::question(QWidget *parent, const QString &title, const QString &text, QFlags<QMessageBox::StandardButton> buttons, QMessageBox::StandardButton defaultButton)
{
    QMessageBox box(QMessageBox::Question, title, text, buttons, parent);
    box.setWindowModality(Qt::WindowModal);
    box.setWindowFlags(Qt::ToolTip);
    box.setStyleSheet("QDialog{ border: 1px solid black; border-style: solid; border-radius: 4px; }");
    box.setDefaultButton(defaultButton);
    return static_cast<QMessageBox::StandardButton>(box.exec());
}

QMessageBox::StandardButton MyMessageBox::warning(QWidget *parent, const QString &title, const QString &text, QFlags<QMessageBox::StandardButton> buttons, QMessageBox::StandardButton defaultButton)
{
    QMessageBox box(QMessageBox::Warning, title, text, buttons, parent);
    box.setWindowModality(Qt::WindowModal);
    box.setWindowFlags(Qt::ToolTip);
    box.setStyleSheet("QDialog{ border: 1px solid black; border-style: solid; border-radius: 4px; }");
    box.setDefaultButton(defaultButton);
    return static_cast<QMessageBox::StandardButton>(box.exec());
}

QMessageBox::StandardButton MyMessageBox::critical(QWidget *parent, const QString &title, const QString &text, QFlags<QMessageBox::StandardButton> buttons, QMessageBox::StandardButton defaultButton)
{
    QMessageBox box(QMessageBox::Critical, title, text, buttons, parent);
    box.setWindowModality(Qt::WindowModal);
    box.setWindowFlags(Qt::ToolTip);
    box.setStyleSheet("QDialog{ border: 1px solid black; border-style: solid; border-radius: 4px; }");
    box.setDefaultButton(defaultButton);
    return static_cast<QMessageBox::StandardButton>(box.exec());
}
