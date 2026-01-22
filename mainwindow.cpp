#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include "calculatormodel.h"
#include "secretmenu.h"

#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QStackedWidget>
#include <QVBoxLayout>

MainWindow::~MainWindow() = default;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui_(std::make_unique<Ui::MainWindow>())
    , model_(std::make_unique<CalculatorModel>())
    , equal_long_press_timer_(std::make_unique<QTimer>())
    , secret_code_timer_(std::make_unique<QTimer>())
    , secret_menu_(std::make_unique<SecretMenu>())
{
    ui_->setupUi(this);

    QWidget* calculator_page = new QWidget(this);
    QVBoxLayout* main_layout = new QVBoxLayout(calculator_page);

    QWidget* calculator_ui = ui_->centralwidget;
    calculator_ui->setParent(calculator_page);
    main_layout->addWidget(calculator_ui);
    main_layout->setContentsMargins(0, 0, 0, 0);

    stacked_widget_ = new QStackedWidget(this);
    stacked_widget_->addWidget(calculator_page);

    secret_menu_->setParent(this);

    connect(secret_menu_.get(), &SecretMenu::BackClicked,
            this, &MainWindow::CloseSecretMenu);

    stacked_widget_->addWidget(secret_menu_.get());

    setCentralWidget(stacked_widget_);
    stacked_widget_->setCurrentIndex(0);

    model_->setParent(this);

    connect(model_.get(), &CalculatorModel::DisplayChanged, this,
            [this](const QString& text) {
                ui_->lbl_display->setText(FormatWithSpaces(text, 15));
            });

    connect(model_.get(), &CalculatorModel::ExpressionChanged, this,
            [this](const QString& text) {
                ui_->lbl_expression->setText(FormatWithSpaces(text, 37));
            });

    equal_long_press_timer_->setSingleShot(true);
    equal_long_press_timer_->setInterval(4000);

    secret_code_timer_->setSingleShot(true);
    secret_code_timer_->setInterval(5000);

    connect(equal_long_press_timer_.get(), &QTimer::timeout, this, [this] {
        secret_armed_ = true;
        secret_code_buffer_.clear();
        secret_code_timer_->start();
    });

    connect(secret_code_timer_.get(), &QTimer::timeout, this, [this] {
        secret_armed_ = false;
        secret_code_buffer_.clear();
    });

    QPushButton* digit_buttons[] = {
        ui_->btn_digit_0, ui_->btn_digit_1, ui_->btn_digit_2,
        ui_->btn_digit_3, ui_->btn_digit_4, ui_->btn_digit_5,
        ui_->btn_digit_6, ui_->btn_digit_7, ui_->btn_digit_8,
        ui_->btn_digit_9
    };

    for (int i = 0; i < 10; ++i) {
        connect(digit_buttons[i], &QPushButton::clicked,
                this, [this, i] { HandleDigit(i); });
    }

    connect(ui_->btn_decimal, &QPushButton::clicked,
            model_.get(), &CalculatorModel::InputDecimalPoint);
    connect(ui_->btn_paren, &QPushButton::clicked,
            model_.get(), &CalculatorModel::InputParen);
    connect(ui_->btn_sign, &QPushButton::clicked,
            model_.get(), &CalculatorModel::ToggleSign);
    connect(ui_->btn_percent, &QPushButton::clicked,
            model_.get(), &CalculatorModel::InputPercent);

    struct OperatorButton {
        QPushButton* button;
        char operation;
    };

    OperatorButton operators[] = {
        {ui_->btn_op_plus, '+'},
        {ui_->btn_op_minus, '-'},
        {ui_->btn_op_mult, '*'},
        {ui_->btn_op_division, '/'}
    };

    for (const auto& op : operators) {
        connect(op.button, &QPushButton::clicked,
                this, [this, op] { model_->InputOperator(op.operation); });
    }

    connect(ui_->btn_clear, &QPushButton::clicked,
            model_.get(), &CalculatorModel::ClearAll);

    connect(ui_->btn_equals, &QPushButton::pressed, this, [this] {
        equal_long_press_timer_->start();
    });

    connect(ui_->btn_equals, &QPushButton::released, this, [this] {
        if (equal_long_press_timer_->isActive()) {
            equal_long_press_timer_->stop();
            model_->Equals();
        }
    });
}

QString MainWindow::FormatWithSpaces(const QString& text, int group_size) {
    if (group_size <= 0 || text.isEmpty()) return text;

    QString result;
    const int len = text.length();
    int count = 0;

    for (int i = 0; i < len; ++i) {
        result.append(text[i]);
        ++count;
        if (count % group_size == 0 && i != len - 1) {
            result.append(' ');
        }
    }

    return result;
}

void MainWindow::HandleDigit(int digit) {
    if (secret_armed_) {
        const QString pattern = QStringLiteral("123");
        secret_code_buffer_.append(QChar('0' + digit));

        if (!pattern.startsWith(secret_code_buffer_)) {
            secret_code_buffer_.clear();
        }

        if (secret_code_buffer_ == pattern) {
            secret_code_timer_->stop();
            secret_armed_ = false;
            secret_code_buffer_.clear();
            OpenSecretMenu();
        }
    }

    model_->InputDigit(digit);
}

void MainWindow::OpenSecretMenu() {
    if (stacked_widget_) {
        stacked_widget_->setCurrentIndex(1);
    }
}

void MainWindow::CloseSecretMenu() {
    if (stacked_widget_) {
        stacked_widget_->setCurrentIndex(0);
    }
}
