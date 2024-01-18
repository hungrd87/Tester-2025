#include "udp.h"

Udp::Udp(QObject *parent) : QObject(parent)
{
    m_udp = new QUdpSocket(this);
    m_udp->bind(__UDP_PORT__, QUdpSocket::ShareAddress);
    connect(m_udp, &QUdpSocket::readyRead, this, &Udp::slotUdpReadyRead);

    resetTest();

    m_armOnline = false;
    m_fpgaOnline = false;

    startTimer(500);
}

Udp::~Udp()
{
    delete m_udp;
}

void Udp::resetTest()
{
    m_enabled = false;

    m_step = 0;
    m_final = RESULT_NONE;
    m_stage = STAGE_IDLING;
    m_ready = false;

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
}

void Udp::slotStartTest(int module)
{
    m_module = module;

    resetTest();

    m_enabled = true;
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
        if((senderPort == __UDP_PORT__) && (sender.toString().contains(__UDP_IP__)))
        {
            if(len == __UDP_FPGA_LEN__)
                decodeFpga(dat);
            else if(len == __UDP_ARM_LEN__)
                decodeArm(dat, len);
        }

        free(dat);
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

    if(m_enabled)
        testLoop();
}

void Udp::decodeFpga(char *data)
{
    if(data[0] == char(0x55) && data[1] == char(0xFF)){
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
    for(int i=0; i<nframes; nframes++){
        if(data[0]==char(0x55)){
            if((data[1] ^ data[2] ^ data[3] ^ data[4] ^ data[5] ^ data[6]) == char(0xFF)){
                m_armOnline = true; cnt_arm = 0;
                switch (data[1]) {
                case 0x01:
                    if(data[2] == char(0x52))
                        m_arm_sta.remote = 1;
                    else if(data[2] == char(0x51))
                        m_arm_sta.remote = 0;
                    break;
                case 0x02:
                    m_arm_sta.mode = (data[2]>>4) & 0x0f;
                    m_arm_sta.codeH1 = (data[3]>>4) & 0x0f;
                    m_arm_sta.codeTL1 = data[3] & 0x0f;
                    m_arm_sta.cheap = (data[4]>>4) & 0x0f;
                    if(m_arm_sta.mode == 3){
                        m_arm_sta.codeTL3 = data[2] & 0x0f;
                        m_arm_sta.codeH3 = data[4] & 0x0f;
                    }
                    break;
                case 0x03:
                    if(data[2] == (char)0xA0)
                        m_arm_sta.working = 0;
                    else if(data[2] == (char)0xB0)
                        m_arm_sta.working = 1;
                    break;
                case 0x04:
                    if(data[2] == (char)0xC0)
                        m_arm_sta.transmit = 1;
                    else if(data[2] == (char)0xD0)
                        m_arm_sta.working = 0;
                    break;
                case 0x3B:
                    m_arm_sta.csphat = (int)data[2] * 256 + data[3];
                    break;
                case 0x3C:
                    m_arm_sta.csphanxa = (int)data[2] * 256 + data[3];
                    break;
                default:
                    break;
                }
            }
        }
    }
}

void Udp::sendToArm(int cmd_id)
{
    QByteArray cmd = QByteArray(9,0x00);
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
    m_udp->writeDatagram(cmd, QHostAddress(__UDP_IP__), __UDP_PORT__);
}

void Udp::sendToFpga()
{
    QByteArray cmd = QByteArray(9,0x00);
    cmd[0] = 0x02;
    cmd[2] = 0xFF;
    cmd[3] = m_cmdFpga;
    cmd[8] = 0x0D;
    cmd[9] = 0x0A;
    cmd[4] = (m_cmdFpga==FPGA_CMD_COMBINE_TEST) ? m_cmdStep : m_module;
    cmd[7] = cmd[0] ^ cmd[1] ^ cmd[2] ^ cmd[3] ^ cmd[4] ^ cmd[5] ^ cmd[6] ^ cmd[8] ^ cmd[9];
    m_udp->writeDatagram(cmd, QHostAddress(__UDP_IP__), __UDP_PORT__);
}

int nchecks = 0;
void Udp::testLoop()
{
    int res = RESULT_NONE;
    switch (m_stage) {
    case STAGE_IDLING:
        m_cmdFpga = FPGA_CMD_IDLE;
        if(m_status==FPGA_STA_IDLE)
            m_stage = STAGE_IDLE;
        break;
    case STAGE_IDLE:
        m_ready = false;
        if(m_cmdPC==PC_CMD_PREPARE){
            if(prepareArm())
                m_stage = STAGE_PREPARING;
        }
        break;
    case STAGE_PREPARING:
        m_cmdFpga = FPGA_CMD_PREPARE;
        if(m_status==FPGA_STA_READY)
            m_stage = STAGE_READY;
        break;
    case STAGE_READY:
        if(m_cmdPC==PC_CMD_TEST){
            m_result = RESULT_NONE;
            m_stage = STAGE_FPGA_TEST;
        }
        break;
    case STAGE_FPGA_TEST:
        m_cmdFpga = FPGA_CMD_FPGA_TEST;
        if(m_status == FPGA_STA_FPGA_TEST){
            if(m_result==RESULT_BAD)
                m_stage = STAGE_BAD;
            else if(m_result==RESULT_GOOD){
                m_cmdStep = 0;
                nchecks = 0;
                m_stage = STAGE_COMBINE_TEST;
            }
        }
        break;
    case STAGE_COMBINE_TEST:
        m_cmdFpga = FPGA_CMD_COMBINE_TEST;
        res = combineTest();
        if(res==RESULT_BAD)
            m_stage = STAGE_BAD;
        else if(res==RESULT_GOOD)
            m_stage = STAGE_GOOD;
        break;
    case STAGE_GOOD:
        m_final = RESULT_GOOD;
        break;
    case STAGE_BAD:
        m_final = RESULT_BAD;
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
            if (m_result == RESULT_BAD)
                return RESULT_BAD;
            else if(++nchecks>6){   //increase 1 for once
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
        if(m_status == FPGA_STA_COMBINE_TEST)
            return m_result;
    }

    return RESULT_NONE;
}

