#pragma once

#include <QObject>
#include <QString>

class CalculatorModel final : public QObject
{
    Q_OBJECT

public:
    explicit CalculatorModel(QObject* parent = nullptr);

    QString expression() const { return expression_; } // то, что показываем в lbl_expression
    QString display() const { return display_; }       // то, что показываем в lbl_display

public slots:
    void clearAll();               // C
    void inputDigit(int digit);    // 0..9
    void inputDecimalPoint();      // .
    void inputOperator(QChar op);  // + - * /
    void inputParen();             // кнопка "()"
    void toggleSign();             // +/-
    void inputPercent();           // %
    void equals();                 // =

signals:
    void expressionChanged(const QString& expr);
    void displayChanged(const QString& display);

private:
    enum class LastToken {
        Start,
        Number,
        Operator,
        OpenParen,
        CloseParen,
        Percent
    };

    QString expression_;
    QString display_ = "0";
    LastToken last_ = LastToken::Start;

    int openParens_ = 0;
    int closeParens_ = 0;

    // позиция начала текущего числа в expression_ (если last_==Number), иначе -1
    int currentNumberStart_ = -1;

    // helpers: текущий "набираемый" кусок числа
    QString currentNumber() const;
    int currentDigitsCount() const;        // цифры без '.' и без '-'
    bool currentHasDecimalPoint() const;

    void startNewNumberIfNeeded();
    void setDisplayFromCurrentOrZero();
    void emitAll();

    // модификации expression_
    void replaceCurrentNumber(const QString& newNumber);
    void appendToken(const QString& token, LastToken newLast);
    void appendChar(QChar c, LastToken newLast);
    void trimTrailingSpaces();

    // правила скобок
    bool canCloseParen() const;
    bool shouldOpenParen() const;

    // вычисление
    bool tryEvaluate(QString* outResult, QString* outError = nullptr);
};

