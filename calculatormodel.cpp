#include "calculatormodel.h"

#include "bignumber.h"

#include <QVector>
#include <stdexcept>

namespace {
constexpr int kMaxDigitsInNumber = 25;

struct Tok {
    enum Kind { Number, Op, LParen, RParen, Percent } kind;
    QString text; // для Number: строка числа; для Op: "+-*/"; для Percent: "%"
};

static int precedence(const Tok& t) {
    // % должен иметь такой же приоритет, как и деление
    if (t.kind == Tok::Percent) return 2;        // postfix unary
    if (t.kind == Tok::Op && (t.text == "*" || t.text == "/")) return 2;
    if (t.kind == Tok::Op && (t.text == "+" || t.text == "-")) return 1;
    return 0;
}

static bool isLeftAssoc(const Tok& t) {
    // % postfix, остальные бинарные left-assoc
    return t.kind != Tok::Percent;
}

static bool isOperatorTok(const Tok& t) {
    return t.kind == Tok::Op || t.kind == Tok::Percent;
}

static bool isDigitQChar(QChar c) { return c >= '0' && c <= '9'; }
} // namespace

CalculatorModel::CalculatorModel(QObject* parent) : QObject(parent)
{
    emitAll();
}

void CalculatorModel::emitAll()
{
    emit expressionChanged(expression_);
    emit displayChanged(display_);
}

void CalculatorModel::clearAll()
{
    expression_.clear();
    display_ = "0";
    last_ = LastToken::Start;
    openParens_ = 0;
    closeParens_ = 0;
    currentNumberStart_ = -1;
    emitAll();
}

void CalculatorModel::trimTrailingSpaces()
{
    while (!expression_.isEmpty() && expression_.back().isSpace())
        expression_.chop(1);
}

QString CalculatorModel::currentNumber() const
{
    if (last_ != LastToken::Number || currentNumberStart_ < 0)
        return QString();
    return expression_.mid(currentNumberStart_);
}

int CalculatorModel::currentDigitsCount() const
{
    const QString n = currentNumber();
    int cnt = 0;
    for (QChar c : n) {
        if (isDigitQChar(c))
            ++cnt;
    }
    return cnt;
}

bool CalculatorModel::currentHasDecimalPoint() const
{
    return currentNumber().contains('.');
}

void CalculatorModel::appendToken(const QString& token, LastToken newLast)
{
    expression_ += token;
    last_ = newLast;
    if (newLast != LastToken::Number)
        currentNumberStart_ = -1;
}

void CalculatorModel::appendChar(QChar c, LastToken newLast)
{
    expression_ += c;
    last_ = newLast;
    if (newLast != LastToken::Number)
        currentNumberStart_ = -1;
}

void CalculatorModel::replaceCurrentNumber(const QString& newNumber)
{
    if (last_ != LastToken::Number || currentNumberStart_ < 0)
        return;
    expression_ = expression_.left(currentNumberStart_) + newNumber;
}

void CalculatorModel::startNewNumberIfNeeded()
{
    if (last_ == LastToken::Number)
        return;
    trimTrailingSpaces();
    currentNumberStart_ = expression_.size();
    appendToken("0", LastToken::Number);
}

void CalculatorModel::setDisplayFromCurrentOrZero()
{
    if (last_ == LastToken::Number) {
        display_ = currentNumber();
        if (display_.isEmpty())
            display_ = "0";
        return;
    }
    display_ = "0";
}

void CalculatorModel::inputDigit(int digit)
{
    if (digit < 0 || digit > 9)
        return;

    if (last_ != LastToken::Number) {
        // После ')' или '%' блокируем ввод цифр (не начинаем новое число)
        if (last_ == LastToken::CloseParen || last_ == LastToken::Percent) {
            emitAll();
            return;
        }
        trimTrailingSpaces();
        currentNumberStart_ = expression_.size();
        appendToken(QString::number(digit), LastToken::Number);
        display_ = currentNumber();
        emitAll();
        return;
    }

    if (currentDigitsCount() >= kMaxDigitsInNumber) {
        // лимит 25 цифр
        emitAll();
        return;
    }

    QString n = currentNumber();
    // ведущий 0 заменяем (если это ровно "0" или "-0")
    if (n == "0") {
        replaceCurrentNumber(QString::number(digit));
    } else if (n == "-0") {
        replaceCurrentNumber(QString("-") + QString::number(digit));
    } else {
        appendChar(QChar('0' + digit), LastToken::Number);
    }
    display_ = currentNumber();
    emitAll();
}

