#include "domain.h"
#include "mathsolver.h"

#include <cmath>
#include <functional>
#include <iostream>
int count = 0;
void check(bool b, const char *s) {
    ++count;
    if (!b)
        throw std::runtime_error(s);
}
void near(double a, double b, double tol = 1e-8) {
    check(std::isfinite(a) && std::abs(a - b) < tol, "numeric mismatch");
}
void bad(std::function<void()> f) {
    bool caught = false;
    try {
        f();
    } catch (...) {
        caught = true;
    }
    check(caught, "expected error");
}
int main() {
    using namespace ms;
    try {
        near(eval(parse("\\frac{\\sin x}{x}"), 1), std::sin(1.));
        near(eval(parse("e^{x^2}+\\ln(1+x)"), 1), std::exp(1.) + std::log(2.));
        near(eval(parse("\\sqrt{x^2+1}"), 2), std::sqrt(5.));
        near(eval(parse("\\arctan x+\\frac{1}{1+x^2}"), 1), std::atan(1.) + .5);
        near(eval(parse("2x+3\\sin x"), 2), 4 + 3 * std::sin(2.));
        near(eval(parse("-x^2"), 2), -4);
        near(eval(parse("2^3^2"), 0), 512);
        near(eval(parse("\\sqrt[3]{x}"), -8), -2);
        near(eval(parse("\\left|x\\right|"), -3), 3);
        near(eval(parse("\\frac1x"), 2), .5);
        for (auto s : {"\\frac", "\\unknown{x}", "(x+1", "x+", "1.2.3", "x;system(1)", "y"})
            bad([s] { parse(s); });
        bad([] { parse(std::string(5000, 'x')); });
        auto f = parse("x^3+\\sin x"), d = derivative(f);
        for (double x : {-.8, 0., .5, 2.})
            near(eval(d, x), 3 * x * x + std::cos(x));
        for (auto s : {"e^{x^2}", "\\ln(1+x^2)", "\\arctan x", "\\sqrt{x^2+1}", "\\frac{x}{1+x^2}",
                       "\\log x", "\\arcsin x", "\\tan x"}) {
            auto e = parse(s);
            double x = .3, h = 1e-6;
            near(eval(derivative(e), x), (eval(e, x + h) - eval(e, x - h)) / (2 * h), 1e-5);
        }
        for (auto s :
             {"x^2", "3x^4+2x+1", "\\sin x", "\\cos x", "e^x", "\\frac1x", "\\frac{1}{1+x^2}"}) {
            auto e = parse(s);
            near(eval(derivative(integrate(e)), .7), eval(e, .7));
        }
        bad([] { integrate(parse("\\sin(x^2)")); });
        near(quadrature(parse("x^2"), 0, 1, 100, "simpson"), 1. / 3);
        near(quadrature(parse("x^2"), 1, 0, 100, "simpson"), -1. / 3);
        bad([] { quadrature(parse("x"), 0, 1, 3, "simpson"); });
        bad([] { quadrature(parse("\\ln x"), -1, 1, 20, "trapezoid"); });
        auto r = bisection(parse("x^2-2"), 0, 2, 1e-10, 100);
        check(r.converged, "bisection");
        near(r.value, std::sqrt(2.), 1e-9);
        r = newton(parse("x^2-2"), 1, 1e-10, 100);
        check(r.converged, "Newton");
        near(r.value, std::sqrt(2.));
        check(!newton(parse("x^2+1"), 0, 1e-8, 20).converged, "zero derivative");
        check(!bisection(parse("x^2-2"), 0, 2, 1e-15, 1).converged, "iteration bound");
        bad([] { bisection(parse("x^2+1"), -1, 1, 1e-8, 100); });
        bad([] { bisection(parse("1/x"), -1, 1, 1e-8, 100); });
        auto t = taylor(parse("e^x"), 0, 4);
        for (double x : {0., .2, 1.})
            near(eval(t, x), 1 + x + x * x / 2 + x * x * x / 6 + std::pow(x, 4) / 24);
        check(limit(parse("\\frac{\\sin x}{x}"), 0, 0).text.starts_with("1"), "sinc limit");
        check(limit(parse("\\frac1x"), 0, 0).text.find("不存在") != std::string::npos, "two sides");
        check(limit(parse("\\frac1x"), 0, 1).text.starts_with("+∞"), "right infinity");
        check(limit(parse("(x^3-1)/(x-1)"), 1, 0).text.starts_with("3"), "removable");
        check(!limit(parse("\\sin(1/x)"), 0, 0).exact, "oscillatory observation");
        near(eval(derivative(parse("\\sqrt[3]{x}")), -8), 1. / 12);
        bad([] { validateInterval(parse("1/x"), -1, 1); });
        bad([] { quadrature(parse("0*(1/x)"), -1, 1, 100, "simpson"); });
        check(std::isnan(eval(parse("0*\\ln x"), -1)), "preserve original domain");
        check(r.steps.empty() || r.steps.back().error <= 1e-10, "Newton step tolerance");
        near(quadrature(parse("\\sqrt{x}"), 0, 1, 1000, "simpson"), 2. / 3, 1e-5);
        std::cout << "PASS " << count << " checks\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "FAIL after " << count << ": " << e.what() << '\n';
        return 1;
    }
}
