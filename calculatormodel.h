#pragma once

#include <QObject>
#include <QString>
#include <memory>

class CalculatorModel final : public QObject
{
    Q_OBJECT

public:
    explicit CalculatorModel(QObject* parent = nullptr);
    ~CalculatorModel() override = default;

    QString Expression() const { return expression_; }
    QString Display() const { return display_; }

public slots:
    void ClearAll();
    void InputDigit(int digit);
    void InputDecimalPoint();
    void InputOperator(QChar op);
    void InputParen();
    void ToggleSign();
    void InputPercent();
    void Equals();

signals:
    void DisplayChanged(const QString& display);
    void ExpressionChanged(const QString& expr);

private:
    enum class LastToken {
        kStart,
        kNumber,
        kOperator,
        kOpenParen,
        kCloseParen,
        kPercent
    };

    QString expression_;
    QString display_ = "0";
    LastToken last_ = LastToken::kStart;

    int open_parens_ = 0;
    int close_parens_ = 0;
    int current_number_start_ = -1;

    QString CurrentNumber() const;
    int CurrentDigitsCount() const;
    bool CurrentHasDecimalPoint() const;

    void StartNewNumberIfNeeded();
    void SetDisplayFromCurrentOrZero();
    void EmitAll();

    void ReplaceCurrentNumber(const QString& new_number);
    void AppendToken(const QString& token, LastToken new_last);
    void AppendChar(QChar c, LastToken new_last);
    void TrimTrailingSpaces();
    QString TruncateNumber(const QString& number) const;

    bool CanCloseParen() const;
    bool ShouldOpenParen() const;
    bool TryEvaluate(QString* out_result, QString* out_error = nullptr);
};
