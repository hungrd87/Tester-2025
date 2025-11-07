#include "udp.h"

ARM_DATA_STRUCT g_arm_sta;
FPGA_DATA_STRUCT g_fpga_sta;

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

    g_arm_sta.mode = -1;
    g_arm_sta.remote = -1;
    g_arm_sta.working = -1;
    g_arm_sta.transmit = -1;
    g_arm_sta.codeH1 = -1;
    g_arm_sta.codeTL1 = -1;
    g_arm_sta.codeH3 = -1;
    g_arm_sta.codeTL3 = -1;
    g_arm_sta.cheap = -1;
    g_arm_sta.anten = -1;
    g_arm_sta.anten_sp = -1;
    g_arm_sta.csphat = -1;
    g_arm_sta.csphanxa = -1;

    g_fpga_sta.status = FPGA_STA_NONE;
    g_fpga_sta.module = -1;
    g_fpga_sta.step = 0;
    g_fpga_sta.result = RESULT_NONE;

    m_armOnline = false;
    m_fpgaOnline = false;

    startTimer(500);
}

Udp::~Udp()
{

}

void Udp::slotUdpReadyRead()
{
    QHostAddress sender;
    quint16 senderPort;
    while(m_udp->hasPendingDatagrams()){
        qint64 len = m_udp->pendingDatagramSize();
        char *dat = (char*) malloc(len);
        m_udp->readDatagram(dat, len, &sender, &senderPort);
        if((senderPort == __FPGA_PORT__) && (sender.toString().contains(__FPGA_IP__))){
            if(len == __UDP_FPGA_LEN__ || len == 26){
                decodeFpga(dat);
            }
            // else if(len == __UDP_ARM_LEN__){
            else if((len%__FRAME_ARM_LEN__==0)) {
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
    if(m_pseduo_process!=nullptr){
        while(m_pseduo_process->hasPendingDatagrams()){
            qint64 len = m_pseduo_process->pendingDatagramSize();
            char *dat = (char*) malloc(len);
            m_pseduo_process->readDatagram(dat, len, &sender, &senderPort);
            if((senderPort == __INTERCONNECT_PORT__) && (sender.toString().contains(__INTERCONNECT_IP__))){
                emit signalInterconnectOk();
            }

            free(dat);
        }
    }
}

QByteArray data_ip_found("\x55\xaa\x11\x11\x11\x11\x55");
QByteArray data_ip_notfound("\x55\xaa\x00\x00\x00\x00\x55");
void Udp::slotPseudoInterConnectReadyRead()
{
    QHostAddress sender;
    quint16 senderPort;
    if(m_pseduo_interconnect!=nullptr){
        while(m_pseduo_interconnect->hasPendingDatagrams()){
            qint64 len = m_pseduo_interconnect->pendingDatagramSize();
            char *dat = (char*) malloc(len);
            m_pseduo_interconnect->readDatagram(dat, len, &sender, &senderPort);
            if((senderPort == __PROCESS_PORT__) && (sender.toString().contains(__PROCESS_IP__))){
                if(len == 7){
                    if(dat[0]==char(0x55) && dat[1]==char(0xaa) && dat[6]==char(0x55))
                        emit signalProcessOk();
                }
            }

            free(dat);
        }
    }
}

void Udp::slotIpChanged()
{
    m_udp->disconnectFromHost();
    m_pseduo_process->disconnectFromHost();
    m_pseduo_interconnect->disconnectFromHost();

    m_udp->bind(QHostAddress(__HOST_IP__),__HOST_PORT__, QUdpSocket::ShareAddress);
    m_pseduo_process->bind(QHostAddress(__PROCESS_IP__),__PROCESS_PORT__, QUdpSocket::ShareAddress);
    m_pseduo_interconnect->bind(QHostAddress(__INTERCONNECT_IP__),__INTERCONNECT_PORT__, QUdpSocket::ShareAddress);

    qDebug() << "Rebinded UDP sockets!";
}

void Udp::slotSendToArm(int cmd_id)
{
    sendToArm(cmd_id);
}

void Udp::slotSendToInterconnect()
{
    QByteArray data_found_ip("\x55\xaa\x11\x11\x11\x11\x55");
    m_pseduo_process->writeDatagram(data_found_ip, QHostAddress(__INTERCONNECT_IP__), __INTERCONNECT_PORT__);
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
}

void Udp::decodeFpga(char *data)
{
    if(data[0] == char(0x55) && data[1] == char(0xff)){
        if((data[2] ^ data[3] ^ data[4] ^ data[5] ^ data[6]) == 0){
            m_fpgaOnline = true; cnt_fpga = 0;
            switch (data[2]) {
            case FPGA_STA_IDLE:
                g_fpga_sta.status = FPGA_STA_IDLE;
                break;
            case FPGA_STA_READY:
                g_fpga_sta.status = FPGA_STA_READY;
                break;
            case FPGA_STA_FPGA_TEST:
                g_fpga_sta.status = FPGA_STA_FPGA_TEST;
                g_fpga_sta.result = data[3];
                break;
            case FPGA_STA_COMBINE_TEST:
                g_fpga_sta.status = FPGA_STA_COMBINE_TEST;
                g_fpga_sta.step = data[3]&0x0f;
                g_fpga_sta.result = data[3]&0xf0;
                g_fpga_sta.result ^= 0x10;  //Giao thuc moi
                break;
            default:
                g_fpga_sta.status = FPGA_STA_UNKNOWN;
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
                        g_arm_sta.remote = 1;
                    else if(dat[2] == char(0x51))
                        g_arm_sta.remote = 0;
                    break;
                case 0x02:
                    g_arm_sta.mode = (dat[2]>>4) & 0x0f;
                    g_arm_sta.codeH1 = (dat[3]>>4) & 0x0f;
                    g_arm_sta.codeTL1 = dat[3] & 0x0f;
                    g_arm_sta.cheap = (dat[4]>>4) & 0x0f;
                    if(g_arm_sta.mode == 3){
                        g_arm_sta.codeTL3 = dat[2] & 0x0f;
                        g_arm_sta.codeH3 = dat[4] & 0x0f;
                    }
                    break;
                case 0x03:
                    if(dat[2] == (char)0xA0)
                        g_arm_sta.working = 0;
                    else if(dat[2] == (char)0xB0)
                        g_arm_sta.working = 1;
                    break;
                case 0x04:
                    if(dat[2] == (char)0xC0)
                        g_arm_sta.transmit = 1;
                    else if(dat[2] == (char)0xD0)
                        g_arm_sta.transmit = 0;
                    break;
                case 0x3B:
                    g_arm_sta.csphat = (int)dat[2] * 256 + dat[3];
                    break;
                case 0x3C:
                    g_arm_sta.csphanxa = (int)dat[2] * 256 + dat[3];
                    break;
                default:
                    break;
                }
            }
        }
    }

   //Test
   qDebug() << "mode" << g_arm_sta.mode << "working" << g_arm_sta.working << "transmit" << g_arm_sta.transmit;
   qDebug() << "codeH1" << g_arm_sta.codeH1 << "codeTL1" << g_arm_sta.codeTL1 << "codeH3" << g_arm_sta.codeH3 << "codeTL3" << g_arm_sta.codeTL3;
   qDebug() << "csphat" << g_arm_sta.csphat << "csphanxa" << g_arm_sta.csphanxa;
}

void Udp::sendToArm(int cmd_id)
{
    QByteArray cmd = QByteArray(10,0x00);
    cmd[0] = 0x02;
    cmd[8] = 0x0D;
    cmd[9] = 0x0A;
    switch (cmd_id) {
    case ARM_CMD_MODE:
        cmd[2] = g_arm_cmd.mode;
        break;
    case ARM_CMD_WORKING:
        if(g_arm_cmd.working == 1)
            cmd[2] = 11;
        else
            cmd[2] = 10;
        break;
    case ARM_CMD_TRANSMIT:
        if(g_arm_cmd.transmit == 1)
            cmd[2] = 12;
        else
            cmd[2] = 13;
        break;
    case ARM_CMD_CODEH1:
        cmd[2] = 23;
        cmd[4] = g_arm_cmd.codeH1;
        break;
    case ARM_CMD_CODETL1:
        cmd[2] = 25;
        cmd[4] = g_arm_cmd.codeTL1;
        break;
    case ARM_CMD_CODEH3:
        cmd[2] = 26;
        cmd[4] = g_arm_cmd.codeH3;
        break;
    case ARM_CMD_CODETL3:
        cmd[2] = 24;
        cmd[4] = g_arm_cmd.codeTL3;
        break;
    default:
        break;
    }

    cmd[7] = cmd[0] ^ cmd[1] ^ cmd[2] ^ cmd[3] ^ cmd[4] ^ cmd[5] ^ cmd[6] ^ cmd[8] ^ cmd[9];
    m_udp->writeDatagram(cmd, QHostAddress(__FPGA_IP__), __FPGA_PORT__);
}

void Udp::sendToFpga()
{
    QByteArray cmd = QByteArray(10,0x00);
    cmd[0] = 0x02;
    cmd[2] = 0xFF;
    cmd[3] = g_fpga_cmd.cmd0;
    cmd[8] = 0x0D;
    cmd[9] = 0x0A;
    cmd[4] = g_fpga_cmd.cmd1;
    cmd[7] = cmd[0] ^ cmd[1] ^ cmd[2] ^ cmd[3] ^ cmd[4] ^ cmd[5] ^ cmd[6] ^ cmd[8] ^ cmd[9];
    m_udp->writeDatagram(cmd, QHostAddress(__FPGA_IP__), __FPGA_PORT__);
}
