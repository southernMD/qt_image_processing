#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QTimer>
#include <QMargins>
#include <QFileDialog>
#include <QListWidgetItem>
#include <QDir>
#include <QIcon>
#include <QBuffer>
#include <QByteArray>
#include <QtWebEngineWidgets>
#include "toolbar.h"
#include <QFile>
#include <QTextStream>
#include "imagelist.h"
#include <QSlider>
#include <QLabel>
#include <QVBoxLayout>
#include <QPushButton>
#include <QMessageBox>
#include <QSizePolicy>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , toolbarWasVisible(true) // 默认工具栏可见
    , imageListWasVisible(true) // 默认图片列表可见
{
    ui->setupUi(this);
    ui->actionOpen->setShortcut(QKeySequence("Ctrl+N"));
    
    // 设置更大的初始窗口尺寸
    resize(1200, 800);  // 根据需要调整尺寸
    
    // 1. 首先确保centralWidget有一个布局
    QWidget *central = centralWidget();
    if (!central->layout()) {
        QVBoxLayout *layout = new QVBoxLayout(central);
        layout->setContentsMargins(0, 0, 0, 0); // 移除边距
        layout->setSpacing(0); // 移除间距
        central->setLayout(layout);
    }

    // 2. 初始化WebView并添加到中央部件的布局中
    webView = ui->webView;
    central->layout()->addWidget(webView);

    // 3. 设置WebView的大小策略为在两个方向上都扩展
    webView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    webView->setMinimumSize(QSize(0, 0));

    // 4. 确保WebView可见
    webView->setVisible(true);

    qputenv("QTWEBENGINE_REMOTE_DEBUGGING", "5566");   

    // 创建图片列表
    imageList = new ImageList(this);
    addDockWidget(Qt::BottomDockWidgetArea, imageList->getDockWidget());
    
    // 设置图片列表的初始大小 - 使其更小
    imageList->getDockWidget()->setMaximumHeight(150);  // 限制最大高度
    
    // 连接图片列表信号
    connect(imageList, &ImageList::imageSelected, this, &MainWindow::onImageSelected);
    connect(imageList, &ImageList::imageDeleted, this, &MainWindow::onImageDeleted);
    
    // 初始化Canvas - 不使用定时器
    initializeCanvas();
    
    // 创建工具栏
    toolbar = new ToolBar(this);
    
    // 连接工具栏按钮点击信号
    connect(toolbar, &ToolBar::buttonClicked, this, &MainWindow::handleToolbarButtonClicked);
    
    // 将工具栏DockWidget添加到主窗口
    addDockWidget(Qt::LeftDockWidgetArea, toolbar->getDockWidget());   
    
    // 创建视图菜单并添加工具栏显示/隐藏选项
    QMenu *viewMenu = menuBar()->addMenu(tr("视图(&V)"));
    toggleToolbarAction = new QAction(tr("工具栏"), this);
    toggleToolbarAction->setCheckable(true);
    toggleToolbarAction->setChecked(true);
    viewMenu->addAction(toggleToolbarAction);
    
    // 添加图片列表显示/隐藏选项
    toggleImageListAction = new QAction(tr("图片列表"), this);
    toggleImageListAction->setCheckable(true);
    toggleImageListAction->setChecked(true);
    viewMenu->addAction(toggleImageListAction);

    QMenu *helpMenu = menuBar()->addMenu(tr("帮助(&H)"));
    QAction *aboutAction = new QAction(tr("关于(&A)"), this);
    helpMenu->addAction(aboutAction);
    // 连接关于菜单项的点击信号
    // 连接菜单动作和工具栏可见性
    connect(toggleToolbarAction, &QAction::toggled, toolbar, &ToolBar::setVisible);
    connect(toggleImageListAction, &QAction::toggled, imageList, &ImageList::setVisible);
    connect(aboutAction, &QAction::triggered, this, &MainWindow::showAboutDialog);
    
    // 当工具栏可见性改变时更新菜单选项
    connect(toolbar->getDockWidget(), &QDockWidget::visibilityChanged, toggleToolbarAction, &QAction::setChecked);
    connect(imageList->getDockWidget(), &QDockWidget::visibilityChanged, toggleImageListAction, &QAction::setChecked);
    
    // 监听工具栏停靠位置变化
    connect(toolbar->getDockWidget(), &QDockWidget::dockLocationChanged, toolbar, &ToolBar::adjustLayout);
    
    connect(ui->actionOpen, &QAction::triggered, this, &MainWindow::onActionOpenTriggered);
    
    // 立即应用布局，不使用定时器
    toolbar->adjustLayout(Qt::LeftDockWidgetArea);
    
    // 隐藏原来的图片列表控件
    if (ui->imageListWidget) {
        ui->imageListWidget->setVisible(false);
    }

    
}

