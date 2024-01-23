#include "udp.h"
#include <QThread>
#include <QApplication>

Udp::Udp(QObject *parent) :
    QObject(parent)
  , m_pseduo_process(nullptr)
  , m_pseduo_interconnect(nullptr)
{
    m_udp = new QUdpSocket(this);
    m_udp->bind(QHostAddress(__HOST_IP__),__HOST_PORT__, QUdpSocket::ShareAddress);
    connect(m_udp, &QUdpSocket::readyRead, this, &Udp::slotUdpReadyRead);

    m_pseduo_process = new QUdpSocket(this);
    m_pseduo_process->bind(QHostAddress(__PROCESS_IP__),__PROCESS_PORT__, QUdpSocket::ShareAddress);
    connect(m_pseduo_process, &QUdpSocket::readyRead, this, &Udp::slotPseudoProcessReadyRead);

    m_pseduo_interconnect = new QUdpSocket(this);
    m_pseduo_interconnect->bind(QHostAddress(__INTERCONNECT_IP__),__INTERCONNECT_PORT__, QUdpSocket::ShareAddress);
    connect(m_pseduo_interconnect, &QUdpSocket::readyRead, this, &Udp::slotPseudoInterConnectReadyRead);

    m_ping = new QProcess(this);
    connect(m_ping, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, QOverload<int, QProcess::ExitStatus>::of(&Udp::slotPingFinished));

    m_requestStart = false;
    m_requestReset = false;
    m_requestStop = false;

    m_arm_sta.mode = -1;
    m_arm_sta.remote = -1;
    m_arm_sta.working = -1;
    m_arm_sta.transmit = -1;
    m_arm_sta.codeH1 = -1;
    m_arm_sta.codeTL1 = -1;
    m_arm_sta.codeH3 = -1;
    m_arm_sta.codeTL3 = -1;
    m_arm_sta.cheap = -1;
    m_arm_sta.anten = -1;
    m_arm_sta.anten_sp = -1;
    m_arm_sta.csphat = -1;
    m_arm_sta.csphanxa = -1;

    resetTest();

    m_armOnline = false;
    m_fpgaOnline = false;
    m_stage = STAGE_IDLING;

    startTimer(500);
}

Udp::~Udp()
{
    delete m_udp;
}

void Udp::resetTest()
{
    m_step = 0;
    m_final = RESULT_NONE;
    m_interconnect_ok = false;
    m_process_ok = false;
    m_ping_ok = false;

    m_arm_cmd.mode = 1;
    m_arm_cmd.remote = 1;
    m_arm_cmd.working = 1;
    m_arm_cmd.transmit = 1;
    m_arm_cmd.codeH1 = 0;
    m_arm_cmd.codeTL1 = 1;
    m_arm_cmd.codeH3 = 1;
    m_arm_cmd.codeTL3 = 1;
    m_arm_cmd.cheap = 1;
    m_arm_cmd.csphat = 0;
    m_arm_cmd.anten = 1;
    m_arm_cmd.anten_sp = 1;
}

void Udp::slotStartTest(int module)
{
    m_module = module;
    m_requestStart = true;
}

void Udp::slotResetTest()
{
    m_requestReset = true;
}

void Udp::slotStopTest()
{
    m_requestStop = true;
}

void Udp::slotUdpReadyRead()
{
    QHostAddress sender;
    quint16 senderPort;
    while(m_udp->hasPendingDatagrams())
    {
        qint64 len = m_udp->pendingDatagramSize();
        char *dat = (char*) malloc(len);
        m_udp->readDatagram(dat, len, &sender, &senderPort);
        if((senderPort == __FPGA_PORT__) && (sender.toString().contains(__FPGA_IP__)))
        {
            if(len == __UDP_FPGA_LEN__ || len == 26){
                decodeFpga(dat);
            }
            else if(len == __UDP_ARM_LEN__){
                decodeArm(dat, len);
            }
        }

        free(dat);
    }
}

