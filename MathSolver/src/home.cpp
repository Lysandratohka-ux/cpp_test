#include "calculus.h"
#include "domain.h"
#include "window.h"
#include <QDesktopServices>
#include <QMouseEvent>
#include <QWebEngineView>
#include <QtConcurrent>
#include <QtWidgets>
#include <cmath>
#include <numbers>

namespace {
class PortalCard : public QAbstractButton {
    bool graph;
    qreal hover = 0;
    QVariantAnimation animation;
    void animate(qreal end) {
        animation.stop();
        animation.setStartValue(hover);
        animation.setEndValue(end);
        animation.start();
    }

  protected:
    void enterEvent(QEnterEvent *e) override {
        animate(1);
        QAbstractButton::enterEvent(e);
    }
    void leaveEvent(QEvent *e) override {
        animate(0);
        QAbstractButton::leaveEvent(e);
    }
    void paintEvent(QPaintEvent *) override {
        bool dark = window()->property("darkTheme").toBool();
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        QColor surface =
            dark ? QColor(25 + int(hover * 7), 33 + int(hover * 7), 44 + int(hover * 7))
                 : QColor(255 - int(hover * 6), 255 - int(hover * 5), 255 - int(hover * 3));
        p.setBrush(surface);
        p.setPen(QPen(hasFocus() ? QColor("#aac7e3")
                      : dark     ? QColor("#303d4c")
                                 : QColor("#d8e0e8"),
                      hasFocus() ? 2 : 1));
        p.drawRoundedRect(QRectF(rect()).adjusted(2, 2, -2, -2), 20, 20);
        p.setPen(
            QPen(dark ? QColor("#bad0e5") : QColor("#3d6287"), 2.4, Qt::SolidLine, Qt::RoundCap));
        if (graph) {
            p.drawLine(QPointF(30, 65), QPointF(75, 65));
            p.drawLine(QPointF(39, 76), QPointF(39, 28));
            QPainterPath path;
            path.moveTo(30, 57);
            path.cubicTo(43, 77, 55, 22, 76, 37);
            p.drawPath(path);
        } else {
            QPainterPath path;
            path.moveTo(65, 28);
            path.cubicTo(38, 15, 62, 87, 34, 71);
            p.drawPath(path);
            p.drawLine(QPointF(70, 48), QPointF(79, 48));
            p.drawLine(QPointF(70, 58), QPointF(79, 58));
        }
        QFont title = font();
        title.setPointSize(22);
        title.setWeight(QFont::DemiBold);
        p.setFont(title);
        p.setPen(dark ? QColor("#e5ebf3") : QColor("#172334"));
        p.drawText(QRect(30, 97, width() - 55, 34), Qt::AlignLeft | Qt::AlignVCenter,
                   graph ? "函数绘图" : "计算器");
        QFont detail = font();
        detail.setPointSize(12);
        p.setFont(detail);
        p.setPen(dark ? QColor("#a4b1c2") : QColor("#53667b"));
        p.drawText(QRect(30, 141, width() - 70, 48), Qt::TextWordWrap,
                   graph ? "让曲线呈现关系。\n多函数、选点与切线。"
                         : "从表达式出发。\n求导、积分与极限。");
        p.drawLine(QPointF(width() - 42, height() - 39), QPointF(width() - 35, height() - 32));
        p.drawLine(QPointF(width() - 35, height() - 32), QPointF(width() - 42, height() - 25));
    }

  public:
    explicit PortalCard(bool isGraph, QWidget *parent = nullptr)
        : QAbstractButton(parent), graph(isGraph) {
        setMinimumSize(260, 215);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        setFocusPolicy(Qt::StrongFocus);
        setCursor(Qt::PointingHandCursor);
        setAccessibleName(graph ? "函数绘图" : "计算器");
        animation.setDuration(150);
        connect(&animation, &QVariantAnimation::valueChanged, this, [this](const QVariant &v) {
            hover = v.toReal();
            update();
        });
    }
};
class IntroCanvas : public QWidget {
  public:
    qreal opacity = 1, progress = 0;
    explicit IntroCanvas(QWidget *p = nullptr) : QWidget(p) {
        setMinimumHeight(310);
        setAccessibleName("Infinity，Lysandra开发，启动动画");
    }

  protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.setOpacity(opacity);
        qreal x = width() / 2., y = height() / 2. - 95;
        QIcon(":/icons/infinity.png").paint(&p, QRect(int(x - 42), int(y), 84, 84));
        p.setPen(QColor("#e5ebf3"));
        auto name = font();
        name.setPointSize(36);
        name.setWeight(QFont::DemiBold);
        p.setFont(name);
        p.drawText(QRectF(0, y + 105, width(), 52), Qt::AlignCenter, "Infinity");
        auto author = font();
        author.setPointSize(14);
        p.setFont(author);
        p.setPen(QColor("#a6b7c9"));
        p.drawText(QRectF(0, y + 166, width(), 30), Qt::AlignCenter, "Lysandra开发");
        p.setPen(QPen(QColor("#283340"), 2, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(QPointF(x - 55, y + 224), QPointF(x + 55, y + 224));
        p.setPen(QPen(QColor("#aac7e3"), 2, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(QPointF(x - 55, y + 224), QPointF(x - 55 + 110 * progress, y + 224));
    }
};
class ArticleView : public QTextBrowser {
    void fitDocument() {
        int desired = qBound(65, int(std::ceil(document()->size().height())) + 12, 6000);
        if (height() != desired)
            setFixedHeight(desired);
    }

  protected:
    void resizeEvent(QResizeEvent *event) override {
        QTextBrowser::resizeEvent(event);
        document()->setTextWidth(viewport()->width());
        fitDocument();
    }

  public:
    ArticleView() {
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        connect(document()->documentLayout(), &QAbstractTextDocumentLayout::documentSizeChanged,
                this, [this] { fitDocument(); });
    }
};
struct PageBody {
    QScrollArea *scroll;
    QVBoxLayout *layout;
};
PageBody bodyPage() {
    auto scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    auto outer = new QWidget;
    auto row = new QHBoxLayout(outer);
    row->setContentsMargins(32, 28, 32, 32);
    row->addStretch();
    auto body = new QWidget;
    body->setMaximumWidth(920);
    auto layout = new QVBoxLayout(body);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(18);
    row->addWidget(body, 1);
    row->addStretch();
    scroll->setWidget(outer);
    return {scroll, layout};
}
QLabel *text(QString content, QString style = "muted") {
    auto label = new QLabel(content);
    label->setObjectName(style);
    label->setWordWrap(true);
    return label;
}
QString operation(const QJsonObject &o) {
    const QStringList labels{"极限",   "求导", "积分",     "求根",
                             "Taylor", "绘图", "数值实验", "多项式化简"};
    int i = o["mode"].toInt(1);
    return i >= 0 && i < labels.size() ? labels[i] : "计算";
}
} // namespace

void Window::buildPages() {
    auto headerLayout = qobject_cast<QHBoxLayout *>(appHeader->layout());
    historyWindow = new QDialog(this);
    historyWindow->setObjectName("historyWindow");
    historyWindow->setWindowTitle("Infinity · 历史记录");
    historyWindow->resize(580, 460);
    auto historyLayout = new QVBoxLayout(historyWindow);
    historyLayout->setContentsMargins(22, 22, 22, 22);
    historyLayout->addWidget(text("历史记录", "heading"));
    historyLayout->addWidget(text("点击记录恢复输入与参数，再手动计算。", "muted"));
    recentList = new QListWidget;
    recentList->setObjectName("recentList");
    recentList->setWordWrap(true);
    recentList->setAccessibleName("计算与绘图历史；点击恢复输入");
    historyLayout->addWidget(recentList, 1);
    auto closeHistory = new QPushButton("关闭");
    historyLayout->addWidget(closeHistory, 0, Qt::AlignRight);
    connect(closeHistory, &QPushButton::clicked, historyWindow, &QDialog::close);
    connect(recentList, &QListWidget::itemClicked, this, [this](QListWidgetItem *item) {
        auto record = item->data(Qt::UserRole).toJsonObject();
        if (!record.isEmpty()) {
            historyWindow->close();
            restoreHistory(record);
        }
    });
    auto historyButton = new QToolButton;
    historyButton->setText("历史记录");
    historyButton->setObjectName("navText");
    historyButton->setProperty("historyEntry", true);
    historyButton->setMinimumHeight(36);
    connect(historyButton, &QToolButton::clicked, this, [this] {
        refreshRecent();
        historyWindow->show();
        historyWindow->raise();
        historyWindow->activateWindow();
    });
    headerLayout->addWidget(historyButton);
    for (int i = 0; i < 2; ++i) {
        auto button = new QToolButton;
        button->setObjectName("navText");
        button->setProperty("destination", i == 0 ? LearnPage : AboutPage);
        button->setText(i == 0 ? "LaTeX 教学" : "软件说明");
        button->setToolButtonStyle(Qt::ToolButtonTextOnly);
        button->setMinimumHeight(36);
        button->setToolTip(i == 0 ? "LaTeX 入门、学习书目与 Overleaf" : "计算能力、使用方法与限制");
        button->setAccessibleName(button->toolTip());
        connect(button, &QToolButton::clicked, this,
                [this, i] { showPage(i == 0 ? LearnPage : AboutPage); });
        headerLayout->addWidget(button);
    }
    // Home stays focused on its two destinations. History lives in a separate window.
    auto home = bodyPage();
    home.layout->setSpacing(12);
    home.layout->addSpacing(8);
    home.layout->addWidget(text("思考，从这里开始。", "heroTitle"));
    home.layout->addWidget(
        text("一个安静的数学空间。选择计算，或让函数成为曲线。", "heroSubtitle"));
    home.layout->addSpacing(12);
    auto entries = new QHBoxLayout;
    entries->setSpacing(20);
    auto calculator = new PortalCard(false), grapher = new PortalCard(true);
    calculator->setObjectName("openCalculator");
    grapher->setObjectName("openGrapher");
    entries->addWidget(calculator);
    entries->addWidget(grapher);
    home.layout->addLayout(entries);
    connect(calculator, &QAbstractButton::clicked, this,
            [this] { openCalculator(nav->currentRow() < 3 ? nav->currentRow() : 1); });
    connect(grapher, &QAbstractButton::clicked, this, [this] {
        showPage(GraphPage);
        graphInput->setFocus();
    });
    home.layout->addStretch();
    home.layout->addWidget(text("Infinity  ·  Lysandra开发  ·  基于 LaTeX 语言输入", "eyebrow"));
    pages->addWidget(home.scroll);

    auto openExternal = [this](const QUrl &url) {
        if (QCoreApplication::arguments().contains("--smoke")) {
            setProperty("lastExternalUrl", url.toString());
            return;
        }
        QDesktopServices::openUrl(url);
    };
    auto article = [&](PageBody &page, QString html) {
        auto browser = new ArticleView;
        browser->setObjectName("guideText");
        browser->setOpenLinks(false);
        browser->setHtml(html);
        browser->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        connect(browser, &QTextBrowser::anchorClicked, this, openExternal);
        page.layout->addWidget(browser);
    };
    auto learn = bodyPage();
    learn.layout->addWidget(text("用 LaTeX，写下数学。", "heading"));
    learn.layout->addWidget(text("先学会几个符号，就能开始。", "heroSubtitle"));
    article(
        learn,
        R"(<p>LaTeX 是一种用文本描述文档结构与数学公式的排版语言。例如，<code>\frac{a}{b}</code> 表示分数，<code>x^2</code> 表示平方。</p><p>命令通常以反斜杠 <code>\</code> 开头；花括号 <code>{ }</code> 用来把一组内容作为整体。函数建议写成 <code>\sin(x)</code>，复杂的参数加括号更清楚。</p><p>Infinity 的编辑器只接收数学表达式，无需输入 <code>$...$</code> 或整篇文档。排版与计算是独立步骤；不是所有能排版的 LaTeX 都属于计算器支持的语法。</p>)");
    learn.layout->addWidget(text("试着写一条公式", "eyebrow"));
    int sampleIndex = 0;
    for (auto pair : {std::pair{R"(\frac{x^2+1}{x+1})", "分数 · 分子与分母分别放在花括号中"},
                      std::pair{R"(\sqrt{x^2+1})", "根式 · 花括号中的内容在根号内"},
                      std::pair{R"(\sin(x)+\ln(1+x))", "函数 · 使用括号明确参数范围"}}) {
        auto card = new QFrame;
        card->setObjectName("card");
        auto row = new QHBoxLayout(card);
        row->setContentsMargins(16, 12, 16, 12);
        auto col = new QVBoxLayout;
        auto code = text(pair.first, "eyebrow");
        code->setTextFormat(Qt::PlainText);
        code->setTextInteractionFlags(Qt::TextSelectableByMouse);
        col->addWidget(code);
        col->addWidget(text(pair.second));
        row->addLayout(col, 1);
        auto tryIt = new QPushButton("试一试");
        tryIt->setObjectName(QString("sample%1").arg(sampleIndex++));
        row->addWidget(tryIt);
        connect(tryIt, &QPushButton::clicked, this, [this, pair] {
            openCalculator();
            input->setPlainText(pair.first);
        });
        learn.layout->addWidget(card);
    }
    learn.layout->addWidget(text("继续学习", "eyebrow"));
    article(
        learn,
        R"(<p><b>中文入门：</b><a href="https://ctan.org/pkg/lshort-zh-cn">《一份（不太）简短的 LaTeX2ε 介绍》</a>。可免费阅读，适合从基本命令与数学排版开始。</p><p><b>系统学习：</b><i>LaTeX: A Document Preparation System</i>，Leslie Lamport 著。<a href="https://www.latex-project.org/help/books/">查看官方书目</a>。</p><p><b>进阶参考：</b><i>The LaTeX Companion</i>（第3版），Frank Mittelbach、Ulrike Fischer 著。需要了解宏包与更完整的排版能力时再查阅。<a href="https://www.latex-project.org/help/books/">官方介绍</a>。</p><p>Overleaf 是在线 LaTeX 编辑器，适合练习完整文档；下面的按钮将在默认浏览器中打开，不会上传本应用里的表达式。</p>)");
    auto overleaf = new QPushButton("在 Overleaf 开始学习  ↗");
    overleaf->setObjectName("overleaf");
    connect(overleaf, &QPushButton::clicked, this, [openExternal] {
        openExternal(QUrl("https://www.overleaf.com/learn/latex/Learn_LaTeX_in_30_minutes"));
    });
    learn.layout->addWidget(overleaf, 0, Qt::AlignLeft);
    pages->addWidget(learn.scroll);

    auto about = bodyPage();
    about.layout->addWidget(text("认识 Infinity", "heading"));
    about.layout->addWidget(text("专注一元微积分，也为函数留一张画布。", "heroSubtitle"));
    article(
        about,
        R"(<h3>可以计算什么</h3><p><b>求导：</b>多项式、有理函数、三角与反三角、指数、对数、根式及其复合。支持乘积、商、链式法则，1～8阶导数和指定点导数。</p><p><b>积分：</b>不定积分与正常定积分；覆盖部分换元、分部、三角有理式、指数有理式和根式积分；新增同一线性表达式的混合根式换元（根次数最小公倍数不超过12）。优先符号结果，正常定积分可提供标明类型的数值近似。部分原函数仅通过求导抽查，并不等同于完整恒等证明。</p><p><b>极限：</b>有限点、单侧及正负无穷处的部分极限；高阶抵消、部分振荡与夹逼规则；新增趋于1的复合底数幂函数等价变换，可处理部分可去奇点底数。区分有限、无穷、不存在和无法判定，不用采样代替极限证明。</p><p><b>绘图：</b>一张图最多8个单变量实函数；支持缩放、平移、选点切线、指定横坐标求切线、PNG导出。与计算器共用安全的LaTeX解析器。</p><p><b>多项式化简：</b>计算器 → 更多工具 → 多项式化简。展开括号、合并同类项、保留精确分数，最高64次；含变量分母的有理式不在此功能范围。</p><h3>可输入的函数</h3><p><code>\sin \cos \tan \sec \csc \cot</code>；<code>\arcsin \arccos \arctan \arccot</code>；<code>e^x \exp(x) \ln(x) \log(x) \log_3(x) \sqrt{x} \sqrt[3]{x} |x|</code>。使用变量 <b>x</b>，角度为弧度；<code>\log</code> 和 <code>\lg</code> 默认以10为底。</p><h3>目前的边界</h3><p>不是完整的通用符号计算系统，不保证解出教材所有题。积分和极限可使用实数参数a、b、c、n；条件直接显示在结果中。支持部分参数多项式、初等函数积分及解析商极限，暂不保证自动穷尽全部参数分支。绘图、求导及数值工具仍要求先将参数换成数值。暂不支持分段/隐函数/参数方程、反常积分、证明题与空间解析几何。上下限与趋近点目前为数值参数。</p><p>绘图采用有限采样，可能漏掉极窄间断或高速振荡；无定义处会尽量断开。竖直切线或不能可靠确认可导的点，不会伪造有限斜率。特殊函数结果不保证能重新输入绘图。</p><h3>使用方式</h3><p>首页选择计算或绘图。计算页切换求导、积分、极限；绘图页每行一个表达式。点击曲线附近即可选点；拖动是平移，滚轮是缩放。交点附近可用“选择曲线”明确指定对象。</p><p>历史记录仅保存在本机，点击恢复后由你决定何时重新计算。Mac使用⌘ Return计算、⌘ L编辑；Windows使用Ctrl+Enter、Ctrl+L。点击顶部“返回首页”即可回到主页。</p><p><b>Lysandra开发</b> · C++ / Qt · 本地离线计算</p>)");
    pages->addWidget(about.scroll);

    auto graph = bodyPage();
    graph.layout->setSpacing(10);
    graph.layout->addWidget(text("看见函数的形状。", "heading"));
    graph.layout->addWidget(
        text("每行一个 LaTeX 表达式。点击曲线选点，拖动平移，滚轮缩放。", "heroSubtitle"));
    auto workspace = new QHBoxLayout;
    workspace->setSpacing(18);
    auto controlWidget = new QWidget;
    controlWidget->setFixedWidth(280);
    auto controls = new QVBoxLayout(controlWidget);
    controls->setContentsMargins(0, 0, 0, 0);
    controls->setSpacing(12);
    auto canvas = new QVBoxLayout;
    canvas->setSpacing(10);
    auto graphCard = new QFrame;
    graphCard->setObjectName("card");
    auto gl = new QVBoxLayout(graphCard);
    gl->setContentsMargins(18, 14, 18, 12);
    graphInput = new QPlainTextEdit;
    graphInput->setObjectName("expression");
    graphInput->setAccessibleName("绘图函数，每行一个LaTeX表达式");
    graphInput->setPlaceholderText("每行一个函数，例如 \\sin(x)\n最多 8 条曲线，点击绘制后显示");
    graphInput->setFixedHeight(80);
    gl->addWidget(graphInput);
    graphPreview = new QWebEngineView;
    graphPreview->setFixedHeight(88);
    gl->addWidget(graphPreview);
    controls->addWidget(graphCard);
    auto actions = new QHBoxLayout;
    auto draw = new QPushButton("绘制图像");
    draw->setObjectName("primary");
    draw->setMinimumHeight(40);
    draw->setObjectName("drawGraph"); // Stable automation/accessibility hook.
    auto clear = new QPushButton("清空");
    auto reset = new QPushButton("重置视图");
    auto exportPng = new QPushButton("导出 PNG");
    actions->addWidget(draw);
    actions->addWidget(clear);
    actions->addStretch();
    controls->addLayout(actions);
    graphStatus = text("输入函数，然后点击绘制。");
    graphStatus->setAccessibleName("绘图状态");
    controls->addWidget(graphStatus);
    graphPlot = new Plot;
    graphPlot->setObjectName("graphCanvas");
    graphPlot->setMinimumHeight(300);
    graphPlot->setToolTip("点击曲线附近求切线 · 拖动平移 · 滚轮缩放");
    canvas->addWidget(graphPlot, 1);
    auto pick = new QHBoxLayout;
    pick->setSpacing(6);
    graphSelection = new QComboBox;
    graphSelection->setAccessibleName("选择切线所属曲线");
    graphSelection->setMinimumWidth(160);
    graphSelection->setMaximumWidth(350);
    graphPoint = new QDoubleSpinBox;
    graphPoint->setRange(-1e6, 1e6);
    graphPoint->setDecimals(6);
    graphPoint->setAccessibleName("切点横坐标");
    graphPoint->setFixedWidth(118);
    auto tangent = new QPushButton("求此点切线");
    tangent->setObjectName("findTangent");
    auto removeTangent = new QPushButton("清除切线");
    controls->addWidget(text("指定切点", "eyebrow"));
    controls->addWidget(graphSelection);
    auto xCaption = text("x =");
    xCaption->setWordWrap(false);
    xCaption->setFixedWidth(22);
    pick->addWidget(xCaption);
    pick->addWidget(graphPoint);
    pick->addWidget(tangent);
    controls->addLayout(pick);
    controls->addStretch();
    auto tools = new QHBoxLayout;
    tools->addWidget(reset);
    tools->addWidget(removeTangent);
    tools->addStretch();
    tools->addWidget(exportPng);
    canvas->addLayout(tools);
    tangentLabel = text("选中一条曲线上的点，或输入横坐标。切线方程会显示在这里。");
    canvas->addWidget(tangentLabel);
    tangentPreview = new QWebEngineView;
    tangentPreview->setFixedHeight(88);
    canvas->addWidget(tangentPreview);
    workspace->addWidget(controlWidget);
    workspace->addLayout(canvas, 1);
    graph.layout->addLayout(workspace, 1);
    pages->addWidget(graph.scroll);
    auto refreshTimer = new QTimer(this);
    refreshTimer->setSingleShot(true);
    refreshTimer->setInterval(200);
    connect(graphInput, &QPlainTextEdit::textChanged, this, [=, this] {
        refreshTimer->start();
        graphStatus->setText("输入已修改 · 点击绘制更新图像");
    });
    connect(refreshTimer, &QTimer::timeout, this, [this] {
        auto lines = graphInput->toPlainText().split('\n', Qt::SkipEmptyParts);
        render(graphPreview, lines.join("\\quad;\\quad"));
    });
    connect(draw, &QPushButton::clicked, this, &Window::drawGraph);
    connect(clear, &QPushButton::clicked, this, [this] {
        ++graphRevision;
        graphInput->clear();
        graphCurves.clear();
        graphSources.clear();
        graphPlot->setCurves({});
        graphSelection->clear();
        render(graphPreview, "");
        render(tangentPreview, "");
        tangentLabel->setText("选中一条曲线上的点，或输入横坐标。");
        graphStatus->setText("输入函数，然后点击绘制。");
    });
    connect(reset, &QPushButton::clicked, this, [this] { graphPlot->setRange(-5, 5, -3, 3); });
    connect(exportPng, &QPushButton::clicked, this, [this] {
        auto path =
            QFileDialog::getSaveFileName(this, "导出函数图像", "Infinity-graph.png", "PNG (*.png)");
        if (!path.isEmpty())
            graphStatus->setText(graphPlot->grab().save(path) ? "图像已导出" : "图像导出失败");
    });
    connect(tangent, &QPushButton::clicked, this,
            [this] { selectGraphPoint(graphSelection->currentIndex(), graphPoint->value()); });
    connect(removeTangent, &QPushButton::clicked, this, [this] {
        ++graphRevision;
        graphPlot->setTangent({});
        graphPlot->setMarkers({});
        render(tangentPreview, "");
        tangentLabel->setText("切线已清除。");
    });
    graphPlot->pointSelected = [this](int i, double x) {
        graphSelection->setCurrentIndex(i);
        graphPoint->setValue(x);
        selectGraphPoint(i, x);
    };
    connect(&tangentWatcher, &QFutureWatcher<TangentResult>::finished, this, [this] {
        auto r = tangentWatcher.result();
        if (tangentRevision != graphRevision)
            return;
        tangentLabel->setText(r.message);
        graphPlot->setTangent(r.line);
        graphPlot->setMarkers(r.line ? std::vector<QPointF>{{r.x, r.y}} : std::vector<QPointF>{});
        render(tangentPreview, r.formula);
    });
    // One application shortcut routes to the visible workspace; the home page never calculates.
    calculate->setShortcut(QKeySequence());
    auto compute = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Return), this);
    connect(compute, &QShortcut::activated, this, [this] {
        if (pages->currentIndex() == GraphPage)
            drawGraph();
        else if (pages->currentIndex() == CalculatorPage)
            run();
    });
    auto back = new QShortcut(QKeySequence(Qt::ALT | Qt::Key_Left), this);
    connect(back, &QShortcut::activated, this, [this] { showPage(HomePage); });

    auto intro = new QWidget;
    auto introLayout = new QVBoxLayout(intro);
    auto center = new IntroCanvas;
    introLayout->addWidget(center, 1);
    auto skip = new QPushButton("跳过");
    skip->setObjectName("quiet");
    skip->setFixedWidth(80);
    introLayout->addWidget(skip, 0, Qt::AlignHCenter);
    introLayout->addSpacing(20);
    pages->addWidget(intro);
    auto sequence = new QSequentialAnimationGroup(this);
    introAnimation = sequence;
    auto appear = new QParallelAnimationGroup;
    auto fade = new QVariantAnimation;
    fade->setDuration(650);
    fade->setStartValue(.65);
    fade->setEndValue(1.);
    fade->setEasingCurve(QEasingCurve::OutCubic);
    connect(fade, &QVariantAnimation::valueChanged, center, [center](const QVariant &v) {
        center->opacity = v.toReal();
        center->update();
    });
    appear->addAnimation(fade);
    auto sweep = new QVariantAnimation;
    sweep->setDuration(1000);
    sweep->setStartValue(0.);
    sweep->setEndValue(1.);
    connect(sweep, &QVariantAnimation::valueChanged, center, [center](const QVariant &v) {
        center->progress = v.toReal();
        center->update();
    });
    appear->addAnimation(sweep);
    sequence->addAnimation(appear);
    sequence->addPause(180);
    auto disappear = new QVariantAnimation;
    disappear->setDuration(220);
    disappear->setStartValue(1.);
    disappear->setEndValue(0.);
    connect(disappear, &QVariantAnimation::valueChanged, center, [center](const QVariant &v) {
        center->opacity = v.toReal();
        center->update();
    });
    sequence->addAnimation(disappear);
    connect(sequence, &QSequentialAnimationGroup::finished, this, [this] { showPage(HomePage); });
    connect(skip, &QPushButton::clicked, this, [this] { showPage(HomePage); });
    showPage(IntroPage);
    QTimer::singleShot(0, this, [this, sequence] {
        sequence->start();
        if (QCoreApplication::arguments().contains("--smoke"))
            QTimer::singleShot(900, this, [this] {
                QDir().mkpath("screenshots");
                auto snapshot = grab();
                snapshot.save("screenshots/startup.png");
                auto image = snapshot.toImage().scaled(size());
                int bright = 0;
                for (int y = height() / 2 - 160; y < height() / 2 + 160; ++y)
                    for (int x = width() / 2 - 180; x < width() / 2 + 180; ++x)
                        if (image.rect().contains(x, y) && qGray(image.pixel(x, y)) > 110)
                            ++bright;
                setProperty("introVisible", bright > 400);
                qInfo() << "INTRO bright pixels" << bright;
            });
    });
}

void Window::showPage(int page) {
    if (!pages || page < 0 || page >= pages->count())
        return;
    if (introAnimation && page != IntroPage)
        introAnimation->stop();
    pages->setCurrentIndex(page);
    findChild<QPushButton *>("brandButton")->setVisible(page != HomePage && page != IntroPage);
    findChild<QLabel *>("brandName")->setVisible(page == HomePage);
    appHeader->setVisible(page != IntroPage);
    calculatorSegments->setVisible(page == CalculatorPage);
    moreTools->setVisible(page == CalculatorPage);
    settingsButton->hide();
    if (page == HomePage)
        refreshRecent();
}
void Window::openCalculator(int mode) {
    nav->setCurrentRow(mode);
    showPage(CalculatorPage);
    input->setFocus();
}
void Window::refreshRecent() {
    if (!recentList)
        return;
    recentList->clear();
    if (history->count() <= 1) {
        auto item =
            new QListWidgetItem("还没有历史记录。完成计算或绘图后，可在此恢复输入。", recentList);
        item->setFlags(Qt::NoItemFlags);
        return;
    }
    for (int i = 1; i < history->count(); ++i) {
        auto o = history->itemData(i).toJsonObject();
        auto time = QDateTime::fromString(o["time"].toString(), Qt::ISODate);
        QString stamp = time.isValid() ? time.toLocalTime().toString("MM-dd HH:mm") : "早前";
        QString expr = o["input"].toString();
        expr.replace('\n', " ； ");
        QString summary = o["summary"].toString();
        auto item = new QListWidgetItem(operation(o) + "  ·  " + stamp + "\n" + expr.left(100) +
                                            (summary.isEmpty() ? "" : "\n" + summary.left(90)),
                                        recentList);
        item->setData(Qt::UserRole, o);
        item->setToolTip("点击恢复输入与参数；不会自动重新计算");
    }
}
void Window::restoreHistory(const QJsonObject &o) {
    if (o["mode"].toInt() == 5) {
        showPage(GraphPage);
        ++graphRevision;
        graphCurves.clear();
        graphSources.clear();
        graphPlot->setCurves({});
        graphSelection->clear();
        render(tangentPreview, "");
        tangentLabel->setText("点击绘制恢复图像，再选择切点。");
        graphInput->setPlainText(o["input"].toString());
        graphStatus->setText("历史输入已恢复 · 点击绘制查看图像");
        return;
    }
    openCalculator(o["mode"].toInt(1));
    output->clear();
    lastFormula.clear();
    numericLabel->hide();
    render(resultPreview, "");
    if (auto card = findChild<QFrame *>("resultCard"))
        card->hide();
    input->setPlainText(o["input"].toString());
    point->setText(o["point"].toString());
    lower->setValue(o["lower"].toDouble());
    upper->setValue(o["upper"].toDouble(1));
    method->setCurrentIndex(o["method"].toInt());
    if (nav->currentRow() == 2)
        integralType->setCurrentIndex(o["method"].toInt() == 2 ? 0 : 1);
    order->setValue(o["order"].toInt(1));
    direction->setCurrentIndex(o["direction"].toInt());
    parts->setValue(o["parts"].toInt(100));
    tolerance->setValue(o["tolerance"].toDouble(1e-8));
    iterations->setValue(o["iterations"].toInt(100));
    status->setText("历史输入已恢复 · 点击计算更新结果");
}
void Window::drawGraph() {
    try {
        auto lines = graphInput->toPlainText().split('\n', Qt::SkipEmptyParts);
        if (lines.isEmpty() || lines.size() > 8)
            throw std::runtime_error("请输入1～8行函数，每行一个LaTeX表达式");
        std::vector<ms::Expr> expressions;
        for (auto line : lines)
            expressions.push_back(ms::parse(line.toStdString()));
        for (auto f : expressions)
            if (!ms::parameters(f).empty())
                throw std::runtime_error("绘图需要具体参数值，请将 a、b、c、n 换成数值后绘制");
        ++graphRevision;
        graphCurves = std::move(expressions);
        graphSources = lines;
        graphPlot->setCurves(graphCurves);
        graphSelection->clear();
        const QStringList colors{"青色", "橙色",   "紫色",   "粉色",
                                 "蓝色", "黄绿色", "玫瑰色", "银色"};
        for (int i = 0; i < lines.size(); ++i)
            graphSelection->addItem(
                QString("%1 · %2 · %3").arg(i + 1).arg(colors[i % 8], lines[i].left(35)));
        graphStatus->setText(
            QString("已绘制 %1 条曲线 · 点击曲线附近选点，也可指定横坐标").arg(lines.size()));
        tangentLabel->setText("选中一条曲线上的点，或输入横坐标。");
        render(tangentPreview, "");
        recordHistory(QJsonObject{{"input", graphInput->toPlainText()},
                                  {"mode", 5},
                                  {"summary", QString("%1 条函数曲线").arg(lines.size())}});
    } catch (const std::exception &e) {
        graphStatus->setText("错误：" + QString::fromUtf8(e.what()));
    }
}
void Window::selectGraphPoint(int index, double x) {
    if (tangentWatcher.isRunning()) {
        tangentLabel->setText("正在计算切线，请稍候再选择。");
        return;
    }
    if (index < 0 || index >= int(graphCurves.size())) {
        tangentLabel->setText("请先绘制函数并选择一条曲线。");
        return;
    }
    auto f = graphCurves[index];
    tangentRevision = graphRevision;
    tangentLabel->setText(
        QString("正在计算第%1条曲线在 x=%2 处的切线…").arg(index + 1).arg(x, 0, 'g', 9));
    tangentWatcher.setFuture(QtConcurrent::run([f, x, index] {
        TangentResult r;
        r.x = x;
        try {
            auto derivative = ms::solveDerivative(f, 1, x);
            if (!derivative.numeric || !std::isfinite(*derivative.numeric))
                throw std::runtime_error("未能得到有限斜率");
            r.y = ms::eval(f, x);
            r.slope = *derivative.numeric;
            if (!std::isfinite(r.y))
                throw std::runtime_error("该点不在函数的实数定义域内");
            r.line = ms::make(
                "+", ms::number(r.y),
                ms::make("*", ms::number(r.slope), ms::make("-", ms::variable(), ms::number(x))));
            auto coordinate = [](double value) {
                return QString::fromStdString(ms::latex(ms::number(std::abs(value))));
            };
            QString left = "y";
            if (r.y != 0)
                left += (r.y > 0 ? "-" : "+") + coordinate(r.y);
            QString right = QString::fromStdString(ms::latex(ms::number(r.slope))) + "\\left(x";
            if (x != 0)
                right += (x > 0 ? "-" : "+") + coordinate(x);
            r.formula = left + "\\approx " + right + "\\right)";
            r.message = QString("第%1条曲线 · 切点 (%2, %3) · 斜率 ≈ %4 · 切线系数为数值近似")
                            .arg(index + 1)
                            .arg(x, 0, 'g', 9)
                            .arg(r.y, 0, 'g', 9)
                            .arg(r.slope, 0, 'g', 9);
        } catch (const std::exception &e) {
            r.message = "无法绘制有限斜率切线：" + QString::fromUtf8(e.what()) +
                        "。可能是无定义点、竖直切线或不可导点。";
        }
        return r;
    }));
}

void Window::navigationSmoke(std::function<void(bool)> done) {
    auto stage = std::make_shared<int>(0), waits = std::make_shared<int>(0);
    auto timer = new QTimer(this);
    timer->setInterval(320);
    QDir().mkpath("screenshots");
    connect(timer, &QTimer::timeout, this, [=, this] {
        auto ensure = [](bool ok, const char *message) {
            if (!ok)
                throw std::runtime_error(message);
        };
        try {
            if (tangentWatcher.isRunning()) {
                if (++*waits > 80)
                    throw std::runtime_error("tangent worker timeout");
                return;
            }
            switch (*stage) {
            case 0:
                ensure(pages->currentIndex() == HomePage, "startup must finish on home");
                ensure(!QIcon(":/icons/infinity.png").isNull(), "infinity icon resource");
                ensure(!findChild<QPushButton *>("brandButton")->isVisible(),
                       "home hides return home");
                ensure(!recentList->isVisible(), "history is not embedded in home");
                ensure(property("introVisible").toBool(),
                       "startup name and developer must be visible");
                ensure(input->toPlainText().isEmpty() && graphInput->toPlainText().isEmpty() &&
                           lastFormula.isEmpty() && !watcher.isRunning(),
                       "startup must be empty and idle");
                grab().save("screenshots/home.png");
                for (auto button : findChildren<QToolButton *>())
                    if (button->property("destination").toInt() == LearnPage) {
                        button->click();
                        break;
                    }
                ensure(pages->currentIndex() == LearnPage, "learn navigation");
                break;
            case 1:
                grab().save("screenshots/learn.png");
                findChild<QPushButton *>("overleaf")->click();
                ensure(property("lastExternalUrl")
                           .toString()
                           .startsWith("https://www.overleaf.com/learn/"),
                       "Overleaf external link");
                findChild<QPushButton *>("sample0")->click();
                ensure(pages->currentIndex() == CalculatorPage && !input->toPlainText().isEmpty(),
                       "explicit sample fills calculator");
                input->clear();
                showPage(AboutPage);
                break;
            case 2:
                grab().save("screenshots/about.png");
                findChild<QPushButton *>("brandButton")->click();
                ensure(pages->currentIndex() == HomePage, "text home button returns home");
                findChild<QAbstractButton *>("openCalculator")->click();
                ensure(pages->currentIndex() == CalculatorPage && input->toPlainText().isEmpty(),
                       "empty calculator entry");
                grab().save("screenshots/calculator-empty.png");
                showPage(HomePage);
                findChild<QAbstractButton *>("openGrapher")->click();
                ensure(pages->currentIndex() == GraphPage && graphInput->toPlainText().isEmpty(),
                       "empty graph entry");
                graphInput->setPlainText("x^2\n\\sin(x)");
                findChild<QPushButton *>("drawGraph")->click();
                ensure(graphCurves.size() == 2, "multiple curves");
                break;
            case 3: {
                if (!graphPreview->property("formulaRendered").toBool() ||
                    !graphPreview->property("renderedFormula").toString().contains("sin")) {
                    if (++*waits > 30)
                        throw std::runtime_error("graph formula preview timeout");
                    return;
                }
                auto location = graphPlot->positionForPoint(1, 1);
                auto global = QPointF(graphPlot->mapToGlobal(location.toPoint()));
                QMouseEvent press(QEvent::MouseButtonPress, location, global, Qt::LeftButton,
                                  Qt::LeftButton, Qt::NoModifier);
                QMouseEvent release(QEvent::MouseButtonRelease, location, global, Qt::LeftButton,
                                    Qt::NoButton, Qt::NoModifier);
                QApplication::sendEvent(graphPlot, &press);
                QApplication::sendEvent(graphPlot, &release);
                ensure(tangentWatcher.future().isValid(), "native click must select a point");
                break;
            }
            case 4: {
                auto r = tangentWatcher.result();
                if (!tangentPreview->property("formulaRendered").toBool() ||
                    tangentPreview->property("renderedFormula").toString() != r.formula) {
                    if (++*waits > 30)
                        throw std::runtime_error("tangent formula preview timeout");
                    return;
                }
                ensure(r.line && std::abs(r.slope - 2) < 1e-8 &&
                           std::abs(ms::eval(r.line, 3) - 5) < 1e-8,
                       "x squared tangent y=2x-1");
                grab().save("screenshots/graph-tangent.png");
                graphSelection->setCurrentIndex(1);
                graphPoint->setValue(0);
                findChild<QPushButton *>("findTangent")->click();
                break;
            }
            case 5: {
                auto r = tangentWatcher.result();
                if (!tangentPreview->property("formulaRendered").toBool() ||
                    tangentPreview->property("renderedFormula").toString() != r.formula) {
                    if (++*waits > 30)
                        throw std::runtime_error("second tangent preview timeout");
                    return;
                }
                ensure(r.line && std::abs(r.slope - 1) < 1e-8 &&
                           std::abs(ms::eval(r.line, 2) - 2) < 1e-8,
                       "second curve selection");
                grab().save("screenshots/graph-multiple.png");
                auto previous = size();
                resize(800, 680);
                grab().save("screenshots/graph-compact.png");
                resize(previous);
                bool pickedWhileDragging = false;
                auto handler = graphPlot->pointSelected;
                graphPlot->pointSelected = [&](int, double) { pickedWhileDragging = true; };
                auto start = graphPlot->positionForPoint(0, 0), finish = start + QPointF(35, 20);
                auto global = QPointF(graphPlot->mapToGlobal(start.toPoint()));
                QMouseEvent press(QEvent::MouseButtonPress, start, global, Qt::LeftButton,
                                  Qt::LeftButton, Qt::NoModifier);
                QMouseEvent move(QEvent::MouseMove, finish, global, Qt::NoButton, Qt::LeftButton,
                                 Qt::NoModifier);
                QMouseEvent release(QEvent::MouseButtonRelease, finish, global, Qt::LeftButton,
                                    Qt::NoButton, Qt::NoModifier);
                QApplication::sendEvent(graphPlot, &press);
                QApplication::sendEvent(graphPlot, &move);
                QApplication::sendEvent(graphPlot, &release);
                graphPlot->pointSelected = handler;
                ensure(!pickedWhileDragging, "drag must not select a tangent point");
                ensure(graphPlot->positionForPoint(0, 0) != start, "drag must pan the chart");
                graphPlot->setRange(-5, 5, -3, 3);
                graphInput->setPlainText(
                    "x\nx^2\n\\sin x\n\\cos x\n\\tan x\n\\ln(1+x^2)\n\\sqrt{x^2+1}\n\\sec x");
                drawGraph();
                ensure(graphCurves.size() == 8, "eight supported curves");
                graphInput->appendPlainText("x^3");
                drawGraph();
                ensure(graphStatus->text().startsWith("错误：") && graphCurves.size() == 8,
                       "nine curves rejected without losing last graph");
                graphInput->setPlainText("|x|");
                drawGraph();
                selectGraphPoint(0, 0);
                break;
            }
            case 6:
                ensure(!tangentWatcher.result().line,
                       "absolute value cusp must reject finite tangent");
                graphInput->setPlainText("1/x");
                drawGraph();
                selectGraphPoint(0, 0);
                break;
            case 7:
                ensure(!tangentWatcher.result().line, "undefined point must reject tangent");
                graphInput->setPlainText("\\frac");
                drawGraph();
                ensure(graphStatus->text().startsWith("错误："), "malformed graph input");
                showPage(HomePage);
                refreshRecent();
                ensure(recentList->count() >= 2, "graph history rows");
                for (int i = 1; i < history->count(); ++i) {
                    auto record = history->itemData(i).toJsonObject();
                    if (record["input"].toString() == "x^2\n\\sin(x)") {
                        restoreHistory(record);
                        break;
                    }
                }
                ensure(pages->currentIndex() == GraphPage &&
                           graphInput->toPlainText().contains("sin"),
                       "history restores multi-function input");
                recordHistory(
                    QJsonObject{{"input", "x^3"}, {"mode", 1}, {"order", 1}, {"summary", "3x²"}});
                showPage(HomePage);
                recentList->itemClicked(recentList->item(0));
                ensure(pages->currentIndex() == CalculatorPage && input->toPlainText() == "x^3" &&
                           !watcher.isRunning(),
                       "history restores without auto-computing");
                break;
            case 8:
                showPage(HomePage);
                for (auto button : findChildren<QToolButton *>())
                    if (button->property("historyEntry").toBool())
                        button->click();
                ensure(historyWindow->isVisible() && recentList->count() > 0,
                       "toolbar history window");
                historyWindow->grab().save("screenshots/history-window.png");
                historyWindow->close();
                grab().save("screenshots/home-history.png");
                {
                    auto previous = size();
                    resize(800, 680);
                    grab().save("screenshots/home-compact.png");
                    resize(previous);
                }
                input->clear();
                graphInput->clear();
                point->clear();
                openCalculator(1);
                setProperty("navigationChecked", true);
                timer->stop();
                timer->deleteLater();
                qInfo() << "NAV PASS startup, empty input, home entries, learning, Overleaf, "
                           "about, eight curves, curve count limit, native point click, drag pan, "
                           "two tangents, cusp "
                           "and undefined point rejection, graph error, history restore, compact "
                           "windows";
                selfTest(done);
                return;
            }
            *waits = 0;
            ++*stage;
        } catch (const std::exception &e) {
            qCritical() << "NAV FAIL" << *stage << e.what();
            grab().save("screenshots/navigation-failure.png");
            timer->stop();
            timer->deleteLater();
            done(false);
        }
    });
    timer->start();
}
