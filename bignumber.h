#pragma once

#include <QString>
#include <string>

class BigNumber final
{
public:
    BigNumber();
    explicit BigNumber(const QString& s);
    explicit BigNumber(const std::string& s);

    static BigNumber zero();
    static BigNumber one();

    QString toQString() const;
    std::string toStdString() const;

    bool isZero() const;
    bool isNegative() const;

    BigNumber operator+(const BigNumber& rhs) const;
    BigNumber operator-(const BigNumber& rhs) const;
    BigNumber operator*(const BigNumber& rhs) const;
    BigNumber operator/(const BigNumber& rhs) const;

    BigNumber operator%(int /*percentToken*/) const;
    BigNumber percent() const;

    friend bool operator==(const BigNumber& a, const BigNumber& b);
    friend bool operator!=(const BigNumber& a, const BigNumber& b) { return !(a == b); }
    friend bool operator<(const BigNumber& a, const BigNumber& b);
    friend bool operator>(const BigNumber& a, const BigNumber& b) { return b < a; }
    friend bool operator<=(const BigNumber& a, const BigNumber& b) { return !(b < a); }
    friend bool operator>=(const BigNumber& a, const BigNumber& b) { return !(a < b); }

private:

    std::string digits_;
    int scale_ = 0;
    bool negative_ = false;

    static BigNumber fromParts(std::string digits, int scale, bool negative);

    static BigNumber parse(const std::string& s);
    void normalize();

    static void stripLeadingZeros(std::string& s);
    static int compareAbsIntStrings(const std::string& a, const std::string& b); // a vs b, как целые
    static std::string addAbsIntStrings(const std::string& a, const std::string& b);
    static std::string subAbsIntStrings(const std::string& a, const std::string& b); // предполагается a>=b
    static std::string mulAbsIntStrings(const std::string& a, const std::string& b);

    static void alignScales(BigNumber& a, BigNumber& b);

    static std::pair<std::string, std::string> divModAbsIntStrings(const std::string& num,
                                                                   const std::string& den);

    static BigNumber divDecimal(const BigNumber& a, const BigNumber& b, int fractionalPrecision);
};

