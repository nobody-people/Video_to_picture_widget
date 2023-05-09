#include <opencv2/opencv.hpp>
#include <iostream>
#include <iomanip>
#include <string>
#include <filesystem>
#include <QApplication>
#include <QWidget>
#include <QBoxLayout>
#include <QLabel>
#include <QDropEvent>
#include <QUrl>
#include <QMimeData>
#include <QFileDialog>
#include <QMessageBox>
#include <QPushButton>
#include <QProgressBar>
#include <QProgressDialog>
#include <QLineEdit>
#include <QDoubleValidator>

using namespace cv;
using namespace std;
namespace fs = std::filesystem;

class VideoToImageConverter : public QWidget {
public:
    VideoToImageConverter(QWidget *parent = nullptr) : QWidget(parent) {


        // 创建UI界面
        QVBoxLayout *layout = new QVBoxLayout(this);

        // 创建文本标签
        QLabel *label = new QLabel("将视频文件拖拽到【这里】，或者点击选择输入路径按钮", this);
        layout->addWidget(label, 0, Qt::AlignHCenter);
        // 设置窗口标题
        setWindowTitle("视频转图片小工具");
        // 添加输入框
        QHBoxLayout *frame_rate_layout = new QHBoxLayout;
        frame_rate_layout->addWidget(new QLabel("请输入要转换的每秒帧数："));
        m_frame_rate_edit = new QLineEdit;
        m_frame_rate_edit->setValidator(new QDoubleValidator(0.1, 100.0, 2, this));
        m_frame_rate_edit->setText(QString::number(getOriginalFrameRate()));
        frame_rate_layout->addWidget(m_frame_rate_edit);


        // 创建按钮
        QPushButton *input_button = new QPushButton("选择待转换视频", this);
        layout->addWidget(input_button, 0, Qt::AlignHCenter);
        QPushButton *output_button = new QPushButton("选择输出文件夹", this);
        layout->addWidget(output_button, 0, Qt::AlignHCenter);
        QPushButton *start_button = new QPushButton("开始转换", this);
        layout->addWidget(start_button, 0, Qt::AlignHCenter);

        // 创建布局
        QHBoxLayout *button_layout = new QHBoxLayout;
        button_layout->addWidget(input_button);
        button_layout->addStretch();
        button_layout->addWidget(output_button);

        // 将布局添加到主布局中
        layout->addStretch();
        layout->addLayout(button_layout);
        layout->addStretch();
        layout->addLayout(frame_rate_layout);
        layout->addWidget(start_button, 0, Qt::AlignHCenter);
        setLayout(layout);

        // 设置接受拖拽事件
        setAcceptDrops(true);

        // 连接按钮的点击事件
        connect(input_button, &QPushButton::clicked, this, &VideoToImageConverter::selectInputPath);
        connect(output_button, &QPushButton::clicked, this, &VideoToImageConverter::selectOutputFolder);
        connect(start_button, &QPushButton::clicked, this, &VideoToImageConverter::startConversion);
    }



    ~VideoToImageConverter() {
        if (m_progress_dialog) {
            m_progress_dialog->reset();
            m_progress_dialog->close();
            delete m_progress_dialog;
            m_progress_dialog = nullptr;
        }
    }
protected:
    QProgressDialog *m_progress_dialog;
    QLineEdit *m_frame_rate_edit;
    QLabel *m_original_frame_rate_label;
    void dragEnterEvent(QDragEnterEvent *event) override {
        // 验证拖拽事件
        if (event->mimeData()->hasUrls() && event->mimeData()->urls().count() == 1) {
            QUrl url = event->mimeData()->urls()[0];
            if (url.isLocalFile()) {
                QString path = url.toLocalFile();
                QFileInfo file_info(path);
                if (file_info.isFile() && file_info.suffix() == "mp4") {
                    event->acceptProposedAction();
                    return;
                }
            }
        }

        event->ignore();
    }

    void dropEvent(QDropEvent *event) override {
        // 处理拖拽事件
        QUrl url = event->mimeData()->urls()[0];
        QString path = url.toLocalFile();

        // 记录视频路径
        m_video_path = path;

        // 更新界面文本标签
        QLabel *label = findChild<QLabel *>();
        label->setText("视频文件已选择: " + path);
    }

