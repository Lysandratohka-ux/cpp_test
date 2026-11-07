#include "calculus.h"
#include <cmath>
#include <functional>
#include <iostream>
#include <string>
using namespace ms;
int passed = 0;
void require(bool b, const std::string &name) {
    if (!b)
        throw std::runtime_error(name);
    ++passed;
}
void near(double a, double b, const std::string &name, double tolerance = 1e-7) {
    require(std::isfinite(a) && std::abs(a - b) <= tolerance * (1 + std::abs(b)), name);
}
void rejects(std::function<void()> f, const std::string &name) {
    bool error = false;
    try {
        f();
    } catch (...) {
        error = true;
    }
    require(error, name);
}
int main(int argc, char **argv) {
    std::string category = argc > 1 ? argv[1] : "";
    try {
        if (category == "derivative") {
            for (auto s :
                 {"x^5-3x^2+7", "(x^2+1)/(x+2)", "\\sin(x^2)", "\\cos(3x)", "\\tan x", "\\arcsin x",
                  "\\arccos x", "\\arctan(2x)", "e^{x^2}", "\\ln(1+x^2)", "\\log(1+x)", "x^x",
                  "\\sqrt{x^2+1}", "\\frac{e^{x^2}\\sin x}{\\ln(1+x)}"}) {
                auto f = parse(s);
                double x = .4, h = 1e-5;
                auto result = solveDerivative(f, 1, x);
                require(result.kind == ResultKind::Exact && !result.formula.empty(), s);
                near(result.numeric.value_or(NAN), (eval(f, x + h) - eval(f, x - h)) / (2 * h), s,
                     2e-6);
                near(eval(parse(result.formula), x), result.numeric.value_or(NAN),
                     std::string(s) + " rendered formula");
            }
            near(solveDerivative(parse("x^7"), 4, 2).numeric.value_or(NAN), 6720,
                 "fourth derivative");
            near(solveDerivative(parse("e^x"), 8, 0).numeric.value_or(NAN), 1, "eighth derivative");
            rejects([] { solveDerivative(parse("|x|"), 1, 0); }, "absolute nondifferentiability");
            rejects([] { solveDerivative(parse("1/x"), 1, 0); }, "original domain");
        } else if (category == "integral") {
            for (auto s : {"x^5-3x^2+7", "1/(1+x^2)", "1/x", "\\sin(3x+1)", "\\cos(2x)", "e^{2x+1}",
                           "x e^{x^2}", "2x/(1+x^2)", "x\\sin x", "x e^x", "\\ln x", "x\\ln x",
                           "\\sin^2 x", "1/(x^2-1)", "(x^2+1)/(x^4+1)", "e^{-x^2}"}) {
                auto r = solveIntegral(parse(s));
                std::cerr << "INTEGRAL " << s << " -> " << r.plain
                          << " verification=" << int(r.verification) << '\n';
                require(r.kind == ResultKind::Exact, s);
                require(r.verification == Verification::Symbolic ||
                            r.verification == Verification::SamplesOnly,
                        s);
                if (r.formula.find("\\operatorname") == std::string::npos) {
                    auto primitive = parse(r.formula.substr(0, r.formula.size() - 2));
                    near(eval(derivative(primitive), .4), eval(parse(s), .4),
                         std::string(s) + " rendered primitive");
                }
            }
            auto r = solveIntegral(parse("x^2"), {{0, 1}});
            require(r.kind == ResultKind::Exact, "exact definite");
            near(r.numeric.value_or(NAN), 1. / 3, "one third");
            r = solveIntegral(parse("e^{x^2}"), {{0, 1}});
            require(r.kind == ResultKind::Exact || r.kind == ResultKind::Approximate,
                    "definite fallback");
            rejects([] { solveIntegral(parse("1/x"), {{-1, 1}}); }, "singular interval");
            r = solveIntegral(parse("0.1x+1/3"));
            require(r.kind == ResultKind::Exact, "exact decimal coefficients");
            require(r.formula.find("20") != std::string::npos &&
                        r.formula.find("3") != std::string::npos,
                    "decimal and rational preservation");
        } else if (category == "limit") {
            struct Case {
                const char *text;
                double point;
                int side;
                ResultKind kind;
                double value;
            };
            for (auto c : {Case{"(e^x-1-x)/x^2", 0, 0, ResultKind::Exact, .5},
                           Case{"(\\sin x-x)/x^3", 0, 0, ResultKind::Exact, -1. / 6},
                           Case{"\\sin(1/x)", 0, 0, ResultKind::DoesNotExist, 0},
                           Case{"x\\sin(1/x)", 0, 0, ResultKind::Exact, 0},
                           Case{"\\sin x/x", 0, 0, ResultKind::Exact, 1},
                           Case{"1/x", 0, 0, ResultKind::DoesNotExist, 0},
                           Case{"1/x", 0, 1, ResultKind::PositiveInfinity, 0},
                           Case{"1/x", 0, -1, ResultKind::NegativeInfinity, 0},
                           Case{"(x^3-1)/(x-1)", 1, 0, ResultKind::Exact, 3},
                           Case{"\\ln(1+x)/x", 0, 0, ResultKind::Exact, 1},
                           Case{"(1-\\cos x)/x^2", 0, 0, ResultKind::Exact, .5},
                           Case{"(1+1/x)^x", INFINITY, 0, ResultKind::Exact, std::exp(1.)},
                           Case{"(2x^2+1)/(x^2+3)", -INFINITY, 0, ResultKind::Exact, 2},
                           Case{"e^{-x}", INFINITY, 0, ResultKind::Exact, 0},
                           Case{"\\cos(2/x)", 0, 1, ResultKind::DoesNotExist, 0},
                           Case{"1/(x-0.1)", .1, 0, ResultKind::DoesNotExist, 0},
                           Case{"1/(x-0.1)", .1, 1, ResultKind::PositiveInfinity, 0},
                           Case{"1/(x-0.1)", .1, -1, ResultKind::NegativeInfinity, 0},
                           Case{"\\ln x", 0, 1, ResultKind::NegativeInfinity, 0},
                           Case{"0*\\ln x", 0, -1, ResultKind::Unknown, 0},
                           Case{"x^2\\cos(3/x)", 0, 0, ResultKind::Exact, 0}}) {
                auto r = solveLimit(parse(c.text), c.point, c.side);
                std::cerr << "LIMIT " << c.text << " -> " << r.plain << " kind=" << int(r.kind)
                          << '\n';
                require(r.kind == c.kind, c.text);
                if (c.kind == ResultKind::Exact)
                    near(r.numeric.value_or(NAN), c.value, c.text);
            }
            require(solveLimit(parse("\\sin(\\sin(1/x))"), 0, 0).kind == ResultKind::Unknown,
                    "unsupported oscillation is not a guessed finite limit");
        } else if (category == "parser") {
            near(eval(parse("\\int x e^{x^2}\\,dx"), .5), .5 * std::exp(.25), "integral wrapper");
            near(eval(parse("\\frac{e^{x^2}\\sin x}{\\ln(1+x)}"), .4),
                 std::exp(.16) * std::sin(.4) / std::log(1.4), "complex latex");
            near(eval(parse("x^2+\n\\sin x"), .4), .16 + std::sin(.4), "multiline expression");
            near(eval(parse("exp(x^2)"), .4), std::exp(.16), "plain exp token");
            for (auto s : {"\\input{file}", "\\frac{1}", "x+)", "\\int x^2", "\\lim_{x=0}x"})
                rejects([s] { parse(s); }, s);
        } else
            throw std::runtime_error("unknown test category");
        std::cout << category << ": PASS " << passed << " checks\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << category << ": FAIL after " << passed << " checks: " << e.what() << '\n';
        return 1;
    }
}
