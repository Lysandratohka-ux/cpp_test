#include "plot.h"
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QWheelEvent>
#include <cmath>
Plot::Plot(QWidget *p) : QWidget(p) {
    setMinimumSize(320, 260);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setToolTip("滚轮缩放 · 拖动平移 · 曲线颜色按输入行排列");
}
QPointF Plot::screen(double x, double y) const {
    return {45 + (x - xmin) / (xmax - xmin) * (width() - 65),
            20 + (ymax - y) / (ymax - ymin) * (height() - 55)};
}
void Plot::setCurves(std::vector<ms::Expr> c) {
    curves = std::move(c);
    steps.clear();
    experiment.reset();
    markers.clear();
    tangent.reset();
    update();
}
void Plot::setTangent(ms::Expr line) {
    tangent = std::move(line);
    update();
}
void Plot::setRange(double a, double b, double c, double d) {
    if (a < b && c < d) {
        xmin = a;
        xmax = b;
        ymin = c;
        ymax = d;
        update();
    }
}
void Plot::setMarkers(std::vector<QPointF> m) {
    markers = std::move(m);
    update();
}
void Plot::setExperiment(ms::Expr e, std::string kind, const std::vector<ms::Step> &s, double a,
                         double b, int n) {
    experiment = e;
    experimentKind = kind;
    steps = s;
    integralA = a;
    integralB = b;
    partitions = n;
    update();
}
void Plot::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), QColor("#111b2d"));
    p.setPen(QColor("#23344c"));
    for (int i = 0; i <= 10; ++i) {
        double x = xmin + (xmax - xmin) * i / 10, y = ymin + (ymax - ymin) * i / 10;
        p.drawLine(screen(x, ymin), screen(x, ymax));
        p.drawLine(screen(xmin, y), screen(xmax, y));
        p.setPen(QColor("#90a4bd"));
        p.drawText(QPointF(screen(x, ymin).x() - 8, height() - 10), QString::number(x, 'g', 3));
        p.drawText(QPointF(2, screen(xmin, y).y() + 4), QString::number(y, 'g', 3));
        p.setPen(QColor("#23344c"));
    }
    p.setClipRect(QRect(45, 20, width() - 65, height() - 55));
    p.setPen(QPen(QColor("#8196b4"), 1));
    p.drawLine(screen(xmin, 0), screen(xmax, 0));
    p.drawLine(screen(0, ymin), screen(0, ymax));
    const QColor colors[] = {QColor("#63dfcc"), QColor("#f5b865"), QColor("#b9a3ff"),
                             QColor("#ff819b"), QColor("#84b8fb"), QColor("#c8db78"),
                             QColor("#ed9ada"), QColor("#bbc7d4")};
    int color = 0;
    for (auto f : curves) {
        p.setPen(QPen(colors[color++ % 8], 2));
        QPainterPath path;
        bool have = false;
        double prev = 0, px = 0;
        int samples = std::max(400, width() * 2);
        for (int i = 0; i <= samples; ++i) {
            double x = xmin + (xmax - xmin) * i / samples, y = ms::eval(f, x);
            bool valid =
                std::isfinite(y) && y > ymin - (ymax - ymin) * 2 && y < ymax + (ymax - ymin) * 2;
            if (valid && have) {
                double mid = ms::eval(f, (px + x) / 2);
                if (!std::isfinite(mid) || std::abs(y - prev) > (ymax - ymin) * .45 ||
                    std::abs(mid - (y + prev) / 2) > (ymax - ymin) * .1)
                    have = false;
            }
            if (valid) {
                if (have)
                    path.lineTo(screen(x, y));
                else
                    path.moveTo(screen(x, y));
            }
            have = valid;
            prev = y;
            px = x;
        }
        p.drawPath(path);
    }
    if (tangent) {
        p.setPen(QPen(QColor("#bcd8f2"), 2, Qt::DashLine));
        double a = ms::eval(tangent, xmin), b = ms::eval(tangent, xmax);
        if (std::isfinite(a) && std::isfinite(b))
            p.drawLine(screen(xmin, a), screen(xmax, b));
    }
    if (experiment) {
        p.setPen(QPen(QColor("#f5b865"), 1));
        p.setBrush(QColor(245, 184, 101, 35));
        if (partitions > 0) {
            // Simpson uses one interpolating parabola per pair of subintervals.
            int n = experimentKind == "simpson" ? std::min(partitions / 2, 200)
                                                : std::min(partitions, 200);
            double h = (integralB - integralA) / n;
            for (int i = 0; i < n; ++i) {
                double a = integralA + i * h, b = a + h, ya = ms::eval(experiment, a),
                       yb = ms::eval(experiment, b), ym = ms::eval(experiment, (a + b) / 2);
                if (!std::isfinite(ya) || !std::isfinite(yb) || !std::isfinite(ym))
                    continue;
                QPainterPath shape;
                shape.moveTo(screen(a, 0));
                if (experimentKind == "rectangle") {
                    shape.lineTo(screen(a, ym));
                    shape.lineTo(screen(b, ym));
                } else {
                    shape.lineTo(screen(a, ya));
                    if (experimentKind == "simpson") {
                        for (int k = 1; k <= 8; ++k) {
                            double t = k / 8.;
                            double y = ya * (2 * t * t - 3 * t + 1) + ym * (4 * t - 4 * t * t) +
                                       yb * (2 * t * t - t);
                            shape.lineTo(screen(a + t * h, y));
                        }
                    } else
                        shape.lineTo(screen(b, yb));
                }
                shape.lineTo(screen(b, 0));
                shape.closeSubpath();
                p.drawPath(shape);
            }
        }
        for (size_t i = 0; i < steps.size() && i < 80; ++i) {
            auto s = steps[i];
            if (experimentKind == "newton") {
                double d = ms::eval(ms::derivative(experiment), s.x), y = ms::eval(experiment, s.x);
                if (std::isfinite(d) && std::isfinite(y) && d != 0)
                    p.drawLine(screen(s.x, y), screen(s.x - y / d, 0));
            } else
                p.drawLine(screen(s.x, 0), screen(s.x, s.fx));
        }
    }
    p.setPen(QPen(QColor("#ff819b"), 2));
    p.setBrush(QColor("#ff819b"));
    for (auto pt : markers)
        p.drawEllipse(screen(pt.x(), pt.y()), 4, 4);
}
void Plot::wheelEvent(QWheelEvent *e) {
    double f = e->angleDelta().y() > 0 ? .8 : 1.25;
    double cx = (xmin + xmax) / 2, cy = (ymin + ymax) / 2;
    double hx = (xmax - xmin) * f / 2, hy = (ymax - ymin) * f / 2;
    if (hx > 1e-9 && hx < 1e9 && hy > 1e-9 && hy < 1e9)
        setRange(cx - hx, cx + hx, cy - hy, cy + hy);
}
void Plot::mousePressEvent(QMouseEvent *e) {
    last = e->pos();
    pressed = last;
    dragged = false;
}
void Plot::mouseMoveEvent(QMouseEvent *e) {
    if (e->buttons() & Qt::LeftButton) {
        if (!dragged && (e->pos() - pressed).manhattanLength() < 5)
            return;
        dragged = true;
        auto d = e->pos() - last;
        double dx = -d.x() * (xmax - xmin) / (width() - 65),
               dy = d.y() * (ymax - ymin) / (height() - 55);
        setRange(xmin + dx, xmax + dx, ymin + dy, ymax + dy);
        last = e->pos();
    }
}
void Plot::mouseReleaseEvent(QMouseEvent *e) {
    if (e->button() != Qt::LeftButton || dragged || !pointSelected ||
        !QRect(45, 20, width() - 65, height() - 55).contains(e->pos()))
        return;
    double x = xmin + (e->position().x() - 45) * (xmax - xmin) / (width() - 65);
    int selected = -1;
    double closest = 20;
    for (size_t i = 0; i < curves.size(); ++i) {
        double y = ms::eval(curves[i], x);
        if (!std::isfinite(y))
            continue;
        double distance = std::abs(screen(x, y).y() - e->position().y());
        if (distance < closest) {
            closest = distance;
            selected = int(i);
        }
    }
    if (selected >= 0)
        pointSelected(selected, x);
}
void ErrorPlot::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), QColor("#111b2d"));
    p.setPen(QColor("#c3d5e9"));
    p.drawText(20, 25, "收敛趋势：log10(误差指标)，不是严格的真实误差");
    std::vector<QPointF> points;
    for (auto s : data)
        if (s.error > 0 && std::isfinite(s.error))
            points.push_back({double(s.iteration), std::log10(s.error)});
    if (points.empty()) {
        p.drawText(30, 75, "求根或数值积分后显示误差指标趋势。");
        return;
    }
    double xmin = points.front().x(), xmax = points.back().x(), ymin = points.front().y(),
           ymax = ymin;
    for (auto s : points) {
        ymin = std::min(ymin, s.y());
        ymax = std::max(ymax, s.y());
    }
    if (xmin == xmax)
        xmax++;
    if (ymin == ymax)
        ymax++;
    auto pos = [&](QPointF s) {
        return QPointF(65 + (s.x() - xmin) / (xmax - xmin) * (width() - 95),
                       45 + (ymax - s.y()) / (ymax - ymin) * (height() - 90));
    };
    for (int i = 0; i <= 5; ++i) {
        double y = ymin + (ymax - ymin) * i / 5;
        p.setPen(QColor("#2c405c"));
        p.drawLine(QPointF(65, pos({xmin, y}).y()), QPointF(width() - 30, pos({xmin, y}).y()));
        p.setPen(QColor("#90a4bd"));
        p.drawText(QPointF(8, pos({xmin, y}).y() + 4), QString::number(y, 'f', 2));
    }
    p.drawText(65, height() - 15, QString::number(xmin));
    p.drawText(width() - 70, height() - 15, QString::number(xmax));
    QPainterPath path;
    path.moveTo(pos(points.front()));
    for (auto s : points)
        path.lineTo(pos(s));
    p.setPen(QPen(QColor("#63dfcc"), 2));
    p.drawPath(path);
    p.setBrush(QColor("#f5b865"));
    for (auto s : points)
        p.drawEllipse(pos(s), 3, 3);
}
