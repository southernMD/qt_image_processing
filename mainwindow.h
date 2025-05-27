#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDockWidget>
#include <QPushButton>
#include <QListWidgetItem>
#include <QMenu>
#include <QPixmap>
#include <QLabel>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onActionOpenTriggered();
    void onImageItemClicked(QListWidgetItem *item);
    void showImageListContextMenu(const QPoint &pos);

protected:
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    void changeEvent(QEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    Ui::MainWindow *ui;
    bool toolbarWasVisible;
    QDockWidget *toolDockWidget;
    QWidget *toolWidget;
    QList<QPushButton*> toolButtons;
    QAction *toggleToolbarAction;
    QPixmap currentPixmap;  // 当前显示的图片
    
    // 使用QLabel替代WebView和GraphicsView
    QLabel *imageLabel;

    void recreateToolbarLayout(bool isHorizontal);
    void adjustToolbarLayout(Qt::DockWidgetArea area);
    void initImageList();
    void displayImageInLabel(const QPixmap &pixmap); // 在Label中显示图片
};
#endif // MAINWINDOW_H
