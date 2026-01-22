#include "secretmenu.h"
#include "ui_secretmenu.h"

SecretMenu::SecretMenu(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::SecretMenu)
{
    ui->setupUi(this);
    connect(ui->btn_back, &QPushButton::clicked, this, &SecretMenu::backClicked);
}

SecretMenu::~SecretMenu()
{
    delete ui;
}
