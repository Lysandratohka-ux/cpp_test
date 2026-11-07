#include "calculus.h"
#include <cmath>
#include <functional>
#include <iostream>
using namespace ms;
int checks = 0, cases = 0;
void check(bool ok, const std::string &message) {
    if (!ok)
        throw std::runtime_error(message);
    ++checks;
}
void near(double a, double b, const std::string &name, double tolerance = 2e-7) {
    check(std::isfinite(a) && std::abs(a - b) < tolerance * (1 + std::abs(b)), name);
}
void rejects(const char *s) {
    bool rejected = false;
    try {
        simplifyPolynomial(parse(s));
    } catch (const std::exception &) {
        rejected = true;
    }
    check(rejected, std::string("reject ") + s);
    ++cases;
}
int main(int argc, char **argv) {
    std::string category = argc > 1 ? argv[1] : "";
    try {
        if (category == "integral") {
            for (auto s :
                 {R"(1/(\sqrt{x}+\sqrt[3]{x}))", R"(\sqrt{x}/(1+\sqrt[3]{x}))",
                  R"(1/(\sqrt{2x+1}+\sqrt[3]{2x+1}))", R"(1/(\sqrt[3]{x}+\sqrt[4]{x}))",
                  R"(\sqrt[3]{3x+2}/(1+\sqrt{3x+2}))", R"(1/(\sqrt{5-2x}+\sqrt[3]{5-2x}))"}) {
                auto f = parse(s);
                auto r = solveIntegral(f);
                std::cerr << s << " -> " << r.plain << '\n';
                check(r.kind == ResultKind::Exact, s);
                check(r.verification == Verification::Symbolic ||
                          r.verification == Verification::SamplesOnly,
                      s);
                auto p = parse(r.formula.substr(0, r.formula.size() - 2));
                auto d = derivative(p);
                // Independent verification uses points not used by the engine's verifier.
                for (double x : {.13, .57, 1.23})
                    near(eval(d, x), eval(f, x), s);
                ++cases;
            }
        } else if (category == "limit") {
            struct Case {
                const char *s;
                int side;
                ResultKind kind;
                double value;
            };
            for (auto c :
                 {Case{R"(((1+\sin x)/(1-x))^{1/x})", 0, ResultKind::Exact, std::exp(2.)},
                  Case{R"((1+\sin x)^{1/x})", 0, ResultKind::Exact, std::exp(1.)},
                  Case{R"((1+x+x^2)^{1/x})", 0, ResultKind::Exact, std::exp(1.)},
                  Case{R"(((e^x-1)/x)^{1/x})", 0, ResultKind::Exact, std::exp(.5)},
                  Case{R"(((\sin x)/x)^{1/x^2})", 0, ResultKind::Exact, std::exp(-1. / 6)},
                  Case{R"(((1-\cos x)*2/x^2)^{1/x^2})", 0, ResultKind::Exact, std::exp(-1. / 12)},
                  Case{R"((1+x^2)^{1/x^3})", 1, ResultKind::PositiveInfinity, 0},
                  Case{R"((1+x^2)^{1/x^3})", -1, ResultKind::Exact, 0},
                  Case{R"((1+x^2)^{1/x^3})", 0, ResultKind::DoesNotExist, 0},
                  Case{R"((1+2x^4)^{3/x^4})", 0, ResultKind::Exact, std::exp(6.)},
                  Case{R"((-1+x)^{1/x})", 0, ResultKind::Unknown, 0},
                  Case{R"(1^{1/(x-x)})", 0, ResultKind::Unknown, 0},
                  Case{R"(1^{\ln(-x)})", 1, ResultKind::Unknown, 0},
                  Case{R"((1+\sin(1/x))^{1/x})", 0, ResultKind::Unknown, 0}}) {
                auto r = solveLimit(parse(c.s), 0, c.side);
                std::cerr << c.s << " -> " << r.plain << '\n';
                check(r.kind == c.kind, c.s);
                if (c.kind == ResultKind::Exact)
                    near(r.numeric.value_or(NAN), c.value, c.s);
                ++cases;
            }
        } else if (category == "polynomial") {
            for (auto s : {"(x+1)^5-(x-1)^5", "(2x-3)(x+4)-(x^2+x)", "0.1(x+1)^2+(x-1)^2/3",
                           "(x^2+1)^8", "(x-x)^2", "3/7", "x^64"}) {
                auto f = parse(s);
                auto r = simplifyPolynomial(f);
                check(r.kind == ResultKind::Exact && r.verification == Verification::Symbolic, s);
                auto p = parse(r.formula);
                for (double x : {-1.2, -.23, .71, 1.13})
                    near(eval(p, x), eval(f, x), s);
                ++cases;
            }
            for (auto s : {"1/x", "(x^2-1)/(x-1)", "\\sin x", "x^{1/2}", "x^65", "x^32*x^33",
                           "x^(x-x)", "1/(2-2)", "x^{2.0000000000000001}"})
                rejects(s);
        } else
            throw std::runtime_error("unknown category");
        std::cout << category << ": PASS " << cases << " cases, " << checks << " checks\n";
    } catch (const std::exception &e) {
        std::cerr << category << ": FAIL after " << cases << " cases: " << e.what() << '\n';
        return 1;
    }
}
