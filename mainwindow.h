#pragma once

#include <QMainWindow>
#include <memory>

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
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private:
    std::unique_ptr<Ui::MainWindow> ui_;
    std::unique_ptr<CalculatorModel> model_;

    QStackedWidget* stacked_widget_ = nullptr;
    std::unique_ptr<SecretMenu> secret_menu_;

    std::unique_ptr<QTimer> equal_long_press_timer_;
    std::unique_ptr<QTimer> secret_code_timer_;
    bool secret_armed_ = false;
    QString secret_code_buffer_;

    QString FormatWithSpaces(const QString& text, int group_size);

    void HandleDigit(int digit);
    void OpenSecretMenu();
    void CloseSecretMenu();
};
