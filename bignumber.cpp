#include "bignumber.h"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <vector>

namespace {
constexpr int kDefaultDivPrecision = 50; // знаков после точки при делении

static bool isAllDigits(const std::string& s) {
    return std::all_of(s.begin(), s.end(), [](unsigned char c) { return std::isdigit(c) != 0; });
}

static int compareIntStrings(const std::string& a, const std::string& b) {
    // a и b без лидирующих нулей (кроме "0")
    if (a.size() != b.size())
        return (a.size() < b.size()) ? -1 : 1;
    if (a == b)
        return 0;
    return (a < b) ? -1 : 1; // лексикографически работает при одинаковой длине
}

static std::string mulIntStringByDigit(const std::string& a, int digit) {
    if (digit == 0 || a == "0")
        return "0";
    int carry = 0;
    std::string out;
    out.reserve(a.size() + 1);
    for (int i = static_cast<int>(a.size()) - 1; i >= 0; --i) {
        int x = (a[i] - '0') * digit + carry;
        out.push_back(static_cast<char>('0' + (x % 10)));
        carry = x / 10;
    }
    while (carry > 0) {
        out.push_back(static_cast<char>('0' + (carry % 10)));
        carry /= 10;
    }
    std::reverse(out.begin(), out.end());
    return out;
}
} // namespace

BigNumber::BigNumber() : digits_("0"), scale_(0), negative_(false) {}

BigNumber::BigNumber(const QString& s) : BigNumber(s.toStdString()) {}

BigNumber::BigNumber(const std::string& s) : BigNumber(parse(s)) {}

BigNumber BigNumber::zero() { return BigNumber(); }

BigNumber BigNumber::one() { return BigNumber::fromParts("1", 0, false); }

BigNumber BigNumber::fromParts(std::string digits, int scale, bool negative)
{
    BigNumber n;
    n.digits_ = std::move(digits);
    n.scale_ = scale;
    n.negative_ = negative;
    n.normalize();
    return n;
}

BigNumber BigNumber::parse(const std::string& input)
{
    std::string s;
    s.reserve(input.size());
    for (unsigned char c : input) {
        if (!std::isspace(c))
            s.push_back(static_cast<char>(c));
    }

    if (s.empty())
        throw std::invalid_argument("BigNumber: empty string");

    bool neg = false;
    size_t pos = 0;
    if (s[0] == '+') {
        pos = 1;
    } else if (s[0] == '-') {
        neg = true;
        pos = 1;
    }

    if (pos >= s.size())
        throw std::invalid_argument("BigNumber: sign without digits");

    std::string intPart;
    std::string fracPart;
    bool seenDot = false;

    for (; pos < s.size(); ++pos) {
        char c = s[pos];
        if (c == '.') {
            if (seenDot)
                throw std::invalid_argument("BigNumber: multiple dots");
            seenDot = true;
            continue;
        }
        if (!std::isdigit(static_cast<unsigned char>(c)))
            throw std::invalid_argument("BigNumber: invalid char");
        if (!seenDot)
            intPart.push_back(c);
        else
            fracPart.push_back(c);
    }

    if (intPart.empty())
        intPart = "0";
    if (!isAllDigits(intPart) || !isAllDigits(fracPart))
        throw std::invalid_argument("BigNumber: invalid digits");

    std::string digits = intPart + fracPart;
    int scale = static_cast<int>(fracPart.size());
    return BigNumber::fromParts(std::move(digits), scale, neg);
}

void BigNumber::stripLeadingZeros(std::string& s)
{
    size_t i = 0;
    while (i + 1 < s.size() && s[i] == '0')
        ++i;
    if (i > 0)
        s.erase(0, i);
}

void BigNumber::normalize()
{
    if (digits_.empty())
        digits_ = "0";

    // Убираем лидирующие нули (в целом числе без учета scale это безопасно)
    stripLeadingZeros(digits_);

    // Убираем лишние нули в дробной части (с конца digits_) и уменьшаем scale
    while (scale_ > 0 && digits_.size() > 1 && digits_.back() == '0') {
        digits_.pop_back();
        --scale_;
    }

    // Если осталось меньше цифр чем scale, дополним слева нулями
    if (scale_ > 0 && static_cast<int>(digits_.size()) <= scale_) {
        const int need = scale_ - static_cast<int>(digits_.size()) + 1;
        digits_.insert(digits_.begin(), need, '0');
    }

    // Ноль всегда без знака
    if (digits_ == "0")
        negative_ = false;
}

QString BigNumber::toQString() const
{
    std::string s = toStdString();
    return QString::fromStdString(s);
}

