#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include "calculatormodel.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    model_ = new CalculatorModel(this);

    // Обновление UI от модели
    connect(model_, &CalculatorModel::displayChanged, ui->lbl_display, &QLabel::setText);
    connect(model_, &CalculatorModel::expressionChanged, ui->lbl_expression, &QLabel::setText);

    // Цифры
    connect(ui->btn_digit_0, &QPushButton::clicked, this, [this]{ model_->inputDigit(0); });
    connect(ui->btn_digit_1, &QPushButton::clicked, this, [this]{ model_->inputDigit(1); });
    connect(ui->btn_digit_2, &QPushButton::clicked, this, [this]{ model_->inputDigit(2); });
    connect(ui->btn_digit_3, &QPushButton::clicked, this, [this]{ model_->inputDigit(3); });
    connect(ui->btn_digit_4, &QPushButton::clicked, this, [this]{ model_->inputDigit(4); });
    connect(ui->btn_digit_5, &QPushButton::clicked, this, [this]{ model_->inputDigit(5); });
    connect(ui->btn_digit_6, &QPushButton::clicked, this, [this]{ model_->inputDigit(6); });
    connect(ui->btn_digit_7, &QPushButton::clicked, this, [this]{ model_->inputDigit(7); });
    connect(ui->btn_digit_8, &QPushButton::clicked, this, [this]{ model_->inputDigit(8); });
    connect(ui->btn_digit_9, &QPushButton::clicked, this, [this]{ model_->inputDigit(9); });

    // Точка / скобки / знак / процент
    connect(ui->btn_decimal, &QPushButton::clicked, model_, &CalculatorModel::inputDecimalPoint);
    connect(ui->btn_paren, &QPushButton::clicked, model_, &CalculatorModel::inputParen);
    connect(ui->btn_sign, &QPushButton::clicked, model_, &CalculatorModel::toggleSign);
    connect(ui->btn_percent, &QPushButton::clicked, model_, &CalculatorModel::inputPercent);

    // Операции
    connect(ui->btn_op_plus, &QPushButton::clicked, this, [this]{ model_->inputOperator('+'); });
    connect(ui->btn_op_minus, &QPushButton::clicked, this, [this]{ model_->inputOperator('-'); });
    connect(ui->btn_op_mult, &QPushButton::clicked, this, [this]{ model_->inputOperator('*'); });
    connect(ui->btn_op_division, &QPushButton::clicked, this, [this]{ model_->inputOperator('/'); });

    // Clear / Equals
    connect(ui->btn_clear, &QPushButton::clicked, model_, &CalculatorModel::clearAll);
    connect(ui->btn_equals, &QPushButton::clicked, model_, &CalculatorModel::equals);
}

MainWindow::~MainWindow()
{
    delete ui;
}
