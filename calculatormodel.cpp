#include "calculatormodel.h"
#include "bignumber.h"

#include <vector>
#include <stdexcept>

namespace {

constexpr int kMaxDigitsInNumber = 25;

struct Token {
    enum Kind { kNumber, kOp, kLParen, kRParen, kPercent } kind;
    QString text;
};

int Precedence(const Token& t) {
    if (t.kind == Token::kPercent) return 2;
    if (t.kind == Token::kOp && (t.text == "*" || t.text == "/")) return 2;
    if (t.kind == Token::kOp && (t.text == "+" || t.text == "-")) return 1;
    return 0;
}

bool IsLeftAssoc(const Token& t) {
    return t.kind != Token::kPercent;
}

bool IsOperatorToken(const Token& t) {
    return t.kind == Token::kOp || t.kind == Token::kPercent;
}

bool IsDigitQChar(QChar c) {
    return c >= '0' && c <= '9';
}

std::vector<Token> Tokenize(const QString& expr) {
    std::vector<Token> tokens;
    Token::Kind prev_kind = Token::kOp;
    int i = 0;

    while (i < expr.size()) {
        const QChar c = expr[i];
        if (c.isSpace() || c == '=') {
            ++i;
            continue;
        }

        if (c == '(') {
            tokens.push_back({Token::kLParen, "("});
            prev_kind = Token::kLParen;
            ++i;
            continue;
        }

        if (c == ')') {
            tokens.push_back({Token::kRParen, ")"});
            prev_kind = Token::kRParen;
            ++i;
            continue;
        }

        if (c == '%') {
            tokens.push_back({Token::kPercent, "%"});
            prev_kind = Token::kPercent;
            ++i;
            continue;
        }

        if (c == '+' || c == '-' || c == '*' || c == '/') {
            const bool may_be_unary_minus =
                (c == '-') &&
                (prev_kind == Token::kOp || prev_kind == Token::kLParen ||
                 prev_kind == Token::kPercent);

            if (!may_be_unary_minus) {
                tokens.push_back({Token::kOp, QString(c)});
                prev_kind = Token::kOp;
                ++i;
                continue;
            }
        }

        if (IsDigitQChar(c) || c == '.' || c == '-') {
            int start = i;
            bool seen_dot = false;
            bool seen_digit = false;

            if (expr[i] == '-') {
                ++i;
            }

            while (i < expr.size()) {
                const QChar ch = expr[i];
                if (IsDigitQChar(ch)) {
                    seen_digit = true;
                    ++i;
                    continue;
                }
                if (ch == '.') {
                    if (seen_dot) break;
                    seen_dot = true;
                    ++i;
                    continue;
                }
                break;
            }

            const QString num = expr.mid(start, i - start);
            if (!seen_digit) {
                throw std::runtime_error("bad number");
            }

            tokens.push_back({Token::kNumber, num});
            prev_kind = Token::kNumber;
            continue;
        }

        throw std::runtime_error("unknown token");
    }

    return tokens;
}

std::vector<Token> ToRpn(const std::vector<Token>& tokens) {
    std::vector<Token> out;
    std::vector<Token> stack;

    for (const Token& t : tokens) {
        if (t.kind == Token::kNumber) {
            out.push_back(t);
            continue;
        }

        if (t.kind == Token::kLParen) {
            stack.push_back(t);
            continue;
        }

        if (t.kind == Token::kRParen) {
            while (!stack.empty() && stack.back().kind != Token::kLParen) {
                out.push_back(stack.back());
                stack.pop_back();
            }

            if (stack.empty() || stack.back().kind != Token::kLParen) {
                throw std::runtime_error("mismatched parens");
            }

            stack.pop_back();
            continue;
        }

        if (IsOperatorToken(t)) {
            while (!stack.empty() && IsOperatorToken(stack.back())) {
                const Token& top = stack.back();
                const int p1 = Precedence(t);
                const int p2 = Precedence(top);

                if ((IsLeftAssoc(t) && p1 <= p2) ||
                    (!IsLeftAssoc(t) && p1 < p2)) {
                    out.push_back(top);
                    stack.pop_back();
                } else {
                    break;
                }
            }

            stack.push_back(t);
            continue;
        }

        throw std::runtime_error("bad token");
    }

    while (!stack.empty()) {
        if (stack.back().kind == Token::kLParen ||
            stack.back().kind == Token::kRParen) {
            throw std::runtime_error("mismatched parens");
        }

        out.push_back(stack.back());
        stack.pop_back();
    }

    return out;
}

QString EvalRpn(const std::vector<Token>& rpn) {
    std::vector<BigNumber> stack;

    for (const Token& t : rpn) {
        if (t.kind == Token::kNumber) {
            stack.push_back(BigNumber(t.text));
            continue;
        }

        if (t.kind == Token::kPercent) {
            if (stack.empty()) {
                throw std::runtime_error("percent without operand");
            }

            BigNumber x = stack.back();
            stack.pop_back();
            stack.push_back(x.Percent());
            continue;
        }

        if (t.kind == Token::kOp) {
            if (stack.size() < 2) {
                throw std::runtime_error("op without operands");
            }

            BigNumber b = stack.back();
            stack.pop_back();
            BigNumber a = stack.back();
            stack.pop_back();

            if (t.text == "+") {
                stack.push_back(a + b);
            } else if (t.text == "-") {
                stack.push_back(a - b);
            } else if (t.text == "*") {
                stack.push_back(a * b);
            } else if (t.text == "/") {
                stack.push_back(a / b);
            } else {
                throw std::runtime_error("unknown op");
            }

            continue;
        }

        throw std::runtime_error("bad rpn");
    }

    if (stack.size() != 1) {
        throw std::runtime_error("bad expression");
    }

    return stack.back().ToQString();
}

} // namespace

