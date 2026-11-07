#include "calculus.h"
#include <cmath>
#include <functional>
#include <iostream>
using namespace ms;
int checks = 0, cases = 0;
void check(bool ok, const std::string &what) {
    if (!ok)
        throw std::runtime_error(what);
    ++checks;
}
void near(double a, double b, const std::string &s) {
    check(std::isfinite(a) && std::abs(a - b) < 3e-7 * (1 + std::abs(b)), s);
}
void rejects(std::function<void()> action, const std::string &what) {
    bool fail = false;
    try {
        action();
    } catch (const std::exception &) {
        fail = true;
    }
    check(fail, what);
    ++cases;
}
std::string expression(const CalculusResult &r, int branch = 0) {
    std::string s = r.formula;
    auto pos = s.find("\\begin{gathered}");
    if (pos != std::string::npos)
        s.erase(pos, 16);
    pos = s.find("\\\\[4pt]");
    if (pos != std::string::npos)
        s.resize(pos);
    pos = s.find("\\begin{cases}");
    if (pos != std::string::npos) {
        s.erase(pos, 13);
        for (int i = 0; i < branch; ++i) {
            pos = s.find("\\\\");
            check(pos != std::string::npos, "missing branch");
            s.erase(0, pos + 2);
        }
        pos = s.find('&');
        check(pos != std::string::npos, "missing case predicate");
        s.resize(pos);
    }
    pos = s.find("+C");
    if (pos != std::string::npos)
        s.resize(pos);
    return s;
}
int main(int argc, char **argv) {
    std::string category = argc > 1 ? argv[1] : "";
    try {
        if (category == "parser") {
            for (auto s : {"ax+b", "abcx", R"(\sin(ax))", R"(\sin ax)", "sin(2ax)", "x^n",
                           "a/(b+c)", "a*x^2+b*x+c"}) {
                auto f = parse(s);
                check(!parameters(f).empty(), s);
                auto bound = bindParameters(f, {{"a", 2}, {"b", 3}, {"c", 4}, {"n", -.5}});
                near(eval(bound, .7),
                     eval(bindParameters(parse(latex(f)),
                                         {{"a", 2}, {"b", 3}, {"c", 4}, {"n", -.5}}),
                          .7),
                     s);
                ++cases;
            }
            near(eval(bindParameters(parse(R"(\sin ax)"), {{"a", 2}}), .7), std::sin(1.4),
                 "unbraced argument");
            rejects([] { parse("\\input{a}"); }, "command whitelist");
            rejects([] { parse("\\a"); }, "parameters are not latex commands");
            rejects([] { parse("y+x"); }, "parameter whitelist");
            rejects([] { eval(parse("a+x"), 1); }, "unbound evaluation");
            rejects([] { bindParameters(parse("a+b"), {{"a", 1}}); }, "partial binding");
        } else if (category == "integral") {
            for (auto s : {"a*x^2+b*x+c", R"(\sin(ax))", R"(\cos(ax))", "e^{ax}", "x*e^{a*x^2}",
                           "1/(a*x+b)"}) {
                auto f = parse(s);
                auto r = solveIntegral(f);
                check(r.kind == ResultKind::Exact || r.kind == ResultKind::Conditional, s);
                check(r.verification == Verification::Symbolic, s);
                bool split = r.formula.find("\\begin{cases}") != std::string::npos;
                for (double a : {-.8, 1.3}) {
                    auto values = std::map<std::string, double>{{"a", a}, {"b", 2.1}, {"c", -.4}};
                    auto p = bindParameters(parse(expression(r)), values);
                    auto d = derivative(p), bound = bindParameters(f, values);
                    for (double x : {.37, .91, 1.17})
                        near(eval(d, x), eval(bound, x), s);
                }
                if (split) {
                    auto p = bindParameters(parse(expression(r, 1)), {{"a", 0}});
                    near(eval(derivative(p), .73),
                         eval(bindParameters(f, {{"a", 0}, {"b", 2.1}, {"c", -.4}}), .73), s);
                }
                ++cases;
            }
            for (auto s : {"x^a", "a*x^n", "x^{a+b}"}) {
                auto f = parse(s);
                auto r = solveIntegral(f);
                check(r.kind == ResultKind::Conditional &&
                          r.formula.find("=-1") != std::string::npos,
                      s);
                for (double exponent : {-1., 0., .4, -2.3}) {
                    std::map<std::string, double> values;
                    if (std::string(s) == "a*x^n")
                        values = {{"a", 1.7}, {"n", exponent}};
                    else if (std::string(s) == "x^{a+b}")
                        values = {{"a", exponent - .2}, {"b", .2}};
                    else
                        values = {{"a", exponent}};
                    auto p = bindParameters(parse(expression(r, exponent == -1 ? 1 : 0)), values);
                    for (double x : {.37, 1.17})
                        near(eval(derivative(p), x), eval(bindParameters(f, values), x), s);
                }
                ++cases;
            }
            auto r = solveIntegral(parse("x^{a-a-1}"));
            check(r.formula.find("frac{1}{0}") == std::string::npos,
                  "collapsed exceptional exponent");
            near(eval(derivative(bindParameters(parse(expression(r)), {{"a", 7}})), .7), 1 / .7,
                 "collapsed exponent derivative");
            ++cases;
            for (auto s : {"e^{ax}", R"(\sin(ax))"}) {
                r = solveIntegral(parse(s), {{0, 1}});
                check(r.kind == ResultKind::Conditional, s);
                for (double a : {0., -.8, 1.3}) {
                    auto value =
                        eval(bindParameters(parse(expression(r, a == 0 ? 1 : 0)), {{"a", a}}), 0);
                    double expected = std::string(s) == "e^{ax}"
                                          ? (a == 0 ? 1 : std::expm1(a) / a)
                                          : (a == 0 ? 0 : (1 - std::cos(a)) / a);
                    near(value, expected, s);
                }
                ++cases;
            }
            r = solveIntegral(parse("x^a"), {{1, 2}});
            near(eval(bindParameters(parse(expression(r, 1)), {{"a", -1}}), 0), std::log(2.),
                 "positive definite exceptional branch");
            ++cases;
        } else if (category == "limit") {
            struct C {
                const char *s;
                const char *expected;
                double point = 0;
            };
            for (auto c : {C{R"(\sin(ax)/x)", "a"}, C{R"(\sin ax/x)", "a"},
                           C{R"((e^{ax}-1-ax)/x^2)", "a^2/2"}, C{R"(\ln(1+ax)/x)", "a"},
                           C{R"((\sqrt{1+ax}-1)/x)", "a/2"}, C{R"(\sin(ax)/\sin(bx))", "a/b"},
                           C{"(a*x^2+b)/(c*x^2+1)", "a/c", INFINITY},
                           C{"(a*x^2+b)/(c*x^2+1)", "a/c", -INFINITY}, C{"a*x^2+b*x+c", "a+b+c", 1},
                           C{R"((\sin(ax)-ax)/x^3)", "-a^3/6"}}) {
                auto r = solveLimit(parse(c.s), c.point, 0);
                check(r.kind == ResultKind::Exact || r.kind == ResultKind::Conditional, c.s);
                for (double a : {-1.3, 0., .7}) {
                    auto values = std::map<std::string, double>{{"a", a}, {"b", 2.1}, {"c", -.4}};
                    near(eval(bindParameters(parse(expression(r)), values), 0),
                         eval(bindParameters(parse(c.expected), values), 0), c.s);
                }
                if (std::string(c.s) == R"(\sin(ax)/\sin(bx))")
                    check(r.formula.find("b\\ne0") != std::string::npos,
                          "denominator parameter condition");
                ++cases;
            }
            for (double center : {0., double(INFINITY)}) {
                auto r = solveLimit(parse("x^a"), center, 1);
                check(r.kind == ResultKind::Conditional &&
                          r.formula.find("+\\infty") != std::string::npos,
                      "power sign cases");
                ++cases;
            }
            for (auto s : {"a/x", R"(\sin(a/x))", "0/(a-a)", "x^a"}) {
                check(solveLimit(parse(s), 0, 0).kind == ResultKind::Unknown, s);
                ++cases;
            }
        } else if (category == "guards") {
            rejects([] { solveIntegral(parse("x^a"), {{0, 1}}); }, "parameter improper boundary");
            rejects([] { solveIntegral(parse("1/(x+a)"), {{0, 1}}); }, "unproved interval domain");
            rejects([] { solveDerivative(parse("a*x")); }, "parameter derivative not advertised");
            rejects([] { quadrature(parse("a*x"), 0, 1, 100, "simpson"); },
                    "numeric tool requires binding");
            check(solveLimit(parse("0*\\ln(a)"), 0).kind == ResultKind::Conditional,
                  "zero factor preserves log domain");
            ++cases;
            auto r = solveIntegral(parse("1/(a*x+b)"));
            check(r.formula.find("a\\ne0") != std::string::npos, "generic reciprocal exclusion");
            ++cases;
        } else
            throw std::runtime_error("unknown category");
        std::cout << category << ": PASS " << cases << " cases, " << checks << " checks\n";
    } catch (const std::exception &e) {
        std::cerr << category << ": FAIL after " << cases << " cases: " << e.what() << '\n';
        return 1;
    }
}
