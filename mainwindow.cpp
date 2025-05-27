#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QTimer>
#include <QMargins>
#include <QFileDialog>
#include <QListWidgetItem>
#include <QDir>
#include <QIcon>
#include <QPainter>
#include <QFont>
#include <QSizePolicy>
#include <QResizeEvent>
#include <QWebEngineView>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , toolbarWasVisible(true) // 默认工具栏可见
{
    ui->setupUi(this);
    ui->actionOpen->setShortcut(QKeySequence("Ctrl+N"));

    // 创建图像标签并设置属性
    imageLabel = new QLabel(ui->canvasContainer);
    imageLabel->setAlignment(Qt::AlignCenter);
    imageLabel->setBackgroundRole(QPalette::Base);
    imageLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    imageLabel->setScaledContents(false);
    imageLabel->setStyleSheet("background-color: white; border: 1px solid #d0d0d0;");
    
    // 设置布局，使imageLabel填充整个canvasContainer
    QVBoxLayout *layout = new QVBoxLayout(ui->canvasContainer);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(imageLabel);
    ui->canvasContainer->setLayout(layout);

    // 初始化图片列表
    initImageList();
    
    // 连接图片列表的点击信号
    connect(ui->imageListWidget, &QListWidget::itemClicked, this, &MainWindow::onImageItemClicked);
    
    // 显示欢迎文字
    QPixmap welcomePix(ui->canvasContainer->size());
    welcomePix.fill(Qt::white);
    QPainter painter(&welcomePix);
    painter.setPen(QColor(102, 102, 102));
    painter.setFont(QFont("Arial", 12));
    painter.drawText(welcomePix.rect(), Qt::AlignCenter, tr("在左侧列表选择图片以在此查看"));
    imageLabel->setPixmap(welcomePix);
    
    // 创建可停靠工具部件
    toolDockWidget = new QDockWidget(tr("工具栏"), this);
    toolDockWidget->setAllowedAreas(Qt::AllDockWidgetAreas);
    toolDockWidget->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    
    // 设置工具栏风格和内边距，使其更加紧凑
    toolDockWidget->setStyleSheet(
        "QDockWidget {"
        "    border: 2px solid #a0a0a0;"
        "    border-radius: 4px;"
        "}"
        "QDockWidget::title {"
        "    background: #f0f0f0;"
        "    padding: 3px;"
        "    border-bottom: 1px solid #a0a0a0;"
        "}"
    );
    
    // 创建新的工具栏容器
    toolWidget = new QWidget(toolDockWidget);
    
    // 设置工具栏内容容器的样式
    toolWidget->setStyleSheet(
        "QWidget {"
        "    background-color: #f8f8f8;"
        "    border-radius: 2px;"
        "    border: 1px solid #c0c0c0;"
        "}"
    );
    
    // 从原UI克隆所有按钮
    QVBoxLayout *originalLayout = qobject_cast<QVBoxLayout*>(ui->layoutWidget->layout());
    if (originalLayout) {
        // 遍历原布局中的所有水平布局
        for (int i = 0; i < originalLayout->count(); ++i) {
            QHBoxLayout *hLayout = qobject_cast<QHBoxLayout*>(originalLayout->itemAt(i)->layout());
            if (hLayout) {
                // 复制水平布局中的所有按钮
                for (int j = 0; j < hLayout->count(); ++j) {
                    QWidget *widget = hLayout->itemAt(j)->widget();
                    if (widget) {
                        // 创建新按钮
                        QPushButton *btn = qobject_cast<QPushButton*>(widget);
                        if (btn) {
                            QPushButton *newBtn = new QPushButton(toolWidget);
                            // 保留原始样式中除尺寸外的设置，但修改尺寸相关的部分
                            QString style = btn->styleSheet();
                            style.replace("min-width: 30px;", "min-width: 100px;");
                            style.replace("max-width: 30px;", "max-width: 100px;");
                            style.replace("min-height: 30px;", "min-height: 60px;");
                            style.replace("max-height: 30px;", "max-height: 60px;");
                            newBtn->setStyleSheet(style);
                            
                            newBtn->setText(btn->text());
                            newBtn->setToolTip(btn->toolTip());
                            newBtn->setCursor(btn->cursor());
                            
                            // 立即设置按钮尺寸，确保初始状态就是大的
                            newBtn->setMinimumSize(100, 60);
                            newBtn->setMaximumSize(100, 60);
                            
                            // 连接新旧按钮的信号
                            connect(newBtn, &QPushButton::clicked, [=]() {
                                btn->click();
                            });
                            
                            // 保存按钮引用以便后续调整布局
                            toolButtons.append(newBtn);
                        }
                    }
                }
            }
        }
    }
    
    // 初始化为垂直布局（因为默认左侧停靠）
    recreateToolbarLayout(false);
    
    // 设置工具栏的Widget
    toolDockWidget->setWidget(toolWidget);
    
    // 将DockWidget添加到主窗口
    addDockWidget(Qt::LeftDockWidgetArea, toolDockWidget);
    
    // 隐藏原来的工具栏
    ui->layoutWidget->setVisible(false);
    
    // 创建视图菜单并添加工具栏显示/隐藏选项
    QMenu *viewMenu = menuBar()->addMenu(tr("视图(&V)"));
    toggleToolbarAction = new QAction(tr("工具栏"), this);
    toggleToolbarAction->setCheckable(true);
    toggleToolbarAction->setChecked(true);
    viewMenu->addAction(toggleToolbarAction);
    
    // 连接菜单动作和工具栏可见性
    connect(toggleToolbarAction, &QAction::toggled, toolDockWidget, &QDockWidget::setVisible);
    // 当工具栏可见性改变时更新菜单选项
    connect(toolDockWidget, &QDockWidget::visibilityChanged, toggleToolbarAction, &QAction::setChecked);
    
    // 监听工具栏停靠位置变化
    connect(toolDockWidget, &QDockWidget::dockLocationChanged, this, &MainWindow::adjustToolbarLayout);
    
    connect(ui->actionOpen, &QAction::triggered, this, &MainWindow::onActionOpenTriggered);

    // 添加浮动状态更改的处理
    connect(toolDockWidget, &QDockWidget::topLevelChanged, this, [this](bool topLevel) {
        // 浮动状态下使用水平布局
        if (topLevel) {
            recreateToolbarLayout(true);
            // 浮动时增大工具栏尺寸以适应更宽的按钮
            int estimatedWidth = 20 + (toolButtons.size() * (100 + 8)); // 边距 + (按钮数 * (按钮宽度 + 间距))
            toolDockWidget->resize(qMax(estimatedWidth, 400), 120); // 宽度至少400，高度增加以适应更高的按钮
            
            // 设置浮动状态下的边框样式
            toolDockWidget->setStyleSheet(
                "QDockWidget {"
                "    border: 2px solid #a0a0a0;"
                "    border-radius: 4px;"
                "    background-color: #f0f0f0;"
                "}"
                "QDockWidget::title {"
                "    background: #e0e0e0;"
                "    padding: 4px;"
                "    border-bottom: 1px solid #a0a0a0;"
                "}"
            );
        } else {
            // 回到停靠状态，根据停靠区域决定布局
            adjustToolbarLayout(this->dockWidgetArea(toolDockWidget));
        }
    });
    
    // 立即应用当前布局，确保尺寸正确
    QTimer::singleShot(0, this, [this]() {
        adjustToolbarLayout(Qt::LeftDockWidgetArea);
    });
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::recreateToolbarLayout(bool isHorizontal)
{
    // 删除旧布局
    if (toolWidget->layout()) {
        delete toolWidget->layout();
    }
    
    // 创建新布局
    QBoxLayout *layout = nullptr;
    if (isHorizontal) {
        // 水平布局
        layout = new QHBoxLayout(toolWidget);
        layout->setContentsMargins(10, 10, 10, 10); // 增加边距，适应大按钮
        layout->setSpacing(10);                     // 增加间距
        
        // 设置工具栏在水平布局时的高度 - 增加以适应更高的按钮
        toolWidget->setMaximumHeight(80);
        
        // 当水平布局时设置DockWidget的最大高度 - 增加以适应更高的按钮
        toolDockWidget->setMaximumHeight(100);
        
        // 设置水平布局时的边框样式
        toolDockWidget->setStyleSheet(
            "QDockWidget {"
            "    border: 2px solid #a0a0a0;"
            "    border-radius: 4px;"
            "}"
            "QDockWidget::title {"
            "    background: #f0f0f0;"
            "    padding: 3px;"
            "    border-bottom: 1px solid #a0a0a0;"
            "}"
        );
        
        // 调整按钮在水平布局时的尺寸 - 高度设为60
        for (QPushButton *btn : toolButtons) {
            btn->setMinimumSize(100, 60);
            btn->setMaximumSize(100, 60);
        }
    } else {
        // 垂直布局
        layout = new QVBoxLayout(toolWidget);
        layout->setContentsMargins(6, 6, 6, 6); // 正常边距
        layout->setSpacing(8);                  // 增加间距以适应更高的按钮
        
        // 垂直布局时移除最大高度限制
        toolWidget->setMaximumHeight(QWIDGETSIZE_MAX);
        toolDockWidget->setMaximumHeight(QWIDGETSIZE_MAX);
        
        // 设置垂直布局时的边框样式
        toolDockWidget->setStyleSheet(
            "QDockWidget {"
            "    border: 2px solid #a0a0a0;"
            "    border-radius: 4px;"
            "}"
            "QDockWidget::title {"
            "    background: #f0f0f0;"
            "    padding: 3px;"
            "    border-bottom: 1px solid #a0a0a0;"
            "}"
        );
        
        // 垂直布局时也设置按钮尺寸 - 高度设为60
        for (QPushButton *btn : toolButtons) {
            btn->setMinimumSize(100, 60);
            btn->setMaximumSize(100, 60);
        }
    }
    
    // 添加按钮到布局
    for (QPushButton *btn : toolButtons) {
        layout->addWidget(btn);
    }
    
    layout->addStretch(); // 添加弹性空间，使按钮靠边显示
    toolWidget->setLayout(layout);
}

void MainWindow::adjustToolbarLayout(Qt::DockWidgetArea area)
{
    // 根据停靠区域调整布局
    bool isHorizontal = (area == Qt::TopDockWidgetArea || area == Qt::BottomDockWidgetArea);
    
    // 重新创建适当的布局
    recreateToolbarLayout(isHorizontal);
    
    // 确保在水平停靠时立即应用高度限制
    if (isHorizontal) {
        QTimer::singleShot(0, this, [this]() {
            toolDockWidget->setMaximumHeight(100); // 与recreateToolbarLayout中的值一致
            toolWidget->updateGeometry();
            toolDockWidget->adjustSize();
        });
    }
}

void MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
    
    // 如果工具栏应该可见，确保它显示出来
    if (toolbarWasVisible) {
        toolDockWidget->show();
        toggleToolbarAction->setChecked(true);
    } else {
        toolDockWidget->hide();
        toggleToolbarAction->setChecked(false);
    }
    
    // 初始更新图像
    QTimer::singleShot(500, this, [this]() {
        if (!currentPixmap.isNull()) {
            displayImageInLabel(currentPixmap);
        }
    });
}