void CalculatorModel::inputDecimalPoint()
{
    if (last_ != LastToken::Number) {
        // После ')' или '%' точку не добавляем (неявное умножение запрещено)
        if (last_ == LastToken::CloseParen || last_ == LastToken::Percent) {
            emitAll();
            return;
        }
        // начинаем число с 0.
        trimTrailingSpaces();
        currentNumberStart_ = expression_.size();
        appendToken("0.", LastToken::Number);
        display_ = currentNumber();
        emitAll();
        return;
    }

    if (currentHasDecimalPoint()) {
        emitAll();
        return;
    }

    appendChar('.', LastToken::Number);
    display_ = currentNumber();
    emitAll();
}

void CalculatorModel::inputOperator(QChar op)
{
    if (op != '+' && op != '-' && op != '*' && op != '/')
        return;

    // Нельзя ставить оператор сразу после Start или '('
    if (last_ == LastToken::Start || last_ == LastToken::OpenParen) {
        emitAll();
        return;
    }

    trimTrailingSpaces();

    // Если оператор уже стоит — заменяем его (удобно менять знак операции)
    if (last_ == LastToken::Operator) {
        if (!expression_.isEmpty())
            expression_.chop(1);
        expression_ += op;
        emitAll();
        return;
    }

    // После '%' тоже можно ставить оператор
    // После числа или ')' — ок
    expression_ += op;
    last_ = LastToken::Operator;
    currentNumberStart_ = -1;
    emitAll();
}

bool CalculatorModel::canCloseParen() const
{
    if (openParens_ <= closeParens_)
        return false;
    return (last_ == LastToken::Number || last_ == LastToken::CloseParen || last_ == LastToken::Percent);
}

bool CalculatorModel::shouldOpenParen() const
{
    return (last_ == LastToken::Start || last_ == LastToken::Operator || last_ == LastToken::OpenParen);
}

void CalculatorModel::inputParen()
{
    trimTrailingSpaces();

    if (shouldOpenParen() || !canCloseParen()) {
        expression_ += '(';
        ++openParens_;
        last_ = LastToken::OpenParen;
        currentNumberStart_ = -1;
        emitAll();
        return;
    }

    expression_ += ')';
    ++closeParens_;
    last_ = LastToken::CloseParen;
    currentNumberStart_ = -1;
    emitAll();
}

void CalculatorModel::toggleSign()
{
    if (last_ != LastToken::Number) {
        // начинаем новое число, чтобы можно было ввести "-..."
        trimTrailingSpaces();
        currentNumberStart_ = expression_.size();
        appendToken("0", LastToken::Number);
    }

    QString n = currentNumber();
    if (n.startsWith('-')) {
        n.remove(0, 1);
    } else {
        n.prepend('-');
    }
    replaceCurrentNumber(n);
    display_ = currentNumber();
    emitAll();
}

void CalculatorModel::inputPercent()
{
    // Разрешаем % только после числа или ')'
    if (last_ == LastToken::Number || last_ == LastToken::CloseParen) {
        expression_ += '%';
        last_ = LastToken::Percent;
        currentNumberStart_ = -1;
    }
    emitAll();
}

void CalculatorModel::equals()
{
    // автозакрытие скобок
    if (!expression_.isEmpty()) {
        // Если выражение заканчивается оператором/открывающей скобкой — просто ничего не делаем
        if (last_ == LastToken::Operator || last_ == LastToken::OpenParen) {
            emitAll();
            return;
        }
        while (openParens_ > closeParens_) {
            expression_ += ')';
            ++closeParens_;
            last_ = LastToken::CloseParen;
        }
    }

    QString result;
    QString err;
    if (!tryEvaluate(&result, &err)) {
        // простое поведение: показываем Error в display, выражение оставляем
        display_ = "Error";
        emitAll();
        return;
    }

    // показываем итог; выражение переносим в верхнюю строку с "="
    const QString oldExpr = expression_;
    display_ = result;
    expression_ = oldExpr + "=";
    emit expressionChanged(expression_);
    emit displayChanged(display_);

    // Дальше начинаем новое выражение с результата (типичное поведение)
    expression_ = result;
    last_ = LastToken::Number;
    currentNumberStart_ = 0;
    openParens_ = closeParens_ = 0;
}

