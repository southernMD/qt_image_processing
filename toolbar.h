#ifndef TOOLBAR_H
#define TOOLBAR_H

#include <QDockWidget>
#include <QWidget>
#include <QPushButton>
#include <QList>
#include <QBoxLayout>

class ToolBar : public QObject
{
    Q_OBJECT

public:
    explicit ToolBar(QWidget *parent = nullptr);
    ~ToolBar();

    // 获取工具栏部件
    QDockWidget* getDockWidget() const;

    // 布局相关
    void recreateLayout(bool isHorizontal);
    void adjustLayout(Qt::DockWidgetArea area);

    // 状态操作
    void setVisible(bool visible);
    bool isVisible() const;

signals:
    // 工具栏按钮点击信号
    void buttonClicked(int index);

private slots:
    // 处理工具栏浮动状态改变
    void handleFloatingChanged(bool isFloating);

private:
    QDockWidget *toolDockWidget;
    QWidget *toolWidget;
    QList<QPushButton*> toolButtons;

    // 初始化样式和按钮
    void initStyles();
    void createButtons(); // 新方法，用于创建按钮
};

#endif // TOOLBAR_H