// Реализация методов CalculatorModel
CalculatorModel::CalculatorModel(QObject* parent) : QObject(parent) {
    EmitAll();
}

void CalculatorModel::EmitAll() {
    emit ExpressionChanged(expression_);
    emit DisplayChanged(display_);
}

void CalculatorModel::ClearAll() {
    expression_.clear();
    display_ = "0";
    last_ = LastToken::kStart;
    open_parens_ = 0;
    close_parens_ = 0;
    current_number_start_ = -1;
    EmitAll();
}

void CalculatorModel::TrimTrailingSpaces() {
    while (!expression_.isEmpty() && expression_.back().isSpace())
        expression_.chop(1);
}

QString CalculatorModel::TruncateNumber(const QString& number) const {
    if (number.isEmpty() || number == "Error")
        return number;

    const int max_digits = 25;

    bool negative = number.startsWith('-');
    QString n = negative ? number.mid(1) : number;

    int dot_pos = n.indexOf('.');
    QString int_part = (dot_pos == -1) ? n : n.left(dot_pos);
    QString frac_part = (dot_pos == -1) ? QString() : n.mid(dot_pos + 1);

    int int_digits = 0;
    for (QChar c : int_part) {
        if (c.isDigit())
            ++int_digits;
    }

    if (int_digits > max_digits) {
        return QString(max_digits, '9');
    }

    QString result;
    if (negative)
        result.append('-');

    int used_digits = 0;

    for (QChar c : int_part) {
        if (c.isDigit()) {
            if (used_digits >= max_digits)
                break;
            ++used_digits;
        }
        result.append(c);
    }
    if (!frac_part.isEmpty() && used_digits < max_digits) {
        result.append('.');
        for (QChar c : frac_part) {
            if (!c.isDigit())
                continue;
            if (used_digits >= max_digits)
                break;
            result.append(c);
            ++used_digits;
        }
    }

    return result;
}

QString CalculatorModel::CurrentNumber() const {
    if (last_ != LastToken::kNumber || current_number_start_ < 0)
        return QString();
    return expression_.mid(current_number_start_);
}

int CalculatorModel::CurrentDigitsCount() const {
    const QString n = CurrentNumber();
    int cnt = 0;
    for (QChar c : n) {
        if (IsDigitQChar(c))
            ++cnt;
    }
    return cnt;
}

bool CalculatorModel::CurrentHasDecimalPoint() const {
    return CurrentNumber().contains('.');
}

void CalculatorModel::AppendToken(const QString& token, LastToken new_last) {
    expression_ += token;
    last_ = new_last;
    if (new_last != LastToken::kNumber)
        current_number_start_ = -1;
}

void CalculatorModel::AppendChar(QChar c, LastToken new_last) {
    expression_ += c;
    last_ = new_last;
    if (new_last != LastToken::kNumber)
        current_number_start_ = -1;
}

void CalculatorModel::ReplaceCurrentNumber(const QString& new_number) {
    if (last_ != LastToken::kNumber || current_number_start_ < 0)
        return;
    expression_ = expression_.left(current_number_start_) + new_number;
}

void CalculatorModel::StartNewNumberIfNeeded() {
    if (last_ == LastToken::kNumber)
        return;
    TrimTrailingSpaces();
    current_number_start_ = expression_.size();
    AppendToken("0", LastToken::kNumber);
}

void CalculatorModel::SetDisplayFromCurrentOrZero() {
    if (last_ == LastToken::kNumber) {
        display_ = CurrentNumber();
        if (display_.isEmpty())
            display_ = "0";
        return;
    }
    display_ = "0";
}