static QVector<Tok> tokenize(const QString& expr)
{
    QVector<Tok> toks;
    Tok::Kind prevKind = Tok::Op; // как будто перед началом стоит оператор, чтобы первый '-' считался унарным
    int i = 0;
    while (i < expr.size()) {
        const QChar c = expr[i];
        if (c.isSpace() || c == '=') {
            ++i;
            continue;
        }
        if (c == '(') {
            toks.push_back({Tok::LParen, "("});
            prevKind = Tok::LParen;
            ++i;
            continue;
        }
        if (c == ')') {
            toks.push_back({Tok::RParen, ")"});
            prevKind = Tok::RParen;
            ++i;
            continue;
        }
        if (c == '%') {
            toks.push_back({Tok::Percent, "%"});
            prevKind = Tok::Percent;
            ++i;
            continue;
        }
        if (c == '+' || c == '-' || c == '*' || c == '/') {
            // Обрабатываем '-' как унарный минус, если он стоит в начале
            // или после другого оператора/открывающей скобки/процента.
            const bool mayBeUnaryMinus =
                (c == '-') &&
                (prevKind == Tok::Op || prevKind == Tok::LParen || prevKind == Tok::Percent);

            if (!mayBeUnaryMinus) {
                toks.push_back({Tok::Op, QString(c)});
                prevKind = Tok::Op;
                ++i;
                continue;
            }
            // Иначе трактуем '-' как знак числа — обработаем ниже в ветке Number.
        }

        // Number: возможный ведущий '-' (знак), цифры и одна точка
        if (isDigitQChar(c) || c == '.' || c == '-') {
            int start = i;
            bool seenDot = false;
            bool seenDigit = false;
            if (expr[i] == '-') {
                ++i;
            }
            while (i < expr.size()) {
                const QChar ch = expr[i];
                if (isDigitQChar(ch)) {
                    seenDigit = true;
                    ++i;
                    continue;
                }
                if (ch == '.') {
                    if (seenDot)
                        break;
                    seenDot = true;
                    ++i;
                    continue;
                }
                break;
            }
            const QString num = expr.mid(start, i - start);
            if (!seenDigit)
                throw std::runtime_error("bad number");
            toks.push_back({Tok::Number, num});
            prevKind = Tok::Number;
            continue;
        }

        throw std::runtime_error("unknown token");
    }
    return toks;
}

static QVector<Tok> toRpn(const QVector<Tok>& toks)
{
    QVector<Tok> out;
    QVector<Tok> st;

    for (const Tok& t : toks) {
        if (t.kind == Tok::Number) {
            out.push_back(t);
            continue;
        }
        if (t.kind == Tok::LParen) {
            st.push_back(t);
            continue;
        }
        if (t.kind == Tok::RParen) {
            while (!st.isEmpty() && st.back().kind != Tok::LParen) {
                out.push_back(st.back());
                st.pop_back();
            }
            if (st.isEmpty() || st.back().kind != Tok::LParen)
                throw std::runtime_error("mismatched parens");
            st.pop_back();
            continue;
        }

        if (isOperatorTok(t)) {
            while (!st.isEmpty() && isOperatorTok(st.back())) {
                const Tok& top = st.back();
                const int p1 = precedence(t);
                const int p2 = precedence(top);
                if ((isLeftAssoc(t) && p1 <= p2) || (!isLeftAssoc(t) && p1 < p2)) {
                    out.push_back(top);
                    st.pop_back();
                } else {
                    break;
                }
            }
            st.push_back(t);
            continue;
        }
        throw std::runtime_error("bad token");
    }

    while (!st.isEmpty()) {
        if (st.back().kind == Tok::LParen || st.back().kind == Tok::RParen)
            throw std::runtime_error("mismatched parens");
        out.push_back(st.back());
        st.pop_back();
    }
    return out;
}

static QString evalRpn(const QVector<Tok>& rpn)
{
    QVector<BigNumber> st;
    for (const Tok& t : rpn) {
        if (t.kind == Tok::Number) {
            st.push_back(BigNumber(t.text));
            continue;
        }
        if (t.kind == Tok::Percent) {
            if (st.isEmpty())
                throw std::runtime_error("percent without operand");
            BigNumber x = st.back();
            st.pop_back();
            st.push_back(x.percent());
            continue;
        }
        if (t.kind == Tok::Op) {
            if (st.size() < 2)
                throw std::runtime_error("op without operands");
            BigNumber b = st.back(); st.pop_back();
            BigNumber a = st.back(); st.pop_back();
            if (t.text == "+") st.push_back(a + b);
            else if (t.text == "-") st.push_back(a - b);
            else if (t.text == "*") st.push_back(a * b);
            else if (t.text == "/") st.push_back(a / b);
            else throw std::runtime_error("unknown op");
            continue;
        }
        throw std::runtime_error("bad rpn");
    }
    if (st.size() != 1)
        throw std::runtime_error("bad expression");
    return st.back().toQString();
}

bool CalculatorModel::tryEvaluate(QString* outResult, QString* outError)
{
    try {
        const QVector<Tok> toks = tokenize(expression_);
        const QVector<Tok> rpn = toRpn(toks);
        *outResult = evalRpn(rpn);
        return true;
    } catch (const std::exception& e) {
        if (outError)
            *outError = QString::fromLatin1(e.what());
        return false;
    }
}

