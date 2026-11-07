#include "window.h"
#include "calculus.h"
#include "domain.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QStandardPaths>
#include <QWebEnginePage>
#include <QWebEngineSettings>
#include <QWebEngineView>
#include <QtConcurrent>
#include <QtWidgets>
#include <algorithm>
#include <cmath>
static QString q(const std::string &s) {
    return QString::fromStdString(s);
}
static QString num(double v) {
    return QString::number(v, 'g', 13);
}
static double pointValue(QString s) {
    s = s.trimmed();
    if (s == "inf" || s == "+inf" || s == "∞")
        return INFINITY;
    if (s == "-inf" || s == "-∞")
        return -INFINITY;
    bool ok;
    double v = s.toDouble(&ok);
    if (!ok || !std::isfinite(v))
        throw std::runtime_error("请输入数字；无穷极限可使用 inf 或 -inf");
    return v;
}
Window::Window() {
    setWindowTitle("Infinity");
    resize(1120, 900);
    setMinimumSize(760, 640);
    auto root = new QWidget;
    auto rootLayout = new QVBoxLayout(root);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);
    setCentralWidget(root);
    nav = new QListWidget(this);
    nav->addItems(
        {"极限", "求导", "积分", "方程求根", "Taylor 展开", "函数绘图", "数值实验", "多项式化简"});
    nav->hide();
    auto header = new QFrame;
    header->setObjectName("header");
    auto hl = new QHBoxLayout(header);
    hl->setContentsMargins(16, 8, 16, 8);
    hl->setSpacing(5);
    auto logo = new QLabel;
    logo->setPixmap(QIcon(":/icons/infinity.png").pixmap(32, 32));
    hl->addWidget(logo);
    auto brand = new QPushButton("返回首页");
    brand->setObjectName("brandButton");
    brand->setToolTip("返回 Infinity 首页");
    brand->setAccessibleName("返回 Infinity 首页");
    connect(brand, &QPushButton::clicked, this, [this] { showPage(HomePage); });
    hl->addWidget(brand);
    auto brandName = new QLabel("Infinity");
    brandName->setObjectName("brandName");
    hl->addWidget(brandName);
    hl->addStretch();
    auto segmented = new QFrame;
    segmented->setObjectName("segments");
    auto sl = new QHBoxLayout(segmented);
    sl->setContentsMargins(4, 4, 4, 4);
    sl->setSpacing(3);
    auto group = new QButtonGroup(this);
    group->setExclusive(true);
    for (auto pair : {std::pair{1, "求导"}, std::pair{2, "积分"}, std::pair{0, "极限"}}) {
        auto btn = new QPushButton(pair.second);
        btn->setObjectName("mode");
        btn->setCheckable(true);
        btn->setMinimumWidth(64);
        group->addButton(btn, pair.first);
        sl->addWidget(btn);
    }
    hl->addWidget(segmented);
    hl->addStretch();
    auto more = new QPushButton("更多工具");
    more->setObjectName("quiet");
    auto menu = new QMenu(more);
    for (int i : {7, 4, 3, 5, 6}) {
        auto act = menu->addAction(nav->item(i)->text());
        connect(act, &QAction::triggered, this, [this, i] { nav->setCurrentRow(i); });
    }
    more->setMenu(menu);
    hl->addWidget(more);
    auto settings = new QToolButton;
    settings->setText("•••");
    settings->setToolTip("外观与设置");
    settings->setPopupMode(QToolButton::InstantPopup);
    auto sm = new QMenu(settings);
    auto light = sm->addAction("切换深色 / 浅色外观");
    connect(light, &QAction::triggered, this, [this] {
        dark = !dark;
        theme();
        refresh();
        render(resultPreview, lastFormula);
    });
    settings->setMenu(sm);
    menu->addSeparator();
    menu->addAction(light);
    hl->addWidget(settings);
    rootLayout->addWidget(header);
    appHeader = header;
    calculatorSegments = segmented;
    moreTools = more;
    settingsButton = settings;
    pages = new QStackedWidget;
    rootLayout->addWidget(pages, 1);
    auto scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    auto outer = new QWidget;
    auto ol = new QHBoxLayout(outer);
    ol->setContentsMargins(28, 16, 28, 24);
    ol->addStretch();
    auto content = new QWidget;
    content->setMaximumWidth(980);
    auto layout = new QVBoxLayout(content);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(10);
    ol->addWidget(content, 1);
    ol->addStretch();
    scroll->setWidget(outer);
    pages->addWidget(scroll);
    auto heading = new QLabel("一元函数求导");
    heading->setObjectName("heading");
    auto subtitle = new QLabel("从表达式，到清晰的数学结果。");
    subtitle->setObjectName("muted");
    layout->addWidget(heading);
    layout->addWidget(subtitle);
    auto editor = new QFrame;
    editor->setObjectName("card");
    auto el = new QVBoxLayout(editor);
    el->setContentsMargins(20, 10, 20, 8);
    el->setSpacing(4);
    auto er = new QHBoxLayout;
    auto label = new QLabel("数学表达式");
    label->setObjectName("eyebrow");
    er->addWidget(label);
    er->addStretch();
    templates = new QComboBox;
    templates->setObjectName("templates");
    templates->setMaximumWidth(180);
    templates->addItems({"插入示例…", "x^3+\\sin x", "\\frac{e^{x^2}\\sin x}{\\ln(1+x)}",
                         "\\int x e^{x^2}\\,dx", "\\frac{x^2+1}{x^4+1}", "\\frac{e^x-1-x}{x^2}",
                         "\\frac{\\sin x-x}{x^3}", "x\\sin\\frac1x", "\\ln(\\sec x+\\tan x)",
                         "\\csc^2 x", "\\arccot x", "\\sin(ax)/x", "x^a", "a x^2+b x+c"});
    er->addWidget(templates);
    auto clear = new QPushButton("清空");
    clear->setObjectName("quiet");
    er->addWidget(clear);
    el->addLayout(er);
    input = new QPlainTextEdit;
    input->setObjectName("expression");
    input->setPlaceholderText("输入 LaTeX；积分、极限支持实数参数 a、b、c、n，例如 \\sin(ax)/x");
    input->setFixedHeight(52);
    input->setTabChangesFocus(true);
    el->addWidget(input);
    auto line = new QFrame;
    line->setObjectName("hairline");
    line->setFixedHeight(1);
    el->addWidget(line);
    preview = new QWebEngineView;
    preview->setFixedHeight(96);
    el->addWidget(preview);
    layout->addWidget(editor);
    auto paramRow = new QWidget;
    auto pr = new QHBoxLayout(paramRow);
    pr->setContentsMargins(0, 0, 0, 0);
    pr->setSpacing(16);
    auto field = [](QString caption, QWidget *w) {
        auto box = new QWidget;
        auto l = new QVBoxLayout(box);
        l->setContentsMargins(0, 0, 0, 0);
        l->setSpacing(7);
        auto title = new QLabel(caption);
        title->setObjectName("muted");
        l->addWidget(title);
        l->addWidget(w);
        return box;
    };
    point = new QLineEdit;
    point->setMaximumWidth(180);
    point->setPlaceholderText("留空仅求导函数");
    auto pointBox = field("指定点（可选）", point);
    pointBox->setMaximumWidth(240);
    pr->addWidget(pointBox);
    order = new QSpinBox;
    order->setRange(1, 8);
    order->setValue(1);
    order->setFixedWidth(110);
    auto orderBox = field("求导阶数", order);
    orderBox->setMaximumWidth(140);
    pr->addWidget(orderBox);
    direction = new QComboBox;
    direction->addItems({"双侧", "从左侧趋近", "从右侧趋近"});
    auto directionBox = field("趋近方向", direction);
    pr->addWidget(directionBox);
    integralType = new QComboBox;
    integralType->addItems({"不定积分", "定积分"});
    auto integralBox = field("积分类型", integralType);
    pr->addWidget(integralBox);
    lower = new QDoubleSpinBox;
    upper = new QDoubleSpinBox;
    for (auto w : {lower, upper}) {
        w->setRange(-1e6, 1e6);
        w->setDecimals(6);
        w->setMaximumWidth(160);
    }
    upper->setValue(1);
    auto lowerBox = field("下限", lower), upperBox = field("上限", upper);
    pr->addWidget(lowerBox);
    pr->addWidget(upperBox);
    method = new QComboBox;
    method->addItems({"二分法", "Newton 法", "符号不定积分", "符号定积分", "中点矩形法", "梯形法",
                      "Simpson 法"});
    auto methodBox = field("算法", method);
    pr->addWidget(methodBox);
    pr->addStretch();
    layout->addWidget(paramRow);
    status = new QLabel("LaTeX · 单变量实函数 · 弧度制");
    status->setObjectName("status");
    status->setWordWrap(true);
    calculate = new QPushButton("计算");
    calculate->setObjectName("primary");
    calculate->setMinimumSize(142, 44);
    calculate->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Return));
