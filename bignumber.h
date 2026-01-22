#pragma once

#include <QString>
#include <string>
#include <memory>


#include <QString>
#include <string>
#include <memory>
#include <utility>

class BigNumber final
{
public:
    BigNumber();
    explicit BigNumber(const QString& s);
    explicit BigNumber(const std::string& s);

    static BigNumber Zero();
    static BigNumber One();

    QString ToQString() const;
    std::string ToStdString() const;

    bool IsZero() const;
    bool IsNegative() const;

    BigNumber operator+(const BigNumber& rhs) const;
    BigNumber operator-(const BigNumber& rhs) const;
    BigNumber operator*(const BigNumber& rhs) const;
    BigNumber operator/(const BigNumber& rhs) const;

    BigNumber Percent() const;
    BigNumber operator%(int /*percent_token*/) const { return Percent(); }

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

    static BigNumber FromParts(std::string digits, int scale, bool negative);
    static BigNumber Parse(const std::string& s);

    void Normalize();
    static void StripLeadingZeros(std::string& s);

    static int CompareAbsIntStrings(const std::string& a, const std::string& b);
    static std::string AddAbsIntStrings(const std::string& a, const std::string& b);
    static std::string SubAbsIntStrings(const std::string& a, const std::string& b);
    static std::string MulAbsIntStrings(const std::string& a, const std::string& b);

    static void AlignScales(BigNumber& a, BigNumber& b);

    static std::pair<std::string, std::string> DivModAbsIntStrings(
        const std::string& num, const std::string& den);

    static BigNumber DivDecimal(const BigNumber& a, const BigNumber& b,
                                int fractional_precision);
};
