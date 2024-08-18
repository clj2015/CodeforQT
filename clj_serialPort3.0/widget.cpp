#include "widget.h"
#include "ui_widget.h"
#include <QDateTime>

Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget)
{
    ui->setupUi(this);

    // 初始化控制参数
    writeCntTotal = 0;
    readCntTotal = 0;
    serialStatus = false;
    buttonIndex = 0;

    // 初始化控件
    ui->btnSendText->setEnabled(false);
    ui->checkBoxSendInTime->setEnabled(false);
    ui->checkBoxSendNewLine->setEnabled(false);
    ui->checkBoxHexSend->setEnabled(false);

    // 创建并初始化串口对象
    serialPort = new QSerialPort(this);

    // 定义一个定时器，每100ms刷新系统时间
    QTimer *getSysTimeTimer = new QTimer(this);
    connect(getSysTimeTimer, &QTimer::timeout, this, &Widget::time_reFlash);
    getSysTimeTimer->start(100);

    // 定时发送的定时器
    timer = new QTimer(this);
    buttonConTimer = new QTimer(this);
    connect(buttonConTimer, &QTimer::timeout, this, &Widget::buttons_handler);
    connect(serialPort, &QSerialPort::readyRead, this, &Widget::on_SerialData_readyToRead);
    connect(timer, &QTimer::timeout, this, &Widget::on_btnSendText_clicked);
    connect(ui->comboBox_serialnum, &MyComboBox::refresh, this, &Widget::refreshSerialName);

    // 设置默认选项
    ui->comboBox_boautrate->setCurrentIndex(6);
    ui->comboBox_databit->setCurrentIndex(3);

    // 刷新串口名
    refreshSerialName();

    // 显示当前串口状态
    ui->labelSendStatus->setText(ui->comboBox_serialnum->currentText() + " Not Open!");

    // 动态添加按钮、文本框和复选框的关联
    for(int i = 1; i <= 9; i++){
        QString btnName = QString("pushButton_%1").arg(i);
        QPushButton *btn = findChild<QPushButton *>(btnName);
        if(btn){
            btn->setProperty("buttonId", i);
            buttons.append(btn);
            connect(btn, &QPushButton::clicked, this, &Widget::on_command_button_clicked);
        }

        QString lineEditName = QString("lineEdit_%1").arg(i);
        QLineEdit *lineEdit = findChild<QLineEdit *>(lineEditName);
        lineEdits.append(lineEdit);

        QString checkBoxName = QString("checkBox_%1").arg(i);
        QCheckBox *checkBox = findChild<QCheckBox *>(checkBoxName);
        checkBoxs.append(checkBox);
    }
}

Widget::~Widget()
{
    delete ui;
}

void Widget::on_btnSendText_clicked()
{
    qint64 writeCnt = 0;  // 使用 qint64 代替 int
    QString sendData = ui->lineEditSendText->text();

    if(ui->checkBoxHexSend->isChecked()){
        //去除输入字符中的空格
        QString hexString=sendData.remove(' ');
        QByteArray tmpArray = hexString.toLocal8Bit();

        for(char c : tmpArray)
        {
            if(!std::isxdigit(c))
            {
                ui->labelSendStatus->setText("Error Input!");
                return;
            }
        }
        QByteArray arraySend = QByteArray::fromHex(tmpArray);

        if(ui->checkBoxSendNewLine->isChecked()){
            arraySend.append("\r\n");
        }


        writeCnt = serialPort->write(arraySend);  // writeCnt 现在是 qint64 类型
    } else {
        if(ui->checkBoxSendNewLine->isChecked()){
            sendData.append("\r\n");
        }
        writeCnt = serialPort->write(sendData.toLocal8Bit());  // 同样在这里使用 qint64
    }

    if(writeCnt == -1){
        ui->labelSendStatus->setText("Send Error!");
    } else {
        writeCntTotal += writeCnt;
        ui->labelSendStatus->setText("Send OK!");
        ui->labelSendCnt->setText("Sent:" + QString::number(writeCntTotal));

        if(sendData != sendBak){
            ui->textEditRecord->append(sendData);
            sendBak = sendData;
        }
    }
}

void Widget::on_SerialData_readyToRead()
{
    QByteArray revMessage = serialPort->readAll();
    if(!revMessage.isEmpty()){
        QString message;
        if(ui->checkBoxDisplay->isChecked()){
            message = revMessage.toHex().toUpper();
        } else {
            message = QString(revMessage);
        }

        if(ui->checkBoxLine->isChecked()){
            message.append("\n");
        }

        if(ui->checkBoxRevTime->isChecked()){
            getSysTime();
            message.prepend("[" + myTime + "] ");
        }

        ui->textEditRev->insertPlainText(message);
        readCntTotal += revMessage.size();
        ui->labelRevCnt->setText("Received:" + QString::number(readCntTotal));
        ui->textEditRev->moveCursor(QTextCursor::End);
        ui->textEditRev->ensureCursorVisible();
    }
}

