#include "mathsolver.h"
#include <cmath>
#include <iomanip>
#include <iostream>
int main() {
    using namespace ms;
    std::cout << std::setprecision(14) << "method,n,result,absolute_error\n";
    auto f = parse("x^2");
    for (auto method : {"rectangle", "trapezoid", "simpson"})
        for (int n : {4, 8, 16, 32}) {
            double v = quadrature(f, 0, 1, n, method);
            std::cout << method << ',' << n << ',' << v << ',' << std::abs(v - 1. / 3) << '\n';
        }
    for (auto method : {"bisection", "newton"}) {
        auto r = std::string(method) == "bisection" ? bisection(parse("x^2-2"), 0, 2, 1e-10, 100)
                                                    : newton(parse("x^2-2"), 1, 1e-10, 100);
        std::cout << method << ',' << r.steps.size() << ',' << r.value << ','
                  << std::abs(r.value - std::sqrt(2.)) << '\n';
    }
    return 0;
}
