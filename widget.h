#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QTimer>
#include <QPushButton>
#include <QLineEdit>
#include <QCheckBox>
#include <QTextStream>
#include <QFileDialog>
#include <QMessageBox>

namespace Ui {
class Widget;
}

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = nullptr);
    ~Widget();

private slots:
    void on_btnSendText_clicked();                   // 发送数据
    void on_SerialData_readyToRead();                // 读取串口数据
    void on_btnCloseOpenSerial_clicked(bool checked); // 打开/关闭串口
    void on_checkBoxSendInTime_clicked(bool checked); // 定时发送
    void on_btnrevClear_clicked();                   // 清空接收区
    void on_btnrevSave_clicked();                    // 保存接收数据
    void on_checkBoxDisplay_clicked(bool checked);   // 切换 HEX 显示
    void on_btnHideTable_clicked(bool checked);      // 隐藏/显示面板
    void on_btnHideHistory_clicked(bool checked);    // 隐藏/显示历史记录
    void refreshSerialName();                        // 刷新串口号
    void on_command_button_clicked();                // 处理多文本区按钮点击
    void buttons_handler();                          // 循环发送处理
    void on_checkBox_send_clicked(bool checked);     // 循环发送
    void on_btnInit_clicked();                       // 重置多文本区
    void on_btnSave_clicked();                       // 保存多文本区设置
    void on_btnLoad_clicked();                       // 载入多文本区设置
    void time_reFlash();                             // 刷新当前时间

private:
    Ui::Widget *ui;
    QSerialPort *serialPort;
    QTimer *timer;
    QTimer *buttonConTimer;
    QList<QPushButton *> buttons;
    QList<QLineEdit *> lineEdits;
    QList<QCheckBox *> checkBoxs;
    QString myTime;
    QString sendBak;
    qint64 writeCntTotal;
    qint64 readCntTotal;
    int buttonIndex;
    bool serialStatus;

    void getSysTime(); // 获取系统时间
};

#endif // WIDGET_H