MainWindow::~MainWindow()
{
    delete ui;
    // toolbar 会作为子对象自动删除
}

void MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
    
    // 安装事件过滤器
    webView->installEventFilter(this);
    
    // 设置工具栏可见性
    toolbar->setVisible(toolbarWasVisible);
    toggleToolbarAction->setChecked(toolbarWasVisible);
    
    // 设置图片列表可见性
    imageList->setVisible(imageListWasVisible);
    toggleImageListAction->setChecked(imageListWasVisible);
}

void MainWindow::hideEvent(QHideEvent *event)
{
    // 保存工具栏的可见性状态
    toolbarWasVisible = toolbar->isVisible();
    // 保存图片列表的可见性状态
    imageListWasVisible = imageList->isVisible();
    QMainWindow::hideEvent(event);
}

void MainWindow::changeEvent(QEvent *event)
{
    // 处理窗口状态变化事件
    if (event->type() == QEvent::WindowStateChange) {
        // 如果窗口从最小化状态恢复
        if (!(windowState() & Qt::WindowMinimized) && toolbarWasVisible) {
            // 确保工具栏显示
            QTimer::singleShot(100, [this]() {
                toolbar->setVisible(true);
                toggleToolbarAction->setChecked(true);
            });
        }
    }
    QMainWindow::changeEvent(event);
}

void MainWindow::initializeCanvas()
{
    // 从资源文件加载HTML
    QFile htmlFile(":/html/canvas_viewer.html");
    if (!htmlFile.open(QIODevice::ReadOnly)) {
        qDebug() << "无法打开HTML资源文件";
        return;
    }
    
    // 读取HTML内容
    QTextStream stream(&htmlFile);
    QString htmlContent = stream.readAll();
    htmlFile.close();
    
    // 将HTML内容加载到WebView
    webView->setHtml(htmlContent);
}

void MainWindow::displayImageInCanvas(const QPixmap &pixmap)
{
    if(pixmap.isNull()) return;
    
    QByteArray byteArray;
    QBuffer buffer(&byteArray);
    buffer.open(QIODevice::WriteOnly);
    pixmap.save(&buffer, "PNG");
    buffer.close();
    
    QString imageData = "data:image/png;base64," + QString::fromLatin1(byteArray.toBase64().data());
    
    QString script = QString("displayImage('%1')").arg(imageData);
    webView->page()->runJavaScript(script);
}

void MainWindow::onImageSelected(const QPixmap &pixmap, const QString &path)
{
    currentPixmap = pixmap;
    displayImageInCanvas(pixmap);
}

void MainWindow::onImageDeleted(const QString &path)
{
    currentPixmap = QPixmap();
    webView->page()->runJavaScript("drawWelcomeText();");
}

void MainWindow::onActionOpenTriggered()
{
    // 打开文件对话框选择图片
    QStringList files = QFileDialog::getOpenFileNames(
        this,
        tr("选择图片文件"),
        QDir::homePath(),
        tr("图片文件 (*.png *.jpg *.jpeg *.bmp *.svg)")
    );
    
    if(files.isEmpty()) return;
    
    // 使用 ImageList 类添加图片
    imageList->addImages(files);
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    // 传递事件给基类处理
    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    
    // 使用静态变量作为防抖机制，避免频繁更新
    static QTimer resizeTimer;
    resizeTimer.setSingleShot(true);
    
    // 取消之前的定时器，只处理最后一次 resize 事件
    resizeTimer.disconnect();
    connect(&resizeTimer, &QTimer::timeout, this, [this]() {
        // 只在有图片时更新
        if (!currentPixmap.isNull()) {
            displayImageInCanvas(currentPixmap);
        }
    });
    
    // 设置较长的延时，减少触发频率
    resizeTimer.start(300);
}

// 修改工具栏按钮点击处理方法

