#include "imagelist.h"
#include <QDebug>
#include <QFileInfo>
#include <QIcon>
#include <QMainWindow>

ImageList::ImageList(QWidget *parent) : QObject(parent)
{
    // 初始化界面
    initUi();
    
    // 连接信号槽
    connect(imageListWidget, &QListWidget::itemClicked, this, &ImageList::onItemClicked);
    connect(imageListWidget, &QListWidget::customContextMenuRequested, this, &ImageList::showContextMenu);
    connect(imageListDock, &QDockWidget::topLevelChanged, this, &ImageList::handleFloatingChanged);
    
    // 连接停靠区域变化信号
    connect(imageListDock, &QDockWidget::dockLocationChanged, this, &ImageList::onDockLocationChanged);
}

ImageList::~ImageList()
{
    // QDockWidget 会作为子对象自动删除
}

void ImageList::initUi()
{
    // 创建停靠窗口
    imageListDock = new QDockWidget(tr("图片列表"));
    imageListDock->setObjectName("imageListDock");
    imageListDock->setAllowedAreas(Qt::AllDockWidgetAreas);
    imageListDock->setFeatures(QDockWidget::DockWidgetMovable | 
                              QDockWidget::DockWidgetFloatable |
                              QDockWidget::DockWidgetClosable);
    
    // 创建列表控件
    imageListWidget = new QListWidget(imageListDock);
    
    // 设置列表样式 - 使用更小的图标和网格
    imageListWidget->setViewMode(QListWidget::IconMode);
    imageListWidget->setIconSize(QSize(48, 48));  // 更小的图标
    imageListWidget->setGridSize(QSize(60, 60));  // 更小的网格
    imageListWidget->setResizeMode(QListWidget::Adjust);
    imageListWidget->setMovement(QListWidget::Static);
    imageListWidget->setSpacing(3);  // 更小的间距
    
    // 自适应内容大小
    imageListWidget->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    
    // 设置上下文菜单
    imageListWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    
    // 创建容器和布局
    QWidget *container = new QWidget(imageListDock);
    QVBoxLayout *layout = new QVBoxLayout(container);
    layout->setContentsMargins(1, 1, 1, 1);  // 减少内边距
    layout->addWidget(imageListWidget);
    container->setLayout(layout);
    
    // 设置内容部件
    imageListDock->setWidget(container);
    
    // 应用样式
    initStyles();
    
    // 设置停靠窗口的高度限制
    imageListDock->setMinimumHeight(80);   // 最小高度
    imageListDock->setMaximumHeight(150);  // 最大高度
}

void ImageList::initStyles()
{
    // 设置停靠窗口样式
    imageListDock->setStyleSheet(
        "QDockWidget {"
        "    border: 2px solid #a0a0a0;"
        "    border-radius: 4px;"
        "}"
        "QDockWidget::title {"
        "    background: #f0f0f0;"
        "    padding: 5px;"
        "    border-bottom: 1px solid #a0a0a0;"
        "}"
    );
}

QDockWidget* ImageList::getDockWidget() const
{
    return imageListDock;
}

void ImageList::setVisible(bool visible)
{
    imageListDock->setVisible(visible);
}

bool ImageList::isVisible() const
{
    return imageListDock->isVisible();
}

void ImageList::addImage(const QString &path)
{
    if (path.isEmpty()) return;
    
    QListWidgetItem *item = new QListWidgetItem(imageListWidget);
    item->setIcon(QIcon(path));
    item->setText(QFileInfo(path).fileName());
    item->setTextAlignment(Qt::AlignCenter);
    item->setData(Qt::UserRole, path);
    imageListWidget->addItem(item);
}

void ImageList::addImages(const QStringList &paths)
{
    if (paths.isEmpty()) return;
    
    // 记录添加前的图片数量，用于后续选中第一张新图片
    int originalCount = imageListWidget->count();
    
    // 添加所有图片
    for (const QString &path : paths) {
        addImage(path);
    }
    
    // 如果添加了新图片，选中第一张新添加的图片
    if (imageListWidget->count() > originalCount) {
        imageListWidget->setCurrentRow(originalCount);
        onItemClicked(imageListWidget->item(originalCount));
    }
}