void Widget::on_btnCloseOpenSerial_clicked(bool checked)
{
    if(checked){
        serialPort->setPortName(ui->comboBox_serialnum->currentText());
        serialPort->setBaudRate(ui->comboBox_boautrate->currentText().toInt());
        serialPort->setDataBits(QSerialPort::DataBits(ui->comboBox_databit->currentText().toUInt()));

        switch(ui->comboBox_jiaoyan->currentIndex()){
            case 0: serialPort->setParity(QSerialPort::NoParity); break;
            case 1: serialPort->setParity(QSerialPort::EvenParity); break;
            case 2: serialPort->setParity(QSerialPort::MarkParity); break;
            case 3: serialPort->setParity(QSerialPort::OddParity); break;
            case 4: serialPort->setParity(QSerialPort::SpaceParity); break;
            default: serialPort->setParity(QSerialPort::UnknownParity); break;
        }

        serialPort->setStopBits(QSerialPort::StopBits(ui->comboBox_stopbit->currentText().toUInt()));
        serialPort->setFlowControl(QSerialPort::NoFlowControl);

        if(serialPort->open(QIODevice::ReadWrite)){
            ui->comboBox_databit->setEnabled(false);
            ui->comboBox_fileCon->setEnabled(false);
            ui->comboBox_jiaoyan->setEnabled(false);
            ui->comboBox_stopbit->setEnabled(false);
            ui->comboBox_boautrate->setEnabled(false);
            ui->comboBox_serialnum->setEnabled(false);
            ui->btnCloseOpenSerial->setText("关闭串口");
            ui->btnSendText->setEnabled(true);
            ui->checkBoxSendInTime->setEnabled(true);
            ui->checkBoxSendNewLine->setEnabled(true);
            ui->checkBoxHexSend->setEnabled(true);
            ui->labelSendStatus->setText(ui->comboBox_serialnum->currentText() + " isOpen!");
        } else {
            QMessageBox::warning(this, "打开串口错误", "打开失败，串口可能被占用或拔出！");
        }
    } else {
        serialPort->close();
        ui->btnCloseOpenSerial->setText("打开串口");
        ui->comboBox_databit->setEnabled(true);
        ui->comboBox_fileCon->setEnabled(true);
        ui->comboBox_jiaoyan->setEnabled(true);
        ui->comboBox_stopbit->setEnabled(true);
        ui->comboBox_boautrate->setEnabled(true);
        ui->comboBox_serialnum->setEnabled(true);
        ui->btnSendText->setEnabled(false);
        ui->checkBoxSendInTime->setEnabled(false);
        ui->checkBoxSendInTime->setCheckState(Qt::Unchecked);
        timer->stop();
        ui->lineEditTimeEach->setEnabled(true);
        ui->lineEditSendText->setEnabled(true);
        ui->checkBoxSendNewLine->setEnabled(false);
        ui->checkBoxHexSend->setEnabled(false);
        ui->labelSendStatus->setText(ui->comboBox_serialnum->currentText() + " isClose!");
    }
}

void Widget::on_checkBoxSendInTime_clicked(bool checked)
{
    if(checked){
        ui->lineEditTimeEach->setEnabled(false);
        ui->lineEditSendText->setEnabled(false);
        timer->start(ui->lineEditTimeEach->text().toInt());
    } else {
        timer->stop();
        ui->lineEditTimeEach->setEnabled(true);
        ui->lineEditSendText->setEnabled(true);
    }
}

void Widget::on_btnrevClear_clicked()
{
    ui->textEditRev->clear();
}

void Widget::on_btnrevSave_clicked()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("保存文件"),
                                                    "D:/Study/serialData.txt",
                                                    tr("Text (*.txt)"));
    if(!fileName.isEmpty()){
        QFile file(fileName);
        if(file.open(QIODevice::WriteOnly | QIODevice::Text)){
            QTextStream out(&file);
            out << ui->textEditRev->toPlainText();
            file.close();
        }
    }
}

void Widget::time_reFlash()
{
    getSysTime();
    ui->labelCurrentTime->setText(myTime);
}

void Widget::getSysTime()
{
    QDateTime currentTime = QDateTime::currentDateTime();
    myTime = currentTime.toString("yyyy-MM-dd HH:mm:ss");
}