void CalculatorModel::InputDigit(int digit) {
    if (digit < 0 || digit > 9)
        return;

    if (last_ != LastToken::kNumber) {
        if (last_ == LastToken::kCloseParen || last_ == LastToken::kPercent) {
            EmitAll();
            return;
        }
        TrimTrailingSpaces();
        current_number_start_ = expression_.size();
        AppendToken(QString::number(digit), LastToken::kNumber);
        display_ = CurrentNumber();
        EmitAll();
        return;
    }

    if (CurrentDigitsCount() >= kMaxDigitsInNumber) {
        EmitAll();
        return;
    }

    QString n = CurrentNumber();
    if (n == "0") {
        ReplaceCurrentNumber(QString::number(digit));
    } else if (n == "-0") {
        ReplaceCurrentNumber(QString("-") + QString::number(digit));
    } else {
        AppendChar(QChar('0' + digit), LastToken::kNumber);
    }
    display_ = CurrentNumber();
    EmitAll();
}

void CalculatorModel::InputDecimalPoint() {
    if (last_ != LastToken::kNumber) {
        if (last_ == LastToken::kCloseParen || last_ == LastToken::kPercent) {
            EmitAll();
            return;
        }
        TrimTrailingSpaces();
        current_number_start_ = expression_.size();
        AppendToken("0.", LastToken::kNumber);
        display_ = CurrentNumber();
        EmitAll();
        return;
    }

    if (CurrentHasDecimalPoint()) {
        EmitAll();
        return;
    }

    AppendChar('.', LastToken::kNumber);
    display_ = CurrentNumber();
    EmitAll();
}

void CalculatorModel::InputOperator(QChar op) {
    if (op != '+' && op != '-' && op != '*' && op != '/')
        return;

    if (last_ == LastToken::kStart || last_ == LastToken::kOpenParen) {
        EmitAll();
        return;
    }

    TrimTrailingSpaces();

    if (last_ == LastToken::kOperator) {
        if (!expression_.isEmpty())
            expression_.chop(1);
        expression_ += op;
        EmitAll();
        return;
    }

    expression_ += op;
    last_ = LastToken::kOperator;
    current_number_start_ = -1;
    EmitAll();
}

bool CalculatorModel::CanCloseParen() const {
    if (open_parens_ <= close_parens_)
        return false;
    return (last_ == LastToken::kNumber || last_ == LastToken::kCloseParen ||
            last_ == LastToken::kPercent);
}

bool CalculatorModel::ShouldOpenParen() const {
    return (last_ == LastToken::kStart || last_ == LastToken::kOperator ||
            last_ == LastToken::kOpenParen);
}

void CalculatorModel::InputParen() {
    TrimTrailingSpaces();

    bool open_allowed = ShouldOpenParen();
    bool close_allowed = CanCloseParen();

    if (open_allowed) {
        expression_ += '(';
        ++open_parens_;
        last_ = LastToken::kOpenParen;
        current_number_start_ = -1;
        EmitAll();
        return;
    }

    if (close_allowed) {
        expression_ += ')';
        ++close_parens_;
        last_ = LastToken::kCloseParen;
        current_number_start_ = -1;
        EmitAll();
        return;
    }

    EmitAll();
}

void CalculatorModel::ToggleSign() {
    if (last_ != LastToken::kNumber) {
        TrimTrailingSpaces();
        current_number_start_ = expression_.size();
        AppendToken("0", LastToken::kNumber);
    }

    QString n = CurrentNumber();
    if (n.startsWith('-')) {
        n.remove(0, 1);
    } else {
        n.prepend('-');
    }
    ReplaceCurrentNumber(n);
    display_ = CurrentNumber();
    EmitAll();
}

void CalculatorModel::InputPercent() {
    if (last_ == LastToken::kNumber || last_ == LastToken::kCloseParen) {
        expression_ += '%';
        last_ = LastToken::kPercent;
        current_number_start_ = -1;
    }
    EmitAll();
}

void CalculatorModel::Equals() {
    if (!expression_.isEmpty()) {
        if (last_ == LastToken::kOperator || last_ == LastToken::kOpenParen) {
            EmitAll();
            return;
        }
        while (open_parens_ > close_parens_) {
            expression_ += ')';
            ++close_parens_;
            last_ = LastToken::kCloseParen;
        }
    }

    QString result;
    QString err;
    if (!TryEvaluate(&result, &err)) {
        display_ = "Error";
        EmitAll();
        return;
    }

    result = TruncateNumber(result);
    const QString old_expr = expression_;
    display_ = result;
    expression_ = old_expr + "=";
    emit ExpressionChanged(expression_);
    emit DisplayChanged(display_);
    expression_ = result;
    last_ = LastToken::kNumber;
    current_number_start_ = 0;
    open_parens_ = close_parens_ = 0;
}

bool CalculatorModel::TryEvaluate(QString* out_result, QString* out_error) {
    try {
        const std::vector<Token> tokens = Tokenize(expression_);
        const std::vector<Token> rpn = ToRpn(tokens);
        *out_result = EvalRpn(rpn);
        return true;
    } catch (const std::exception& e) {
        if (out_error)
            *out_error = QString::fromLatin1(e.what());
        return false;
    }
}
