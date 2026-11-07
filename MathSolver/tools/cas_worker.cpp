#include "calculus.h"
#include <iomanip>
#include <iostream>
// A native C++ boundary between MSVC Qt and MinGW libqalculate on Windows.
// One framed request per process. No shell, code evaluation, or network access.
int main() {
    try {
        char operation;
        std::string input, point;
        int order, direction;
        bool hasBounds;
        double a, b;
        if (!(std::cin >> operation >> std::quoted(input) >> order >> std::quoted(point) >>
              direction >> hasBounds >> a >> b) ||
            input.size() > 65536)
            throw std::runtime_error("Invalid request");
        auto f = ms::parse(input);
        ms::CalculusResult r;
        if (operation == 'D')
            r = ms::solveDerivative(f, order,
                                    point.empty() ? std::optional<double>{} : std::stod(point));
        else if (operation == 'I')
            r = ms::solveIntegral(f, hasBounds ? std::optional{std::pair{a, b}} : std::nullopt);
        else if (operation == 'L')
            r = ms::solveLimit(f, std::stod(point), direction);
        else if (operation == 'S')
            r = ms::simplifyPolynomial(f);
        else
            throw std::runtime_error("Unknown operation");
        std::cout << 1 << ' ' << int(r.kind) << ' ' << int(r.verification) << ' '
                  << std::quoted(r.formula) << ' ' << std::quoted(r.plain) << ' '
                  << std::quoted(r.detail) << ' ' << bool(r.numeric) << ' ' << std::setprecision(17)
                  << r.numeric.value_or(0) << '\n';
        return 0;
    } catch (const std::exception &e) {
        std::cout << 0 << ' ' << std::quoted(std::string(e.what())) << '\n';
        return 0;
    }
}
