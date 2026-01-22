#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class QTimer;
class QStackedWidget;

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class CalculatorModel;
class SecretMenu;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    CalculatorModel* model_ = nullptr;

    QStackedWidget* stackedWidget_ = nullptr;
    SecretMenu* secretMenu_ = nullptr;

    QTimer* equalLongPressTimer_ = nullptr;
    QTimer* secretCodeTimer_ = nullptr;
    bool secretArmed_ = false;
    QString secretCodeBuffer_;

    QString formatWithSpaces(const QString& text, int groupSize);

    void handleDigit(int digit);
    void openSecretMenu();
    void closeSecretMenu();
};
#endif // MAINWINDOW_H
