#pragma once
#include "plot.h"
#include <QFutureWatcher>
#include <QJsonObject>
#include <QMainWindow>
class QListWidget;
class QPlainTextEdit;
class QLineEdit;
class QComboBox;
class QSpinBox;
class QDoubleSpinBox;
class QPushButton;
class QWebEngineView;
class QTableWidget;
class QLabel;
class QToolButton;
class QStackedWidget;
class QAbstractAnimation;
class QDialog;
struct TangentResult {
    ms::Expr line;
    QString formula, message;
    double x = 0, y = 0, slope = 0;
};
struct Calculation {
    QString message, formula, caption, numeric;
    std::vector<ms::Expr> curves;
    std::vector<ms::Step> steps;
    std::vector<QPointF> markers;
    ms::Expr experiment;
    std::string kind;
    double a = 0, b = 1;
    int n = 0;
};
class Window : public QMainWindow {
    QListWidget *nav;
    QPlainTextEdit *input, *output;
    QWebEngineView *preview, *resultPreview;
    QLineEdit *point;
    QDoubleSpinBox *lower, *upper, *tolerance, *xlo, *xhi, *ylo, *yhi;
    QSpinBox *order, *iterations, *parts;
    QComboBox *method, *direction, *templates, *integralType;
    QPushButton *calculate;
    QTableWidget *table;
    QComboBox *history;
    Plot *plot;
    ErrorPlot *errorPlot;
    QLabel *status;
    QLabel *resultCaption, *numericLabel;
    QToolButton *visualToggle;
    QFutureWatcher<Calculation> watcher;
    QString lastFormula;
    QJsonObject pendingRequest;
    QJsonObject requestSnapshot() const;
    bool dark = true;
    enum Page { CalculatorPage, HomePage, LearnPage, AboutPage, GraphPage, IntroPage };
    QStackedWidget *pages = nullptr;
    QWidget *appHeader = nullptr, *calculatorSegments = nullptr;
    QPushButton *moreTools = nullptr;
    QToolButton *settingsButton = nullptr;
    QAbstractAnimation *introAnimation = nullptr;
    QListWidget *recentList = nullptr;
    QDialog *historyWindow = nullptr;
    QPlainTextEdit *graphInput = nullptr;
    QWebEngineView *graphPreview = nullptr, *tangentPreview = nullptr;
    Plot *graphPlot = nullptr;
    QLabel *graphStatus = nullptr, *tangentLabel = nullptr;
    QComboBox *graphSelection = nullptr;
    QDoubleSpinBox *graphPoint = nullptr;
    std::vector<ms::Expr> graphCurves;
    QStringList graphSources;
    QFutureWatcher<TangentResult> tangentWatcher;
    int graphRevision = 0, tangentRevision = 0;
    void buildPages();
    void showPage(int);
    void openCalculator(int mode = 1);
    void drawGraph();
    void selectGraphPoint(int, double);
    void restoreHistory(const QJsonObject &);
    void recordHistory(QJsonObject);
    void refreshRecent();
    void navigationSmoke(std::function<void(bool)>);
    void render(QWebEngineView *, const QString &);
    void refresh();
    void run();
    void saveHistory();
    void loadHistory();
    void theme();

  public:
    Window();
    ~Window() override;
    bool smokeOk = false;
    void selfTest(std::function<void(bool)>);
};