std::string BigNumber::toStdString() const
{
    std::string out;
    out.reserve(digits_.size() + 2);

    if (negative_ && digits_ != "0")
        out.push_back('-');

    if (scale_ == 0) {
        out += digits_;
        return out;
    }

    const int n = static_cast<int>(digits_.size());
    const int split = n - scale_;
    if (split <= 0) {
        out += "0.";
        out.append(static_cast<size_t>(-split), '0');
        out += digits_;
        return out;
    }

    out.append(digits_.begin(), digits_.begin() + split);
    out.push_back('.');
    out.append(digits_.begin() + split, digits_.end());
    return out;
}

bool BigNumber::isZero() const { return digits_ == "0"; }
bool BigNumber::isNegative() const { return negative_ && !isZero(); }

int BigNumber::compareAbsIntStrings(const std::string& a, const std::string& b)
{
    std::string aa = a, bb = b;
    stripLeadingZeros(aa);
    stripLeadingZeros(bb);
    return compareIntStrings(aa, bb);
}

std::string BigNumber::addAbsIntStrings(const std::string& a, const std::string& b)
{
    int i = static_cast<int>(a.size()) - 1;
    int j = static_cast<int>(b.size()) - 1;
    int carry = 0;
    std::string out;
    out.reserve(std::max(a.size(), b.size()) + 1);
    while (i >= 0 || j >= 0 || carry) {
        int sum = carry;
        if (i >= 0) sum += (a[i--] - '0');
        if (j >= 0) sum += (b[j--] - '0');
        out.push_back(static_cast<char>('0' + (sum % 10)));
        carry = sum / 10;
    }
    std::reverse(out.begin(), out.end());
    stripLeadingZeros(out);
    return out;
}

std::string BigNumber::subAbsIntStrings(const std::string& a, const std::string& b)
{
    // a >= b
    int i = static_cast<int>(a.size()) - 1;
    int j = static_cast<int>(b.size()) - 1;
    int borrow = 0;
    std::string out;
    out.reserve(a.size());
    while (i >= 0) {
        int diff = (a[i] - '0') - borrow - (j >= 0 ? (b[j] - '0') : 0);
        if (diff < 0) {
            diff += 10;
            borrow = 1;
        } else {
            borrow = 0;
        }
        out.push_back(static_cast<char>('0' + diff));
        --i;
        --j;
    }
    std::reverse(out.begin(), out.end());
    stripLeadingZeros(out);
    return out;
}

std::string BigNumber::mulAbsIntStrings(const std::string& a, const std::string& b)
{
    if (a == "0" || b == "0")
        return "0";
    std::vector<int> tmp(a.size() + b.size(), 0);
    for (int i = static_cast<int>(a.size()) - 1; i >= 0; --i) {
        for (int j = static_cast<int>(b.size()) - 1; j >= 0; --j) {
            tmp[static_cast<size_t>(i + j + 1)] += (a[i] - '0') * (b[j] - '0');
        }
    }
    for (int k = static_cast<int>(tmp.size()) - 1; k > 0; --k) {
        tmp[static_cast<size_t>(k - 1)] += tmp[static_cast<size_t>(k)] / 10;
        tmp[static_cast<size_t>(k)] %= 10;
    }
    std::string out;
    out.reserve(tmp.size());
    size_t start = 0;
    while (start + 1 < tmp.size() && tmp[start] == 0)
        ++start;
    for (size_t i = start; i < tmp.size(); ++i)
        out.push_back(static_cast<char>('0' + tmp[i]));
    stripLeadingZeros(out);
    return out;
}

void BigNumber::alignScales(BigNumber& a, BigNumber& b)
{
    if (a.scale_ == b.scale_)
        return;
    if (a.scale_ < b.scale_) {
        const int diff = b.scale_ - a.scale_;
        a.digits_.append(static_cast<size_t>(diff), '0');
        a.scale_ = b.scale_;
    } else {
        const int diff = a.scale_ - b.scale_;
        b.digits_.append(static_cast<size_t>(diff), '0');
        b.scale_ = a.scale_;
    }
    a.normalize();
    b.normalize();
}

std::pair<std::string, std::string> BigNumber::divModAbsIntStrings(const std::string& num,
                                                                   const std::string& den)
{
    if (den == "0")
        throw std::domain_error("BigNumber: division by zero");
    if (num == "0")
        return {"0", "0"};

    std::string d = den;
    stripLeadingZeros(d);
    std::string n = num;
    stripLeadingZeros(n);

    if (compareIntStrings(n, d) < 0)
        return {"0", n};

    std::string quotient;
    quotient.reserve(n.size());

    std::string remainder = "0";
    for (char c : n) {
        // remainder = remainder*10 + digit
        if (remainder != "0")
            remainder.push_back(c);
        else
            remainder = std::string(1, c);
        stripLeadingZeros(remainder);

        int qDigit = 0;
        if (compareIntStrings(remainder, d) >= 0) {
            // подбираем qDigit 1..9
            int lo = 1, hi = 9;
            while (lo <= hi) {
                int mid = (lo + hi) / 2;
                std::string prod = mulIntStringByDigit(d, mid);
                int cmp = compareIntStrings(prod, remainder);
                if (cmp <= 0) {
                    qDigit = mid;
                    lo = mid + 1;
                } else {
                    hi = mid - 1;
                }
            }
            remainder = subAbsIntStrings(remainder, mulIntStringByDigit(d, qDigit));
        }
        quotient.push_back(static_cast<char>('0' + qDigit));
    }
    stripLeadingZeros(quotient);
    stripLeadingZeros(remainder);
    return {quotient, remainder};
}

