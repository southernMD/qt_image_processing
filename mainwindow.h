#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDockWidget>
#include <QPushButton>
#include <QListWidgetItem>
#include <QMenu>
#include <QPixmap>
#include <QWebEngineView>
#include <QAction>
#include "toolbar.h" // 引入新的工具栏类
#include "imagelist.h"
#include <QLabel>
#include <QTcpServer>
#include <QFile>
#include <QMediaPlayer>
#include <QVideoSink>
#include <QVideoFrame>
#include <QImage>


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    void changeEvent(QEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void onActionOpenTriggered();
    void initializeCanvas();
    void handleToolbarButtonClicked(int index);
    void onImageSelected(const QPixmap &pixmap, const QString &path);
    void onImageDeleted(const QString &path);
    void displayImageInCanvas(const QPixmap &pixmap);
    void updateImageWithBase64(const QString &imageDataUrl);

    void createThresholdSlider();
    void applyBinarization(int threshold);

    void createGammaSlider();
    void applyGammaTransform(float gamma);

    void createEdgeDetectionSlider();
    void applyEdgeDetection(int threshold);

    void saveImage();

    void showAboutDialog();
    void onActionOpenVideoTriggered();
    void onVideoFrameChanged(const QVideoFrame &frame);
    // void applyEffectToAllFrames(const QString &effectName, const QVariant &params = QVariant());
    // void saveProcessedFrames();
    // void captureAllFrames(const QString &videoPath);
    // void showFrameAtIndex(int index);

private:
    Ui::MainWindow *ui;
    bool toolbarWasVisible;
    ToolBar *toolbar; // 使用新的工具栏类
    QAction *toggleToolbarAction;
    QPixmap currentPixmap;  // 当前显示的图片

    // 使用WebView替代QLabel
    QWebEngineView *webView;

    QAction *toggleImageListAction;
    bool imageListWasVisible;
    ImageList *imageList;

    QSlider *thresholdSlider;       // 阈值滑块
    QLabel *thresholdLabel;         // 显示当前阈值的标签
    QWidget *sliderContainer;       // 包含滑块和标签的容器
    int currentThreshold = 128;     // 当前阈值
    bool sliderVisible = false; // 阈值滑块是否可见

    QDockWidget *gammaDock = nullptr;
    QSlider *gammaSlider = nullptr;
    QLabel *gammaLabel = nullptr;
    bool gammaVisible = false;

    QDockWidget *edgeDock = nullptr;
    QSlider *edgeSlider = nullptr;
    QLabel *edgeLabel = nullptr;
    bool edgeVisible = false;

    QMediaPlayer *mediaPlayer = nullptr;
    QVideoSink *videoSink = nullptr;
    QPushButton *returnButton = nullptr;
    bool videoProcessingMode = false;

    void setupVideoFrameProcessing();
    void cleanupVideoMode();

    QVector<QImage> capturedFrames;
    bool isCapturingFrames = false;
    int currentFrameIndex = 0;
    QString currentVideoPath;
};
#endif // MAINWINDOW_H
