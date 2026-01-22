#pragma once

#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui {
class SecretMenu;
}
QT_END_NAMESPACE

class SecretMenu : public QWidget
{
    Q_OBJECT

public:
    explicit SecretMenu(QWidget* parent = nullptr);
    ~SecretMenu();

signals:
    void BackClicked();

private:
    std::unique_ptr<Ui::SecretMenu> ui_;
};

