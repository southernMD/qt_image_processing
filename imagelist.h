#ifndef IMAGELIST_H
#define IMAGELIST_H

#include <QDockWidget>
#include <QListWidget>
#include <QListWidgetItem>
#include <QWidget>
#include <QVBoxLayout>
#include <QMenu>
#include <QAction>
#include <QPixmap>

class ImageList : public QObject
{
    Q_OBJECT

public:
    explicit ImageList(QWidget *parent = nullptr);
    ~ImageList();

    // 获取图片列表停靠窗口
    QDockWidget* getDockWidget() const;

    // 设置/获取可见性
    void setVisible(bool visible);
    bool isVisible() const;

    // 图片操作
    void addImage(const QString &path);
    void addImages(const QStringList &paths);
    void clear();
    int count() const;

    // 获取当前选中项
    QListWidgetItem* currentItem() const;
    void setCurrentItem(QListWidgetItem* item);

    // 获取当前显示图片
    QPixmap getCurrentPixmap() const;

signals:
    // 图片选中信号
    void imageSelected(const QPixmap &pixmap, const QString &path);
    // 图片删除信号
    void imageDeleted(const QString &path);

private slots:
    // 处理图片列表项点击
    void onItemClicked(QListWidgetItem *item);
    // 处理右键菜单请求
    void showContextMenu(const QPoint &pos);
    // 处理浮动状态改变
    void handleFloatingChanged(bool isFloating);
    // 处理停靠区域变化
    void onDockLocationChanged(Qt::DockWidgetArea area);

private:
    QDockWidget *imageListDock;
    QListWidget *imageListWidget;
    QPixmap currentPixmap;
    QString currentPath;

    // 初始化界面和样式
    void initUi();
    void initStyles();

    // 删除当前选中项
    void deleteCurrentItem();
};

#endif // IMAGELIST_H
