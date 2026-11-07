#include "domain.h"
#include "mathsolver.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <functional>
#include <iomanip>
#include <limits>
#include <numbers>
#include <regex>
#include <sstream>
namespace ms {
Expr number(double v) {
    return std::make_shared<Node>(Node{"num", v, {}, {}});
}
Expr variable() {
    return std::make_shared<Node>(Node{"x", 0, {}, {}});
}
std::vector<std::string> parameters(Expr e) {
    std::vector<std::string> names;
    std::function<void(Expr)> visit = [&](Expr n) {
        if (!n)
            return;
        if (n->op == "param" && std::find(names.begin(), names.end(), n->literal) == names.end())
            names.push_back(n->literal);
        visit(n->a);
        visit(n->b);
    };
    visit(e);
    std::sort(names.begin(), names.end());
    return names;
}
Expr bindParameters(Expr e, const std::map<std::string, double> &values) {
    if (e->op == "param") {
        auto found = values.find(e->literal);
        if (found == values.end() || !std::isfinite(found->second))
            throw std::runtime_error("请为参数 " + e->literal + " 指定有限实数值");
        return number(found->second);
    }
    if (!e->a)
        return e;
    // Retain the source domain rather than simplifying 0*undefined.
    return std::make_shared<Node>(Node{e->op, e->value, bindParameters(e->a, values),
                                       e->b ? bindParameters(e->b, values) : Expr{}, e->literal});
}
static bool is(Expr e, double v) {
    return e && e->op == "num" && e->value == v;
}
Expr make(std::string op, Expr a, Expr b) {
    if (op == "+") {
        if (is(a, 0))
            return b;
        if (is(b, 0))
            return a;
    }
    if (op == "-") {
        if (is(b, 0))
            return a;
    }
    if (op == "*") {
        if (is(a, 0) || is(b, 0))
            return number(0);
        if (is(a, 1))
            return b;
        if (is(b, 1))
            return a;
    }
    if (op == "/") {
        if (is(b, 1))
            return a;
    }
    if (op == "^") {
        if (is(b, 1))
            return a;
        if (is(b, 0))
            return number(1);
    }
    auto n = std::make_shared<Node>(Node{op, 0, a, b});
    if (a && a->op == "num" && (!b || b->op == "num")) {
        double v = eval(n, 0);
        if (std::isfinite(v))
            return number(v);
    }
    return n;
}
// Parsing preserves the source domain: 0*ln(x) must not become zero at x < 0.
// The separate make() factory simplifies expressions produced by symbolic rules.
static Expr syntaxNode(std::string op, Expr a, Expr b = {}) {
    auto n = std::make_shared<Node>(Node{op, 0, a, b});
    if ((op == "+" || op == "-" || op == "*" || op == "^") && a && a->op == "num" && b &&
        b->op == "num" && a->value == std::trunc(a->value) && b->value == std::trunc(b->value)) {
        double v = eval(n, 0);
        if (std::isfinite(v) && std::abs(v) <= 9007199254740991. && v == std::trunc(v))
            return number(v);
    }
    return n;
}
struct Token {
    std::string s;
    size_t pos;
};
// Recursive descent: sum -> product -> unary -> power -> atom.
// Power recurses through unary on its right, so it is right-associative.
class Parser {
    std::vector<Token> t;
    size_t p = 0;
    int depth = 0;
    [[noreturn]] void error(std::string s) {
        throw std::runtime_error("第 " + std::to_string(t[p].pos + 1) + " 字节：" + s);
    }
    std::string peek() {
        return t[p].s;
    }
    bool eat(const std::string &s) {
        if (peek() == s) {
            ++p;
            return true;
        }
        return false;
    }
    void need(const std::string &s) {
        if (!eat(s))
            error("需要 '" + s + "'");
    }
    bool starts() {
        auto s = peek();
        return s == "(" || s == "{" || s == "x" || s == "e" || s == "a" || s == "b" || s == "c" ||
               s == "n" || s == "pi" || s == "frac" || s == "sqrt" || function(s) ||
               (!s.empty() && (std::isdigit((unsigned char)s[0]) || s[0] == '.'));
    }
    static bool function(const std::string &s) {
        return s == "sin" || s == "cos" || s == "tan" || s == "sec" || s == "csc" || s == "cot" ||
               s == "asin" || s == "acos" || s == "atan" || s == "acot" || s == "ln" ||
               s == "log" || s == "exp" || s == "abs";
    }
    Expr sum() {
        auto a = product();
        while (peek() == "+" || peek() == "-") {
            auto o = t[p++].s;
            a = syntaxNode(o, a, product());
        }
        return a;
    }
    Expr product() {
        auto a = unary();
        while (true) {
            if (eat("*"))
                a = syntaxNode("*", a, unary());
            else if (eat("/"))
                a = syntaxNode("/", a, unary());
            else if (starts())
                a = syntaxNode("*", a, unary());
            else
                break;
        }
        return a;
    }
    Expr unary() {
        if (eat("+"))
            return unary();
        if (eat("-"))
            return syntaxNode("*", number(-1), unary());
        return power();
    }
    Expr power() {
        auto a = atom();
        if (eat("^"))
            a = syntaxNode("^", a, unary());
        return a;
    }
    Expr atom() {
        if (++depth > 128)
            error("表达式嵌套过深");
        auto a = primary();
        --depth;
        return a;
    }
    Expr argument() {
        if (peek() == "{" || peek() == "(")
            return atom();
        auto a = unary();
        while (peek() == "x" || peek() == "a" || peek() == "b" || peek() == "c" || peek() == "n")
            a = syntaxNode("*", a, unary());
        return a;
    }
    Expr fractionArg() {
        if (peek() == "{" || peek() == "(")
            return atom();
        return atom();
    }
    Expr primary() {
        auto s = peek();
        if (eat("(")) {
            auto a = sum();
            need(")");
            return a;
        }
        if (eat("{")) {
            auto a = sum();
            need("}");
            return a;
        }
        if (eat("|")) {
            auto a = sum();
            need("|");
            return syntaxNode("abs", a);
        }
        if (eat("x"))
            return variable();
        if (s == "a" || s == "b" || s == "c" || s == "n") {
            ++p;
            return std::make_shared<Node>(Node{"param", 0, {}, {}, s});
        }
        if (eat("e"))
            return std::make_shared<Node>(Node{"e", 0, {}, {}});
        if (eat("pi"))
            return std::make_shared<Node>(Node{"pi", 0, {}, {}});
        if (eat("frac")) {
            auto a = fractionArg();
            auto b = fractionArg();
            return syntaxNode("/", a, b);
        }
        if (eat("sqrt")) {
            Expr n = number(2);
            if (eat("[")) {
                n = sum();
                need("]");
            }
            auto a = argument();
            return syntaxNode("root", a, n);
        }
        if (function(s)) {
            ++p;
            Expr base;
            if (s == "log" && eat("_"))
                base = atom();
            Expr exponent;
            if (eat("^"))
                exponent = atom();
            auto arg = argument();
            if (base) {
                auto logarithm = syntaxNode("/", syntaxNode("ln", arg), syntaxNode("ln", base));
                return exponent ? syntaxNode("^", logarithm, exponent) : logarithm;
            }
            // Lower reciprocal trig into the existing safe AST. This preserves
            // denominator zeros for domain checks and reuses all calculus rules.
            auto a = s == "sec"   ? syntaxNode("/", number(1), syntaxNode("cos", arg))
                     : s == "csc" ? syntaxNode("/", number(1), syntaxNode("sin", arg))
                     : s == "cot" ? syntaxNode("/", syntaxNode("cos", arg), syntaxNode("sin", arg))
                     : s == "acot"
                         ? syntaxNode("-",
                                      syntaxNode("/", std::make_shared<Node>(Node{"pi", 0, {}, {}}),
                                                 number(2)),
                                      syntaxNode("atan", arg))
                         : syntaxNode(s, arg);
            return exponent ? syntaxNode("^", a, exponent) : a;
        }
        if (!s.empty() && (std::isdigit((unsigned char)s[0]) || s[0] == '.')) {
            ++p;
            try {
                size_t z;
                double v = std::stod(s, &z);
                if (z != s.size() || !std::isfinite(v))
                    error("非法数值");
                return std::make_shared<Node>(Node{"num", v, {}, {}, s});
            } catch (...) {
                error("非法数值");
            }
        }
        error(s.empty() ? "表达式不完整" : "不支持的符号 '" + s + "'");
    }