    void selectInputPath() {
        // 打开文件选择对话框，让用户选择视频文件
        QString file_path = QFileDialog::getOpenFileName(this, "选择视频文件", ".",
                                                         "Video Files (*.mp4 *.avi *.mov)");

        // 记录视频路径
        m_video_path = file_path;

        // 更新界面文本标签
        QLabel *label = findChild<QLabel *>();
        label->setText("视频文件已选择: " + file_path);
    }

    void selectOutputFolder() {
        // 弹出文件选择对话框，让用户选择输出文件夹
        m_output_folder = QFileDialog::getExistingDirectory(this, "选择输出文件夹", ".",
                                                            QFileDialog::ShowDirsOnly);
        if (!m_output_folder.isEmpty()) {
            QLabel *label = findChild<QLabel *>();
            label->setText("输出文件夹已选择: " + m_output_folder);
        }
    }

    void startConversion() {
        // 如果视频路径为空，弹出错误消息
        if (m_video_path.isEmpty()) {
            QMessageBox::warning(this, "提示", "请先选择视频文件");
            return;
        }

        // 如果输出文件夹为空，弹出错误消息
        if (m_output_folder.isEmpty()) {
            QMessageBox::warning(this, "提示", "请先选择输出文件夹");
            return;
        }
        // 创建进度条窗口
        m_progress_dialog = new QProgressDialog("正在处理，请稍候...", "取消", 0, 100, this);
        m_progress_dialog->setWindowModality(Qt::ApplicationModal);
        m_progress_dialog->setCancelButton(nullptr);
        m_progress_dialog->setValue(0);
        m_progress_dialog->show();

        // 获取用户输入的帧率
        double frame_rate = m_frame_rate_edit->text().toDouble();
        if (frame_rate <= 0) {
            frame_rate = getOriginalFrameRate();
        }


        // 处理视频文件
        bool result = processVideo(m_video_path.toStdString(), m_output_folder.toStdString());

        if (result) {
            QMessageBox::information(this, "提示", "视频处理完成");
        } else {
            QMessageBox::warning(this, "提示", "视频处理失败");
        }

        // 关闭进度条窗口
        m_progress_dialog->reset();
        m_progress_dialog->close();
        delete m_progress_dialog;
        m_progress_dialog = nullptr;
    }
private:
    double getOriginalFrameRate() {
        double original_frame_rate = 0.0;
        if (m_video_path.isEmpty()) {
            return original_frame_rate;
        }
        VideoCapture cap(m_video_path.toStdString());
        if (cap.isOpened()) {
            original_frame_rate = cap.get(CAP_PROP_FPS);
            cap.release();
        }
        return original_frame_rate;
    }
    bool processVideo(const string &video_path, const string &output_folder) {
        // 创建输出文件夹
        if (!fs::create_directory(output_folder)) {
            if (!fs::is_directory(output_folder)) {
                cerr << "无法创建输出文件夹" << endl;
                return false;
            }
        }
        // 打开视频文件
        VideoCapture cap(video_path);
        if (!cap.isOpened()) {
            cerr << "无法打开视频文件" << endl;
            return false;
        }

        // 获取视频帧率
        double fps = cap.get(CAP_PROP_FPS);


        // 获取用户输入的帧率
        double frame_rate = fps;
        frame_rate = m_frame_rate_edit->text().toDouble();
        if (frame_rate <= 0) {
            frame_rate = getOriginalFrameRate();
        }

        // 计算间隔帧数
        int interval = static_cast<int>(getOriginalFrameRate() / frame_rate + 0.5);


        // 循环读取视频帧
        int frame_count = 0;
        int total_frames = cap.get(CAP_PROP_FRAME_COUNT) - 1;
        int last_progress_value = -1;
        Mat frame;
        while (cap.read(frame)) {
            if (frame_count % interval == 0) {
                // 保存当前帧到文件夹
                ostringstream filename;
                filename << output_folder << "/frame_" << setfill('0') << setw(6) << frame_count << ".jpg";
                imwrite(filename.str(), frame);
                // 更新进度条
                int progress_value = static_cast<int>(100.0 * frame_count / total_frames + 0.5);
                if (progress_value != last_progress_value) {
                    m_progress_dialog->setValue(progress_value);
                    last_progress_value = progress_value;
                }
            }
            frame_count++;


            // 如果用户点击了取消按钮，停止转换并返回false
            if (m_progress_dialog->wasCanceled()) {
                cap.release();
                return false;
            }
        }

        // 释放资源
        cap.release();

        return true;
    }

    QString m_video_path;
    QString m_output_folder;



};

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    VideoToImageConverter converter;
    converter.show();
    return app.exec();
}