// 成员变量添加
QDockWidget *thresholdDock = nullptr; // 存储二值化DockWidget指针
void MainWindow::handleToolbarButtonClicked(int index){
    qDebug() << "按钮点击" << index;
    
    // 关闭其他设置窗口
    if (index != 1 && thresholdDock && thresholdDock->isVisible()) {
        thresholdDock->close();
    }
    if (index != 3 && gammaDock && gammaDock->isVisible()) {
        gammaDock->close();
    }
    if (index != 4 && edgeDock && edgeDock->isVisible()) {
        edgeDock->close();
    }
    
    switch (index) {
        case 0:
            webView->page()->runJavaScript("grayscale()", [this](const QVariant &result) {
                if (result.isValid() && !result.isNull()) {
                    // 获取返回的base64数据
                    QString imageDataUrl = result.toString();
                    if (!imageDataUrl.isEmpty()) {
                        // 可以选择保存或更新图片
                        updateImageWithBase64(imageDataUrl);
                    }
                }
            });
            break;
        case 1:
            if (currentPixmap.isNull()) return;
            createThresholdSlider();
            break;
        case 2:
            webView->page()->runJavaScript("meanFilter()", [this](const QVariant &result) {
                if (result.isValid() && !result.isNull()) {
                    QString imageDataUrl = result.toString();
                    if (!imageDataUrl.isEmpty()) {
                        updateImageWithBase64(imageDataUrl);
                    }
                }
            });
            break;
        case 3:
            if (currentPixmap.isNull()) return;
            createGammaSlider();
            break;
        case 4:
            if (currentPixmap.isNull()) return;
            createEdgeDetectionSlider();
            break;
        case 5:
            webView->page()->runJavaScript("resetImage()");
            break;
        case 6:
            saveImage();
            break;
        case 7:
            if (currentPixmap.isNull()) return;
            webView->page()->runJavaScript("startMosaicMode()", [this](const QVariant &result) {
                if (result.toBool()) {
                    qDebug() << "已进入马赛克模式";
                    
                    webView->page()->runJavaScript(
                        "window.onMosaicApplied = function(result) { return result; };",
                        [this](const QVariant &) {
                            qDebug() << "马赛克回调设置完成";
                        }
                    );
                }
            });
            break;
        default:
            break;
    }
}

// 添加一个处理Base64数据的方法
void MainWindow::updateImageWithBase64(const QString &imageDataUrl){
    // 移除前缀 "data:image/png;base64,"
    QString base64Data = imageDataUrl.mid(22);
    
    // 转换为QPixmap
    QByteArray imageData = QByteArray::fromBase64(base64Data.toLatin1());
    QPixmap processedPixmap;
    processedPixmap.loadFromData(imageData, "PNG");
    
    if (processedPixmap.isNull()) {
        qDebug() << "处理后的图像无效";
        return;
    }
    
    // 更新当前图像
    currentPixmap = processedPixmap;
}


// 创建二值化阈值滑块
void MainWindow::createThresholdSlider() {
    // 如果已存在二值化设置窗口且可见，则不再创建新窗口
    if (thresholdDock && thresholdDock->isVisible()) {
        applyBinarization(128);
        thresholdDock->setFocus(); // 设置焦点到已有窗口
        return;
    }
    
    // 如果窗口存在但不可见，则显示它
    if (thresholdDock) {
        applyBinarization(128);
        thresholdDock->show();
        return;
    }
    
    // 创建新的二值化设置窗口
    thresholdDock = new QDockWidget(tr("二值化设置"), this);
    thresholdDock->setAllowedAreas(Qt::RightDockWidgetArea);
    thresholdDock->setFeatures(QDockWidget::DockWidgetClosable);
    // 创建内容控件
    QWidget *content = new QWidget(thresholdDock);
    QVBoxLayout *layout = new QVBoxLayout(content);
    
    // 创建滑块
    QSlider *slider = new QSlider(Qt::Horizontal, content);
    slider->setRange(0, 255);
    slider->setValue(128);
    slider->setTickPosition(QSlider::TicksBelow);
    slider->setTickInterval(32);
    
    // 创建标签
    QLabel *label = new QLabel("128", content);
    label->setAlignment(Qt::AlignCenter);
    
    // 添加到布局
    layout->addWidget(new QLabel(tr("调整阈值:"), content));
    layout->addWidget(slider);
    layout->addWidget(label);
    layout->addStretch();
    
    // 设置内容控件
    content->setLayout(layout);
    thresholdDock->setWidget(content);
    thresholdDock->setMinimumWidth(200);
    // 添加到主窗口
    addDockWidget(Qt::RightDockWidgetArea, thresholdDock);
    
    // 连接信号
    connect(slider, &QSlider::valueChanged, [label, this](int value) {
        label->setText(QString::number(value));
        applyBinarization(value);
    });
    
    // 保存引用
    thresholdSlider = slider;
    thresholdLabel = label;
    sliderContainer = content;
    
    // 应用初始阈值
    applyBinarization(128);
}

