#include "bignumber.h"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <vector>

namespace {

constexpr int kDefaultDivPrecision = 40;

bool IsAllDigits(const std::string& s) {
    return std::all_of(s.begin(), s.end(),
                       [](unsigned char c) { return std::isdigit(c) != 0; });
}

int CompareIntStrings(const std::string& a, const std::string& b) {
    if (a.size() != b.size())
        return (a.size() < b.size()) ? -1 : 1;
    if (a == b)
        return 0;
    return (a < b) ? -1 : 1;
}

std::string MulIntStringByDigit(const std::string& a, int digit) {
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

BigNumber::BigNumber(const std::string& s) : BigNumber(Parse(s)) {}

BigNumber BigNumber::Zero() {
    return BigNumber();
}

BigNumber BigNumber::One() {
    return BigNumber::FromParts("1", 0, false);
}

BigNumber BigNumber::FromParts(std::string digits, int scale, bool negative) {
    BigNumber n;
    n.digits_ = std::move(digits);
    n.scale_ = scale;
    n.negative_ = negative;
    n.Normalize();
    return n;
}

BigNumber BigNumber::Parse(const std::string& input) {
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

    std::string int_part;
    std::string frac_part;
    bool seen_dot = false;

    for (; pos < s.size(); ++pos) {
        char c = s[pos];
        if (c == '.') {
            if (seen_dot)
                throw std::invalid_argument("BigNumber: multiple dots");
            seen_dot = true;
            continue;
        }
        if (!std::isdigit(static_cast<unsigned char>(c)))
            throw std::invalid_argument("BigNumber: invalid char");
        if (!seen_dot)
            int_part.push_back(c);
        else
            frac_part.push_back(c);
    }

    if (int_part.empty() && frac_part.empty())
        throw std::invalid_argument("BigNumber: no digits");

    if (int_part.empty())
        int_part = "0";

    StripLeadingZeros(int_part);
    if (int_part.empty())
        int_part = "0";

    while (!frac_part.empty() && frac_part.back() == '0') {
        frac_part.pop_back();
    }

    std::string digits = int_part + frac_part;
    int scale = static_cast<int>(frac_part.size());

    return BigNumber::FromParts(std::move(digits), scale, neg);
}

void BigNumber::StripLeadingZeros(std::string& s) {
    size_t i = 0;
    while (i + 1 < s.size() && s[i] == '0')
        ++i;
    if (i > 0)
        s.erase(0, i);
}

void BigNumber::Normalize() {
    if (digits_.empty())
        digits_ = "0";

    StripLeadingZeros(digits_);
    if (digits_.empty())
        digits_ = "0";

    while (scale_ > 0 && digits_.size() > 1 && digits_.back() == '0') {
        digits_.pop_back();
        --scale_;
    }

    if (scale_ > 0) {
        int integer_digits = static_cast<int>(digits_.size()) - scale_;
        if (integer_digits <= 0) {
            int zeros_to_add = -integer_digits + 1;
            digits_.insert(0, static_cast<size_t>(zeros_to_add), '0');
        }
    }

    if (digits_ == "0") {
        negative_ = false;
        scale_ = 0;
    }
}

QString BigNumber::ToQString() const {
    std::string s = ToStdString();
    return QString::fromStdString(s);
}

std::string BigNumber::ToStdString() const {
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

    while (!out.empty() && out.back() == '0' && out.find('.') != std::string::npos) {
        out.pop_back();
    }
    if (!out.empty() && out.back() == '.') {
        out.pop_back();
    }

    return out;
}

bool BigNumber::IsZero() const {
    return digits_ == "0";
}

bool BigNumber::IsNegative() const {
    return negative_ && !IsZero();
}

int BigNumber::CompareAbsIntStrings(const std::string& a, const std::string& b) {
    std::string aa = a, bb = b;
    StripLeadingZeros(aa);
    StripLeadingZeros(bb);
    return CompareIntStrings(aa, bb);
}

std::string BigNumber::AddAbsIntStrings(const std::string& a, const std::string& b) {
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
    StripLeadingZeros(out);
    return out;
}

std::string BigNumber::SubAbsIntStrings(const std::string& a, const std::string& b) {
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
    StripLeadingZeros(out);
    return out;
}

std::string BigNumber::MulAbsIntStrings(const std::string& a, const std::string& b) {
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
    StripLeadingZeros(out);
    return out;
}

void BigNumber::AlignScales(BigNumber& a, BigNumber& b) {
    if (a.scale_ == b.scale_)
        return;
    int max_scale = std::max(a.scale_, b.scale_);

    if (a.scale_ < max_scale) {
        int diff = max_scale - a.scale_;
        a.digits_.append(static_cast<size_t>(diff), '0');
        a.scale_ = max_scale;
    }

    if (b.scale_ < max_scale) {
        int diff = max_scale - b.scale_;
        b.digits_.append(static_cast<size_t>(diff), '0');
        b.scale_ = max_scale;
    }

    if (a.digits_.empty()) a.digits_ = "0";
    if (b.digits_.empty()) b.digits_ = "0";
}

std::pair<std::string, std::string> BigNumber::DivModAbsIntStrings(
    const std::string& num, const std::string& den) {

    if (den == "0")
        throw std::domain_error("BigNumber: division by zero");
    if (num == "0")
        return {"0", "0"};

    std::string d = den;
    StripLeadingZeros(d);
    std::string n = num;
    StripLeadingZeros(n);

    if (CompareIntStrings(n, d) < 0)
        return {"0", n};

    std::string quotient;
    quotient.reserve(n.size());

    std::string remainder = "0";
    for (char c : n) {
        if (remainder != "0")
            remainder.push_back(c);
        else
            remainder = std::string(1, c);
        StripLeadingZeros(remainder);

        int q_digit = 0;
        if (CompareIntStrings(remainder, d) >= 0) {
            int lo = 1, hi = 9;
            while (lo <= hi) {
                int mid = (lo + hi) / 2;
                std::string prod = MulIntStringByDigit(d, mid);
                int cmp = CompareIntStrings(prod, remainder);
                if (cmp <= 0) {
                    q_digit = mid;
                    lo = mid + 1;
                } else {
                    hi = mid - 1;
                }
            }
            remainder = SubAbsIntStrings(remainder, MulIntStringByDigit(d, q_digit));
        }
        quotient.push_back(static_cast<char>('0' + q_digit));
    }
    StripLeadingZeros(quotient);
    StripLeadingZeros(remainder);
    return {quotient, remainder};
}

BigNumber BigNumber::DivDecimal(const BigNumber& a_in, const BigNumber& b_in,
                                int fractional_precision) {

    if (b_in.IsZero())
        throw std::domain_error("BigNumber: division by zero");

    BigNumber a = a_in;
    BigNumber b = b_in;
    a.negative_ = false;
    b.negative_ = false;
    const int shift = b.scale_ - a.scale_;

    std::string numerator = a.digits_;
    if (shift > 0) {
        numerator.append(static_cast<size_t>(shift), '0');
    }

    numerator.append(static_cast<size_t>(fractional_precision), '0');
    auto [q, /*r*/ _] = DivModAbsIntStrings(numerator, b.digits_);

    int out_scale = fractional_precision;
    if (shift < 0) {
        out_scale += (-shift);
    }

    return BigNumber::FromParts(std::move(q), out_scale, false);
}

BigNumber BigNumber::operator+(const BigNumber& rhs) const {
    BigNumber a = *this;
    BigNumber b = rhs;
    AlignScales(a, b);

    if (a.negative_ == b.negative_) {
        BigNumber out = FromParts(AddAbsIntStrings(a.digits_, b.digits_),
                                  a.scale_, a.negative_);
        return out;
    }

    int cmp = CompareAbsIntStrings(a.digits_, b.digits_);
    if (cmp == 0)
        return BigNumber::Zero();
    if (cmp > 0) {
        return FromParts(SubAbsIntStrings(a.digits_, b.digits_),
                         a.scale_, a.negative_);
    }
    return FromParts(SubAbsIntStrings(b.digits_, a.digits_),
                     a.scale_, b.negative_);
}

BigNumber BigNumber::operator-(const BigNumber& rhs) const {
    BigNumber neg_rhs = rhs;
    if (!neg_rhs.IsZero())
        neg_rhs.negative_ = !neg_rhs.negative_;
    return (*this) + neg_rhs;
}

BigNumber BigNumber::operator*(const BigNumber& rhs) const {
    BigNumber a = *this;
    BigNumber b = rhs;
    const bool neg = (a.IsNegative() != b.IsNegative());
    a.negative_ = false;
    b.negative_ = false;

    std::string prod = MulAbsIntStrings(a.digits_, b.digits_);
    const int scale = a.scale_ + b.scale_;
    return FromParts(std::move(prod), scale, neg);
}

BigNumber BigNumber::operator/(const BigNumber& rhs) const {
    if (rhs.IsZero())
        throw std::domain_error("BigNumber: division by zero");

    const bool neg = (this->IsNegative() != rhs.IsNegative());
    BigNumber q = DivDecimal(*this, rhs, kDefaultDivPrecision);
    q.negative_ = neg && !q.IsZero();
    q.Normalize();
    return q;
}

BigNumber BigNumber::Percent() const {
    return FromParts(digits_, scale_ + 2, negative_);
}

bool operator==(const BigNumber& a, const BigNumber& b) {
    BigNumber aa = a;
    BigNumber bb = b;
    aa.Normalize();
    bb.Normalize();
    return aa.negative_ == bb.negative_ &&
           aa.scale_ == bb.scale_ &&
           aa.digits_ == bb.digits_;
}

bool operator<(const BigNumber& a, const BigNumber& b) {
    if (a == b)
        return false;
    if (a.IsNegative() != b.IsNegative())
        return a.IsNegative();

    BigNumber aa = a;
    BigNumber bb = b;
    aa.negative_ = false;
    bb.negative_ = false;
    BigNumber::AlignScales(aa, bb);

    int cmp = BigNumber::CompareAbsIntStrings(aa.digits_, bb.digits_);
    if (!a.IsNegative()) {
        return cmp < 0;
    }
    return cmp > 0;
}