void Udp::slotPseudoProcessReadyRead()
{
    QHostAddress sender;
    quint16 senderPort;
    if(m_pseduo_process!=nullptr)
    while(m_pseduo_process->hasPendingDatagrams())
    {
        qint64 len = m_pseduo_process->pendingDatagramSize();
        char *dat = (char*) malloc(len);
        m_pseduo_process->readDatagram(dat, len, &sender, &senderPort);
        if((senderPort == __INTERCONNECT_PORT__) && (sender.toString().contains(__INTERCONNECT_IP__)))
        {
            m_interconnect_ok = true;
        }

        free(dat);
    }
}

void Udp::slotPseudoInterConnectReadyRead()
{
    QHostAddress sender;
    quint16 senderPort;
    if(m_pseduo_interconnect!=nullptr)
    while(m_pseduo_interconnect->hasPendingDatagrams())
    {
        qint64 len = m_pseduo_interconnect->pendingDatagramSize();
        char *dat = (char*) malloc(len);
        m_pseduo_interconnect->readDatagram(dat, len, &sender, &senderPort);
        if((senderPort == __PROCESS_PORT__) && (sender.toString().contains(__PROCESS_IP__)))
        {
            QByteArray data_found_ip("\x55\xaa\x11\x11\x11\x11\x55");
            if(len == data_found_ip.length() && (data_found_ip==QByteArray(dat,len)))
                m_process_ok = true;
        }

        free(dat);
    }
}

void Udp::slotIpChanged()
{
    m_udp->disconnectFromHost();
    m_pseduo_process->disconnectFromHost();
    m_pseduo_interconnect->disconnectFromHost();

//    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
//    QThread::sleep(6);
//    QApplication::restoreOverrideCursor();

    m_udp->bind(QHostAddress(__HOST_IP__),__HOST_PORT__, QUdpSocket::ShareAddress);
    m_pseduo_process->bind(QHostAddress(__PROCESS_IP__),__PROCESS_PORT__, QUdpSocket::ShareAddress);
    m_pseduo_interconnect->bind(QHostAddress(__INTERCONNECT_IP__),__INTERCONNECT_PORT__, QUdpSocket::ShareAddress);
}

void Udp::slotPingFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitStatus)
    if(exitCode==0){
        QByteArray output = m_ping->readAllStandardOutput();
        qDebug() << output;
        if(!output.isEmpty()){
            if (-1 != QString(output).indexOf("ttl", 0, Qt::CaseInsensitive)){
                m_ping_ok = true;
            }
        }
    }
}

int cnt_arm = 0, cnt_fpga = 0;
void Udp::timerEvent(QTimerEvent *event)
{   
    Q_UNUSED(event)

    if(cnt_arm>=5) m_armOnline = false;
    else cnt_arm++;

    if(cnt_fpga>=5) m_fpgaOnline = false;
    else cnt_fpga++;

    sendToFpga();

    testLoop();

    if(m_module == MODULE_INTERCONNECT){
        QByteArray data_found_ip("\x55\xaa\x11\x11\x11\x11\x55");
        m_pseduo_process->writeDatagram(data_found_ip, QHostAddress(__INTERCONNECT_IP__), __INTERCONNECT_PORT__);
    }

//    if(m_module == MODULE_PROCESS && m_stage!=STAGE_IDLE && !m_ping_ok){
//        m_ping->start("ping", QStringList() << "-n" << "1" << "192.168.1.240");
//    }
}

