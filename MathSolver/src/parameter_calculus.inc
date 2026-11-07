// Private implementation included by calculus.cpp: all CAS calls share its mutex
// and validated AST conversion. Parameters are real constants, never free code.
namespace {
bool dependsX(Expr e) {
    return e && (e->op == "x" || dependsX(e->a) || dependsX(e->b));
}
struct ParameterConditions {
    std::vector<std::string> items;
    void add(const MathStructure &value, bool positive = false) {
        auto v = value;
        scalarize(v);
        if ((positive && v.representsPositive()) || (!positive && v.representsNonZero()))
            return;
        if (v.isZero() || (positive && v.representsNonPositive()))
            throw std::runtime_error("无法满足所需实数定义域条件");
        auto s = tex(v) + (positive ? ">0" : "\\ne0");
        if (std::find(items.begin(), items.end(), s) == items.end())
            items.push_back(s);
    }
    std::string joined() const {
        std::string s;
        for (const auto &item : items) {
            if (!s.empty())
                s += ",\\quad ";
            s += item;
        }
        return s;
    }
};
void sourceConditions(Expr f, ParameterConditions &conditions) {
    if (!f || !f->a)
        return;
    sourceConditions(f->a, conditions);
    sourceConditions(f->b, conditions);
    if (f->op == "/")
        conditions.add(structure(f->b));
    if (f->op == "ln" || f->op == "log")
        conditions.add(structure(f->a), true);
    if (f->op == "^" && (f->b->op != "num" || std::trunc(f->b->value) != f->b->value))
        conditions.add(structure(f->a), true);
    if (f->op == "root") {
        conditions.add(structure(f->a), true);
        conditions.add(structure(f->b));
    }
    if (f->op == "^" && f->b->op == "num" && f->b->value < 0)
        conditions.add(structure(f->a));
    if (f->op == "tan")
        conditions.add(structure(make("cos", f->a)));
    if (f->op == "asin" || f->op == "acos") {
        auto v = structure(f->a);
        v.raise(MathStructure(2));
        v.negate();
        v += MathStructure(1);
        conditions.add(v, true); // Sufficient open-domain condition for differentiation.
    }
}
void primitiveConditions(const MathStructure &m, ParameterConditions &conditions) {
    if (m.isPower() && !(m[1].isNumber() && m[1].number().isInteger()))
        conditions.add(m[0], true);
    if (m.isPower() && m[1].isNumber() && m[1].number().isNegative())
        conditions.add(m[0]);
    if (m.isFunction() && m.function()->referenceName() == "ln" && m.size() == 1) {
        if (m[0].isFunction() && m[0].function()->referenceName() == "abs")
            conditions.add(m[0][0]);
        else
            conditions.add(m[0], true);
    }
    for (size_t i = 0; i < m.size(); ++i)
        primitiveConditions(m[i], conditions);
}
CalculusResult parameterResult(CalculusResult r, Expr f, const ParameterConditions &conditions) {
    if (r.kind == ResultKind::Unknown)
        return r;
    std::string names;
    for (auto &p : parameters(f)) {
        if (!names.empty())
            names += ",";
        names += p;
    }
    std::string declaration = names + "\\in\\mathbb{R}";
    if (!conditions.items.empty()) {
        declaration += ",\\quad " + conditions.joined();
        r.kind = ResultKind::Conditional;
        r.detail +=
            " 仅在显示条件成立的共同实数定义域连续区间内使用；未覆盖的退化参数值不套用此式。";
    }
    r.formula = "\\begin{gathered}" + r.formula + "\\\\[4pt]\\text{条件：}\\quad " + declaration +
                "\\end{gathered}";
    r.detail += " 参数 a、b、c、n 为实数常数（n 不自动假定为整数），变量为 x。";
    return r;
}
// Only entire arithmetic/trig/exp compositions and x-independent divisors are
// certified for a complete finite interval. This does not sample parameter values.
bool entireForParameters(Expr f) {
    if (!f->a)
        return true;
    if (!entireForParameters(f->a) || (f->b && !entireForParameters(f->b)))
        return false;
    auto o = f->op;
    return o == "+" || o == "-" || o == "*" || o == "sin" || o == "cos" || o == "exp" ||
           (o == "/" && !dependsX(f->b)) ||
           (o == "^" && ((f->a->op == "e") || (f->b->op == "num" && f->b->value >= 0 &&
                                               std::trunc(f->b->value) == f->b->value)));
}
// Separate a constant coefficient from a single power x^p. The p=-1 branch
// follows from the logarithm rule, rather than an assumed nonzero denominator.
bool parameterPower(Expr f, MathStructure &coefficient, Expr &power) {
    if (!dependsX(f)) {
        coefficient = structure(f);
        return true;
    }
    if (f->op == "^" && f->a->op == "x" && !dependsX(f->b) && !parameters(f->b).empty()) {
        if (power)
            return false;
        power = f->b;
        coefficient = MathStructure(1);
        return true;
    }
    if (f->op != "*" && !(f->op == "/" && !dependsX(f->b)))
        return false;
    MathStructure a, b;
    if (!parameterPower(f->a, a, power) || !parameterPower(f->b, b, power))
        return false;
    coefficient = a;
    if (f->op == "*")
        coefficient *= b;
    else
        coefficient /= b;
    coefficient.eval(options());
    return true;
}
bool verifyParameterPrimitive(MathStructure primitive, MathStructure original) {
    auto eo = options();
    eo.expand = true;
    if (!primitive.differentiate(xvar(), eo))
        return false;
    scalarize(primitive);
    scalarize(original);
    primitive -= original;
    primitive.eval(eo);
    scalarize(primitive);
    primitive.eval(eo);
    return primitive.isZero();
}
CalculusResult parameterIntegral(Expr f, std::optional<std::pair<double, double>> interval) {
    std::lock_guard lock(casMutex);
    Control control;
    ParameterConditions conditions;
    sourceConditions(f, conditions);
    auto x = xvar();
    auto eo = options();
    MathStructure coefficient;
    Expr power;
    if (parameterPower(f, coefficient, power) && power) {
        if (interval && (!std::isfinite(interval->first) || !std::isfinite(interval->second) ||
                         interval->first <= 0 || interval->second <= 0))
            throw std::runtime_error("含字母指数的定积分目前要求上下限均大于0；不自动处理反常积分");
        auto p = structure(power), q = p;
        q += MathStructure(1);
        q.eval(eo);
        if (q.isZero()) {
            auto m = x;
            m.transform(cas().getFunction("ln"));
            m *= coefficient;
            if (!verifyParameterPrimitive(m, structure(f)))
                return {};
            if (interval) {
                auto a = m, b = m;
                a.replace(x, cas().parse(decimal(interval->first)));
                b.replace(x, cas().parse(decimal(interval->second)));
                b -= a;
                b.eval(eo);
                m = b;
                conditions.items.erase(
                    std::remove(conditions.items.begin(), conditions.items.end(), "x>0"),
                    conditions.items.end());
            }
            auto r = output(m);
            if (!interval) {
                r.formula += "+C";
                r.plain += " + C";
            }
            r.verification = Verification::Symbolic;
            r.detail = "符号化简确认指数恒等于-1，直接使用对数积分规则。";
            return parameterResult(r, f, conditions);
        }
        auto generic = x;
        generic.raise(q);
        generic /= q;
        generic *= coefficient;
        auto log = x;
        log.transform(cas().getFunction("ln"));
        log *= coefficient;
        auto exceptionalIntegrand = coefficient;
        exceptionalIntegrand /= x;
        if (!verifyParameterPrimitive(generic, structure(f)) ||
            !verifyParameterPrimitive(log, exceptionalIntegrand))
            return {};
        auto atBounds = [&](MathStructure m) {
            if (!interval)
                return m;
            auto a = m, b = m;
            a.replace(x, cas().parse(decimal(interval->first)));
            b.replace(x, cas().parse(decimal(interval->second)));
            b -= a;
            b.eval(eo);
            return b;
        };
        auto regular = atBounds(generic), exceptional = atBounds(log);
        auto r = output(regular), special = output(exceptional);
        if (!interval)
            r.formula = tex(generic);
        r.kind = ResultKind::Conditional;
        r.numeric.reset();
        auto constant = interval ? "" : "+C";
        r.formula = "\\begin{cases}" + r.formula + constant + " & " + tex(p) + "\\ne-1\\\\" +
                    special.formula + constant + " & " + tex(p) + "=-1\\end{cases}";
        r.plain = "幂函数分情况：指数不等于-1使用幂公式；等于-1使用对数公式。";
        r.verification = Verification::Symbolic;
        r.detail = "对 x>0 "
                   "的实数幂使用幂函数与对数规则，两个分支均已通过符号求导恒等验证；保留指数=-"
                   "1的单独分支。";
        if (interval) {
            // x>0 was a condition on the integration variable, not on the result.
            conditions.items.erase(
                std::remove(conditions.items.begin(), conditions.items.end(), "x>0"),
                conditions.items.end());
            r.detail += " 上下限均为正，已按各分支代入端点。";
        }
        return parameterResult(r, f, conditions);
    }
    if (interval && (!std::isfinite(interval->first) || !std::isfinite(interval->second) ||
                     !entireForParameters(f)))
        throw std::runtime_error("尚不能确认含参数函数在整个积分区间的实数定义域；本阶段定积分支持"
                                 "整函数组合、常数分母和正区间幂函数");
    auto original = structure(f), m = original;
    if (m.integrate(x, eo, true, 1) <= 0 || unresolved(m))
        return {ResultKind::Unknown, "\\text{当前算法无法求出参数原函数}",
                "当前算法无法求出参数原函数",
                "不代表积分不存在；未将参数指定为任意数值代替符号计算。"};
    m.eval(eo);
    realPrimitiveLogs(m);
    auto delta = m;
    if (!delta.differentiate(x, eo))
        return {};
    scalarize(delta);
    auto normalizedOriginal = original;
    scalarize(normalizedOriginal);
    delta -= normalizedOriginal;
    auto verify = eo;
    verify.expand = true;
    delta.eval(verify);
    scalarize(delta);
    delta.eval(verify);
    if (!delta.isZero())
        return {ResultKind::Unknown, "\\text{参数原函数未完成恒等验证}", "参数原函数未完成恒等验证",
                "带参数的候选结果不以有限参数采样代替恒等验证。"};
    primitiveConditions(m, conditions);
    if (interval) {
        auto a = m, b = m;
        a.replace(x, cas().parse(decimal(interval->first)));
        b.replace(x, cas().parse(decimal(interval->second)));
        b -= a;
        b.eval(eo);
        m = b;
    }
    auto r = output(m);
    if (!interval) {
        r.formula += "+C";
        r.plain += " + C";
    }
    r.verification = Verification::Symbolic;
    r.detail =
        "原函数通过关于 x 的符号求导恒等验证。参数分母非零条件已列出，条件不满足时需要单独计算。";
    auto names = parameters(f);
    if (names.size() == 1 && conditions.items.size() == 1 &&
        conditions.items[0] == names[0] + "\\ne0") {
        try {
            auto bound = bindParameters(f, {{names[0], 0}});
            ParameterConditions zeroConditions;
            sourceConditions(bound, zeroConditions);
            if (zeroConditions.items.empty()) {
                auto originalZero = structure(bound), primitiveZero = originalZero;
                if (primitiveZero.integrate(x, eo, true, 1) > 0 && !unresolved(primitiveZero)) {
                    auto verifyZero = primitiveZero;
                    if (verifyZero.differentiate(x, eo)) {
                        scalarize(verifyZero);
                        scalarize(originalZero);
                        verifyZero -= originalZero;
                        verifyZero.eval(verify);
                        if (verifyZero.isZero()) {
                            if (interval) {
                                auto a = primitiveZero, b = primitiveZero;
                                a.replace(x, cas().parse(decimal(interval->first)));
                                b.replace(x, cas().parse(decimal(interval->second)));
                                b -= a;
                                b.eval(eo);
                                primitiveZero = b;
                            }
                            auto special = output(primitiveZero);
                            if (!interval)
                                special.formula += "+C";
                            r.formula = "\\begin{cases}" + r.formula + " & " + names[0] +
                                        "\\ne0\\\\" + special.formula + " & " + names[0] +
                                        "=0\\end{cases}";
                            r.kind = ResultKind::Conditional;
                            r.numeric.reset();
                            r.plain += "；参数为0时已单独求原函数并验证。";
                            r.detail += " 参数为0的退化分支已单独积分并通过求导验证。";
                            conditions.items.clear();
                        }
                    }
                }
            }
        } catch (const std::runtime_error &) { /* Keep only the verified conditional branch. */
        }
    }
    return parameterResult(r, f, conditions);
}
// A sufficient local analyticity certificate for each fixed admissible real
// parameter tuple. Positivity/nonzero conditions at the center imply a local
// neighborhood by continuity. They need not hold uniformly over all parameters.
bool parameterAnalytic(Expr f, double center, ParameterConditions &conditions) {
    if (!dependsX(f)) {
        sourceConditions(f, conditions);
        return true;
    }
    if (!f->a)
        return true;
    if (!parameterAnalytic(f->a, center, conditions) ||
        (f->b && !parameterAnalytic(f->b, center, conditions)))
        return false;
    auto at = [&](Expr e) {
        auto m = structure(e);
        m.replace(xvar(), cas().parse(decimal(center)));
        scalarize(m);
        return m;
    };
    auto o = f->op;
    if (o == "/" || o == "ln" || o == "log" || o == "root" || o == "^" || o == "tan") {
        if (o == "/")
            conditions.add(at(f->b));
        else if (o == "ln" || o == "log" || o == "root") {
            conditions.add(at(f->a), true);
            if (o == "root")
                conditions.add(at(f->b));
        } else if (o == "tan")
            conditions.add(at(make("cos", f->a)));
        else if (!(f->b->op == "num" && f->b->value >= 0 && std::trunc(f->b->value) == f->b->value))
            conditions.add(at(f->a), true);
        return true;
    }
    return o == "+" || o == "-" || o == "*" || o == "sin" || o == "cos" || o == "exp" ||
           o == "atan";
}
CalculusResult parameterLimit(Expr f, double center, int direction) {
    std::lock_guard lock(casMutex);
    Control control;
    ParameterConditions conditions;
    if (f->op == "^" && f->a->op == "x" && !dependsX(f->b) &&
        ((center == 0 && direction == 1) || center == INFINITY)) {
        sourceConditions(f->b, conditions);
        auto p = tex(structure(f->b));
        bool zero = center == 0;
        CalculusResult r{ResultKind::Conditional,
                         "\\begin{cases}0 & " + p + (zero ? ">0" : "<0") + "\\\\1 & " + p +
                             "=0\\\\+\\infty & " + p + (zero ? "<0" : ">0") + "\\end{cases}",
                         "实数幂极限按指数的正、零、负分情况。",
                         "在 x>0 上使用实数幂的单调性与基本极限规则。"};
        return parameterResult(r, f, conditions);
    }
    struct Unsupported {};
    struct Fraction {
        Expr n, d;
        size_t ns = 1, ds = 1;
    };
    auto pack = [](Fraction r) {
        if (r.ns + r.ds > 12000)
            throw Unsupported{};
        return r;
    };
    auto node = [](std::string op, Expr a, Expr b) -> Expr {
        return std::make_shared<Node>(Node{op, 0, a, b});
    };
    Expr working = f;
    if (std::isinf(center)) {
        std::function<Expr(Expr)> invert = [&](Expr e) -> Expr {
            if (e->op == "x")
                return node("/", number(1), variable());
            if (!e->a)
                return e;
            return node(e->op, invert(e->a), e->b ? invert(e->b) : Expr{});
        };
        working = invert(f);
        direction = center > 0 ? 1 : -1;
        center = 0;
    }
    std::vector<Expr> divisors;
    int visited = 0;
    std::function<Fraction(Expr)> normalize = [&](Expr e) -> Fraction {
        if (++visited > 512)
            throw Unsupported{};
        auto op = e->op;
        if (op == "^" && e->b->op == "num" && std::trunc(e->b->value) == e->b->value &&
            std::abs(e->b->value) <= 12) {
            auto a = normalize(e->a);
            int k = int(e->b->value);
            if (k < 0) {
                divisors.push_back(a.n);
                std::swap(a.n, a.d);
                std::swap(a.ns, a.ds);
                k = -k;
            }
            // Keep the source base domain even for power zero.
            return pack({node("^", a.n, number(k)), node("^", a.d, number(k)), a.ns + 2, a.ds + 2});
        }
        if (op != "+" && op != "-" && op != "*" && op != "/") {
            if (!parameterAnalytic(e, center, conditions))
                throw Unsupported{};
            return {e, number(1)};
        }
        auto a = normalize(e->a), b = normalize(e->b);
        if (op == "+" || op == "-")
            return pack({node(op, node("*", a.n, b.d), node("*", b.n, a.d)), node("*", a.d, b.d),
                         a.ns + b.ds + b.ns + a.ds + 3, a.ds + b.ds + 1});
        if (op == "*")
            return pack(
                {node("*", a.n, b.n), node("*", a.d, b.d), a.ns + b.ns + 1, a.ds + b.ds + 1});
        divisors.push_back(b.n);
        return pack({node("*", a.n, b.d), node("*", a.d, b.n), a.ns + b.ds + 1, a.ds + b.ns + 1});
    };
    auto eo = options();
    auto atCenter = cas().parse(decimal(center));
    auto coefficient = [&](MathStructure m, int k) {
        for (int j = 0; j < k; ++j) {
            if (!m.differentiate(xvar(), eo))
                throw Unsupported{};
            scalarize(m);
        }
        m.replace(xvar(), atCenter);
        scalarize(m);
        long factorial = 1;
        for (int j = 2; j <= k; ++j)
            factorial *= j;
        m /= MathStructure(int(factorial));
        m.eval(eo);
        return m;
    };
    auto leading = [&](Expr e) -> std::pair<int, MathStructure> {
        auto m = structure(e);
        for (int k = 0; k <= 12; ++k) {
            auto c = coefficient(m, k);
            if (!c.isZero()) {
                conditions.add(c);
                return {k, c};
            }
        }
        throw Unsupported{};
    };
    try {
        auto fraction = normalize(working);
        for (auto d : divisors)
            leading(d);
        auto [degree, denominator] = leading(fraction.d);
        auto numerator = structure(fraction.n);
        for (int k = 0; k < degree; ++k)
            if (!coefficient(numerator, k).isZero())
                throw Unsupported{};
        auto m = coefficient(numerator, degree);
        m /= denominator;
        m.eval(eo);
        auto r = output(m);
        r.detail = "对每组满足条件的固定实数参数，确认局部解析性及分母首项，再比较 Taylor "
                   "系数（最高12阶）；没有使用参数取样或函数采样证明极限。";
        return parameterResult(r, f, conditions);
    } catch (const Unsupported &) {
        return {ResultKind::Unknown, "\\text{当前参数极限无法判定}", "当前参数极限无法判定",
                "参数符号、退化分支、单侧实数定义域或振荡性尚未确认；没有用参数采样猜测结果。"};
    } catch (const std::runtime_error &) {
        return {ResultKind::Unknown, "\\text{当前参数极限无法判定}", "当前参数极限无法判定",
                "尚不能建立足够的实数定义域或参数条件。"};
    }
}
} // namespace