void MainWindow::hideEvent(QHideEvent *event)
{
    // 保存工具栏的可见性状态
    toolbarWasVisible = toolDockWidget->isVisible();
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
                toolDockWidget->setVisible(true);
                toggleToolbarAction->setChecked(true);
            });
        }
    }
    QMainWindow::changeEvent(event);
}

void MainWindow::initImageList()
{
    // 清空列表
    ui->imageListWidget->clear();
    
    // 设置图标模式和显示大小
    ui->imageListWidget->setViewMode(QListWidget::IconMode);
    ui->imageListWidget->setIconSize(QSize(80, 80));
    ui->imageListWidget->setGridSize(QSize(100, 100));
    ui->imageListWidget->setResizeMode(QListWidget::Adjust);
    ui->imageListWidget->setMovement(QListWidget::Static);
    ui->imageListWidget->setSpacing(5);
    ui->imageListWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    
    // 连接右键菜单信号
    connect(ui->imageListWidget, &QListWidget::customContextMenuRequested, 
            this, &MainWindow::showImageListContextMenu);
    
    // 添加几个示例图片项
    QStringList defaultImages;
    defaultImages << ":/images/left.svg" << ":/images/right.svg" << ":/images/reload.svg";
    
    for(int i = 0; i < defaultImages.size(); ++i) {
        QListWidgetItem *item = new QListWidgetItem(ui->imageListWidget);
        item->setIcon(QIcon(defaultImages[i]));
        item->setText(tr("图片%1").arg(i+1));
        item->setTextAlignment(Qt::AlignCenter);
        item->setData(Qt::UserRole, defaultImages[i]);
        ui->imageListWidget->addItem(item);
    }
}

