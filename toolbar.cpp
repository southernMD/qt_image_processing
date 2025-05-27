#include "toolbar.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTimer>

ToolBar::ToolBar(QWidget *parent) : QObject(parent)
{
    // 创建可停靠工具部件
    toolDockWidget = new QDockWidget(tr("工具栏"), parent);
    toolDockWidget->setAllowedAreas(Qt::AllDockWidgetAreas);
    toolDockWidget->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    
    // 创建新的工具栏容器
    toolWidget = new QWidget(toolDockWidget);
    
    // 初始化样式
    initStyles();
    
    // 创建按钮
    createButtons();
    
    // 设置工具栏的Widget
    toolDockWidget->setWidget(toolWidget);
    
    // 默认初始化为垂直布局
    recreateLayout(false);
    
    // 添加浮动状态更改的处理
    connect(toolDockWidget, &QDockWidget::topLevelChanged, this, &ToolBar::handleFloatingChanged);
}

ToolBar::~ToolBar()
{
    // QDockWidget和toolWidget将由父窗口清理
}

QDockWidget* ToolBar::getDockWidget() const
{
    return toolDockWidget;
}

void ToolBar::initStyles()
{
    // 设置工具栏风格和内边距
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
    
    // 设置工具栏内容容器的样式
    toolWidget->setStyleSheet(
        "QWidget {"
        "    background-color: #f8f8f8;"
        "    border-radius: 2px;"
        "    border: 1px solid #c0c0c0;"
        "}"
    );
}

void ToolBar::createButtons()
{
    // 创建按钮基本样式
    QString buttonStyle = 
        "QPushButton {"
        "    min-width: 100px;"
        "    min-height: 60px;"
        "    max-width: 100px;"
        "    max-height: 60px;"
        "    border: none;"
        "    background-color: #e0e0e0;"
        "    padding: 5px;"
        "    color: #333;"
        "    border-radius: 5px;"
        "}"
        "QPushButton:hover {"
        "    background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #e0e0e0, stop:1 #d0d0d0);"
        "}"
        "QPushButton:pressed {"
        "    background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #d0d0d0, stop:1 #c0c0c0);"
        "}";
    
    struct ButtonInfo {
        QString text;
        QString tooltip;
    };
    
    // 定义按钮信息
    QVector<ButtonInfo> buttonInfos = {
        {tr("灰度化"), tr("将图像转换为灰度")},
        {tr("二值化"), tr("将图像进行二值化处理")},
        {tr("均值滤波"), tr("对图像进行均值滤波处理")},
        {tr("伽马变化"), tr("对图像进行均值伽马变化")},
        {tr("边缘检测"), tr("对图像进行边缘检测")},
        {tr("重置"), tr("将图像恢复到原状态")},
        {tr("保存"), tr("保存当前图片")},
        {tr("马赛克"), tr("马赛克")},
    };
    
    // 创建按钮
    for (int i = 0; i < buttonInfos.size(); ++i) {
        const ButtonInfo &info = buttonInfos[i];
        
        QPushButton *btn = new QPushButton(toolWidget);
        btn->setText(info.text);
        btn->setToolTip(info.tooltip);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet(buttonStyle);
        
        // 设置按钮尺寸
        btn->setMinimumSize(100, 60);
        btn->setMaximumSize(100, 60);
        
        // 连接点击信号
        const int buttonIndex = i;
        connect(btn, &QPushButton::clicked, [this, buttonIndex]() {
            emit buttonClicked(buttonIndex);
        });
        
        // 保存按钮引用
        toolButtons.append(btn);
    }
}

void ToolBar::recreateLayout(bool isHorizontal)
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

void ToolBar::adjustLayout(Qt::DockWidgetArea area)
{
    // 根据停靠区域调整布局
    bool isHorizontal = (area == Qt::TopDockWidgetArea || area == Qt::BottomDockWidgetArea);
    
    // 重新创建适当的布局
    recreateLayout(isHorizontal);
    
    // 移除定时器，直接设置高度
    if (isHorizontal) {
        toolDockWidget->setMaximumHeight(100);
        toolWidget->updateGeometry();
    }
}

void ToolBar::handleFloatingChanged(bool isFloating)
{
    // 浮动状态下使用水平布局
    if (isFloating) {
        recreateLayout(true);
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
    }
}

void ToolBar::setVisible(bool visible)
{
    toolDockWidget->setVisible(visible);
}

bool ToolBar::isVisible() const
{
    return toolDockWidget->isVisible();
}