void ImageList::clear()
{
    imageListWidget->clear();
    currentPixmap = QPixmap();
    currentPath = QString();
}

int ImageList::count() const
{
    return imageListWidget->count();
}

QListWidgetItem* ImageList::currentItem() const
{
    return imageListWidget->currentItem();
}

void ImageList::setCurrentItem(QListWidgetItem* item)
{
    if (item) {
        imageListWidget->setCurrentItem(item);
    }
}

QPixmap ImageList::getCurrentPixmap() const
{
    return currentPixmap;
}

void ImageList::onItemClicked(QListWidgetItem *item)
{
    if (!item) return;
    
    // 获取图片路径
    QString imagePath = item->data(Qt::UserRole).toString();
    qDebug() << "选中图片: " << imagePath;
    
    // 如果路径不为空，加载图片
    if (!imagePath.isEmpty()) {
        QPixmap pixmap(imagePath);
        if (!pixmap.isNull()) {
            currentPixmap = pixmap;
            currentPath = imagePath;
            emit imageSelected(pixmap, imagePath);
        }
    }
}

void ImageList::showContextMenu(const QPoint &pos)
{
    // 获取点击位置的项目
    QListWidgetItem *item = imageListWidget->itemAt(pos);
    if (!item) return;
    
    // 创建右键菜单
    QMenu contextMenu;
    QAction *deleteAction = contextMenu.addAction(tr("删除图片"));
    
    // 显示菜单并获取用户选择
    QAction *selectedAction = contextMenu.exec(imageListWidget->mapToGlobal(pos));
    
    // 处理用户选择
    if (selectedAction == deleteAction) {
        imageListWidget->setCurrentItem(item);
        deleteCurrentItem();
    }
}

void ImageList::deleteCurrentItem()
{
    QListWidgetItem *item = imageListWidget->currentItem();
    if (!item) return;
    
    // 获取当前项的行号和路径
    int row = imageListWidget->row(item);
    QString path = item->data(Qt::UserRole).toString();
    bool isCurrentImage = (path == currentPath);
    
    // 删除项目
    delete imageListWidget->takeItem(row);
    
    // 如果删除的是当前显示的图片
    if (isCurrentImage) {
        int count = imageListWidget->count();
        if (count > 0) {
            // 选择下一个或前一个图片
            int newRow = row;
            if (newRow >= count) {
                newRow = count - 1;
            }
            
            QListWidgetItem *newItem = imageListWidget->item(newRow);
            imageListWidget->setCurrentItem(newItem);
            onItemClicked(newItem);
        } else {
            // 如果没有图片了，清空当前图片
            currentPixmap = QPixmap();
            currentPath = QString();
            emit imageDeleted(path);
        }
    }
}

void ImageList::handleFloatingChanged(bool isFloating)
{
    if (isFloating) {
        // 浮动状态下设置合适的大小
        imageListDock->resize(500, 300); // 浮动时更宽但更低
    } else {
        // 简化代码，移除复杂的停靠区域检查
        imageListDock->setMinimumHeight(0);
        imageListDock->setMaximumHeight(150);  // 统一设置最大高度
    }
}

void ImageList::onDockLocationChanged(Qt::DockWidgetArea area)
{
    if (area == Qt::BottomDockWidgetArea || area == Qt::TopDockWidgetArea) {
        // 底部或顶部停靠时优化为水平布局
        imageListWidget->setFlow(QListWidget::LeftToRight);
        imageListWidget->setWrapping(true);
    } else {
        // 左侧或右侧停靠时优化为垂直布局
        imageListWidget->setFlow(QListWidget::TopToBottom);
        imageListWidget->setWrapping(true);
    }
    
    // 移除动态高度调整，使用固定值
    imageListDock->setMaximumHeight(150);
}