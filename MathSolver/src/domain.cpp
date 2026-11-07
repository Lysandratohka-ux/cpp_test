#include "domain.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>
namespace ms {
struct Range {
    double lo, hi;
};
static Range mul(Range a, Range b) {
    double v[] = {a.lo * b.lo, a.lo * b.hi, a.hi * b.lo, a.hi * b.hi};
    return {*std::min_element(v, v + 4), *std::max_element(v, v + 4)};
}
static Range bounds(Expr e, Range x) {
    auto fail = []() -> Range {
        throw std::runtime_error("无法确认整个区间在实数定义域内；可能存在极点、间断或无定义区域，"
                                 "请缩小区间。当前不支持反常积分。");
    };
    auto o = e->op;
    if (o == "param")
        throw std::runtime_error("区间数值检查需要先给字母参数赋值");
    if (o == "num")
        return {e->value, e->value};
    if (o == "e" || o == "pi") {
        double v = eval(e, 0);
        return {v, v};
    }
    if (o == "x")
        return x;
    auto a = bounds(e->a, x);
    auto b = e->b ? bounds(e->b, x) : Range{0, 0};
    Range r;
    if (o == "+")
        r = {a.lo + b.lo, a.hi + b.hi};
    else if (o == "-")
        r = {a.lo - b.hi, a.hi - b.lo};
    else if (o == "*")
        r = mul(a, b);
    else if (o == "/") {
        if (b.lo <= 0 && b.hi >= 0)
            return fail();
        r = mul(a, {1 / b.hi, 1 / b.lo});
    } else if (o == "^" || o == "root") {
        if (o == "root") {
            if (b.lo != b.hi || b.lo == 0)
                return fail();
            if (a.lo < 0 && b.lo == std::trunc(b.lo) && std::fmod(std::abs(b.lo), 2) == 1) {
                if (b.lo < 0 && a.hi >= 0)
                    return fail();
                auto root = [b](double v) {
                    return std::copysign(std::pow(std::abs(v), 1 / b.lo), v);
                };
                double v = root(a.lo), w = root(a.hi);
                return {std::min(v, w), std::max(v, w)};
            }
            b = {1 / b.lo, 1 / b.lo};
        }
        if (b.lo == b.hi && b.lo == std::trunc(b.lo)) {
            double p = b.lo;
            if (p < 0 && a.lo <= 0 && a.hi >= 0)
                return fail();
            double v = std::pow(a.lo, p), w = std::pow(a.hi, p);
            r = {std::min(v, w), std::max(v, w)};
            if (p > 0 && std::fmod(p, 2) == 0 && a.lo <= 0 && a.hi >= 0)
                r.lo = 0;
        } else {
            if (a.lo < 0 || (a.lo == 0 && b.lo <= 0))
                return fail();
            double v[] = {std::pow(a.lo, b.lo), std::pow(a.lo, b.hi), std::pow(a.hi, b.lo),
                          std::pow(a.hi, b.hi)};
            r = {*std::min_element(v, v + 4), *std::max_element(v, v + 4)};
        }
    } else if (o == "ln" || o == "log") {
        if (a.lo <= 0)
            return fail();
        r = {std::log(a.lo), std::log(a.hi)};
        if (o == "log") {
            r.lo /= std::log(10.);
            r.hi /= std::log(10.);
        }
    } else if (o == "exp")
        r = {std::exp(a.lo), std::exp(a.hi)};
    else if (o == "sin" || o == "cos") {
        // Tight enclosures let 1/cos(x) pass on a regular interval while
        // still rejecting every interval that crosses a denominator zero.
        double pi = std::numbers::pi;
        double phase = o == "sin" ? 0 : pi / 2;
        double pad = 8 * std::numeric_limits<double>::epsilon() *
                     std::max({1., std::abs(a.lo), std::abs(a.hi)});
        auto contains = [&](double offset, double period) {
            double k = std::ceil((a.lo - pad - offset) / period);
            return offset + k * period <= a.hi + pad;
        };
        auto fn = o == "sin" ? static_cast<double (*)(double)>(std::sin)
                             : static_cast<double (*)(double)>(std::cos);
        double v = fn(a.lo), w = fn(a.hi);
        r = {std::min(v, w), std::max(v, w)};
        if (contains(pi / 2 - phase, 2 * pi))
            r.hi = 1;
        if (contains(-pi / 2 - phase, 2 * pi))
            r.lo = -1;
        if (contains(-phase, pi)) {
            r.lo = std::min(r.lo, 0.);
            r.hi = std::max(r.hi, 0.);
        }
        r.lo = std::nextafter(r.lo, -INFINITY);
        r.hi = std::nextafter(r.hi, INFINITY);
    } else if (o == "tan") {
        double pi = std::numbers::pi;
        double k = std::ceil((a.lo - pi / 2) / pi);
        if (pi / 2 + k * pi <= a.hi)
            return fail();
        r = {std::tan(a.lo), std::tan(a.hi)};
    } else if (o == "asin" || o == "acos") {
        if (a.lo < -1 || a.hi > 1)
            return fail();
        r = o == "asin" ? Range{std::asin(a.lo), std::asin(a.hi)}
                        : Range{std::acos(a.hi), std::acos(a.lo)};
    } else if (o == "atan")
        r = {std::atan(a.lo), std::atan(a.hi)};
    else if (o == "abs")
        r = {a.lo <= 0 && a.hi >= 0 ? 0 : std::min(std::abs(a.lo), std::abs(a.hi)),
             std::max(std::abs(a.lo), std::abs(a.hi))};
    else
        return fail();
    if (!std::isfinite(r.lo) || !std::isfinite(r.hi))
        return fail();
    return r;
}
void validateInterval(Expr e, double a, double b) {
    if (!std::isfinite(a) || !std::isfinite(b))
        throw std::runtime_error("区间端点必须有限");
    bounds(e, {std::min(a, b), std::max(a, b)});
}
} // namespace ms
