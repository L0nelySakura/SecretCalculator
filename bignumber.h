#pragma once

#include <QString>
#include <string>

// BigNumber: простое "большое" десятичное число:
// - хранит знак, цифры без точки и scale (сколько цифр после точки)
// - поддерживает + - * / и "процент" (x % -> x / 100)
//
// Пример: -12.3400 хранится как:
//   negative=true, digits="1234", scale=2  (т.е. -12.34)
//
// Важно:
// - Деление возвращает число с фиксированной точностью fractionalPrecision.
// - Это не банковское округление; при делении просто "отрезаем" до precision.
class BigNumber final
{
public:
    BigNumber();                      // 0
    explicit BigNumber(const QString& s);
    explicit BigNumber(const std::string& s);

    static BigNumber zero();
    static BigNumber one();

    QString toQString() const;
    std::string toStdString() const;

    bool isZero() const;
    bool isNegative() const;

    // Арифметика
    BigNumber operator+(const BigNumber& rhs) const;
    BigNumber operator-(const BigNumber& rhs) const;
    BigNumber operator*(const BigNumber& rhs) const;
    BigNumber operator/(const BigNumber& rhs) const;

    // Процент: по ТЗ "мы просто записываем его, в конце делим на 100".
    // На уровне BigNumber это эквивалентно x / 100.
    BigNumber operator%(int /*percentToken*/) const; // используем как: x % 1
    BigNumber percent() const;                       // явный вариант

    // Сравнения (полезно дальше для CalculatorModel/парсера)
    friend bool operator==(const BigNumber& a, const BigNumber& b);
    friend bool operator!=(const BigNumber& a, const BigNumber& b) { return !(a == b); }
    friend bool operator<(const BigNumber& a, const BigNumber& b);
    friend bool operator>(const BigNumber& a, const BigNumber& b) { return b < a; }
    friend bool operator<=(const BigNumber& a, const BigNumber& b) { return !(b < a); }
    friend bool operator>=(const BigNumber& a, const BigNumber& b) { return !(a < b); }

private:
    // digits: только цифры '0'..'9', минимум "0"
    // scale:  сколько цифр после десятичной точки (>=0)
    // negative: знак (у нуля всегда false)
    std::string digits_;
    int scale_ = 0;
    bool negative_ = false;

    static BigNumber fromParts(std::string digits, int scale, bool negative);

    // Парсинг/нормализация
    static BigNumber parse(const std::string& s);
    void normalize();

    // Внутренние утилиты для больших целых на строках
    static void stripLeadingZeros(std::string& s);
    static int compareAbsIntStrings(const std::string& a, const std::string& b); // a vs b, как целые
    static std::string addAbsIntStrings(const std::string& a, const std::string& b);
    static std::string subAbsIntStrings(const std::string& a, const std::string& b); // предполагается a>=b
    static std::string mulAbsIntStrings(const std::string& a, const std::string& b);

    // Приведение к общему scale (добавлением нулей справа к digits_)
    static void alignScales(BigNumber& a, BigNumber& b);

    // Деление больших целых: num/den -> (quotient, remainder), где оба неотрицательные
    static std::pair<std::string, std::string> divModAbsIntStrings(const std::string& num,
                                                                   const std::string& den);

    // Деление десятичных: возвращает с fractionalPrecision знаков после точки
    static BigNumber divDecimal(const BigNumber& a, const BigNumber& b, int fractionalPrecision);
};

