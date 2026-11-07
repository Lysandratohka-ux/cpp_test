#include "calculus.h"
#include "domain.h"
#include <algorithm>
#include <charconv>
#include <cmath>
#include <filesystem>
#include <functional>
#include <iomanip>
#include <iostream>
#include <libqalculate/Calculator.h>
#include <libqalculate/Function.h>
#include <libqalculate/MathStructure.h>
#include <libqalculate/Variable.h>
#include <mutex>
#include <numbers>
#include <numeric>
#include <sstream>
namespace ms {
namespace {
std::mutex casMutex;
Calculator &cas() {
    static Calculator c(true);
    static bool initialized = [] {
        if (!CALCULATOR->loadGlobalDefinitions()) {
            std::filesystem::path dir(MATHSOLVER_CAS_DEFINITIONS_DIR);
            for (auto file : {"prefixes.xml", "currencies.xml", "units.xml", "functions.xml",
                              "datasets.xml", "variables.xml"})
                CALCULATOR->loadDefinitions((dir / file).string().c_str(), false);
        }
        CALCULATOR->defaultAssumptions()->setType(ASSUMPTION_TYPE_REAL);
        for (auto name : {"a", "b", "c", "n"}) {
            auto v = new UnknownVariable("", std::string("infinity_parameter_") + name);
            auto assumptions = new Assumptions;
            assumptions->setType(ASSUMPTION_TYPE_REAL);
            v->setAssumptions(assumptions);
            CALCULATOR->addVariable(v);
        }
        return true;
    }();
    (void)initialized;
    return c;
}
struct Control {
    Control() {
        cas().clearMessages();
        cas().startControl(3500);
    }
    ~Control() {
        cas().stopControl();
        cas().clearMessages();
    }
};
EvaluationOptions options() {
    EvaluationOptions eo;
    eo.approximation = APPROXIMATION_EXACT;
    eo.allow_complex = false;
    eo.assume_denominators_nonzero = true;
    eo.warn_about_denominators_assumed_nonzero = false;
    eo.parse_options.angle_unit = ANGLE_UNIT_RADIANS;
    eo.do_polynomial_division = false;
    eo.expand = false;
    return eo;
}
PrintOptions printOptions() {
    PrintOptions po;
    po.use_unicode_signs = false;
    po.use_reference_names = true;
    po.number_fraction_format = FRACTION_FRACTIONAL;
    po.short_multiplication = false;
    po.multiplication_sign = MULTIPLICATION_SIGN_ASTERISK;
    po.min_exp = 0;
    return po;
}
std::string decimal(double v) {
    // Shortest round-trip decimal keeps a UI point such as 0.1 at 1/10,
    // rather than moving a pole to 0.10000000000000001 during CAS conversion.
    char buffer[128];
    auto result = std::to_chars(buffer, buffer + sizeof(buffer), v);
    if (result.ec != std::errc{})
        throw std::runtime_error("无法转换数值参数");
    return {buffer, result.ptr};
}
// Only our validated AST enters libqalculate, never the raw user input language.
std::string serialize(Expr e) {
    auto o = e->op;
    if (o == "param")
        return "infinity_parameter_" + e->literal;
    if (o == "num")
        return e->literal.empty() ? decimal(e->value) : e->literal;
    if (o == "x" || o == "e" || o == "pi")
        return o;
    auto a = serialize(e->a);
    auto b = e->b ? serialize(e->b) : "";
    if (o == "+" || o == "-" || o == "*" || o == "/" || o == "^")
        return "(" + a + o + b + ")";
    if (o == "root") {
        // Even roots are ordinary real powers on their original real domain.
        // Keep odd roots as real roots, including negative arguments.
        if (e->b->op == "num" && e->b->value > 0 && std::fmod(e->b->value, 2) == 0)
            return "(" + a + ")^(1/(" + b + "))";
        return "root(" + a + "," + b + ")";
    }
    if (o == "log")
        return "log(" + a + ",10)";
    if (o == "exp")
        return "(e^(" + a + "))";
    return o + "(" + a + ")";
}
std::string name(const MathStructure &m) {
    if (m.isVariable())
        return m.variable()->referenceName();
    if (m.isSymbolic())
        return m.symbol();
    return "";
}
std::string tex(const MathStructure &m, int parent = 0) {
    auto po = printOptions();
    // Qalculate keeps explicit radians in exact trig values such as tan(7/10 rad).
    // The app is exclusively in radians; this dimensionless unit has value one.
    if (m.isUnit() && m.unit() == cas().getRadUnit())
        return "1";
    if (m.isNumber()) {
        auto n = m.number();
        if (n.isPlusInfinity())
            return "+\\infty";
        if (n.isMinusInfinity())
            return "-\\infty";
        bool negative = n.isNegative();
        if (negative)
            n.negate();
        std::string s;
        if (n.isRational() && !n.denominatorEquals(1))
            s = "\\frac{" + n.numerator().print(po) + "}{" + n.denominator().print(po) + "}";
        else
            s = n.print(po);
        return (negative ? "-" : "") + s;
    }
    if (m.isVariable() || m.isSymbolic()) {
        auto s = name(m);
        if (s.starts_with("infinity_parameter_") && s.size() == 20 &&
            std::string("abcn").find(s.back()) != std::string::npos)
            return s.substr(19);
        if (s == "pi")
            return "\\pi";
        if (s == "e" || s == "x")
            return s;
        throw std::runtime_error("符号结果包含未支持的变量");
    }
    if (m.isAddition()) {
        std::string s;
        for (size_t i = 0; i < m.size(); ++i) {
            auto part = tex(m[i], 1);
            if (i && !part.starts_with('-'))
                s += '+';
            s += part;
        }
        return parent > 1 ? "\\left(" + s + "\\right)" : s;
    }
    if (m.isMultiplication()) {
        bool negative = false;
        std::vector<MathStructure> numerator, denominator;
        for (size_t i = 0; i < m.size(); ++i) {
            auto part = m[i];
            if (part.isNumber()) {
                if (part.number().isNegative()) {
                    negative = !negative;
                    part.negate();
                    part.eval(options());
                }
                if (part.isOne())
                    continue;
                if (part.number().isRational() && !part.number().denominatorEquals(1)) {
                    if (!part.number().numerator().isOne())
                        numerator.emplace_back(part.number().numerator());
                    denominator.emplace_back(part.number().denominator());
                    continue;
                }
            }
            if (part.isPower() && part[1].isNumber() && part[1].number().isNegative()) {
                auto power = part[1];
                power.negate();
                power.eval(options());
                if (power.isOne())
                    denominator.push_back(part[0]);
                else {
                    part[1] = power;
                    part.childUpdated(2);
                    denominator.push_back(part);
                }
                continue;
            }
            numerator.push_back(part);
        }
        auto join = [](const std::vector<MathStructure> &list) {
            std::string s;
            for (auto &factor : list) {
                if (!s.empty())
                    s += "\\,";
                s += tex(factor, 2);
            }
            return s.empty() ? std::string("1") : s;
        };
        auto result = denominator.empty()
                          ? join(numerator)
                          : "\\frac{" + join(numerator) + "}{" + join(denominator) + "}";
        return (negative ? "-" : "") + result;
    }
    if (m.isPower()) {
        auto exponent = m[1];
        if (exponent.isNumber() && exponent.number().isNegative()) {
            exponent.negate();
            exponent.eval(options());
            auto positive = m;
            positive[1] = exponent;
            positive.childUpdated(2);
            return "\\frac{1}{" + (exponent.isOne() ? tex(m[0]) : tex(positive)) + "}";
        }
        if (exponent.isNumber() && exponent.number().isRational() &&
            exponent.number().numeratorIsOne() && !exponent.number().denominatorEquals(1)) {
            auto index = exponent.number().denominator().print(po);
            return index == "2" ? "\\sqrt{" + tex(m[0]) + "}"
                                : "\\sqrt[" + index + "]{" + tex(m[0]) + "}";
        }
        return "{" + tex(m[0], 3) + "}^{" + tex(exponent) + "}";
    }
    if (m.isFunction()) {
        auto fn = m.function()->referenceName();
        if (fn == "abs")
            return "\\left|" + tex(m[0]) + "\\right|";
        if (fn == "sqrt")
            return "\\sqrt{" + tex(m[0]) + "}";
        if (fn == "root")
            return "\\sqrt[" + tex(m[1]) + "]{" + tex(m[0]) + "}";
        if (fn == "asin")
            fn = "arcsin";
        if (fn == "acos")
            fn = "arccos";
        if (fn == "atan")
            fn = "arctan";
        std::string prefix;
        if (fn == "sin" || fn == "cos" || fn == "tan" || fn == "sec" || fn == "csc" ||
            fn == "cot" || fn == "arcsin" || fn == "arccos" || fn == "arctan" || fn == "ln" ||
            fn == "log" || fn == "exp")
            prefix = "\\" + fn;
        else {
            for (char c : fn)
                if (!std::isalnum(static_cast<unsigned char>(c)))
                    throw std::runtime_error("无法排版此特殊函数");
            prefix = "\\operatorname{" + fn + "}";
        }
        std::string args;
        for (size_t i = 0; i < m.size(); ++i) {
            if (i)
                args += ",";
            args += tex(m[i]);
        }
        return prefix + "\\left(" + args + "\\right)";
    }
    throw std::runtime_error("符号结果类型暂不支持展示：" + std::to_string(int(m.type())));
}
MathStructure structure(Expr f) {
    auto eo = options();
    ParseOptions p;
    p.functions_enabled = true;
    p.units_enabled = false;
    p.unknowns_enabled = true;
    p.angle_unit = ANGLE_UNIT_RADIANS;
    auto m = cas().parse(serialize(f), p);
    m.eval(eo);
    return m;
}
MathStructure xvar() {
    return MathStructure(cas().getVariableById(VARIABLE_ID_X));
}
void scalarize(MathStructure &m) {
    m.replace(MathStructure(cas().getRadUnit()), MathStructure(1));
    m.eval(options());
}
bool unresolved(const MathStructure &m) {
    if (m.isUndefined() || m.isAborted())
        return true;
    if (m.isFunction()) {
        auto n = m.function()->referenceName();
        if (n == "integrate" || n == "diff" || n == "limit")
            return true;
    }
    for (size_t i = 0; i < m.size(); ++i)
        if (unresolved(m[i]))
            return true;
    return false;
}
// Real primitives use log|g| on each continuous interval. In particular,
// log(log x) alone loses the valid 0<x<1 branch of integral 1/(x log x).
void realPrimitiveLogs(MathStructure &m) {
    for (size_t i = 0; i < m.size(); ++i) {
        realPrimitiveLogs(m[i]);
        m.childUpdated(i + 1);
    }
    if (m.isFunction() && m.function()->referenceName() == "ln" && m.size() == 1 &&
        !m[0].representsPositive() &&
        !(m[0].isFunction() && m[0].function()->referenceName() == "abs")) {
        auto arg = m[0];
        arg.transform(cas().getFunction("abs"));
        m.setChild(arg, 1);
    }
}
// Reverse the chain rule structurally, rather than recognizing example strings.
// After dividing by u', the quotient must be expressible entirely in u.
bool substitutionPrimitive(const MathStructure &original, Expr f, MathStructure &result) {
    std::vector<Expr> candidates;
    std::function<void(Expr)> collect = [&](Expr e) {
        if (!e || candidates.size() >= 24)
            return;
        if (e->a && e->op != "+" && e->op != "-" && e->op != "*")
            candidates.push_back(e);
        collect(e->a);
        collect(e->b);
    };
    collect(f);
    auto x = xvar();
    auto eo = options();
    MathStructure u("mathsolver_substitution_u");
    for (auto candidate : candidates) {
        if (cas().aborted())
            break;
        auto inner = structure(candidate);
        if (!inner.contains(x) || inner == original)
            continue;
        auto slope = inner;
        if (!slope.differentiate(x, eo))
            continue;
        scalarize(slope);
        if (slope.isZero())
            continue;
        auto quotient = original;
        quotient /= slope;
        quotient.eval(eo);
        quotient.replace(inner, u);
        quotient.eval(eo);
        if (quotient.contains(x))
            continue;
        quotient.replace(u, x);
        if (quotient.integrate(x, eo, true, 1) <= 0 || unresolved(quotient))
            continue;
        quotient.replace(x, inner);
        result = quotient;
        return true;
    }
    return false;
}
// Rational substitutions for entire expression families, not individual exercises:
// t=exp(x), and the tangent half-angle substitution t=tan(x/2).
bool rationalSubstitution(Expr f, bool trigonometric, MathStructure &result) {
    bool valid = true, changed = false;
    auto t = variable();
    auto one = number(1);
    auto square = make("^", t, number(2));
    auto sine = make("/", make("*", number(2), t), make("+", one, square));
    auto cosine = make("/", make("-", one, square), make("+", one, square));
    auto multiple = [](Expr a) -> int {
        try {
            auto d = derivative(a);
            double k = eval(d, 0);
            if (d->op == "num" && eval(a, 0) == 0 && k == std::trunc(k) && std::abs(k) <= 8 &&
                k != 0)
                return int(k);
        } catch (...) {
        }
        return 0;
    };
    std::function<Expr(Expr)> transform = [&](Expr e) -> Expr {
        if (!valid)
            return number(0);
        if (e->op == "x") {
            valid = false; // Bare x prevents a rational function of the new variable.
            return number(0);
        }
        if (trigonometric && (e->op == "sin" || e->op == "cos" || e->op == "tan")) {
            int k = multiple(e->a);
            if (!k) {
                valid = false;
                return number(0);
            }
            changed = true;
            Expr s = number(0), c = one;
            for (int i = 0; i < std::abs(k); ++i) {
                auto nextS = make("+", make("*", s, cosine), make("*", c, sine));
                auto nextC = make("-", make("*", c, cosine), make("*", s, sine));
                s = nextS;
                c = nextC;
            }
            if (k < 0)
                s = make("*", number(-1), s);
            return e->op == "sin" ? s : e->op == "cos" ? c : make("/", s, c);
        }
        if (!trigonometric && ((e->op == "^" && e->a->op == "e") || e->op == "exp")) {
            int k = multiple(e->op == "exp" ? e->a : e->b);
            if (!k) {
                valid = false;
                return number(0);
            }
            changed = true;
            return make("^", t, number(k));
        }
        if (!e->a)
            return e;
        auto a = transform(e->a);
        auto b = e->b ? transform(e->b) : Expr{};
        return make(e->op, a, b);
    };
    auto rational = transform(f);
    if (!valid || !changed)
        return false;
    auto jacobian =
        trigonometric ? make("/", number(2), make("+", one, square)) : make("/", one, t);
    auto m = structure(make("*", rational, jacobian));
    auto eo = options();
    eo.expand = true;
    eo.do_polynomial_division = true;
    m.eval(eo);
    m.simplify(eo);
    m.expandPartialFractions(eo);
    if (m.integrate(xvar(), eo, true, 1) <= 0 || unresolved(m))
        return false;
    auto inner = structure(trigonometric ? parse(R"(\tan(x/2))") : parse("e^x"));
    m.replace(xvar(), inner);
    result = m;
    return true;
}
// Rationalize commensurable radicals of one affine expression u=a*x+b:
// t=u^(1/N), x=(t^N-b)/a. N is the LCM of the root degrees (bounded to 12).
// The transformed integrand must be rational; the shared derivative verifier
// checks the returned primitive on its real branches before it reaches the UI.
bool affineRadicalPrimitive(Expr f, MathStructure &result) {
    std::vector<Expr> bases;
    std::function<void(Expr)> collect = [&](Expr e) {
        if (!e || bases.size() >= 8)
            return;
        if (e->op == "root" && e->b->op == "num" && e->b->value >= 2 && e->b->value <= 12 &&
            std::trunc(e->b->value) == e->b->value)
            bases.push_back(e->a);
        collect(e->a);
        collect(e->b);
    };
    collect(f);
    auto eo = options();
    eo.expand = true;
    eo.do_polynomial_division = true;
    auto x = xvar();
    for (auto base : bases) {
        if (cas().aborted())
            break;
        auto u = structure(base), a = u;
        if (!a.differentiate(x, eo))
            continue;
        a.eval(eo);
        if (a.contains(x) || !a.representsNonZero())
            continue;
        auto b = u;
        b.replace(x, MathStructure(0));
        b.eval(eo);
        int degree = 1;
        std::function<void(Expr)> degrees = [&](Expr e) {
            if (!e)
                return;
            if (e->op == "root" && latex(e->a) == latex(base) && e->b->op == "num" &&
                e->b->value >= 2 && e->b->value <= 12 && std::trunc(e->b->value) == e->b->value &&
                degree <= 12)
                degree = std::lcm(degree, int(e->b->value));
            degrees(e->a);
            degrees(e->b);
        };
        degrees(f);
        if (degree < 2 || degree > 12)
            continue;
        auto inverse = x;
        inverse.raise(MathStructure(degree));
        inverse -= b;
        inverse /= a;
        bool valid = true;
        std::function<MathStructure(Expr)> transform = [&](Expr e) -> MathStructure {
            if (e->op == "x")
                return inverse;
            if (e->op == "root" && latex(e->a) == latex(base) && e->b->op == "num" &&
                e->b->value >= 2 && e->b->value <= 12 && std::trunc(e->b->value) == e->b->value &&
                degree % int(e->b->value) == 0) {
                auto r = x;
                r.raise(MathStructure(degree / int(e->b->value)));
                return r;
            }
            if (!e->a)
                return structure(e);
            if (e->op != "+" && e->op != "-" && e->op != "*" && e->op != "/" &&
                !(e->op == "^" && e->b->op == "num" && std::abs(e->b->value) <= 24 &&
                  std::trunc(e->b->value) == e->b->value)) {
                valid = false;
                return MathStructure(0);
            }
            auto r = transform(e->a), rhs = transform(e->b);
            if (e->op == "+")
                r += rhs;
            else if (e->op == "-")
                r -= rhs;
            else if (e->op == "*")
                r *= rhs;
            else if (e->op == "/")
                r /= rhs;
            else
                r.raise(rhs);
            return r;
        };
        auto m = transform(f), jacobian = inverse;
        if (!valid || !jacobian.differentiate(x, eo))
            continue;
        m *= jacobian;
        m.eval(eo);
        m.expandPartialFractions(eo);
        if (m.integrate(x, eo, true, 1) <= 0 || unresolved(m))
            continue;
        auto root = structure(make("root", base, number(degree)));
        m.replace(x, root);
        result = m;
        return true;
    }
    return false;
}
// Reduction formula for polynomial / sqrt(A+B*x^2), obtained by differentiating
// x^(n-1)*sqrt(A+B*x^2). All coefficients remain exact CAS values.
bool quadraticRadicalPrimitive(const MathStructure &original, Expr f, MathStructure &result) {
    std::vector<Expr> roots;
    std::function<void(Expr)> collect = [&](Expr e) {
        if (!e || roots.size() >= 8)
            return;
        if (e->op == "root" && e->b->op == "num" && e->b->value == 2)
            roots.push_back(e);
        collect(e->a);
        collect(e->b);
    };
    collect(f);
    auto x = xvar();
    auto eo = options();
    eo.expand = true;
    for (auto root : roots) {
        auto q = structure(root->a);
        q.eval(eo);
        if (!q.isRationalPolynomial() || !q.degree(x).equals(2))
            continue;
        MathStructure a, b, linear;
        q.coefficient(x, Number(0), a);
        q.coefficient(x, Number(2), b);
        q.coefficient(x, Number(1), linear);
        if (!linear.isZero() || a.contains(x) || b.contains(x) || b.isZero())
            continue;
        auto radical = structure(root);
        auto p = original;
        p *= radical;
        p.eval(eo);
        if (!p.isRationalPolynomial() || !p.degree(x).isInteger())
            continue;
        int degree = p.degree(x).intValue();
        if (degree < 0 || degree > 12)
            continue;
        auto half = cas().parse("1/2");
        MathStructure base, rootB = b;
        if (a.representsPositive() && b.representsNegative()) {
            rootB.negate();
            rootB.raise(half);
            auto rootA = a;
            rootA.raise(half);
            base = x;
            base *= rootB;
            base /= rootA;
            base.transform(cas().getFunction("asin"));
            base /= rootB;
        } else if (b.representsPositive() && a.representsNonZero()) {
            rootB.raise(half);
            base = x;
            base *= rootB;
            base += radical;
            base.transform(cas().getFunction("abs"));
            base.transform(cas().getFunction("ln"));
            base /= rootB;
        } else
            continue;
        std::vector<MathStructure> primitives{base};
        auto first = radical;
        first /= b;
        primitives.push_back(first);
        for (int n = 2; n <= degree; ++n) {
            auto term = x;
            term.raise(MathStructure(n - 1));
            term *= radical;
            auto lower = primitives[n - 2];
            lower *= a;
            lower *= MathStructure(n - 1);
            term -= lower;
            term /= b;
            term /= MathStructure(n);
            term.eval(eo);
            primitives.push_back(term);
        }
        result = MathStructure(0);
        for (int n = 0; n <= degree; ++n) {
            MathStructure coefficient;
            p.coefficient(x, Number(n), coefficient);
            auto term = primitives[n];
            term *= coefficient;
            result += term;
        }
        return true;
    }
    return false;
}
CalculusResult output(const MathStructure &m) {
    if (cas().aborted())
        throw std::runtime_error("符号运算超过时间预算，请简化表达式");
    if (unresolved(m))
        return {ResultKind::Unknown, "\\text{当前算法无法求出闭式结果}", "当前算法无法求出闭式结果",
                "未求解不代表原函数或极限不存在。"};
    CalculusResult r;
    auto scalar = m;
    scalarize(scalar);
    r.kind = scalar.isApproximate() ? ResultKind::Approximate : ResultKind::Exact;
    r.formula = tex(scalar);
    r.plain = scalar.print(printOptions());
    for (auto p : {"a", "b", "c", "n"}) {
        std::string from = std::string("infinity_parameter_") + p;
        for (size_t pos = 0; (pos = r.plain.find(from, pos)) != std::string::npos; ++pos)
            r.plain.replace(pos, from.size(), p);
    }
    if (scalar.isNumber()) {
        if (scalar.number().isPlusInfinity())
            r.kind = ResultKind::PositiveInfinity;
        else if (scalar.number().isMinusInfinity())
            r.kind = ResultKind::NegativeInfinity;
        else if (!scalar.number().isComplex())
            r.numeric = scalar.number().floatValue();
    } else if (!scalar.contains(xvar())) {
        auto approximate = scalar;
        auto eo = options();
        eo.approximation = APPROXIMATION_APPROXIMATE;
        approximate.eval(eo);
        if (approximate.isNumber() && approximate.number().isReal() &&
            std::isfinite(approximate.number().floatValue()))
            r.numeric = approximate.number().floatValue();
    }
    return r;
}
std::optional<double> evaluate(const MathStructure &m, double x) {
    auto v = m;
    v.replace(xvar(), MathStructure(x));
    auto eo = options();
    eo.approximation = APPROXIMATION_APPROXIMATE;
    v.eval(eo);
    scalarize(v);
    v.eval(eo);
    if (v.isNumber() && v.number().isReal() && std::isfinite(v.number().floatValue()))
        return v.number().floatValue();
    return {};
}
// A structural rule, not a sampled guess: affine reciprocal phase sweeps infinitely
// many periods near its pole. The rule applies to every nonzero coefficient.
bool reciprocalPhase(Expr e, double center) {
    try {
        auto phase = e;
        if (phase->op == "/" && phase->a->op == "num" && phase->a->value != 0) {
            auto d = derivative(phase->b);
            if (d->op == "num" && d->value != 0 && eval(phase->b, center) == 0)
                return true;
        }
    } catch (...) {
    }
    return false;
}
bool oscillatory(Expr f, double c) {
    return (f->op == "sin" || f->op == "cos") && reciprocalPhase(f->a, c);
}
} // namespace
std::string symbolicBackend() {
    return "libqalculate 5.13.1 · C++";
}
CalculusResult solveDerivative(Expr f, int order, std::optional<double> point) {
    if (!parameters(f).empty())
        throw std::runtime_error("带参数输入目前用于积分和极限；求导请先将参数换成具体数值");
    if (order < 1 || order > 8)
        throw std::runtime_error("求导阶数须为 1～8");
    if (point && (!std::isfinite(*point) || !std::isfinite(eval(f, *point))))
        throw std::runtime_error("指定点不在原函数的实数定义域内");
    if (point)
        validateInterval(f, *point, *point);
    std::lock_guard lock(casMutex);
    Control control;
    auto eo = options();
    auto m = structure(f);
    for (int i = 0; i < order; ++i) {
        if (!m.differentiate(xvar(), eo))
            throw std::runtime_error("当前算法无法完成符号求导");
        m.eval(eo);
        scalarize(m);
    }
    auto r = output(m);
    r.detail = "符号法则与代数化简；结果按原函数的实数定义域解释。";
    auto local = f;
    for (int i = 0; i < order; ++i)
        local = derivative(local);
    for (double sample : point ? std::vector<double>{*point} : std::vector<double>{-.6, .3, 1.2}) {
        if (!std::isfinite(eval(f, sample)))
            continue;
        double expected = eval(local, sample);
        auto actual = evaluate(m, sample);
        if (actual && std::isfinite(expected) &&
            std::abs(*actual - expected) > 1e-8 * (1 + std::abs(expected)))
            throw std::runtime_error("符号导数与独立表达式树验证不一致，已拒绝结果");
    }
    if (point) {
        auto d = f;
        for (int i = 0; i < order; ++i)
            d = derivative(d);
        double value = eval(d, *point);
        if (!std::isfinite(value))
            throw std::runtime_error("该点的导数不存在或当前表达式无法在此点求值");
        r.numeric = value;
        r.detail += " 指定点 x=" + decimal(*point) + "。";
    }
    try {
        auto d = f;
        for (int i = 0; i < order; ++i)
            d = derivative(d);
        r.expression = d;
    } catch (...) {
    }
    return r;
}
#include "parameter_calculus.inc"
CalculusResult solveIntegral(Expr f, std::optional<std::pair<double, double>> interval) {
    if (!parameters(f).empty())
        return parameterIntegral(f, interval);
    if (interval)
        validateInterval(f, interval->first, interval->second);
    std::lock_guard lock(casMutex);
    Control control;
    auto eo = options();
    eo.expand = true;
    auto original = structure(f), m = original;
    auto x = xvar();
    if (interval) {
        bool ok = m.integrate(cas().parse(decimal(interval->first)),
                              cas().parse(decimal(interval->second)), x, eo);
        auto r = ok ? output(m) : CalculusResult{};
        if (r.kind != ResultKind::Unknown) {
            r.detail = "定积分：符号积分与端点代入。定义域已通过保守区间检查。";
            return r;
        }
        double value = quadrature(f, interval->first, interval->second, 4096, "simpson");
        return {ResultKind::Approximate, decimal(value), decimal(value),
                "未求出精确结果；4096 等分复合 Simpson 数值近似，未提供严格误差界。", value};
    }
    int status = m.integrate(x, eo, true, 1);
    if (status <= 0 || unresolved(m)) {
        // Preserve nested powers such as (1+x^2)^(-3/2) on a second attempt.
        m = original;
        auto compact = options();
        status = m.integrate(x, compact, false, 1);
    }
    if ((status <= 0 || unresolved(m)) && !substitutionPrimitive(original, f, m) &&
        !quadraticRadicalPrimitive(original, f, m) && !rationalSubstitution(f, false, m) &&
        !rationalSubstitution(f, true, m) && !affineRadicalPrimitive(f, m))
        return {ResultKind::Unknown, "\\text{当前算法无法求出原函数}", "当前算法无法求出原函数",
                "不代表积分不存在；可改为正常定积分求数值近似。"};
    m.eval(eo);
    realPrimitiveLogs(m);
    auto r = output(m);
    if (r.kind == ResultKind::Unknown)
        return r;
    auto delta = m;
    if (delta.differentiate(x, eo)) {
        delta -= original;
        delta.eval(eo);
        if (delta.isZero())
            r.verification = Verification::Symbolic;
        else {
            int valid = 0;
            bool pass = true;
            for (double point : {-2.3, -.7, -.3, -.1, .1, .2, .4, .6, .8, 1.7, 3.1}) {
                double expected = eval(f, point);
                auto actual = evaluate(delta, point);
                auto primitiveValue = evaluate(m, point);
                if (std::isfinite(expected) && actual && primitiveValue) {
                    ++valid;
                    if (std::abs(*actual) > 1e-8 * (1 + std::abs(expected)))
                        pass = false;
                }
            }
            if (!pass)
                throw std::runtime_error("原函数的求导验证失败，已拒绝返回该结果");
            r.verification = valid >= 3 ? Verification::SamplesOnly : Verification::Unverified;
        }
    }
    if (r.verification == Verification::NotApplicable || r.verification == Verification::Unverified)
        return {ResultKind::Unknown, "\\text{原函数未通过实数验证}", "原函数未通过实数验证",
                "候选结果未能完成符号验证或足够的实数点求导检查，已拒绝作为精确原函数返回。"};
    r.formula += "+C";
    r.plain += " + C";
    r.detail = r.verification == Verification::Symbolic
                   ? "原函数已通过符号求导恒等验证。"
                   : "原函数仅通过可计算点的求导抽查，未完成全定义域恒等证明。";
    r.detail += " 在被积函数与结果表达式共同有定义的连续区间内使用；常数 C "
                "可随区间不同。特殊函数名称按标准定义。";
    return r;
}
namespace {
// Certify local real analyticity from syntax and domain conditions, before using
// Taylor leading terms. The numeric values below check open-domain membership,
// never infer convergence from sampled function values.
bool analytic(Expr f, double c) {
    auto o = f->op;
    if (o == "num" || o == "x" || o == "e" || o == "pi")
        return true;
    if (!analytic(f->a, c) || (f->b && !analytic(f->b, c)))
        return false;
    double a = eval(f->a, c), b = f->b ? eval(f->b, c) : 0;
    if (!std::isfinite(a) || !std::isfinite(b))
        return false;
    if (o == "/")
        return b != 0;
    if (o == "ln" || o == "log")
        return a > 0;
    if (o == "tan")
        return std::abs(std::cos(a)) > 1e-12;
    if (o == "asin" || o == "acos")
        return std::abs(a) < 1;
    if (o == "abs")
        return a != 0;
    if (o == "root")
        return a > 0 && b != 0;
    if (o == "^")
        return a > 0 || (f->b->op == "num" && b >= 0 && std::trunc(b) == b);
    return true;
}
struct Leading {
    int order = -1;
    MathStructure coefficient;
    bool allZero = true;
};
Leading leading(Expr f, double c, int maximum) {
    auto m = structure(f);
    auto eo = options();
    auto center = cas().parse(decimal(c));
    long factorial = 1;
    for (int k = 0; k <= maximum; ++k) {
        auto at = m;
        at.replace(xvar(), center);
        scalarize(at);
        if (!at.isZero()) {
            if (at.isApproximate() || !at.representsNonZero())
                return {-1, {}, false};
            at /= MathStructure(int(factorial));
            at.eval(eo);
            return {k, at, false};
        }
        if (k < maximum) {
            if (!m.differentiate(xvar(), eo))
                return {-1, {}, false};
            scalarize(m);
            factorial *= k + 1;
        }
    }
    return {};
}
std::optional<CalculusResult> seriesLimit(Expr f, double c, int side) {
    if (f->op != "/" || !analytic(f->a, c) || !analytic(f->b, c))
        return {};
    auto denominator = leading(f->b, c, 12);
    if (denominator.order < 0)
        return {};
    auto numerator = leading(f->a, c, denominator.order);
    if (numerator.order < 0) {
        if (numerator.allZero)
            return CalculusResult{ResultKind::Exact, "0", "0",
                                  "局部实解析展开：分子消失阶数高于分母，故商趋于零。", 0.};
        return {};
    }
    if (numerator.order > denominator.order)
        return CalculusResult{ResultKind::Exact, "0", "0", "局部实解析展开与无穷小阶数比较。", 0.};
    auto ratio = numerator.coefficient;
    ratio /= denominator.coefficient;
    ratio.eval(options());
    if (numerator.order == denominator.order) {
        auto r = output(ratio);
        r.detail = "分子、分母在趋近点邻域实解析；比较首个非零 Taylor 系数（最高检查 12 阶）。";
        return r;
    }
    bool positive = ratio.representsPositive(), negative = ratio.representsNegative();
    if (!positive && !negative)
        return {};
    if (side < 0 && (denominator.order - numerator.order) % 2)
        positive = !positive;
    return CalculusResult{positive ? ResultKind::PositiveInfinity : ResultKind::NegativeInfinity,
                          positive ? "+\\infty" : "-\\infty", positive ? "+∞" : "−∞",
                          "局部实解析展开：比较零点阶数，并按单侧方向判断符号。"};
}
// Put arithmetic combinations of analytic germs over a common denominator.
// Every original divisor is certified nonzero in a punctured neighborhood;
// clearing fractions must never turn an everywhere-undefined input into a limit.
std::optional<CalculusResult> normalizedSeriesLimit(Expr f, double center, int side) {
    struct Unsupported {};
    struct Fraction {
        Expr n, d;
        size_t ns = 1, ds = 1;
    };
    std::vector<Expr> divisors;
    int visited = 0;
    auto node = [](const std::string &op, Expr a, Expr b) {
        return std::make_shared<Node>(Node{op, 0, a, b});
    };
    std::function<Fraction(Expr)> normalize = [&](Expr e) -> Fraction {
        if (++visited > 1024)
            throw Unsupported{};
        auto op = e->op;
        if (op != "+" && op != "-" && op != "*" && op != "/") {
            if (!analytic(e, center))
                throw Unsupported{};
            return {e, number(1)};
        }
        auto a = normalize(e->a), b = normalize(e->b);
        Fraction r;
        if (op == "+" || op == "-") {
            r = {node(op, node("*", a.n, b.d), node("*", b.n, a.d)), node("*", a.d, b.d),
                 a.ns + b.ds + b.ns + a.ds + 3, a.ds + b.ds + 1};
        } else if (op == "*") {
            r = {node("*", a.n, b.n), node("*", a.d, b.d), a.ns + b.ns + 1, a.ds + b.ds + 1};
        } else {
            divisors.push_back(b.n);
            r = {node("*", a.n, b.d), node("*", a.d, b.n), a.ns + b.ds + 1, a.ds + b.ns + 1};
        }
        if (r.ns + r.ds > 12000)
            throw Unsupported{};
        return r;
    };
    try {
        auto fraction = normalize(f);
        if (divisors.empty())
            return {};
        for (auto d : divisors)
            if (leading(d, center, 12).order < 0)
                return {};
        auto r = seriesLimit(node("/", fraction.n, fraction.d), center, side);
        if (r)
            r->detail = "通分并确认原各分母在去心邻域非零；" + r->detail;
        return r;
    } catch (const Unsupported &) {
        return {};
    }
}
} // namespace
CalculusResult simplifyPolynomial(Expr f) {
    int visited = 0;
    std::function<int(Expr)> degree = [&](Expr e) -> int {
        if (++visited > 2048)
            throw std::runtime_error("多项式过于复杂，请减少项数");
        if (e->op == "num" || e->op == "e" || e->op == "pi")
            return 0;
        if (e->op == "x")
            return 1;
        if (e->op != "+" && e->op != "-" && e->op != "*" && e->op != "/" && e->op != "^")
            throw std::runtime_error("多项式化简仅支持 x 的非负整数次幂及常数系数");
        int a = degree(e->a), b = degree(e->b), d = 0;
        if (e->op == "+" || e->op == "-")
            d = std::max(a, b);
        else if (e->op == "*")
            d = a + b;
        else if (e->op == "/") {
            if (b != 0 || !std::isfinite(eval(e->b, 0)) || eval(e->b, 0) == 0)
                throw std::runtime_error("多项式的分母必须为非零常数；含变量分母的有理式暂不支持");
            d = a;
        } else {
            double n = eval(e->b, 0);
            if (b != 0 || !std::isfinite(n) || n < 0 || n > 64 || std::trunc(n) != n)
                throw std::runtime_error("多项式指数须为 0～64 的整数");
            d = a * int(n);
        }
        if (d > 64)
            throw std::runtime_error("目前支持最高 64 次多项式");
        return d;
    };
    degree(f);
    std::lock_guard lock(casMutex);
    Control control;
    auto eo = options();
    eo.expand = true;
    auto original = structure(f), m = original;
    m.eval(eo);
    if (m.contains(xvar()) && !m.isRationalPolynomial(true))
        throw std::runtime_error("精确表达式不是多项式，请检查指数与系数");
    auto delta = m;
    delta -= original;
    delta.eval(eo);
    if (!delta.isZero())
        return {ResultKind::Unknown, "\\text{未完成恒等验证}", "未完成恒等验证",
                "不返回未经验证的化简结果。"};
    auto r = output(m);
    r.verification = Verification::Symbolic;
    r.detail = "展开括号并合并同类项，保留精确分数；结果与原式的差已符号化简为零。最高支持 64 次。";
    return r;
}
CalculusResult solveLimit(Expr f, double center, int direction) {
    if (std::isnan(center) || direction < -1 || direction > 1)
        throw std::runtime_error("非法极限参数");
    if (!parameters(f).empty())
        return parameterLimit(f, center, direction);
    if (std::isfinite(center) && direction == 0) {
        auto left = solveLimit(f, center, -1), right = solveLimit(f, center, 1);
        if (left.kind == ResultKind::DoesNotExist || right.kind == ResultKind::DoesNotExist)
            return {ResultKind::DoesNotExist, "\\nexists", "不存在", "至少一侧有不同子序列极限。"};
        if (left.kind == ResultKind::Unknown || right.kind == ResultKind::Unknown)
            return {ResultKind::Unknown, "\\text{当前算法无法判定}", "当前算法无法判定",
                    "无法同时确认左右极限；没有以数值采样替代证明。"};
        if (left.kind == right.kind && left.formula == right.formula) {
            left.detail += " 左右极限一致。";
            return left;
        }
        return {ResultKind::DoesNotExist, "\\nexists", "不存在",
                "左右极限不同：左侧 " + left.plain + "；右侧 " + right.plain + "。"};
    }
    if (std::isfinite(center) && oscillatory(f, center))
        return {ResultKind::DoesNotExist, "\\nexists", "不存在",
                "倒数型相位在趋近点附近遍历无穷多个周期；不同子序列上函数值不同。"};
    if (f->op == "*") {
        Expr amplitude, phase;
        if (f->a->op == "sin" || f->a->op == "cos")
            amplitude = f->b, phase = f->a->a;
        else if (f->b->op == "sin" || f->b->op == "cos")
            amplitude = f->a, phase = f->b->a;
        if (amplitude) {
            auto amp = solveLimit(amplitude, center, direction);
            // A bounded sine does not establish that its phase is real-defined.
            std::function<bool(Expr)> phaseDefined = [&](Expr p) {
                if (p->op == "sin" || p->op == "cos" || p->op == "atan")
                    return phaseDefined(p->a);
                if (p->op == "+" || p->op == "-" || p->op == "*")
                    return phaseDefined(p->a) && phaseDefined(p->b);
                auto r = solveLimit(p, center, direction);
                return r.kind == ResultKind::Exact || r.kind == ResultKind::PositiveInfinity ||
                       r.kind == ResultKind::NegativeInfinity;
            };
            if (amp.kind == ResultKind::Exact && amp.numeric && *amp.numeric == 0 &&
                phaseDefined(phase))
                return {ResultKind::Exact, "0", "0", "有界三角因子与趋零因子相乘，依据夹逼定理。",
                        0.};
        }
    }
    // A positive analytic base permits log(f^g)=g*log(f) on a full
    // neighborhood. Reduce 1^infinity (and related positive-base forms)
    // to the existing certified analytic quotient rules, then use continuity
    // of exp on the extended real line. Never use this for a negative base.
    if (f->op == "^" && std::isfinite(center)) {
        auto baseLimit = solveLimit(f->a, center, direction);
        bool tendsToOne = baseLimit.kind == ResultKind::Exact && baseLimit.formula == "1";
        bool positiveAnalytic = analytic(f->a, center) && eval(f->a, center) > 0;
        if (tendsToOne || positiveAnalytic) {
            // log(1+u)/u -> 1 also covers a removable singularity in the base;
            // its real-domain proof comes from the independently certified base limit.
            // Preserve the exponent's domain even when the base is identically 1.
            // make("*", ..., 0) would erase an undefined exponent prematurely.
            auto logarithm = std::make_shared<Node>(
                Node{"*", 0, f->b, tendsToOne ? make("-", f->a, number(1)) : make("ln", f->a)});
            auto exponent = solveLimit(logarithm, center, direction);
            if (exponent.kind == ResultKind::NegativeInfinity)
                return {ResultKind::Exact, "0", "0",
                        "正底数取对数，指数趋于负无穷，由指数函数极限得零。", 0.};
            if (exponent.kind == ResultKind::PositiveInfinity)
                return {ResultKind::PositiveInfinity, "+\\infty", "+∞",
                        "正底数取对数后指数趋于正无穷。"};
            if (exponent.kind == ResultKind::Exact) {
                std::lock_guard lock(casMutex);
                Control control;
                auto m = cas().parse(exponent.plain);
                m.transform(cas().getFunction("exp"));
                m.eval(options());
                auto r = output(m);
                r.detail =
                    tendsToOne
                        ? "已证明底数趋于 1，利用 ln(1+u) 与 u 等价，再由指数函数连续性还原。 " +
                              exponent.detail
                        : "底数在趋近点邻域为正的实解析函数；取对数求极限，再由 exp 连续性还原。 " +
                              exponent.detail;
                return r;
            }
            return {ResultKind::Unknown, "\\text{当前算法无法判定}", "当前算法无法判定",
                    "取对数后的极限或原指数的实数定义域尚未确认；不通过幂化简抹去定义域限制。"};
        }
    }
    std::lock_guard lock(casMutex);
    Control control;
    auto eo = options();
    if (std::isfinite(center)) {
        if (auto result = seriesLimit(f, center, direction))
            return *result;
        if (auto result = normalizedSeriesLimit(f, center, direction))
            return *result;
        if (analytic(f, center)) {
            auto m = structure(f);
            m.replace(xvar(), cas().parse(decimal(center)));
            scalarize(m);
            auto r = output(m);
            r.detail = "趋近点邻域内为实解析函数，由连续性代入求值。";
            return r;
        }
    }
    auto base = structure(f);
    if (std::isfinite(center)) {
        bool evaluable = false;
        for (double h : {.01, .001, .0001})
            if (std::isfinite(eval(f, center + direction * h * (1 + std::abs(center)))))
                evaluable = true;
        // This check can only reject a result as Unknown. It never proves a limit.
        // In particular a CAS simplification must not erase an absent real domain.
        if (!evaluable)
            return {ResultKind::Unknown, "\\text{当前算法无法判定}", "当前算法无法判定",
                    "该侧没有找到可求值的实数定义域，或发生溢出；不返回未经定义域确认的极限。"};
    }
    MathStructure target;
    if (std::isinf(center)) {
        Number n;
        if (center > 0)
            n.setPlusInfinity();
        else
            n.setMinusInfinity();
        target.set(n);
    } else
        target = cas().parse(decimal(center));
    auto m = base;
    if (m.calculateLimit(xvar(), target, eo, direction)) {
        auto r = output(m);
        if (r.kind != ResultKind::Approximate && r.kind != ResultKind::Unknown) {
            r.detail = "libqalculate 符号极限规则；没有使用数值采样作存在性判定。";
            return r;
        }
    }
    return {ResultKind::Unknown, "\\text{当前算法无法判定}", "当前算法无法判定",
            "符号规则未能确认结果；不代表极限不存在。"};
}
} // namespace ms
