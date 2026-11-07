#pragma once
#include "mathsolver.h"
#include <optional>
namespace ms {
enum class ResultKind {
    Exact,
    Approximate,
    PositiveInfinity,
    NegativeInfinity,
    DoesNotExist,
    Unknown,
    Conditional
};
enum class Verification { NotApplicable, Symbolic, SamplesOnly, Unverified };
struct CalculusResult {
    CalculusResult(ResultKind k = ResultKind::Unknown, std::string f = {}, std::string p = {},
                   std::string d = {}, std::optional<double> n = {},
                   Verification v = Verification::NotApplicable, Expr e = {})
        : kind(k), formula(std::move(f)), plain(std::move(p)), detail(std::move(d)), numeric(n),
          verification(v), expression(std::move(e)) {}
    ResultKind kind = ResultKind::Unknown;
    std::string formula, plain, detail;
    std::optional<double> numeric;
    Verification verification = Verification::NotApplicable;
    Expr expression; // Optional local tree for plotting. Special functions may have no local tree.
};
CalculusResult solveDerivative(Expr, int order = 1, std::optional<double> point = {});
CalculusResult solveIntegral(Expr, std::optional<std::pair<double, double>> bounds = {});
CalculusResult solveLimit(Expr, double point, int direction = 0);
CalculusResult simplifyPolynomial(Expr);
std::string symbolicBackend();
} // namespace ms
