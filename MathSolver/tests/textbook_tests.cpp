#include "calculus.h"
#include "domain.h"
#include <cmath>
#include <functional>
#include <iostream>
#include <numbers>
#include <string>
#include <vector>
using namespace ms;
int passed = 0, failed = 0;
void check(bool condition, const std::string &message) {
    if (!condition)
        throw std::runtime_error(message);
}
void near(double a, double b, double eps = 2e-6) {
    check(std::isfinite(a) && std::isfinite(b) && std::abs(a - b) <= eps * (1 + std::abs(b)),
          "numeric mismatch: " + std::to_string(a) + " vs " + std::to_string(b));
}
void run(const std::string &id, std::function<void()> test) {
    try {
        test();
        ++passed;
        std::cout << "PASS " << id << '\n';
    } catch (const std::exception &e) {
        ++failed;
        std::cerr << "FAIL " << id << ": " << e.what() << '\n';
    }
}
void rejects(std::function<void()> test) {
    bool rejected = false;
    try {
        test();
    } catch (const std::exception &) {
        rejected = true;
    }
    check(rejected, "undefined/singular expression must be rejected");
}
int main(int argc, char **argv) {
    std::string category = argc > 1 ? argv[1] : "";
    if (category == "derivative") {
        // Printed pp. 89-90, exercises 3.2. Exact statements without free parameters.
        const std::vector<std::pair<std::string, std::string>> cases = {
            {"3.2-1(1)", R"(x^2-3\cos x+2^x+\ln 2)"},
            {"3.2-1(2)", R"(3\tan x+\csc x+2)"},
            {"3.2-1(3)", R"(\ln x+2\log_3 x+4\lg x)"},
            {"3.2-1(4)", R"(\sqrt{x\sqrt{x\sqrt{x}}})"},
            {"3.2-1(5)", "(2+3x+4x^2)/x"},
            {"3.2-1(6)", "(3x+1)(4x+3)"},
            {"3.2-1(7)", R"(3e^x\sin x)"},
            {"3.2-1(8)", R"(x\ln x+\ln x/x)"},
            {"3.2-1(9)", R"(1/(x+\cos x))"},
            {"3.2-1(10)", R"((1-\ln x)/(1+\ln x))"},
            {"3.2-1(11)", "1/(1-x+x^2)"},
            {"3.2-1(12)", "(1+x-x^2)/(1-x+x^2)"},
            {"3.2-1(13)", R"(x^2\ln x\cos x)"},
            {"3.2-1(14)", R"((1+x^2)\arctan x\arccot x)"},
            {"3.2-2(1)", "(2-5x)^30"},
            {"3.2-2(2)", R"(5\sin(1-2x))"},
            {"3.2-2(3)", R"(e^{-\sin^2 x})"},
            {"3.2-2(6)", R"(\arctan((x+1)/(x-1)))"},
            {"3.2-2(7)", R"(\ln(\sec x+\tan x))"},
            {"3.2-2(9)", R"((\sqrt{1+x}-\sqrt{1-x})/(\sqrt{1+x}+\sqrt{1-x}))"},
            {"3.2-2(10)", R"(\arcsin\sqrt{(1-x)/(1+x)})"},
            {"3.2-2(12)", R"(\arctan(1/x)\ln\cos x)"}};
        for (auto &[id, s] : cases)
            run(id, [&] {
                auto f = parse(s);
                for (double x : {.25, .55, .75}) {
                    auto r = solveDerivative(f, 1, x);
                    check(r.kind == ResultKind::Exact, "no exact derivative");
                    double h = 1e-6;
                    near(r.numeric.value_or(NAN), (eval(f, x + h) - eval(f, x - h)) / (2 * h),
                         1e-4);
                    near(eval(parse(r.formula), x), r.numeric.value_or(NAN));
                }
            });
        run("3.2-2(4)", [] {
            auto r = solveDerivative(parse(R"(\arccos(1/x))"), 1, 2);
            near(r.numeric.value_or(NAN), 1 / (2 * std::sqrt(3.)));
        });
        run("3.2-2(8)", [] {
            auto r = solveDerivative(parse(R"(\ln(\ln(\ln x)))"), 1, 4);
            near(r.numeric.value_or(NAN), 1 / (4 * std::log(4.) * std::log(std::log(4.))));
        });
    } else if (category == "integral") {
        // Printed pp. 172,193-195. No parameter substitution is counted as an original exercise.
        const std::vector<std::pair<std::string, std::string>> cases = {
            {"p172-example5", "1/(3+2x)"},
            {"p172-example6", "2x e^{x^2+1}"},
            {"5.3-1(1)", R"(\sqrt[3]{x}-1/\sqrt{x})"},
            {"5.3-1(2)", "2^x+x^2"},
            {"5.3-1(3)", R"((1-1/x)\sqrt{x\sqrt{x}})"},
            {"5.3-1(5)", "(3x^4+3x^2+1)/(x^2+1)"},
            {"5.3-1(6)", "x^2/(1+x^2)"},
            {"5.3-1(7)", "e^x(1-e^{-x}/x)"},
            {"5.3-1(9)", R"(\sec x(\sec x-\tan x))"},
            {"5.3-1(10)", R"(\cot^2 x)"},
            {"5.3-1(11)", R"(\cos^2(x/2))"},
            {"5.3-1(12)", R"(\cos(2x)/(\cos x+\sin x))"},
            {"5.3-1(13)", R"(1/(1-\cos(2x)))"},
            {"5.3-1(14)", R"(\cos(2x)/(\cos^2 x\sin^2 x))"},
            {"5.3-2(1)", "e^{3x}"},
            {"5.3-2(2)", "(3x+2)^5"},
            {"5.3-2(3)", "1/(1-2x)"},
            {"5.3-2(4)", R"(1/\sqrt[3]{2-3x})"},
            {"5.3-2(5)", R"(x/\sqrt{2-3x^2})"},
            {"5.3-2(6)", R"(\sin\sqrt{x}/\sqrt{x})"},
            {"5.3-2(7)", R"(\sin(1/x)/x^2)"},
            {"5.3-2(8)", R"(\tan\sqrt{x}/\sqrt{x})"},
            {"5.3-2(9)", R"(1/(x\ln x))"},
            {"5.3-2(10)", R"(x/\sqrt{9-x})"},
            {"5.3-2(12)", "x/(16+x^4)"},
            {"5.3-2(13)", "e^x/(1+4e^{2x})"},
            {"5.3-2(14)", "1/(2x^2-1)"},
            {"5.3-2(15)", "e^x/(1+e^x)"},
            {"5.3-2(16)", "1/(1+e^x)"},
            {"5.3-2(17)", "1/(e^x+e^{-x})"},
            {"5.3-2(18)", R"(\sin^5 x\cos x)"},
            {"5.3-2(19)", R"(\sin^3 x\cos^2 x)"},
            {"5.3-2(20)", R"(\tan^{10}x\sec^2 x)"},
            {"5.3-2(22)", R"(\tan^3 x\sec x)"},
            {"5.3-2(23)", R"(\sin^2 x)"},
            {"5.3-2(24)", R"(\cos^3 x)"},
            {"5.3-2(25)", R"(\sin(2x)\cos(3x))"},
            {"5.3-2(26)", R"(\sin(5x)\sin(7x))"},
            {"5.3-2(27)", "1/(x^2+4x+5)"},
            {"5.3-2(28)", "(x+3)/(x^2+6x+1)"},
            {"5.3-2(29)", "(x+1)/(x^2+x+1)"},
            {"5.3-2(31)", R"(1/(\arcsin^2 x\sqrt{1-x^2}))"},
            {"5.3-2(32)", R"(\arctan\sqrt{x}/(\sqrt{x}(1+x)))"},
            {"5.3-3(1)", R"(1/(1+\sqrt{x}))"},
            {"5.3-3(2)", R"(x\sqrt[3]{1+x})"},
            {"5.3-3(3)", R"(x/\sqrt{2+4x})"},
            {"5.3-3(8)", R"(x\sqrt[3]{1+x^2})"},
            {"5.3-3(9)", R"(x^2/\sqrt{1-x^2})"},
            {"5.3-3(10)", R"(1/(x^2\sqrt{1-x^2}))"},
            {"5.3-3(11)", R"(1/\sqrt{(x^2+1)^3})"},
            {"5.3-3(14)", "1/(1+x^2)^2"},
            {"5.3-4(1)", R"(\ln x)"},
            {"5.3-4(2)", R"(\ln^2 x)"},
            {"5.3-4(3)", R"(\ln^3 x/x^2)"},
            {"5.3-4(4)", R"(x^2\cos^2(x/2))"},
            {"5.3-4(6)", R"(x\sin x)"},
            {"5.3-4(7)", R"(x^2\cos x)"},
            {"5.3-4(8)", R"(x\tan^2 x)"},
            {"5.3-4(9)", R"(x/\cos^2 x)"},
            {"5.3-4(10)", R"(\arcsin x)"},
            {"5.3-4(11)", R"(x\arctan x)"},
            {"5.3-4(12)", R"(x^2\arctan x)"},
            {"5.3-4(13)", R"(\arcsin\sqrt{x}/\sqrt{x})"},
            {"5.3-4(14)", R"(\arcsin\sqrt{x}/\sqrt{1-x})"},
            {"5.3-4(15)", R"(\arcsin^2 x)"},
            {"5.3-4(16)", "x e^{-x}"},
            {"5.3-4(17)", "x^2 e^x"},
            {"5.3-4(18)", R"(e^{2x}\cos(3x))"},
            {"5.3-4(19)", R"(e^{-2x}\sin(x/2))"},
            {"5.3-4(20)", R"(\cos(\ln x))"},
            {"5.3-4(21)", R"(e^x\sin^2 x)"},
            {"family-regression-quadratic", R"(x^4/\sqrt{4-9x^2})"},
            {"family-regression-exponential", "1/(2+e^x)"},
            {"family-regression-trigonometric", R"(\cot^2(2x))"}};
        for (auto &[id, s] : cases)
            run(id, [&] {
                auto f = parse(s);
                auto r = solveIntegral(f);
                check(r.kind == ResultKind::Exact, "no exact primitive: " + r.plain);
                check(r.verification == Verification::Symbolic ||
                          r.verification == Verification::SamplesOnly,
                      "unverified primitive");
                // Independent finite differences of the rendered primitive at three valid points.
                check(r.formula.ends_with("+C"), "missing integration constant");
                auto primitive = parse(r.formula.substr(0, r.formula.size() - 2));
                int valid = 0;
                for (double x : {.23, .37, .63}) {
                    double h = 1e-6;
                    double actual = (eval(primitive, x + h) - eval(primitive, x - h)) / (2 * h);
                    double expected = eval(f, x);
                    if (std::isfinite(actual) && std::isfinite(expected)) {
                        near(actual, expected, 2e-5);
                        ++valid;
                    }
                }
                check(valid == 3, "insufficient independent verification points");
            });
    } else if (category == "limit") {
        struct Case {
            std::string id, s;
            double point, value;
        };
        for (const auto &c :
             std::vector<Case>{{"p125-example14", R"((e^x+e^{-x}-2)\cos x/(\sin x(e^x-1)))", 0, 1},
                               {"4.2-1(2)", R"((e^x-e^{-x})/\sin x)", 0, 2},
                               {"4.2-1(3)", R"((1-\cos(x^2))/(x^2\sin(x^2)))", 0, .5},
                               {"4.2-1(6)", R"((\tan x-x)/(x-\sin x))", 0, 2},
                               {"4.2-1(7)", R"(\ln(1+x^2)/(\sec x-\cos x))", 0, 1},
                               {"4.2-1(8)", R"(\ln(1+1/x)/\arccot x)", INFINITY, 1}}) {
            run(c.id, [&] {
                auto r = solveLimit(parse(c.s), c.point, 0);
                check(r.kind == ResultKind::Exact, r.plain);
                near(r.numeric.value_or(NAN), c.value);
            });
        }
        for (auto text : {R"(\frac{\csc x-\cot x}{x})", R"((1-\cos x)/(x\sin x))",
                          R"((\csc(2x)-\cot(2x))/x)"}) {
            run(std::string("reciprocal-trig-cancellation ") + text, [&] {
                double expected = std::string(text).find("2x") == std::string::npos ? .5 : 1.;
                for (int side : {-1, 0, 1}) {
                    auto r = solveLimit(parse(text), 0, side);
                    check(r.kind == ResultKind::Exact, r.plain);
                    near(r.numeric.value_or(NAN), expected);
                }
            });
        }
        run("opposite reciprocal-trig poles", [] {
            check(solveLimit(parse(R"((\csc x+\cot x)/x)"), 0, 0).kind ==
                      ResultKind::PositiveInfinity,
                  "noncancelling poles must not be a finite limit");
        });
        run("everywhere undefined divisor", [] {
            check(solveLimit(parse("1/(1/(x-x))"), 0, 0).kind == ResultKind::Unknown,
                  "clearing fractions erased original domain");
        });
    } else if (category == "syntax_domain") {
        for (auto s : {R"(\sec x)", R"(sec(x))", R"(\csc x)", R"(\cot x)", R"(\sec^2 x)",
                       R"(\arccot x)", R"(\log_{3} x)", R"(\lg x)"})
            run(s, [&] {
                auto f = parse(s);
                near(eval(parse(latex(f)), .4), eval(f, .4));
            });
        run("arccot real branch", [] {
            near(eval(parse(R"(\arccot x)"), -1), 3 * std::numbers::pi / 4);
            near(eval(parse(R"(\arccot x)"), 0), std::numbers::pi / 2);
        });
        run("base logarithm", [] { near(eval(parse(R"(\log_3 9)"), 0), 2); });
        run("powered base logarithm", [] { near(eval(parse(R"(\log_3^2 9)"), 0), 4); });
        run("logarithm negative inner branch", [] {
            auto r = solveIntegral(parse(R"(1/(x\ln x))"));
            check(r.kind == ResultKind::Exact, "no real primitive");
            auto primitive = parse(r.formula.substr(0, r.formula.size() - 2));
            for (double x : {.2, .6, 1.5, 3.})
                near(eval(derivative(primitive), x), 1 / (x * std::log(x)));
        });
        run("sec regular definite", [] {
            auto r = solveIntegral(parse(R"(\sec^2 x)"), {{0, .7}});
            near(r.numeric.value_or(NAN), std::tan(.7));
        });
        run("csc regular definite", [] {
            auto r = solveIntegral(parse(R"(\csc^2 x)"), {{.4, .8}});
            near(r.numeric.value_or(NAN), 1 / std::tan(.4) - 1 / std::tan(.8));
        });
        run("sec pole inside",
            [] { rejects([] { solveIntegral(parse(R"(\sec x)"), {{1, 2}}); }); });
        run("csc pole inside",
            [] { rejects([] { solveIntegral(parse(R"(\csc x)"), {{-1, 1}}); }); });
        run("cot pole inside",
            [] { rejects([] { solveIntegral(parse(R"(\cot x)"), {{-1, 1}}); }); });
        run("sec point pole",
            [] { rejects([] { solveDerivative(parse(R"(\sec x)"), 1, std::numbers::pi / 2); }); });
        run("cot point pole", [] { rejects([] { solveDerivative(parse(R"(\cot x)"), 1, 0); }); });
        run("unmatched sec input", [] { rejects([] { parse(R"(\sec)"); }); });
        run("unsupported command", [] { rejects([] { parse(R"(\input{x})"); }); });
    } else {
        std::cerr << "unknown category\n";
        return 2;
    }
    std::cout << category << ": " << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