#ifdef Q_OS_MACOS
    calculate->setToolTip("⌘ Return 计算 · ⌘ L 编辑表达式");
#else
    calculate->setToolTip("Ctrl+Enter 计算 · Ctrl+L 编辑表达式");
#endif
    pr->addWidget(calculate, 0, Qt::AlignBottom);
    layout->addWidget(status);
    auto resultCard = new QFrame;
    resultCard->hide();
    resultCard->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    resultCard->setObjectName("resultCard");
    auto rl = new QVBoxLayout(resultCard);
    rl->setContentsMargins(20, 10, 20, 10);
    auto rr = new QHBoxLayout;
    resultCaption = new QLabel("符号结果");
    resultCaption->setObjectName("eyebrow");
    rr->addWidget(resultCaption);
    rr->addStretch();
    auto copy = new QPushButton("复制 LaTeX");
    copy->setObjectName("quiet");
    rr->addWidget(copy);
    rl->addLayout(rr);
    resultPreview = new QWebEngineView;
    resultPreview->setFixedHeight(96);
    rl->addWidget(resultPreview);
    numericLabel = new QLabel;
    numericLabel->setObjectName("numeric");
    numericLabel->setWordWrap(true);
    numericLabel->hide();
    rl->addWidget(numericLabel);
    layout->addWidget(resultCard);
    auto fold = [&](QString title, QWidget *body) {
        auto button = new QToolButton;
        button->setText(title);
        button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        button->setArrowType(Qt::RightArrow);
        button->setCheckable(true);
        button->setObjectName("disclosure");
        body->hide();
        layout->addWidget(button);
        layout->addWidget(body);
        connect(button, &QToolButton::toggled, this, [=](bool on) {
            button->setArrowType(on ? Qt::DownArrow : Qt::RightArrow);
            body->setVisible(on);
        });
        return button;
    };
    output = new QPlainTextEdit;
    output->setReadOnly(true);
    output->setMinimumHeight(110);
    output->setMaximumHeight(180);
    auto methodFold = fold("方法与适用条件", output);
    auto visual = new QWidget;
    auto vl = new QVBoxLayout(visual);
    vl->setContentsMargins(0, 0, 0, 0);
    auto exportRow = new QHBoxLayout;
    auto png = new QPushButton("导出图像"), csv = new QPushButton("导出数据");
    exportRow->addStretch();
    exportRow->addWidget(png);
    exportRow->addWidget(csv);
    vl->addLayout(exportRow);
    plot = new Plot;
    plot->setMinimumHeight(300);
    errorPlot = new ErrorPlot;
    auto tabs = new QTabWidget;
    tabs->addTab(plot, "函数");
    tabs->addTab(errorPlot, "误差指标");
    vl->addWidget(tabs);
    table = new QTableWidget(0, 4);
    table->setHorizontalHeaderLabels({"迭代 / 分割数", "x / 积分值", "残差 / 相邻差", "误差指标"});
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->setMaximumHeight(190);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    vl->addWidget(table);
    visualToggle = fold("曲线与数值过程", visual);
    auto advanced = new QWidget;
    auto af = new QFormLayout(advanced);
    af->setContentsMargins(4, 4, 4, 4);
    tolerance = new QDoubleSpinBox;
    tolerance->setDecimals(12);
    tolerance->setRange(1e-12, .1);
    tolerance->setValue(1e-8);
    iterations = new QSpinBox;
    iterations->setRange(1, 10000);
    iterations->setValue(100);
    parts = new QSpinBox;
    parts->setRange(2, 1000000);
    parts->setValue(100);
    af->addRow("数值精度", tolerance);
    af->addRow("最大迭代", iterations);
    af->addRow("积分分割数", parts);
    auto rangeBox = [](double v) {
        auto s = new QDoubleSpinBox;
        s->setRange(-1e6, 1e6);
        s->setDecimals(4);
        s->setValue(v);
        return s;
    };
    xlo = rangeBox(-5);
    xhi = rangeBox(5);
    ylo = rangeBox(-3);
    yhi = rangeBox(3);
    auto ranges = new QWidget;
    auto rg = new QHBoxLayout(ranges);
    rg->setContentsMargins(0, 0, 0, 0);
    for (auto w : {xlo, xhi, ylo, yhi})
        rg->addWidget(w);
    af->addRow("绘图范围 x / y", ranges);
    auto rangeBtn = new QPushButton("应用范围");
    af->addRow(rangeBtn);
    fold("高级参数", advanced);
    history = new QComboBox;
    history->addItem("选择历史计算…");
    fold("最近计算", history);
    layout->addStretch(1);
    connect(group, &QButtonGroup::idClicked, this, [this](int i) { nav->setCurrentRow(i); });
    auto showMode = [=, this](int i) {
        group->setExclusive(false);
        for (auto btn : group->buttons())
            btn->setChecked(group->id(btn) == i);
        group->setExclusive(true);
        heading->setText(i == 1   ? "一元函数求导"
                         : i == 2 ? "一元函数积分"
                         : i == 0 ? "一元函数极限"
                                  : nav->item(i)->text());
        subtitle->setText(i < 3 ? "专注于表达式本身，让数学自然呈现。"
                                : "更多工具 · 保留完整的数值计算与可视化能力");
        pointBox->setVisible(i == 0 || i == 1 || i == 3 || i == 4);
        pointBox->findChild<QLabel *>()->setText(i == 0   ? "趋近点"
                                                 : i == 3 ? "初始值"
                                                 : i == 4 ? "展开点"
                                                          : "指定点（可选）");
        point->setPlaceholderText(i == 0 ? "0、inf 或 -inf" : i == 1 ? "留空仅求导函数" : "0");
        if ((i == 0 || i == 3 || i == 4) && point->text().isEmpty())
            point->setText("0");
        orderBox->setVisible(i == 1 || i == 4);
        orderBox->findChild<QLabel *>()->setText(i == 4 ? "展开阶数" : "求导阶数");
        directionBox->setVisible(i == 0);
        integralBox->setVisible(i == 2);
        methodBox->setVisible(i == 3 || i == 6);
        bool interval = i == 3 || i == 6 || (i == 2 && integralType->currentIndex() == 1);
        lowerBox->setVisible(interval);
        upperBox->setVisible(interval);
        if (i == 2)
            method->setCurrentIndex(integralType->currentIndex() == 0 ? 2 : 3);
        if (i == 3 || i == 6)
            method->setCurrentIndex(0);
        visualToggle->setChecked(i >= 3 && i != 7);
        methodFold->setChecked(false);
    };
    connect(nav, &QListWidget::currentRowChanged, this, showMode);
    connect(integralType, &QComboBox::currentIndexChanged, this, [=, this](int i) {
        if (nav->currentRow() == 2) {
            method->setCurrentIndex(i ? 3 : 2);
            lowerBox->setVisible(i);
            upperBox->setVisible(i);
        }
    });
    auto previewTimer = new QTimer(this);
    previewTimer->setSingleShot(true);
    previewTimer->setInterval(220);
    connect(input, &QPlainTextEdit::textChanged, this, [=, this] {
        previewTimer->start();
        status->setText("编辑中 · 按计算更新结果");
        if (input->toPlainText().trimmed().isEmpty()) {
            output->clear();
            lastFormula.clear();
            resultCard->hide();
            numericLabel->hide();
            plot->setCurves({});
            render(resultPreview, "");
        }
    });
    connect(previewTimer, &QTimer::timeout, this, &Window::refresh);
    connect(templates, &QComboBox::activated, this, [this](int i) {
        if (i)
            input->setPlainText(templates->itemText(i));
    });
    connect(clear, &QPushButton::clicked, this, [this] {
        input->clear();
        input->setFocus();
    });
    connect(calculate, &QPushButton::clicked, this, &Window::run);
    auto focusAction = new QAction(this);
    focusAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L));
    addAction(focusAction);
    connect(focusAction, &QAction::triggered, this, [this] {
        auto editor = pages->currentIndex() == GraphPage ? graphInput : input;
        if (pages->currentIndex() != GraphPage && pages->currentIndex() != CalculatorPage)
            openCalculator();
        editor->setFocus();
        editor->selectAll();
    });
    connect(preview, &QWebEngineView::loadFinished, this, [this](bool ok) {
        if (ok)
            preview->page()->runJavaScript("!!document.querySelector('.katex')",
                                           [this](const QVariant &v) { smokeOk = v.toBool(); });
    });
    connect(copy, &QPushButton::clicked, this, [this, copy] {
        QApplication::clipboard()->setText(lastFormula);
        copy->setText("已复制");
        QTimer::singleShot(1200, copy, [copy] { copy->setText("复制 LaTeX"); });
    });
    connect(rangeBtn, &QPushButton::clicked, this, [this] {
        if (xlo->value() >= xhi->value() || ylo->value() >= yhi->value()) {
            status->setText("坐标下限必须小于上限");
            return;
        }
        plot->setRange(xlo->value(), xhi->value(), ylo->value(), yhi->value());
    });
    connect(png, &QPushButton::clicked, this, [this] {
        auto path = QFileDialog::getSaveFileName(this, "导出图像", "Infinity.png", "PNG (*.png)");
        if (!path.isEmpty())
            status->setText(plot->grab().save(path) ? "图像已导出" : "图像导出失败");
    });
    connect(csv, &QPushButton::clicked, this, [this] {
        auto path = QFileDialog::getSaveFileName(this, "导出数据", "Infinity.csv", "CSV (*.csv)");
        if (path.isEmpty())
            return;
        QSaveFile f(path);
        if (!f.open(QIODevice::WriteOnly)) {
            status->setText("无法写入文件");
            return;
        }
        QTextStream s(&f);
        s << "iteration,x_or_integral,residual,error\n";
        for (int row = 0; row < table->rowCount(); ++row) {
            for (int col = 0; col < 4; ++col) {
                if (col)
                    s << ',';
                if (auto item = table->item(row, col))
                    s << item->text();
            }
            s << '\n';
        }
        status->setText(f.commit() ? "CSV 已导出" : "导出失败");
    });
    connect(history, &QComboBox::activated, this, [this](int i) {
        if (i <= 0)
            return;
        restoreHistory(history->itemData(i).toJsonObject());
    });
    connect(&watcher, &QFutureWatcher<Calculation>::finished, this, [this, methodFold, resultCard] {
        auto v = watcher.result();
        calculate->setEnabled(true);
        status->setText(v.message.startsWith("错误：") ? v.message : "已完成 · 结果已更新");
        if (pendingRequest != requestSnapshot())
            status->setText("输入或模式已修改 · 当前为上次结果，请重新计算");
        output->setPlainText(v.message);
        resultCard->show();
        lastFormula = v.formula;
        render(resultPreview, v.formula);
        resultCaption->setText(v.caption.isEmpty() ? "计算结果" : v.caption);
        numericLabel->setText(v.numeric);
        numericLabel->setVisible(!v.numeric.isEmpty());
        errorPlot->setData(v.steps);
        plot->setCurves(v.curves);
        plot->setMarkers(v.markers);
        if (v.experiment)
            plot->setExperiment(v.experiment, v.kind, v.steps, v.a, v.b, v.n);
        table->setRowCount(int(v.steps.size()));
        for (int i = 0; i < int(v.steps.size()); ++i) {
            auto s = v.steps[i];
            QStringList cells{QString::number(s.iteration), num(s.x), num(s.fx), num(s.error)};
            for (int j = 0; j < 4; ++j)
                table->setItem(i, j, new QTableWidgetItem(cells[j]));
        }
        if (v.message.startsWith("错误："))
            methodFold->setChecked(true);
        if (!v.message.startsWith("错误："))
            saveHistory();
    });
    nav->setCurrentRow(1);
    buildPages();
    loadHistory();
    refreshRecent();
    theme();
    refresh();
    render(resultPreview, "");
}
Window::~Window() {
    watcher.waitForFinished();
    tangentWatcher.waitForFinished();
}
void Window::theme() {
    setProperty("darkTheme", dark);
    QString bg = dark ? "#10151d" : "#f3f5f8", fg = dark ? "#e5ebf3" : "#172334",
            card = dark ? "#19212c" : "#ffffff", border = dark ? "#2b3543" : "#d8e0e8",
            muted = dark ? "#96a5b7" : "#53667b";
    setStyleSheet(
        QString(
            "QWidget{background:%1;color:%2;font-family:'.AppleSystemUIFont','Segoe "
            "UI';font-size:13px;}QFrame#header{background:%3;border-bottom:1px solid "
            "%4;}QPushButton#brandButton{font-size:14px;font-weight:600;border:0;background:"
            "transparent;padding:8px 10px;}QPushButton#brandButton:hover{background:%4;}"
            "QLabel#brandName{font-size:18px;font-weight:600;padding:0 8px;}"
            "QLabel#heading{font-size:27px;font-"
            "weight:600;letter-spacing:-0.6px;}QLabel#muted,QLabel#status{color:%5;}QLabel#eyebrow{"
            "color:%5;font-size:12px;font-weight:600;}QFrame#card,QFrame#resultCard{background:%3;"
            "border:1px solid %4;border-radius:17px;}QFrame#card QWidget,QFrame#resultCard "
            "QWidget{background:transparent;}QFrame#segments{background:%1;border:1px solid "
            "%4;border-radius:11px;}QPushButton#mode{border:0;background:transparent;color:%5;"
            "padding:8px "
            "10px;border-radius:7px;}QPushButton#mode:checked{background:%4;color:%2;}QPushButton,"
            "QToolButton{border:1px solid %4;border-radius:8px;background:%3;padding:8px "
            "12px;}QPushButton:hover,QToolButton:hover{background:%4;}QPushButton#primary,"
            "QPushButton#drawGraph{"
            "background:#aac7e3;color:#102035;border:0;font-weight:600;font-size:14px;}QPushButton#"
            "primary:hover,QPushButton#drawGraph:hover{background:#c0d7ed;}QPushButton#primary:"
            "disabled{background:%4;color:%5;"
            "}QPushButton#quiet,QToolButton#disclosure{border:0;background:transparent;color:%5;}"
            "QToolButton#disclosure{text-align:left;padding:5px "
            "0;}QLineEdit,QSpinBox,QDoubleSpinBox,QComboBox{background:%3;border:1px solid "
            "%4;border-radius:8px;padding:8px "
            "10px;selection-background-color:#426889;}QPlainTextEdit{background:%3;border:1px "
            "solid "
            "%4;border-radius:9px;padding:10px;}QPlainTextEdit#expression{border:0;font-family:'SF "
            "Mono','Cascadia "
            "Code',monospace;font-size:16px;padding:4px;}QLabel#numeric{color:%5;padding-top:6px;}"
            "QFrame#hairline{background:%4;}QScrollArea{border:0;}QTableWidget{background:%3;"
            "border:1px solid "
            "%4;gridline-color:%4;}QHeaderView::section{background:%3;border:0;padding:8px;}QMenu{"
            "background:%3;border:1px solid %4;padding:6px;}QMenu::item{padding:8px "
            "18px;}QMenu::item:selected{background:%4;}QTabWidget::pane{border:1px solid "
            "%4;border-radius:8px;}QLabel{background:transparent;}QToolButton::menu-indicator,"
            "QPushButton::menu-indicator{image:none;}"
            "QScrollBar:vertical{background:transparent;width:8px;margin:2px;}QScrollBar::handle:"
            "vertical{background:%4;border-radius:4px;min-height:40px;}"
            "QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical{height:0px;}QScrollBar::"
            "add-page:vertical,QScrollBar::sub-page:vertical{background:none;}"
            "QLabel#heroTitle{font-size:40px;font-weight:600;letter-spacing:-1px;}"
            "QLabel#heroSubtitle{color:%5;font-size:15px;}"
            "QListWidget#recentList{background:transparent;border:0;outline:0;}"
            "QListWidget#recentList::item{padding:13px 15px;border-bottom:1px solid "
            "%4;border-radius:8px;}"
            "QListWidget#recentList::item:hover{background:%4;}"
            "QToolButton#navText{border:0;background:transparent;padding:8px 12px;}"
            "QToolButton#navText:hover{background:%4;}"
            "QTextBrowser#guideText{background:transparent;border:0;font-size:15px;}")
            .arg(bg, fg, card, border, muted));
    for (auto article : findChildren<QTextBrowser *>())
        article->document()->setDefaultStyleSheet(
            QString("a{color:%1;text-decoration:none;}h3{margin-top:22px;}p{line-height:145%;}code{"
                    "font-family:monospace;}")
                .arg(dark ? "#a6c8e8" : "#315d89"));
    if (graphPreview) {
        render(graphPreview,
               graphInput->toPlainText().split('\n', Qt::SkipEmptyParts).join("\\quad;\\quad"));
        render(tangentPreview, tangentPreview->property("pendingFormula").toString());
    }
    if (pages)
        pages->update();
}
void Window::render(QWebEngineView *v, const QString &formula) {
    v->setProperty("pendingFormula", formula);
    v->setProperty("formulaRendered", false);
    QString bg = dark ? "#19212c" : "#ffffff", fg = dark ? "#e5ebf3" : "#172334";
    v->page()->setBackgroundColor(QColor(bg));
    if (v->property("formulaReady").toBool()) {
        QJsonArray data{formula, bg, fg};
        QString json = QString::fromUtf8(QJsonDocument(data).toJson(QJsonDocument::Compact));
        json.replace("<", "\\u003c");
        v->page()->runJavaScript("window.renderFormula(..." + json + ")",
                                 [this, v, formula](const QVariant &value) {
                                     auto result = value.toMap();
                                     v->setProperty("formulaRendered", result["ok"].toBool());
                                     v->setProperty("renderedFormula", formula);
                                     v->update();
                                     v->setFixedHeight(qBound(v == preview ? 72 : 88,
                                                              result["height"].toInt() + 10, 200));
                                     if (v == preview)
                                         smokeOk = result["ok"].toBool();
                                 });
        return;
    }
    if (v->property("formulaLoading").toBool())
        return;
    v->setProperty("formulaLoading", true);
    QString dir = QCoreApplication::applicationDirPath() + "/../Resources/latex";
    if (!QFile::exists(dir + "/katex.min.js"))
        dir = QCoreApplication::applicationDirPath() + "/resources/latex";
    if (!QFile::exists(dir + "/katex.min.js"))
        dir = RESOURCE_DIR;
    connect(v, &QWebEngineView::loadFinished, this, [this, v](bool ok) {
        if (ok) {
            v->setProperty("formulaReady", true);
            render(v, v->property("pendingFormula").toString());
        }
    });
    QString html =
        R"HTML(<!doctype html><html><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><link rel="stylesheet" href="katex.min.css"><style>html,body{margin:0;padding:0;overflow-x:auto;overflow-y:auto}body{font-size:22px}.katex-display{padding:4px 8px;margin:4px 0;overflow-x:auto;overflow-y:hidden}#formula{display:flow-root;min-height:58px}</style></head><body><div id="formula"></div><script src="katex.min.js"></script><script>window.renderFormula=function(s,bg,fg){document.body.style.background=bg;document.body.style.color=fg;try{katex.render(s,document.getElementById('formula'),{throwOnError:true,displayMode:true,trust:false,macros:{'\\arccot':'\\operatorname{arccot}'},strict:'error',maxExpand:100,maxSize:20});window.currentLatex=s;return {ok:true,height:document.getElementById('formula').getBoundingClientRect().height};}catch(e){document.getElementById('formula').textContent=e.message;window.currentLatex=null;return {ok:false,height:document.getElementById('formula').getBoundingClientRect().height};}};</script></body></html>)HTML";
    v->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, false);
    v->setHtml(html, QUrl::fromLocalFile(dir + "/"));
}
void Window::refresh() {
    auto source = input->toPlainText();
    if (nav->currentRow() != 5)
        source.replace('\n', ' ');
    auto lines = source.split('\n', Qt::SkipEmptyParts);
    if (lines.isEmpty()) {
        status->setText("请输入表达式");
        render(preview, "");
        return;
    }
    try {
        if (lines.size() > 8)
            throw std::runtime_error("最多同时输入 8 个函数");
        for (auto &s : lines)
            ms::parse(s.toStdString());
        status->setText("表达式已就绪 · 按计算更新结果");
        render(preview, lines.join("\\quad;\\quad"));
    } catch (const std::exception &e) {
        status->setText(q(e.what()));
        render(preview, lines.first().left(4096));
    }
}
void Window::run() {
    if (watcher.isRunning())
        return;
    int tab = nav->currentRow(), m = method->currentIndex(), n = parts->value(), k = order->value(),
        max = iterations->value(), side = direction->currentIndex();
    double a = lower->value(), b = upper->value(), tol = tolerance->value();
    QString text = input->toPlainText(), pt = point->text();
    pendingRequest = requestSnapshot();
    if (tab != 5)
        text.replace('\n', ' ');
    calculate->setEnabled(false);
    status->setText("C++ 后台线程计算中…");
    watcher.setFuture(QtConcurrent::run([=] {
        Calculation out;
        try {
            auto lines = text.split('\n', Qt::SkipEmptyParts);
            if (lines.isEmpty() || lines.size() > 8)
                throw std::runtime_error("请输入 1～8 个表达式");
            if (tab != 5 && lines.size() != 1)
                throw std::runtime_error("此功能仅接收一个函数；多行输入请使用函数绘图");
            std::vector<ms::Expr> fs;
            for (auto s : lines)
                fs.push_back(ms::parse(s.toStdString()));
            auto f = fs.front();
            bool symbolicParameters = std::any_of(fs.begin(), fs.end(), [](auto expression) {
                return !ms::parameters(expression).empty();
            });
            if (symbolicParameters && tab != 0 && tab != 2)
                throw std::runtime_error(
                    "字母参数目前用于积分和极限；绘图、求导与数值工具请先将参数换成具体数值");
            out.curves = fs;
            if (symbolicParameters)
                out.curves.clear();
            auto present = [&](const ms::CalculusResult &r) {
                out.formula = q(r.formula.empty() ? "\\text{当前算法无法判定}" : r.formula);
                out.message = q(r.plain) + "\n" + q(r.detail);
                switch (r.kind) {
                case ms::ResultKind::Exact:
                    out.caption = "精确结果";
                    break;
                case ms::ResultKind::Conditional:
                    out.caption = "条件符号结果";
                    break;
                case ms::ResultKind::Approximate:
                    out.caption = "数值近似";
                    break;
                case ms::ResultKind::DoesNotExist:
                    out.caption = "极限不存在";
                    break;
                case ms::ResultKind::PositiveInfinity:
                case ms::ResultKind::NegativeInfinity:
                    out.caption = "无穷极限";
                    break;
                default:
                    out.caption = "当前算法无法判定";
                    break;
                }
                if (r.numeric)
                    out.numeric = "数值近似  ≈  " + num(*r.numeric);
                if (r.kind == ms::ResultKind::Exact &&
                    r.verification == ms::Verification::SamplesOnly)
                    out.caption = "符号结果 · 数值抽查通过";
            };
            if (tab == 5) {
                out.message = "曲线按输入行显示：青、橙、紫、粉。滚轮缩放，拖动平移。\n间断处采用非"
                              "有限值、跳跃幅度和中点检测断开；有限采样仍可能漏掉窄小间断。";
                out.formula = q(ms::latex(f));
                return out;
            }
            if (tab == 0) {
                present(ms::solveLimit(f, pointValue(pt), side == 1 ? -1 : side == 2 ? 1 : 0));
            } else if (tab == 7) {
                present(ms::simplifyPolynomial(f));
            } else if (tab == 1) {
                std::optional<double> x;
                if (!pt.trimmed().isEmpty())
                    x = pointValue(pt);
                auto result = ms::solveDerivative(f, k, x);
                present(result);
                if (result.expression)
                    out.curves.push_back(result.expression);
                if (x && result.numeric) {
                    out.message += "\n指定点导数 = " + num(*result.numeric);
                    double y = ms::eval(f, *x), slope = ms::eval(ms::derivative(f), *x);
                    if (std::isfinite(y) && std::isfinite(slope)) {
                        auto tangent =
                            ms::make("+", ms::number(y),
                                     ms::make("*", ms::number(slope),
                                              ms::make("-", ms::variable(), ms::number(*x))));
                        out.curves.push_back(tangent);
                        out.markers.push_back({*x, y});
                    }
                }
            } else if (tab == 4) {
                double x = pointValue(pt);
                auto t = ms::taylor(f, x, k);
                out.formula = q(ms::latex(t));
                out.curves.push_back(t);
                out.message = "Taylor 多项式：\n" + out.formula +
                              "\n青色为原函数，橙色为多项式。未给出余项界，不自动宣称全区间精度。";
            } else if (tab == 3 || (tab == 6 && m <= 1)) {
                if (tab == 3 && m > 1)
                    throw std::runtime_error("方程求根请选择二分法或 Newton 法");
                auto res = m == 1 ? ms::newton(f, pointValue(pt), tol, max)
                                  : ms::bisection(f, a, b, tol, max);
                out.message = q(res.message) + "\n近似根 = " + num(res.value) +
                              "\nf(x) = " + num(ms::eval(f, res.value)) +
                              "\n迭代次数 = " + QString::number(res.steps.size()) +
                              "\n二分法误差列为区间半宽；Newton 为相邻迭代差。";
                out.steps = res.steps;
                out.experiment = f;
                out.kind = m == 1 ? "newton" : "bisection";
                if (res.converged)
                    out.markers.push_back({res.value, 0});
                out.formula = "x\\approx " + num(res.value);
            } else if (tab == 2 || tab == 6) {
                if (m < 2)
                    throw std::runtime_error("积分请选择符号积分、矩形法、梯形法或 Simpson 法");
                if (m == 2 || m == 3) {
                    std::optional<std::pair<double, double>> bounds;
                    if (m == 3)
                        bounds = {{a, b}};
                    present(ms::solveIntegral(f, bounds));
                } else {
                    std::string kind = m == 4 ? "rectangle" : m == 5 ? "trapezoid" : "simpson";
                    double value = ms::quadrature(f, a, b, n, kind);
                    out.message = "数值积分 = " + num(value) +
                                  "\n下表比较不同分割数，相邻差不是严格误差界。\n仅支持正常定积分；"
                                  "采样不能证明整个区间连续。";
                    out.formula = "I\\approx " + num(value);
                    double prev = NAN;
                    std::vector<int> ns;
                    for (int divisor : {8, 4, 2, 1}) {
                        int count = std::max(2, n / divisor);
                        if (kind == "simpson" && count % 2)
                            ++count;
                        if (std::find(ns.begin(), ns.end(), count) == ns.end())
                            ns.push_back(count);
                    }
                    for (int count : ns) {
                        double v = ms::quadrature(f, a, b, count, kind),
                               diff = std::isfinite(prev) ? std::abs(v - prev) : 0;
                        out.steps.push_back({count, v, diff, diff});
                        prev = v;
                    }
                    out.experiment = f;
                    out.kind = kind;
                    out.a = a;
                    out.b = b;
                    out.n = n;
                    out.message += "\n填色图最多显示 200 组；Simpson 图为局部抛物线示意。";
                }
            }
        } catch (const std::exception &e) {
            out.message = "错误：" + q(e.what());
            out.formula = "\\text{请检查输入和参数}";
            out.curves.clear();
            out.steps.clear();
        }
        return out;
    }));
}
QJsonObject Window::requestSnapshot() const {
    return QJsonObject{{"input", input->toPlainText()},    {"mode", nav->currentRow()},
                       {"point", point->text()},           {"lower", lower->value()},
                       {"upper", upper->value()},          {"method", method->currentIndex()},
                       {"order", order->value()},          {"direction", direction->currentIndex()},
                       {"parts", parts->value()},          {"tolerance", tolerance->value()},
                       {"iterations", iterations->value()}};
}
void Window::saveHistory() {
    auto o = pendingRequest;
    o["result"] = lastFormula;
    o["summary"] = output->toPlainText().section('\n', 0, 0);
    recordHistory(o);
}
void Window::recordHistory(QJsonObject o) {
    o["time"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    history->insertItem(1, o["input"].toString().left(45), o);
    while (history->count() > 51)
        history->removeItem(history->count() - 1);
    refreshRecent();
    if (QCoreApplication::arguments().contains("--smoke"))
        return;
    QJsonArray arr;
    for (int i = 1; i < history->count(); ++i)
        arr.append(history->itemData(i).toJsonObject());
    auto dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    QSaveFile f(dir + "/history.json");
    if (f.open(QIODevice::WriteOnly)) {
        f.write(QJsonDocument(arr).toJson());
        f.commit();
    }
}
void Window::loadHistory() {
    if (QCoreApplication::arguments().contains("--smoke"))
        return;
    auto dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QFile f(dir + "/history.json");
    if (f.open(QIODevice::ReadOnly)) {
        auto arr = QJsonDocument::fromJson(f.readAll()).array();
        for (auto v : arr) {
            auto o = v.toObject();
            if (history->count() <= 50)
                history->addItem(o["input"].toString().left(45), o);
        }
    }
}
void Window::selfTest(std::function<void(bool)> done) {
    if (!property("navigationChecked").toBool()) {
        navigationSmoke(std::move(done));
        return;
    }
    struct Case {
        int tab;
        QString expr;
        int method;
        QString point;
        int order;
        QString expected;
        QString screenshot;
    };
    auto cases = std::make_shared<std::vector<Case>>(std::initializer_list<Case>{
        {0, "\\frac{\\sin x}{x}", 0, "0", 1, "Taylor", ""},
        {0, "\\frac1x", 0, "0", 1, "不存在", ""},
        {1, "x^3+\\sin x", 0, "0", 1, "指定点导数 = 1", ""},
        {2, "x^2", 6, "0", 1, "0.3333333333333", ""},
        {3, "x^2-2", 0, "1", 1, "收敛", ""},
        {4, "e^x", 0, "0", 4, "Taylor 多项式", ""},
        {5, "\\sin x\n\\cos x", 0, "0", 1, "曲线按输入行", ""},
        {6, "x^2-2", 1, "1", 1, "收敛", ""},
        {2, "1/x", 3, "0", 1, "错误：", ""},
        {1, "x^3+\\sin x", 0, "0", 1, "指定点导数 = 1", ""},
        {1, "\\frac{e^{x^2}\\sin x}{\\ln(1+x)}", 0, "", 1, "符号法则", "derivative"},
        {2, "\\int x e^{x^2}\\,dx", 2, "", 1, "求导", "integral"},
        {2, "\\frac{x^2+1}{x^4+1}", 2, "", 1, "arctan", "rational-integral"},
        {0, "\\frac{e^x-1-x}{x^2}", 0, "0", 1, "Taylor", "limit"},
        {0, "\\frac{\\sin x-x}{x^3}", 0, "0", 1, "Taylor", ""},
        {0, "\\sin\\frac1x", 0, "0", 1, "不存在", ""},
        {0, "x\\sin\\frac1x", 0, "0", 1, "夹逼", ""},
        {2, "x^2", 3, "", 1, "定积分", "definite-integral"},
        {1, "\\frac", 0, "", 1, "错误：", "error"},
        {1, "\\ln(\\sec x+\\tan x)", 0, "", 1, "符号法则", "textbook-sec"},
        {1, "\\arccot x", 0, "", 1, "符号法则", "textbook-arccot"},
        {1, "\\log_3 x+\\lg x", 0, "", 1, "符号法则", "textbook-log"},
        {2, "\\sec x(\\sec x-\\tan x)", 2, "", 1, "求导", "textbook-integral"},
        {0, "\\ln(1+x^2)/(\\sec x-\\cos x)", 0, "0", 1, "Taylor", "textbook-limit"},
        {0, "\\frac{\\csc x-\\cot x}{x}", 0, "0", 1, "1 / 2", "csc-cot-limit"},
        {7, "(x+1)^4-(x-1)^4", 0, "", 1, "合并同类项", "polynomial"},
        {2, "1/(\\sqrt{x}+\\sqrt[3]{x})", 2, "", 1, "求导", "mixed-radical"},
        {0, "((e^x-1)/x)^{1/x}", 0, "0", 1, "等价", "composite-power-limit"},
        {0, "\\sin(ax)/x", 0, "0", 1, "Taylor", "parameter-limit"},
        {2, "x^a", 2, "", 1, "分情况", "parameter-integral"},
        {2, "\\sin(ax)", 2, "", 1, "退化分支", "parameter-sine"},
        {2, "e^{ax}", 3, "", 1, "退化分支", "parameter-definite"},
        {5, "\\sin x\na*x", 0, "", 1, "错误：", ""},
        {1, "x^3+\\sin x", 0, "", 1, "符号法则", "main"}});
    auto index = std::make_shared<int>(0), retries = std::make_shared<int>(0);
    auto started = std::make_shared<bool>(false), validating = std::make_shared<bool>(false);
    auto timer = new QTimer(this);
    timer->setInterval(120);
    QDir().mkpath("screenshots");
    connect(timer, &QTimer::timeout, this, [=, this] {
        if (watcher.isRunning() || *validating)
            return;
        if (*started) {
            auto item = cases->at(*index);
            if (!output->toPlainText().contains(item.expected)) {
                qCritical() << "GUI FAIL" << *index << output->toPlainText();
                grab().save("screenshots/failure.png");
                timer->stop();
                timer->deleteLater();
                done(false);
                return;
            }
            *validating = true;
            QTimer::singleShot(450, this, [=, this] {
                QJsonArray a{lastFormula};
                auto expected = QString::fromUtf8(QJsonDocument(a).toJson(QJsonDocument::Compact));
                resultPreview->page()->runJavaScript(
                    "window.currentLatex === " + expected +
                        "[0] && !!document.querySelector('.katex') && "
                        "document.querySelector('.katex-display').getBoundingClientRect().bottom "
                        "<= innerHeight + 1",
                    [=, this](const QVariant &rendered) {
                        if (!rendered.toBool()) {
                            *validating = false;
                            if (++*retries > 20) {
                                qCritical() << "RENDER FAIL" << *index;
                                timer->stop();
                                done(false);
                            }
                            return;
                        }
                        preview->page()->runJavaScript(
                            item.expected == "错误："
                                ? "true"
                                : "!!document.querySelector('.katex-display') && "
                                  "document.querySelector('.katex-display').getBoundingClientRect()"
                                  ".bottom <= innerHeight + 1",
                            [=, this](const QVariant &inputVisible) {
                                if (!inputVisible.toBool()) {
                                    *validating = false;
                                    if (++*retries > 20) {
                                        qCritical() << "INPUT CROP FAIL" << *index;
                                        timer->stop();
                                        done(false);
                                    }
                                    return;
                                }
                                if (!item.screenshot.isEmpty()) {
                                    grab().save("screenshots/" + item.screenshot + ".png");
                                    if (item.screenshot == "main") {
                                        auto previous = size();
                                        resize(800, 680);
                                        grab().save("screenshots/compact.png");
                                        resize(previous);
                                    }
                                }
                                qInfo() << "GUI PASS" << *index;
                                *retries = 0;
                                ++*index;
                                *started = false;
                                *validating = false;
                                if (*index == int(cases->size())) {
                                    timer->stop();
                                    timer->deleteLater();
                                    preview->page()->runJavaScript(
                                        "!!document.querySelector('.katex')",
                                        [=](const QVariant &ok) { done(ok.toBool()); });
                                }
                            });
                    });
            });
            return;
        }
        auto item = cases->at(*index);
        nav->setCurrentRow(item.tab);
        if (item.tab < 3) {
            visualToggle->setChecked(false);
            for (auto button : findChildren<QToolButton *>())
                if (button->objectName() == "disclosure")
                    button->setChecked(false);
        }
        if (item.tab == 2)
            integralType->setCurrentIndex(item.method == 2 ? 0 : 1);
        input->setPlainText(item.expr);
        method->setCurrentIndex(item.method);
        point->setText(item.point);
        order->setValue(item.order);
        lower->setValue(item.expr == "1/x" ? -1 : 0);
        upper->setValue(item.tab == 3 ? 2 : 1);
        run();
        *started = true;
    });
    timer->start();
}
