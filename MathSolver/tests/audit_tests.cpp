#include "calculus.h"
#include <cmath>
#include <iostream>
using namespace ms;
int checks = 0;
void check(bool ok, const char *what) {
    if (!ok)
        throw std::runtime_error(what);
    ++checks;
}
int main() {
    try {
        for (auto s : {R"((\ln(-x))^0)", R"(1^{\ln(-x)})", R"(\arctan(1/(x-x)))"})
            check(!std::isfinite(eval(parse(s), 1)), s);
        check(!std::isfinite(eval(parse(R"(\arctan(1/x))"), 0)), "undefined reciprocal under atan");
        check(std::abs(eval(parse(R"(\arctan(1/x))"), 1) - std::atan(1.)) < 1e-14,
              "regular atan evaluation");
        for (auto s :
             {R"(x*\sin(1/(x-x)))", R"(x*\sin(\ln(-x)))", R"((\ln(-x))^0)", R"(0*\sin(1/(x-x)))"})
            check(solveLimit(parse(s), 0, 1).kind == ResultKind::Unknown, s);
        auto zero = [](const char *s, int side) {
            auto r = solveLimit(parse(s), 0, side);
            check(r.kind == ResultKind::Exact && r.numeric && *r.numeric == 0, s);
        };
        zero(R"(x*\sin(1/x))", 0);
        zero(R"(x*\sin(\sin(1/x)))", 0);
        zero(R"(x*\sin(\ln(-x)))", -1);
        zero(R"(x*\sin(\ln(x)))", 1);
        check(solveLimit(parse(R"(x*\sin(\ln(x)))"), 0, -1).kind == ResultKind::Unknown,
              "wrong side of log domain");
        auto r = solveLimit(parse(R"((\ln(-x))^0)"), 0, -1);
        check(r.kind == ResultKind::Exact && r.numeric && *r.numeric == 1,
              "valid side of zero power");
        std::cout << "audit_domain: PASS " << checks << " checks\n";
    } catch (const std::exception &e) {
        std::cerr << "audit_domain: FAIL after " << checks << " checks: " << e.what() << '\n';
        return 1;
    }
}