void Widget::on_checkBoxDisplay_clicked(bool checked)
{
    if(checked){
        QString text = ui->textEditRev->toPlainText().toUtf8().toHex(' ').toUpper();
        ui->textEditRev->setText(text);
    } else {
        QByteArray text = QByteArray::fromHex(ui->textEditRev->toPlainText().remove(' ').toUtf8());
        ui->textEditRev->setText(text);
    }
    ui->textEditRev->moveCursor(QTextCursor::End);
    ui->textEditRev->ensureCursorVisible();
}

void Widget::on_btnHideTable_clicked(bool checked)
{
    if(checked){
        ui->btnHideTable->setText("拓展面板");
        ui->groupBoxTexts->hide();
    } else {
        ui->btnHideTable->setText("隐藏面板");
        ui->groupBoxTexts->show();
    }
}

void Widget::on_btnHideHistory_clicked(bool checked)
{
    if(checked){
        ui->btnHideHistory->setText("显示历史");
        ui->groupBoxRecord->hide();
    } else {
        ui->btnHideHistory->setText("隐藏历史");
        ui->groupBoxRecord->show();
    }
}

void Widget::refreshSerialName()
{
    ui->comboBox_serialnum->clear();
    QList<QSerialPortInfo> serialList = QSerialPortInfo::availablePorts();
    for(const QSerialPortInfo &serialInfo : serialList){
        ui->comboBox_serialnum->addItem(serialInfo.portName());
    }
    ui->labelSendStatus->setText("Com Refreshed!");
}

void Widget::on_command_button_clicked()
{
    QPushButton *btn = qobject_cast<QPushButton *>(sender());
    if(btn){
        int num = btn->property("buttonId").toInt();
        QString lineEditName = QString("lineEdit_%1").arg(num);
        QLineEdit *lineEdit = findChild<QLineEdit *>(lineEditName);
        if(lineEdit && !lineEdit->text().isEmpty()){
            ui->lineEditSendText->setText(lineEdit->text());
        }
        QString checkBoxName = QString("checkBox_%1").arg(num);
        QCheckBox *checkBox = findChild<QCheckBox *>(checkBoxName);
        if(checkBox){
            ui->checkBoxHexSend->setChecked(checkBox->isChecked());
        }
        on_btnSendText_clicked();
    }
}

void Widget::buttons_handler()
{
    if(buttonIndex < buttons.size()){
        QPushButton *btnTmp = buttons[buttonIndex];
        emit btnTmp->clicked();
        buttonIndex++;
    } else {
        buttonIndex = 0;
    }
}

void Widget::on_checkBox_send_clicked(bool checked)
{
    if(checked){
        ui->spinBox->setEnabled(false);
        buttonConTimer->start(ui->spinBox->text().toUInt());
    } else {
        ui->spinBox->setEnabled(true);
        buttonConTimer->stop();
    }
}

void Widget::on_btnInit_clicked()
{
    QMessageBox msgBox;
    msgBox.setWindowTitle("提示");
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setText("重置列表不可逆，确认是否重置？");
    QPushButton *yesButton = msgBox.addButton("是", QMessageBox::YesRole);
    msgBox.addButton("否", QMessageBox::NoRole);
    msgBox.exec();
    if(msgBox.clickedButton() == yesButton){
        for(int i = 0; i < lineEdits.size(); i++){
            lineEdits[i]->clear();
            checkBoxs[i]->setChecked(false);
        }
    }
}

void Widget::on_btnSave_clicked()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("保存文件"),
                                                    "D:/Study",
                                                    tr("文件类型 (*.txt)"));
    if(!fileName.isEmpty()){
        QFile file(fileName);
        if(file.open(QIODevice::WriteOnly | QIODevice::Text)){
            QTextStream out(&file);
            for(int i = 0; i < lineEdits.size(); i++){
                out << checkBoxs[i]->isChecked() << "," << lineEdits[i]->text() << "\n";
            }
            file.close();
        }
    }
}

void Widget::on_btnLoad_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("打开文件"),
                                                    "D:/Study",
                                                    tr("文件类型 (*.txt)"));
    if(!fileName.isEmpty()){
        QFile file(fileName);
        if(file.open(QIODevice::ReadOnly | QIODevice::Text)){
            QTextStream in(&file);
            int i = 0;
            while(!in.atEnd() && i < lineEdits.size()){
                QString line = in.readLine();
                QStringList parts = line.split(",");
                if(parts.size() == 2){
                    checkBoxs[i]->setChecked(parts[0].toUInt());
                    lineEdits[i]->setText(parts[1]);
                }
                i++;
            }
        }
    }
}