BigNumber BigNumber::divDecimal(const BigNumber& aIn, const BigNumber& bIn, int fractionalPrecision)
{
    if (bIn.isZero())
        throw std::domain_error("BigNumber: division by zero");

    BigNumber a = aIn;
    BigNumber b = bIn;
    a.negative_ = false;
    b.negative_ = false;

    // a = A / 10^sa, b = B / 10^sb
    // a/b = (A * 10^(sb - sa)) / B
    const int shift = b.scale_ - a.scale_;

    std::string numerator = a.digits_;
    if (shift > 0) {
        numerator.append(static_cast<size_t>(shift), '0');
    } else if (shift < 0) {
        // домножим на 10^precision позже; тут оставим как есть
    }

    // Домножаем числитель на 10^precision, чтобы получить дробную часть
    numerator.append(static_cast<size_t>(fractionalPrecision), '0');

    // Если shift < 0, это означает деление на 10^(-shift) => можно эквивалентно увеличить scale результата.
    // Мы обработаем это через итоговый scale.
    auto [q, /*r*/ _] = divModAbsIntStrings(numerator, b.digits_);

    int outScale = fractionalPrecision;
    if (shift < 0) {
        outScale += (-shift);
    }

    return BigNumber::fromParts(std::move(q), outScale, false);
}

BigNumber BigNumber::operator+(const BigNumber& rhs) const
{
    BigNumber a = *this;
    BigNumber b = rhs;
    alignScales(a, b);

    if (a.negative_ == b.negative_) {
        BigNumber out = fromParts(addAbsIntStrings(a.digits_, b.digits_), a.scale_, a.negative_);
        return out;
    }

    // разные знаки => вычитание модулей
    int cmp = compareAbsIntStrings(a.digits_, b.digits_);
    if (cmp == 0)
        return BigNumber::zero();
    if (cmp > 0) {
        return fromParts(subAbsIntStrings(a.digits_, b.digits_), a.scale_, a.negative_);
    }
    return fromParts(subAbsIntStrings(b.digits_, a.digits_), a.scale_, b.negative_);
}

BigNumber BigNumber::operator-(const BigNumber& rhs) const
{
    BigNumber negRhs = rhs;
    if (!negRhs.isZero())
        negRhs.negative_ = !negRhs.negative_;
    return (*this) + negRhs;
}

BigNumber BigNumber::operator*(const BigNumber& rhs) const
{
    BigNumber a = *this;
    BigNumber b = rhs;
    const bool neg = (a.isNegative() != b.isNegative());
    a.negative_ = false;
    b.negative_ = false;

    std::string prod = mulAbsIntStrings(a.digits_, b.digits_);
    const int scale = a.scale_ + b.scale_;
    return fromParts(std::move(prod), scale, neg);
}

BigNumber BigNumber::operator/(const BigNumber& rhs) const
{
    if (rhs.isZero())
        throw std::domain_error("BigNumber: division by zero");

    const bool neg = (this->isNegative() != rhs.isNegative());
    BigNumber q = divDecimal(*this, rhs, kDefaultDivPrecision);
    q.negative_ = neg && !q.isZero();
    q.normalize();
    return q;
}

BigNumber BigNumber::percent() const
{
    // /100 == scale+2
    return fromParts(digits_, scale_ + 2, negative_);
}

BigNumber BigNumber::operator%(int /*percentToken*/) const
{
    return percent();
}

bool operator==(const BigNumber& a, const BigNumber& b)
{
    BigNumber aa = a;
    BigNumber bb = b;
    aa.normalize();
    bb.normalize();
    return aa.negative_ == bb.negative_ && aa.scale_ == bb.scale_ && aa.digits_ == bb.digits_;
}

bool operator<(const BigNumber& a, const BigNumber& b)
{
    if (a == b)
        return false;
    if (a.isNegative() != b.isNegative())
        return a.isNegative();

    // одинаковый знак: сравниваем модули
    BigNumber aa = a;
    BigNumber bb = b;
    aa.negative_ = false;
    bb.negative_ = false;
    BigNumber::alignScales(aa, bb);

    int cmp = BigNumber::compareAbsIntStrings(aa.digits_, bb.digits_);
    if (!a.isNegative()) {
        return cmp < 0;
    }
    // оба отрицательные: больше модуль => меньше число
    return cmp > 0;
}