void Udp::decodeFpga(char *data)
{
    if(data[0] == char(0x55) && data[1] == char(0xff)){
        if((data[2] ^ data[3] ^ data[4] ^ data[5] ^ data[6]) == 0){
            m_fpgaOnline = true; cnt_fpga = 0;
            switch (data[2]) {
            case FPGA_STA_IDLE:
                m_status = FPGA_STA_IDLE;
                break;
            case FPGA_STA_READY:
                if(data[3] == m_module)
                    m_status = FPGA_STA_READY;
                else
                    m_status = FPGA_STA_MODULE_WRONG;
                break;
            case FPGA_STA_FPGA_TEST:
                m_status = FPGA_STA_FPGA_TEST;
                m_result = data[3];
                break;
            case FPGA_STA_COMBINE_TEST:
                m_status = FPGA_STA_COMBINE_TEST;
                m_step = data[3]&0x0f;
                m_result = data[3]&0xf0;
                break;
            default:
                m_status = FPGA_STA_UNKNOWN;
                break;
            }
        }
    }
}

void Udp::decodeArm(char *data, int len)
{
    int nframes = len/__FRAME_ARM_LEN__;
    for(int i=0; i<nframes; i++){
        char *dat = &data[i*__FRAME_ARM_LEN__];
        if(dat[0]==char(0x55)){
            if((dat[1] ^ dat[2] ^ dat[3] ^ dat[4] ^ dat[5] ^ dat[6]) == char(0xFF)){
                m_armOnline = true; cnt_arm = 0;
                switch (dat[1]) {
                case 0x01:
                    if(dat[2] == char(0x52))
                        m_arm_sta.remote = 1;
                    else if(dat[2] == char(0x51))
                        m_arm_sta.remote = 0;
                    break;
                case 0x02:
                    m_arm_sta.mode = (dat[2]>>4) & 0x0f;
                    m_arm_sta.codeH1 = (dat[3]>>4) & 0x0f;
                    m_arm_sta.codeTL1 = dat[3] & 0x0f;
                    m_arm_sta.cheap = (dat[4]>>4) & 0x0f;
                    if(m_arm_sta.mode == 3){
                        m_arm_sta.codeTL3 = dat[2] & 0x0f;
                        m_arm_sta.codeH3 = dat[4] & 0x0f;
                    }
                    break;
                case 0x03:
                    if(dat[2] == (char)0xA0)
                        m_arm_sta.working = 0;
                    else if(dat[2] == (char)0xB0)
                        m_arm_sta.working = 1;
                    break;
                case 0x04:
                    if(dat[2] == (char)0xC0)
                        m_arm_sta.transmit = 1;
                    else if(dat[2] == (char)0xD0)
                        m_arm_sta.transmit = 0;
                    break;
                case 0x3B:
                    m_arm_sta.csphat = (int)dat[2] * 256 + dat[3];
                    break;
                case 0x3C:
                    m_arm_sta.csphanxa = (int)dat[2] * 256 + dat[3];
                    break;
                default:
                    break;
                }
            }
        }
    }

//    //Test
//    qDebug() << "mode" << m_arm_sta.mode << "working" << m_arm_sta.working << "transmit" << m_arm_sta.transmit;
//    qDebug() << "codeH1" << m_arm_sta.codeH1 << "codeTL1" << m_arm_sta.codeTL1 << "codeH3" << m_arm_sta.codeH3 << "codeTL3" << m_arm_sta.codeTL3;
//    qDebug() << "csphat" << m_arm_sta.csphat << "csphanxa" << m_arm_sta.csphanxa;

}

void Udp::sendToArm(int cmd_id)
{
    QByteArray cmd = QByteArray(10,0x00);
    cmd[0] = 0x02;
    cmd[8] = 0x0D;
    cmd[9] = 0x0A;
    switch (cmd_id) {
    case ARM_CMD_MODE:
        cmd[2] = m_arm_cmd.mode;
        break;
    case ARM_CMD_WORKING:
        if(m_arm_cmd.working == 1)
            cmd[2] = 11;
        else
            cmd[2] = 10;
        break;
    case ARM_CMD_TRANSMIT:
        if(m_arm_cmd.transmit == 1)
            cmd[2] = 12;
        else
            cmd[2] = 13;
        break;
    case ARM_CMD_CODEH1:
        cmd[2] = 23;
        cmd[4] = m_arm_cmd.codeH1;
        break;
    case ARM_CMD_CODETL1:
        cmd[2] = 25;
        cmd[4] = m_arm_cmd.codeTL1;
        break;
    case ARM_CMD_CODEH3:
        cmd[2] = 26;
        cmd[4] = m_arm_cmd.codeH3;
        break;
    case ARM_CMD_CODETL3:
        cmd[2] = 24;
        cmd[4] = m_arm_cmd.codeTL3;
        break;
    default:
        break;
    }

    cmd[7] = cmd[0] ^ cmd[1] ^ cmd[2] ^ cmd[3] ^ cmd[4] ^ cmd[5] ^ cmd[6] ^ cmd[8] ^ cmd[9];
    m_udp->writeDatagram(cmd, QHostAddress(__FPGA_IP__), __FPGA_PORT__);
}

