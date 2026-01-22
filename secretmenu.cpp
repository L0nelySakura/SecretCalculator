#include "secretmenu.h"
#include "ui_secretmenu.h"

SecretMenu::SecretMenu(QWidget* parent)
    : QWidget(parent)
    , ui_(std::make_unique<Ui::SecretMenu>())
{
    ui_->setupUi(this);
    connect(ui_->btn_back, &QPushButton::clicked, this, [this]() { emit BackClicked(); });

}
SecretMenu::~SecretMenu() = default;
