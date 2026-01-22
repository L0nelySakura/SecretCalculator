#pragma once

#include <QObject>
#include <QString>

class CalculatorModel final : public QObject
{
    Q_OBJECT

public:
    explicit CalculatorModel(QObject* parent = nullptr);

    QString expression() const { return expression_; }
    QString display() const { return display_; }

public slots:
    void clearAll();
    void inputDigit(int digit);
    void inputDecimalPoint();
    void inputOperator(QChar op);
    void inputParen();
    void toggleSign();
    void inputPercent();
    void equals();

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

    int currentNumberStart_ = -1;

    QString currentNumber() const;
    int currentDigitsCount() const;
    bool currentHasDecimalPoint() const;

    void startNewNumberIfNeeded();
    void setDisplayFromCurrentOrZero();
    void emitAll();

    void replaceCurrentNumber(const QString& newNumber);
    void appendToken(const QString& token, LastToken newLast);
    void appendChar(QChar c, LastToken newLast);
    void trimTrailingSpaces();
    QString truncateNumber(const QString& number) const;

    bool canCloseParen() const;
    bool shouldOpenParen() const;
    bool tryEvaluate(QString* outResult, QString* outError = nullptr);
};