void Udp::sendToFpga()
{
    QByteArray cmd = QByteArray(9,0x00);
    cmd[0] = 0x02;
    cmd[2] = 0xFF;
    cmd[3] = m_cmdFpga;
    cmd[8] = 0x0D;
    cmd[9] = 0x0A;
    if(m_cmdFpga==FPGA_CMD_COMBINE_TEST)
        cmd[4] = m_cmdStep;
    else if(m_cmdFpga==FPGA_CMD_RESULT)
        cmd[4] = m_final;
    else
        cmd[4] = m_module;
    cmd[7] = cmd[0] ^ cmd[1] ^ cmd[2] ^ cmd[3] ^ cmd[4] ^ cmd[5] ^ cmd[6] ^ cmd[8] ^ cmd[9];
    m_udp->writeDatagram(cmd, QHostAddress(__FPGA_IP__), __FPGA_PORT__);
}

void Udp::testLoop()
{
    //Process Requests
    if(m_requestStart && m_stage==STAGE_IDLE){
        m_stage = STAGE_START;
        m_requestStart = false;
    }
    else if(m_requestReset){
        m_stage = STAGE_IDLING;
        m_requestReset = false;
        m_requestStart = true;
    }
    else if(m_requestStop){
        m_stage = STAGE_IDLING;
        m_requestReset = false;
        m_requestStop = false;
    }

    //One Loop
    int res = RESULT_NONE;
    switch (m_stage) {
    case STAGE_IDLING:
        resetTest();
        m_cmdFpga = FPGA_CMD_IDLE;
        if(m_status==FPGA_STA_IDLE){
            m_stage = STAGE_IDLE;
            emit signalStageChanged(m_stage);
        }
        break;
    case STAGE_IDLE:    //Waiting for START
        break;
    case STAGE_START:
        emit signalStageChanged(m_stage);
        QThread::sleep(2);
        m_stage = STAGE_ARM_PREPARE;
        break;
    case STAGE_ARM_PREPARE:
        if(prepareArm()){
            QThread::sleep(2);
            m_stage = STAGE_FPGA_PREPARE;
            m_cmdFpga = FPGA_CMD_PREPARE;
        }
        break;
    case STAGE_FPGA_PREPARE:
        if(m_status==FPGA_STA_READY){
            QThread::sleep(2);
            m_stage = STAGE_FPGA_READY;
        }
        break;
    case STAGE_FPGA_READY:
        if(m_status==FPGA_STA_READY){
            QThread::sleep(2);
            m_stage = STAGE_FPGA_TEST;
            m_cmdFpga = FPGA_CMD_FPGA_TEST;
            m_result = RESULT_NONE;
        }
        break;
    case STAGE_FPGA_TEST:
        if(m_status == FPGA_STA_FPGA_TEST){
            QThread::sleep(2);
            if(m_result==RESULT_BAD)
                m_stage = STAGE_BAD;
            else if(m_result==RESULT_GOOD){
                m_cmdStep = 0;
                m_timeout = 0;
                m_stage = STAGE_COMBINE_TEST;
                m_cmdFpga = FPGA_CMD_COMBINE_TEST;
            }
        }
        break;
    case STAGE_COMBINE_TEST:
        res = combineTest();
        if(res==RESULT_BAD)
            m_stage = STAGE_BAD;
        else if(res==RESULT_GOOD)
            m_stage = STAGE_GOOD;
        break;
    case STAGE_GOOD:
        m_cmdFpga = FPGA_CMD_RESULT;
        m_final = RESULT_GOOD;
        emit signalStageChanged(m_stage);
        m_stage = STAGE_FINISHED;
        break;
    case STAGE_BAD:
        m_cmdFpga = FPGA_CMD_RESULT;
        m_final = RESULT_BAD;
        emit signalStageChanged(m_stage);
        m_stage = STAGE_FINISHED;
        break;
    case STAGE_FINISHED:
        break;
    default:
        break;
    }
}

