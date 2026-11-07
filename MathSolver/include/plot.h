#pragma once
#include "mathsolver.h"
#include <QPointF>
#include <QWidget>
#include <functional>
class Plot : public QWidget {
    std::vector<ms::Expr> curves;
    std::vector<ms::Step> steps;
    ms::Expr experiment;
    std::string experimentKind;
    int partitions = 0;
    double integralA = 0, integralB = 1;
    double xmin = -5, xmax = 5, ymin = -3, ymax = 3;
    QPoint last;
    QPoint pressed;
    bool dragged = false;
    ms::Expr tangent;
    std::vector<QPointF> markers;
    QPointF screen(double, double) const;

  protected:
    void paintEvent(QPaintEvent *) override;
    void wheelEvent(QWheelEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;

  public:
    explicit Plot(QWidget *parent = nullptr);
    std::function<void(int, double)> pointSelected;
    void setTangent(ms::Expr);
    QPointF positionForPoint(double x, double y) const {
        return screen(x, y);
    }
    void setCurves(std::vector<ms::Expr>);
    void setRange(double, double, double, double);
    void setMarkers(std::vector<QPointF>);
    void setExperiment(ms::Expr, std::string, const std::vector<ms::Step> &, double = 0, double = 1,
                       int = 0);
};
class ErrorPlot : public QWidget {
    std::vector<ms::Step> data;

  protected:
    void paintEvent(QPaintEvent *) override;

  public:
    explicit ErrorPlot(QWidget *p = nullptr) : QWidget(p) {
        setMinimumSize(320, 260);
    }
    void setData(std::vector<ms::Step> d) {
        data = std::move(d);
        update();
    }
};
