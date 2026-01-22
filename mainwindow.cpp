#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include "calculatormodel.h"
#include "secretmenu.h"

#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QStackedWidget>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    stackedWidget_ = new QStackedWidget(this);

    QWidget* calculatorPage = ui->centralwidget;
    stackedWidget_->addWidget(calculatorPage);

    secretMenu_ = new SecretMenu(this);
    connect(secretMenu_, &SecretMenu::backClicked, this, &MainWindow::closeSecretMenu);
    stackedWidget_->addWidget(secretMenu_);

    setCentralWidget(stackedWidget_);

    stackedWidget_->setCurrentIndex(0);

    model_ = new CalculatorModel(this);


    connect(model_, &CalculatorModel::displayChanged, this, [this](const QString& text) {
        ui->lbl_display->setText(formatWithSpaces(text, 15));
    });

    connect(model_, &CalculatorModel::expressionChanged, this, [this](const QString& text) {
        ui->lbl_expression->setText(formatWithSpaces(text, 37));
    });

    equalLongPressTimer_ = new QTimer(this);
    equalLongPressTimer_->setSingleShot(true);
    equalLongPressTimer_->setInterval(4000);

    secretCodeTimer_ = new QTimer(this);
    secretCodeTimer_->setSingleShot(true);
    secretCodeTimer_->setInterval(5000);

    connect(equalLongPressTimer_, &QTimer::timeout, this, [this] {
        secretArmed_ = true;
        secretCodeBuffer_.clear();
        secretCodeTimer_->start();
    });

    connect(secretCodeTimer_, &QTimer::timeout, this, [this] {
        secretArmed_ = false;
        secretCodeBuffer_.clear();
    });

    // Цифры
    connect(ui->btn_digit_0, &QPushButton::clicked, this, [this]{ handleDigit(0); });
    connect(ui->btn_digit_1, &QPushButton::clicked, this, [this]{ handleDigit(1); });
    connect(ui->btn_digit_2, &QPushButton::clicked, this, [this]{ handleDigit(2); });
    connect(ui->btn_digit_3, &QPushButton::clicked, this, [this]{ handleDigit(3); });
    connect(ui->btn_digit_4, &QPushButton::clicked, this, [this]{ handleDigit(4); });
    connect(ui->btn_digit_5, &QPushButton::clicked, this, [this]{ handleDigit(5); });
    connect(ui->btn_digit_6, &QPushButton::clicked, this, [this]{ handleDigit(6); });
    connect(ui->btn_digit_7, &QPushButton::clicked, this, [this]{ handleDigit(7); });
    connect(ui->btn_digit_8, &QPushButton::clicked, this, [this]{ handleDigit(8); });
    connect(ui->btn_digit_9, &QPushButton::clicked, this, [this]{ handleDigit(9); });

    connect(ui->btn_decimal, &QPushButton::clicked, model_, &CalculatorModel::inputDecimalPoint);
    connect(ui->btn_paren, &QPushButton::clicked, model_, &CalculatorModel::inputParen);
    connect(ui->btn_sign, &QPushButton::clicked, model_, &CalculatorModel::toggleSign);
    connect(ui->btn_percent, &QPushButton::clicked, model_, &CalculatorModel::inputPercent);

    connect(ui->btn_op_plus, &QPushButton::clicked, this, [this]{ model_->inputOperator('+'); });
    connect(ui->btn_op_minus, &QPushButton::clicked, this, [this]{ model_->inputOperator('-'); });
    connect(ui->btn_op_mult, &QPushButton::clicked, this, [this]{ model_->inputOperator('*'); });
    connect(ui->btn_op_division, &QPushButton::clicked, this, [this]{ model_->inputOperator('/'); });

    connect(ui->btn_clear, &QPushButton::clicked, model_, &CalculatorModel::clearAll);

    connect(ui->btn_equals, &QPushButton::pressed, this, [this] {
        equalLongPressTimer_->start();
    });
    connect(ui->btn_equals, &QPushButton::released, this, [this] {
        if (equalLongPressTimer_->isActive()) {
            equalLongPressTimer_->stop();
            model_->equals();
        }
    });
}

MainWindow::~MainWindow()
{
    delete ui;
}
QString MainWindow::formatWithSpaces(const QString& text, int groupSize) {
    if (groupSize <= 0 || text.isEmpty()) return text;

    QString result;
    int len = text.length();
    int count = 0;
    for (int i = 0; i < len; i++) {
        result.append(text[i]);
        count++;
        if (count % groupSize == 0 && i != len - 1) {
            result.append(' ');
        }
    }

    return result;
}

void MainWindow::handleDigit(int digit)
{
    if (secretArmed_) {
        const QString pattern = QStringLiteral("123");
        secretCodeBuffer_.append(QChar('0' + digit));

        if (!pattern.startsWith(secretCodeBuffer_)) {
            secretCodeBuffer_.clear();
        }

        if (secretCodeBuffer_ == pattern) {
            secretCodeTimer_->stop();
            secretArmed_ = false;
            secretCodeBuffer_.clear();
            openSecretMenu();
        }
    }

    model_->inputDigit(digit);
}

void MainWindow::openSecretMenu()
{
    if (stackedWidget_) {
        stackedWidget_->setCurrentIndex(1);
    }
}

void MainWindow::closeSecretMenu()
{
    if (stackedWidget_) {
        stackedWidget_->setCurrentIndex(0);
    }
}