void MainWindow::applyBinarization(int threshold)
{
    if (!currentPixmap.isNull()) {
        // 调用JavaScript的二值化函数
        QString script = QString("binarizeImage(%1)").arg(threshold);
        webView->page()->runJavaScript(script, [this](const QVariant &result) {
            if (result.isValid() && !result.toString().isEmpty()) {
                // 更新图像
                updateImageWithBase64(result.toString());
            }
        });
    }
}


// 创建伽马调整滑块
void MainWindow::createGammaSlider() {
    // 如果已存在伽马设置窗口且可见，则不再创建新窗口
    if (gammaDock && gammaDock->isVisible()) {
        applyGammaTransform(1.0);  // 默认值1.0
        gammaDock->setFocus();     // 设置焦点到已有窗口
        return;
    }
    
    // 如果窗口存在但不可见，则显示它
    if (gammaDock) {
        applyGammaTransform(1.0);
        gammaDock->show();
        return;
    }
    
    // 创建新的伽马设置窗口
    gammaDock = new QDockWidget(tr("伽马调整"), this);
    gammaDock->setAllowedAreas(Qt::RightDockWidgetArea);
    gammaDock->setFeatures(QDockWidget::DockWidgetClosable);
    
    // 创建内容控件
    QWidget *content = new QWidget(gammaDock);
    QVBoxLayout *layout = new QVBoxLayout(content);
    
    // 创建标题标签
    QLabel *titleLabel = new QLabel(tr("伽马值调整:"), content);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("font-weight: bold;");
    
    // 创建滑块 - 伽马值范围从0.1到3.0，使用100倍表示
    gammaSlider = new QSlider(Qt::Horizontal, content);
    gammaSlider->setRange(10, 300);  // 伽马值0.1到3.0，乘以100
    gammaSlider->setValue(100);      // 默认1.0
    gammaSlider->setTickPosition(QSlider::TicksBelow);
    gammaSlider->setTickInterval(10); // 改为10，即0.1的步长
    
    // 创建显示当前伽马值的标签
    gammaLabel = new QLabel("1.00", content);
    gammaLabel->setAlignment(Qt::AlignCenter);
    
    // 添加一个水平布局用于显示最小值和最大值
    QHBoxLayout *rangeLayout = new QHBoxLayout();
    QLabel *minLabel = new QLabel("0.10", content);
    QLabel *maxLabel = new QLabel("3.00", content);
    rangeLayout->addWidget(minLabel);
    rangeLayout->addStretch();
    rangeLayout->addWidget(maxLabel);
    
    // 添加到布局
    layout->addWidget(titleLabel);
    layout->addWidget(gammaSlider);
    layout->addWidget(gammaLabel);
    layout->addLayout(rangeLayout);
    
    // 添加说明文字
    QLabel *infoLabel = new QLabel(tr("<1.0: 增亮图像\n=1.0: 保持不变\n>1.0: 加深图像"), content);
    infoLabel->setAlignment(Qt::AlignCenter);
    infoLabel->setStyleSheet("color: #666; font-size: 12px;");
    layout->addWidget(infoLabel);
    
    layout->addStretch();
    
    // 设置内容控件
    content->setLayout(layout);
    gammaDock->setWidget(content);
    gammaDock->setMinimumWidth(200);
    
    // 添加到主窗口
    addDockWidget(Qt::RightDockWidgetArea, gammaDock);
    
    // 连接信号
    connect(gammaSlider, &QSlider::valueChanged, [this](int value) {
        float gamma = value / 100.0f;
        gammaLabel->setText(QString::number(gamma, 'f', 2));
        applyGammaTransform(gamma);
    });
    
    // 连接关闭信号
    connect(gammaDock, &QDockWidget::visibilityChanged, this, [this](bool visible) {
        gammaVisible = visible;
    });
    
    // 应用初始伽马值
    applyGammaTransform(1.0);
}

// 应用伽马变换
void MainWindow::applyGammaTransform(float gamma) {
    if (!currentPixmap.isNull()) {
        // 调用JavaScript的伽马变换函数
        QString script = QString("gammaTransform(%1)").arg(gamma);
        webView->page()->runJavaScript(script, [this](const QVariant &result) {
            if (result.isValid() && !result.toString().isEmpty()) {
                // 更新图像
                updateImageWithBase64(result.toString());
            }
        });
    }
}

