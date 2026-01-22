#ifndef SECRETMENU_H
#define SECRETMENU_H

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
    void backClicked();

private:
    Ui::SecretMenu *ui;
};

#endif // SECRETMENU_H