void MainWindow::showImageListContextMenu(const QPoint &pos)
{
    QPoint globalPos = ui->imageListWidget->mapToGlobal(pos);
    QMenu contextMenu;
    
    QAction *addAction = contextMenu.addAction(tr("添加图片"));
    QAction *removeAction = contextMenu.addAction(tr("删除选中图片"));
    QAction *clearAction = contextMenu.addAction(tr("清空列表"));
    
    // 如果没有选中项，禁用删除操作
    if(ui->imageListWidget->selectedItems().isEmpty()) {
        removeAction->setEnabled(false);
    }
    
    // 如果列表为空，禁用清空操作
    if(ui->imageListWidget->count() == 0) {
        clearAction->setEnabled(false);
    }
    
    // 显示菜单并获取用户选择的操作
    QAction *selectedAction = contextMenu.exec(globalPos);
    
    if(selectedAction == addAction) {
        onActionOpenTriggered();
    }
    else if(selectedAction == removeAction) {
        // 删除选中的图片项
        QList<QListWidgetItem*> items = ui->imageListWidget->selectedItems();
        for(QListWidgetItem *item : items) {
            delete ui->imageListWidget->takeItem(ui->imageListWidget->row(item));
        }
    }
    else if(selectedAction == clearAction) {
        // 清空列表
        ui->imageListWidget->clear();
    }
}