  public:
    Parser(const std::string &src) {
        if (src.size() > 4096)
            throw std::runtime_error("表达式最长 4096 字节");
        for (size_t i = 0; i < src.size();) {
            size_t st = i;
            unsigned char c = src[i];
            if (std::isspace(c)) {
                ++i;
                continue;
            }
            std::string s;
            if (c == '\\') {
                ++i;
                if (i < src.size() && (src[i] == ',' || src[i] == '!' || src[i] == ' ')) {
                    ++i;
                    continue;
                }
                while (i < src.size() && std::isalpha((unsigned char)src[i]))
                    s += src[i++];
                if (s == "left" || s == "right")
                    continue;
                if (s == "cdot" || s == "times")
                    s = "*";
                if (s == "arcsin")
                    s = "asin";
                if (s == "arccos")
                    s = "acos";
                if (s == "arctan")
                    s = "atan";
                if (s == "arccot")
                    s = "acot";
                if (s == "lg")
                    s = "log";
                if (s == "dfrac" || s == "tfrac")
                    s = "frac";
                if (s == "lvert" || s == "rvert")
                    s = "|";
                if (s.empty())
                    throw std::runtime_error("未知 LaTeX 转义，第 " + std::to_string(st + 1) +
                                             " 字节");
                if (s == "a" || s == "b" || s == "c" || s == "n")
                    throw std::runtime_error("字母参数直接输入 a、b、c、n，不使用反斜杠命令");
            } else if (std::isdigit(c) || c == '.') {
                while (i < src.size() && (std::isdigit((unsigned char)src[i]) || src[i] == '.'))
                    s += src[i++];
            } else if (std::isalpha(c)) {
                std::string keyword;
                for (auto word :
                     {"arcsin", "arccos", "arctan", "arccot", "asin", "acos", "atan", "acot",
                      "sqrt",   "frac",   "exp",    "sin",    "cos",  "tan",  "sec",  "csc",
                      "cot",    "abs",    "log",    "ln",     "lg",   "pi"})
                    if (src.compare(i, std::string(word).size(), word) == 0) {
                        keyword = word;
                        break;
                    }
                if (!keyword.empty()) {
                    s = keyword;
                    i += keyword.size();
                } else if (src.compare(i, 3, "exp") == 0) {
                    s = "exp";
                    i += 3;
                } else if (c == 'x' || c == 'e' || c == 'a' || c == 'b' || c == 'c' || c == 'n') {
                    s += src[i++];
                } else
                    while (i < src.size() && std::isalpha((unsigned char)src[i]))
                        s += src[i++];
            } else {
                s += src[i++];
            }
            if (s == "arccot")
                s = "acot";
            if (s == "lg")
                s = "log";
            t.push_back({s, st});
        }
        t.push_back({"", src.size()});
    }
    Expr run() {
        auto a = sum();
        if (!peek().empty())
            error("多余或不匹配的符号 '" + peek() + "'");
        return a;
    }
};
Expr parse(const std::string &s) {
    if (s.size() > 4096)
        throw std::runtime_error("表达式最长 4096 字节");
    // A display-style indefinite integral wrapper is accepted; bounds remain UI parameters.
    if (s.find("\\int") != std::string::npos) {
        static const std::regex wrapper(R"(^\s*\\int\s+(.+?)(?:\\,|\\!|\s)*d\s*x\s*$)");
        std::smatch match;
        if (!std::regex_match(s, match, wrapper))
            throw std::runtime_error("积分输入可写为 \\int f(x)\\,dx；定积分上下限请使用参数栏");
        return Parser(match[1].str()).run();
    }
    return Parser(s).run();
}
double eval(Expr n, double x) {
    auto o = n->op;
    if (o == "param")
        throw std::runtime_error("含未赋值的字母参数 " + n->literal + "；此操作需要具体数值");
    if (o == "num")
        return n->value;
    if (o == "x")
        return x;
    if (o == "e")
        return std::numbers::e;
    if (o == "pi")
        return std::numbers::pi;
    double a = eval(n->a, x), b = n->b ? eval(n->b, x) : 0;
    // pow(1, NaN), pow(NaN, 0), and atan(infinity) must not mask an
    // invalid child operation. Overflow is also an unavailable numeric value.
    if (!std::isfinite(a) || !std::isfinite(b))
        return NAN;
    if (o == "+")
        return a + b;
    if (o == "-")
        return a - b;
    if (o == "*")
        return a * b;
    if (o == "/")
        return a / b;
    if (o == "^")
        return std::pow(a, b);
    if (o == "root") {
        if (b == 0)
            return NAN;
        if (a < 0 && b == std::trunc(b) && std::fmod(std::abs(b), 2) == 1)
            return -std::pow(-a, 1 / b);
        return std::pow(a, 1 / b);
    }
    if (o == "sin")
        return std::sin(a);
    if (o == "cos")
        return std::cos(a);
    if (o == "tan")
        return std::tan(a);
    if (o == "asin")
        return std::asin(a);
    if (o == "acos")
        return std::acos(a);
    if (o == "atan")
        return std::atan(a);
    if (o == "ln")
        return std::log(a);
    if (o == "log")
        return std::log10(a);
    if (o == "exp")
        return std::exp(a);
    if (o == "abs")
        return std::abs(a);
    throw std::runtime_error("未知 AST 节点");
}
std::string latex(Expr n) {
    auto o = n->op;
    if (o == "param")
        return n->literal;
    if (o == "num") {
        if (!n->literal.empty())
            return n->literal;
        std::ostringstream s;
        s << std::setprecision(12) << n->value;
        auto text = s.str();
        auto exponent = text.find('e');
        if (exponent != std::string::npos)
            return text.substr(0, exponent) + "\\times 10^{" + text.substr(exponent + 1) + "}";
        return text;
    }
    if (o == "x")
        return "x";
    if (o == "e")
        return "e";
    if (o == "pi")
        return "\\pi";
    auto a = latex(n->a), b = n->b ? latex(n->b) : "";
    if (o == "+")
        return "\\left(" + a + "+" + b + "\\right)";
    if (o == "-")
        return "\\left(" + a + "-" + b + "\\right)";
    if (o == "*")
        return "\\left(" + a + "\\right)\\cdot\\left(" + b + "\\right)";
    if (o == "/")
        return "\\frac{" + a + "}{" + b + "}";
    if (o == "^")
        return "\\left(" + a + "\\right)^{" + b + "}";
    if (o == "root")
        return "\\sqrt[" + b + "]{" + a + "}";
    if (o == "abs")
        return "\\left|" + a + "\\right|";
    if (o == "asin")
        o = "arcsin";
    if (o == "acos")
        o = "arccos";
    if (o == "atan")
        o = "arctan";
    return "\\" + o + "\\left(" + a + "\\right)";
}
static Expr add(Expr a, Expr b) {
    return make("+", a, b);
}
static Expr mul(Expr a, Expr b) {
    return make("*", a, b);
}
static Expr pow(Expr a, double b) {
    return make("^", a, number(b));
}
// Limit recursive symbolic work without making the GUI thread wait on algebra.
struct WorkGuard {
    int &depth;
    int &count;
    WorkGuard(int &d, int &c) : depth(d), count(c) {
        if (depth == 0)
            count = 0;
        if (++count > 30000)
            throw std::runtime_error("符号表达式过于复杂，请降低阶数或简化输入");
        ++depth;
    }
    ~WorkGuard() {
        --depth;
    }
};
Expr derivative(Expr n) {
    static thread_local int depth = 0, count = 0;
    WorkGuard guard(depth, count);
    auto o = n->op;
    auto a = n->a, b = n->b;
    if (o == "num" || o == "e" || o == "pi" || o == "param")
        return number(0);
    if (o == "x")
        return number(1);
    auto da = derivative(a);
    if (o == "+")
        return add(da, derivative(b));
    if (o == "-")
        return make("-", da, derivative(b));
    // Product and quotient rules retain the subtrees by shared ownership.
    if (o == "*")
        return add(mul(da, b), mul(a, derivative(b)));
    if (o == "/")
        return make("/", make("-", mul(da, b), mul(a, derivative(b))), pow(b, 2));
    if (o == "^") {
        if (a->op == "e")
            return mul(n, derivative(b));
        if (b->op == "num")
            return mul(mul(b, pow(a, b->value - 1)), da);
        if (a->op == "num")
            return mul(mul(n, number(std::log(a->value))), derivative(b));
        return mul(n, add(mul(derivative(b), make("ln", a)), make("/", mul(b, da), a)));
    }
    if (o == "root") {
        if (b->op == "num")
            return make("/", da, mul(b, make("^", n, number(b->value - 1))));
        return derivative(make("^", a, make("/", number(1), b)));
    }
    Expr outer;
    if (o == "sin")
        outer = make("cos", a);
    else if (o == "cos")
        outer = mul(number(-1), make("sin", a));
    else if (o == "tan")
        outer = make("/", number(1), pow(make("cos", a), 2));
    else if (o == "ln")
        outer = make("/", number(1), a);
    else if (o == "log")
        outer = make("/", number(1), mul(a, number(std::log(10.0))));
    else if (o == "exp")
        outer = n;
    else if (o == "asin" || o == "acos") {
        outer = make("/", number(o == "asin" ? 1 : -1),
                     make("root", make("-", number(1), pow(a, 2)), number(2)));
    } else if (o == "atan")
        outer = make("/", number(1), add(number(1), pow(a, 2)));
    else if (o == "abs")
        outer = make("/", a, make("abs", a));
    else
        throw std::runtime_error("暂不支持该函数求导");
    return mul(outer, da);
}
Expr integrate(Expr n) {
    auto o = n->op;
    auto a = n->a, b = n->b;
    auto x = variable();
    if (o == "num")
        return mul(n, x);
    if (o == "x")
        return make("/", pow(x, 2), number(2));
    if (o == "+" || o == "-")
        return make(o, integrate(a), integrate(b));
    if (o == "*" && a->op == "num")
        return mul(a, integrate(b));
    if (o == "*" && b->op == "num")
        return mul(b, integrate(a));
    if (o == "/" && b->op == "num")
        return make("/", integrate(a), b);
    if (o == "^" && a->op == "x" && b->op == "num") {
        if (b->value == -1)
            return make("ln", make("abs", x));
        return make("/", pow(x, b->value + 1), number(b->value + 1));
    }
    if (o == "/" && is(a, 1) && b->op == "x")
        return make("ln", make("abs", x));
    if (o == "/" && is(a, 1) && b->op == "+" && is(b->a, 1) && b->b->op == "^" &&
        b->b->a->op == "x" && is(b->b->b, 2))
        return make("atan", x);
    if (a && a->op == "x") {
        if (o == "sin")
            return mul(number(-1), make("cos", x));
        if (o == "cos")
            return make("sin", x);
        if (o == "exp")
            return n;
    }
    if (o == "^" && a->op == "num" && b->op == "x" && a->value > 0 && a->value != 1)
        return make("/", n, number(std::log(a->value)));
    if (o == "^" && a->op == "e" && b->op == "x")
        return n;
    throw std::runtime_error("暂不支持此表达式的符号积分；可选择数值积分");
}
Expr taylor(Expr f, double center, int order) {
    if (order < 0 || order > 8 || !std::isfinite(center))
        throw std::runtime_error("Taylor 阶数须在 0～8，展开点须有限");
    Expr sum = number(0);
    double factorial = 1;
    for (int k = 0; k <= order; ++k) {
        if (k)
            factorial *= k;
        double c = eval(f, center) / factorial;
        if (!std::isfinite(c))
            throw std::runtime_error("展开点存在未定义的导数");
        sum = add(sum, mul(number(c), pow(make("-", variable(), number(center)), k)));
        if (k < order)
            f = derivative(f);
    }
    return sum;
}
static double finite(Expr f, double x) {
    double y = eval(f, x);
    if (!std::isfinite(y))
        throw std::runtime_error("计算点不在定义域内或发生数值溢出");
    return y;
}
static void settings(double tol, int n) {
    if (!std::isfinite(tol) || tol <= 0 || n < 1 || n > 10000)
        throw std::runtime_error("精度须为正数，迭代次数须为 1～10000");
}
Result bisection(Expr f, double a, double b, double tol, int max) {
    // The intermediate value theorem needs continuity, not merely opposite signs.
    settings(tol, max);
    validateInterval(f, a, b);
    if (!std::isfinite(a) || !std::isfinite(b) || a >= b)
        throw std::runtime_error("二分区间须有限且左端小于右端");
    double fa = finite(f, a), fb = finite(f, b);
    Result r;
    if (fa == 0 || fb == 0) {
        r.value = fa == 0 ? a : b;
        r.converged = true;
        r.message = "端点为根";
        return r;
    }
    if (std::signbit(fa) == std::signbit(fb))
        throw std::runtime_error("区间两端函数值必须异号；还须由使用者确认区间连续");
    for (int i = 1; i <= max; ++i) {
        double x = a + (b - a) / 2, fx = finite(f, x), err = (b - a) / 2;
        r.steps.push_back({i, x, fx, err});
        r.value = x;
        if (fx == 0 || (err <= tol && std::abs(fx) <= tol)) {
            r.converged = true;
            r.message = "收敛（连续性为前提）";
            return r;
        }
        if (std::signbit(fx) == std::signbit(fa)) {
            a = x;
            fa = fx;
        } else
            b = x;
    }
    r.message = "达到最大迭代次数，未满足区间与残差双重条件";
    return r;
}
Result newton(Expr f, double x, double tol, int max) {
    // Intersection of the tangent at x with the x-axis: x_next = x - f(x)/f'(x).
    settings(tol, max);
    if (!std::isfinite(x))
        throw std::runtime_error("初值须有限");
    auto df = derivative(f);
    Result r;
    r.value = x;
    for (int i = 1; i <= max; ++i) {
        double fx = finite(f, x);
        if (i == 1 && std::abs(fx) <= tol) {
            r.value = x;
            r.converged = true;
            r.message = "残差达到精度";
            return r;
        }
        double d = finite(df, x);
        if (std::abs(d) < 1e-14) {
            r.message = "导数为零或过小，Newton 法停止";
            return r;
        }
        double next = x - fx / d;
        if (!std::isfinite(next)) {
            r.message = "迭代溢出";
            return r;
        }
        double fn = finite(f, next), err = std::abs(next - x);
        r.steps.push_back({i, next, fn, err});
        x = next;
        r.value = x;
        if (err <= tol && std::abs(fn) <= tol) {
            r.converged = true;
            r.message = "收敛";
            return r;
        }
    }
    r.message = "达到最大迭代次数，未收敛";
    return r;
}
double quadrature(Expr f, double a, double b, int n, const std::string &method) {
    // Composite Riemann midpoint, trapezoid, or Simpson (weights 1,4,2,...,4,1).
    if (n < 1 || n > 1000000 || !std::isfinite(a) || !std::isfinite(b))
        throw std::runtime_error("分割数须为 1～1000000，端点须有限");
    if (method != "rectangle" && method != "trapezoid" && method != "simpson")
        throw std::runtime_error("未知积分方法");
    if (method == "simpson" && n % 2)
        throw std::runtime_error("Simpson 法分割数必须为偶数");
    validateInterval(f, a, b);
    double h = (b - a) / n, s = 0;
    if (method == "rectangle") {
        for (int i = 0; i < n; ++i)
            s += finite(f, a + (i + .5) * h);
        return s * h;
    }
    s = finite(f, a) + finite(f, b);
    for (int i = 1; i < n; ++i)
        s += (method == "simpson" ? (i % 2 ? 4 : 2) : 2) * finite(f, a + i * h);
    return s * h / (method == "simpson" ? 3 : 2);
}
using Poly = std::vector<double>;
static Poly pmul(const Poly &a, const Poly &b) {
    if (a.size() + b.size() > 66)
        throw std::runtime_error("多项式次数过高");
    Poly c(a.size() + b.size() - 1);
    for (size_t i = 0; i < a.size(); ++i)
        for (size_t j = 0; j < b.size(); ++j)
            c[i + j] += a[i] * b[j];
    return c;
}
static Poly polynomial(Expr e) {
    if (e->op == "num")
        return {e->value};
    if (e->op == "x")
        return {0, 1};
    auto o = e->op;
    if (o == "+" || o == "-") {
        auto a = polynomial(e->a), b = polynomial(e->b);
        a.resize(std::max(a.size(), b.size()));
        for (size_t i = 0; i < b.size(); ++i)
            a[i] += (o == "+" ? 1 : -1) * b[i];
        return a;
    }
    if (o == "*")
        return pmul(polynomial(e->a), polynomial(e->b));
    if (o == "^" && e->b->op == "num" && e->b->value >= 0 && e->b->value <= 32 &&
        std::trunc(e->b->value) == e->b->value) {
        auto a = polynomial(e->a);
        Poly r{1};
        for (int k = 0; k < e->b->value; ++k)
            r = pmul(r, a);
        return r;
    }
    throw std::runtime_error("非多项式");
}
static std::string numeric(double v) {
    std::ostringstream s;
    s << std::setprecision(12) << v;
    return s.str();
}
static bool continuous(Expr e, double x) {
    auto o = e->op;
    if (o == "x" || o == "num" || o == "e" || o == "pi")
        return true;
    if (!continuous(e->a, x) || (e->b && !continuous(e->b, x)))
        return false;
    double a = eval(e->a, x), b = e->b ? eval(e->b, x) : 0;
    if (o == "/")
        return b != 0;
    if (o == "ln" || o == "log")
        return a > 0;
    if (o == "asin" || o == "acos")
        return std::abs(a) < 1;
    if (o == "tan")
        return std::abs(std::cos(a)) > 1e-12;
    if (o == "^")
        return a > 0 || (e->b->op == "num" && b >= 0 && b == std::trunc(b));
    if (o == "root")
        return a > 0 && b != 0;
    return std::isfinite(eval(e, x));
}
Limit limit(Expr f, double c, int direction) {
    if (direction < -1 || direction > 1 || std::isnan(c))
        throw std::runtime_error("非法极限参数");
    if (std::isfinite(c) && continuous(f, c))
        return {numeric(eval(f, c)) + "（由连续性）", true};
    if (f->op == "/" && f->a->op == "sin" && f->a->a->op == "x" && f->b->op == "x" && c == 0)
        return {"1（标准极限 sin(x)/x）", true};
    try {
        Poly a = polynomial(f->op == "/" ? f->a : f), b = f->op == "/" ? polynomial(f->b) : Poly{1};
        auto trim = [](Poly &p) {
            while (p.size() > 1 && p.back() == 0)
                p.pop_back();
        };
        trim(a);
        trim(b);
        if (b.size() == 1 && b[0] == 0)
            throw std::runtime_error("分母恒零");
        if (std::isinf(c)) {
            int degree = int(a.size()) - int(b.size());
            if (degree < 0)
                return {"0（有理函数次数比较）", true};
            double ratio = a.back() / b.back();
            if (degree == 0)
                return {numeric(ratio) + "（首项系数比）", true};
            if (c < 0 && degree % 2)
                ratio = -ratio;
            return {ratio > 0 ? "+∞（有理函数首项）" : "−∞（有理函数首项）", true};
        }
        // Shift polynomial into powers of (x-c), using binomial expansion.
        auto shift = [c](const Poly &p) {
            Poly q(p.size());
            for (size_t i = 0; i < p.size(); ++i) {
                double choose = 1;
                for (size_t j = 0; j <= i; ++j) {
                    q[j] += p[i] * choose * std::pow(c, int(i - j));
                    choose *= double(i - j) / double(j + 1);
                }
            }
            return q;
        };
        a = shift(a);
        b = shift(b);
        auto leading = [](const Poly &p) {
            size_t k = 0;
            while (k < p.size() && p[k] == 0)
                ++k;
            return k;
        };
        size_t ia = leading(a), ib = leading(b);
        if (ib == b.size())
            throw std::runtime_error("分母零");
        if (ia == a.size() || ia > ib)
            return {"0（多项式消去零因子）", true};
        double r = a[ia] / b[ib];
        if (ia == ib)
            return {numeric(r) + "（多项式消去零因子）", true};
        bool odd = (ib - ia) % 2;
        if (direction == 0 && odd)
            return {"不存在：左右分别趋于相反无穷", true};
        if (direction < 0 && odd)
            r = -r;
        return {r > 0 ? "+∞（有理函数零点阶数）" : "−∞（有理函数零点阶数）", true};
    } catch (const std::exception &) {
    }
    std::string s = "暂不支持精确判定。数值观察（不构成极限证明）：\n";
    for (int k = 2; k <= 6; ++k) {
        double h = std::pow(10., -k);
        double l = std::isfinite(c) ? c - h : -std::pow(10., k),
               r = std::isfinite(c) ? c + h : std::pow(10., k);
        if (std::isinf(c)) {
            s += "x=" + numeric(c > 0 ? r : l) + ": " + numeric(eval(f, c > 0 ? r : l)) + "\n";
        } else {
            s += "h=" + numeric(h);
            if (direction <= 0)
                s += " 左=" + numeric(eval(f, l));
            if (direction >= 0)
                s += " 右=" + numeric(eval(f, r));
            s += "\n";
        }
    }
    return {s, false};
}
} // namespace ms
