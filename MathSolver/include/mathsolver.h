#pragma once
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
namespace ms {
struct Node;
using Expr = std::shared_ptr<const Node>;
struct Node {
    std::string op;
    double value = 0;
    Expr a, b;
    std::string literal = {}; // Original decimal token, retained for exact CAS conversion.
};
Expr number(double);
Expr variable();
Expr make(std::string, Expr, Expr = {});
Expr parse(const std::string &);
std::vector<std::string> parameters(Expr);
Expr bindParameters(Expr, const std::map<std::string, double> &);
double eval(Expr, double);
std::string latex(Expr);
Expr derivative(Expr);
Expr integrate(Expr);
Expr taylor(Expr, double, int);
struct Step {
    int iteration;
    double x, fx, error;
};
struct Result {
    double value = 0;
    bool converged = false;
    std::string message;
    std::vector<Step> steps;
};
Result bisection(Expr, double, double, double, int);
Result newton(Expr, double, double, int);
double quadrature(Expr, double, double, int, const std::string &);
struct Limit {
    std::string text;
    bool exact = false;
};
Limit limit(Expr, double, int); // direction: -1 left, 0 both, 1 right; infinity allowed
} // namespace ms