// 创建边缘检测滑块
void MainWindow::createEdgeDetectionSlider() {
    // 如果已存在边缘检测设置窗口且可见，则不再创建新窗口
    if (edgeDock && edgeDock->isVisible()) {
        applyEdgeDetection(30);  // 默认值30
        edgeDock->setFocus();    // 设置焦点到已有窗口
        return;
    }
    
    // 如果窗口存在但不可见，则显示它
    if (edgeDock) {
        applyEdgeDetection(30);
        edgeDock->show();
        return;
    }
    
    // 创建新的边缘检测设置窗口
    edgeDock = new QDockWidget(tr("边缘检测"), this);
    edgeDock->setAllowedAreas(Qt::RightDockWidgetArea);
    edgeDock->setFeatures(QDockWidget::DockWidgetClosable);
    
    // 创建内容控件
    QWidget *content = new QWidget(edgeDock);
    QVBoxLayout *layout = new QVBoxLayout(content);
    
    // 创建标题标签
    QLabel *titleLabel = new QLabel(tr("阈值调整:"), content);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("font-weight: bold;");
    
    // 创建滑块 - 边缘检测阈值范围从0到100
    edgeSlider = new QSlider(Qt::Horizontal, content);
    edgeSlider->setRange(0, 100);  // 阈值范围0-100
    edgeSlider->setValue(30);      // 默认值30
    edgeSlider->setTickPosition(QSlider::TicksBelow);
    edgeSlider->setTickInterval(5); // 每5个单位一个刻度
    
    // 创建显示当前阈值的标签
    edgeLabel = new QLabel("30", content);
    edgeLabel->setAlignment(Qt::AlignCenter);
    
    // 添加一个水平布局用于显示最小值和最大值
    QHBoxLayout *rangeLayout = new QHBoxLayout();
    QLabel *minLabel = new QLabel("0", content);
    QLabel *maxLabel = new QLabel("100", content);
    rangeLayout->addWidget(minLabel);
    rangeLayout->addStretch();
    rangeLayout->addWidget(maxLabel);
    
    // 添加到布局
    layout->addWidget(titleLabel);
    layout->addWidget(edgeSlider);
    layout->addWidget(edgeLabel);
    layout->addLayout(rangeLayout);
    
    // 添加说明文字
    QLabel *infoLabel = new QLabel(tr("较低的值: 检测更多边缘\n较高的值: 仅检测明显边缘"), content);
    infoLabel->setAlignment(Qt::AlignCenter);
    infoLabel->setStyleSheet("color: #666; font-size: 12px;");
    layout->addWidget(infoLabel);
    
    layout->addStretch();
    
    // 设置内容控件
    content->setLayout(layout);
    edgeDock->setWidget(content);
    edgeDock->setMinimumWidth(200);
    
    // 添加到主窗口
    addDockWidget(Qt::RightDockWidgetArea, edgeDock);
    
    // 连接信号
    connect(edgeSlider, &QSlider::valueChanged, [this](int value) {
        edgeLabel->setText(QString::number(value));
        applyEdgeDetection(value);
    });
    
    // 连接关闭信号
    connect(edgeDock, &QDockWidget::visibilityChanged, this, [this](bool visible) {
        edgeVisible = visible;
    });
    
    // 应用初始阈值
    applyEdgeDetection(30);
}

// 应用边缘检测
void MainWindow::applyEdgeDetection(int threshold) {
    if (!currentPixmap.isNull()) {
        // 调用JavaScript的边缘检测函数
        QString script = QString("edgeDetection(%1)").arg(threshold);
        webView->page()->runJavaScript(script, [this](const QVariant &result) {
            if (result.isValid() && !result.toString().isEmpty()) {
                // 更新图像
                updateImageWithBase64(result.toString());
            }
        });
    }
}

void MainWindow::saveImage() {
    if (!currentPixmap.isNull()) {
        // 使用QFileDialog保存图片
        QString fileName = QFileDialog::getSaveFileName(
            this,  tr("Save Image"), "", tr("Images (*.png *.jpg *.bmp)"));
        if (!fileName.isEmpty()) {
            // 保存图片
            currentPixmap.save(fileName);
        }
    }
}

void MainWindow::showAboutDialog()
{
    QMessageBox::about(this, tr("关于"),
        tr("<h3>大智慧图像处理</h3>"
           "<p>版本 1.0</p>"
           "<p>一个功能齐全的图像处理工具</p>"
           "<p>© 2023 开发者姓名</p>"));
}