bool Udp::prepareArm()
{
    if(m_module == MODULE_RECEIVER)
        m_arm_cmd.working = 0;
    else if (m_module == MODULE_TEST){
        m_arm_cmd.working = 0;
        m_arm_cmd.codeTL1 = 2;
    }
    else if (m_module == MODULE_INTERCONNECT)
        m_arm_cmd.codeTL1 = 3;

    //else default

    if(m_arm_sta.mode != m_arm_cmd.mode)
        sendToArm(ARM_CMD_MODE);
    else if(m_arm_sta.working != m_arm_cmd.working)
        sendToArm(ARM_CMD_WORKING);
    else if(m_arm_sta.transmit != m_arm_cmd.transmit)
        sendToArm(ARM_CMD_TRANSMIT);
//    else if(m_arm_sta.codeH1 != m_arm_cmd.codeH1)
//        sendToArm(ARM_CMD_CODEH1);
    else if(m_arm_sta.codeTL1 != m_arm_cmd.codeTL1)
        sendToArm(ARM_CMD_CODETL1);
//    else if(m_arm_sta.codeH3 != m_arm_cmd.codeH3)
//        sendToArm(ARM_CMD_CODEH3);
//    else if(m_arm_sta.codeTL3 != m_arm_cmd.codeTL3)
//        sendToArm(ARM_CMD_CODETL3);
    else
        return true;

    return false;
}

const int mode_id[8] = {1, 2, 3, 4, 6, 7, 8, 9};  // MAX_STEP
const int max_step = 5;
int Udp::combineTest()
{
    if (m_module == MODULE_TRANSMITTER){
        if(m_status == FPGA_STA_COMBINE_TEST){
            QThread::sleep(3);
            if (m_result == RESULT_BAD)
                return RESULT_BAD;
            else {
                int csphat = m_arm_sta.csphat;
                int csphanxa = m_arm_sta.csphanxa;
                if (csphat<=0 ||csphanxa<=0)
                    return RESULT_BAD;
                else
                    return RESULT_GOOD;
            }
        }
    }
    else if(m_module == MODULE_RECEIVER){
        if (m_status == FPGA_STA_COMBINE_TEST && m_step == m_cmdStep){
            QThread::sleep(1);
            if (m_result == RESULT_BAD)
                return RESULT_BAD;
            else if(m_cmdStep == max_step-1)
                return RESULT_GOOD;
            else{
                m_arm_cmd.mode = mode_id[m_cmdStep+1];
                if (m_arm_sta.mode != m_arm_cmd.mode)
                    sendToArm(ARM_CMD_MODE);
                else
                    m_cmdStep++;
            }
        }
    }
    else{
        if(m_status == FPGA_STA_COMBINE_TEST){
            if(m_module == MODULE_INTERCONNECT){
                QThread::sleep(2);
                return m_interconnect_ok ? RESULT_GOOD : RESULT_BAD;
            }
            else if(m_module == MODULE_PROCESS){
                if (m_process_ok)
                    return RESULT_GOOD;
                else if (++m_timeout>60)
                    return RESULT_BAD;
                else
                    return RESULT_NONE;
            }

            //Others
            return m_result;
        }
    }

    return RESULT_NONE;
}