void MainWindow::displayImageInLabel(const QPixmap &pixmap)
{
    if(pixmap.isNull()) return;
    
    // 获取QLabel的大小
    QSize labelSize = imageLabel->size();
    
    // 如果标签尺寸有效，则计算缩放后的图片
    if(labelSize.width() > 0 && labelSize.height() > 0) {
        // 等比例缩放图片以适应标签大小
        QPixmap scaledPixmap = pixmap.scaled(
            labelSize, 
            Qt::KeepAspectRatio, 
            Qt::SmoothTransformation
        );
        
        // 设置图片到标签
        imageLabel->setPixmap(scaledPixmap);
    } else {
        // 直接设置原始图片
        imageLabel->setPixmap(pixmap);
    }
}

void MainWindow::onImageItemClicked(QListWidgetItem *item)
{
    // 获取选中的图片路径
    QString imagePath = item->data(Qt::UserRole).toString();
    qDebug() << "选中图片: " << imagePath;
    
    // 在Label中显示选中的图片
    if(!imagePath.isEmpty()) {
        QPixmap pixmap(imagePath);
        if(!pixmap.isNull()) {
            currentPixmap = pixmap; // 保存当前图片
            displayImageInLabel(pixmap); // 在Label中显示图片
        } else {
            // 显示错误信息
            QPixmap errorPix(imageLabel->size());
            errorPix.fill(Qt::white);
            QPainter painter(&errorPix);
            painter.setPen(Qt::red);
            painter.setFont(QFont("Arial", 12));
            painter.drawText(errorPix.rect(), Qt::AlignCenter, tr("无法加载图片"));
            imageLabel->setPixmap(errorPix);
        }
    }
}

void MainWindow::onActionOpenTriggered() {
    // 打开文件对话框选择图片
    QStringList files = QFileDialog::getOpenFileNames(
        this,
        tr("选择图片文件"),
        QDir::homePath(),
        tr("图片文件 (*.png *.jpg *.jpeg *.bmp *.svg)")
    );
    
    if(files.isEmpty()) return;
    
    // 清空现有列表
    ui->imageListWidget->clear();
    
    // 添加选择的图片到列表
    for(int i = 0; i < files.size(); ++i) {
        QListWidgetItem *item = new QListWidgetItem(ui->imageListWidget);
        item->setIcon(QIcon(files[i]));
        item->setText(QFileInfo(files[i]).fileName());
        item->setTextAlignment(Qt::AlignCenter);
        item->setData(Qt::UserRole, files[i]);
        ui->imageListWidget->addItem(item);
    }
    
    // 如果有图片，选中第一张
    if(ui->imageListWidget->count() > 0) {
        ui->imageListWidget->setCurrentRow(0);
        onImageItemClicked(ui->imageListWidget->item(0));
    }
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    // 传递事件给基类处理
    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    
    // 窗口大小改变时重新调整图像大小
    if (!currentPixmap.isNull()) {
        displayImageInLabel(currentPixmap);
    }
}
