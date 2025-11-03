#include "tester.h"
#include <QDebug>
#include <QThread>

ARM_DATA_STRUCT g_arm_cmd;
FPGA_DATA_STRUCT g_fpga_cmd;
int g_cs_offset = 2000;

Tester::Tester(QObject *parent)
    : QObject{parent}
{
    m_requestStart = false;
    m_requestReset = false;
    m_requestStop = false;

    resetTest();

    m_stage = STAGE_IDLING;

    startTimer(500);
}

void Tester::resetTest()
{
    m_final = RESULT_NONE;
    m_interconnect_ok = false;
    m_process_ok = false;

    g_arm_cmd.mode = 1;
    g_arm_cmd.remote = 1;
    g_arm_cmd.working = 1;
    g_arm_cmd.transmit = 1;
    g_arm_cmd.codeH1 = 0;
    g_arm_cmd.codeTL1 = 1;
    g_arm_cmd.codeH3 = 1;
    g_arm_cmd.codeTL3 = 1;
    g_arm_cmd.cheap = 1;
    g_arm_cmd.csphat = 0;
    g_arm_cmd.anten = 1;
    g_arm_cmd.anten_sp = 1;

    g_fpga_cmd.cmd0 = 0;
    g_fpga_cmd.cmd1 = 0;
}

void Tester::slotStartTest(int module)
{
    m_module = module;
    m_requestStart = true;
}

void Tester::slotResetTest()
{
    m_requestReset = true;
}

void Tester::slotStopTest()
{
    m_requestStop = true;
}

void Tester::slotInterconnectOk()
{
    m_interconnect_ok = true;
}

void Tester::slotProcessOk()
{
    m_process_ok = true;
}

void Tester::timerEvent(QTimerEvent *event)
{
    Q_UNUSED(event)

    testLoop();

    if(m_module == MODULE_INTERCONNECT)
        emit signalSendToInterconnect();
}

void Tester::testLoop()
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
        qInfo() << "Enter IDLING";
    }
    else if(m_requestStop){
        m_stage = STAGE_IDLING;
        m_requestReset = false;
        m_requestStop = false;
        qInfo() << "Enter IDLING";
    }

    //One Loop
    int result = RESULT_NONE;
    switch (m_stage) {
    case STAGE_IDLING:
        resetTest();
        g_fpga_cmd.cmd0 = FPGA_CMD_IDLE;
        if(g_fpga_sta.status==FPGA_STA_IDLE){
            m_stage = STAGE_IDLE;
            emit signalStageChanged(m_stage);
            emit signalStageUpdate(m_stage);
            qInfo() << "Enter IDLE";
        }
        break;
    case STAGE_IDLE:    //Waiting for START
        break;
    case STAGE_START:
        emit signalStageChanged(m_stage);
        QThread::sleep(2);
        m_stage = STAGE_ARM_PREPARE;
        emit signalStageUpdate(m_stage);
        qInfo() << "Enter ARM_PREPARE";
        break;
    case STAGE_ARM_PREPARE:
        if(prepareArm()){
            QThread::sleep(2);
            m_stage = STAGE_FPGA_PREPARE;
            emit signalStageUpdate(m_stage);
            g_fpga_cmd.cmd0 = FPGA_CMD_PREPARE;
            g_fpga_cmd.cmd1 = m_module;
            qInfo() << "Enter FPGA_PREPARE";
        }
        break;
    case STAGE_FPGA_PREPARE:
        if(g_fpga_sta.status==FPGA_STA_READY){
            QThread::sleep(2);
            m_stage = STAGE_FPGA_READY;
            qInfo() << "Enter FPGA_READY";
        }
        break;
    case STAGE_FPGA_READY:
        QThread::sleep(2);
        m_stage = STAGE_FPGA_TEST;
        emit signalStageUpdate(m_stage);
        g_fpga_cmd.cmd0 = FPGA_CMD_FPGA_TEST;
        g_fpga_sta.result = RESULT_NONE;
        qInfo() << "Enter FPGA_TEST";
        break;
    case STAGE_FPGA_TEST:
        if(g_fpga_sta.status == FPGA_STA_FPGA_TEST){
            QThread::sleep(2);
            result = g_fpga_sta.result;
            if(result==RESULT_GOOD){
                m_step = 0;
                m_timeout = 0;
                m_stage = STAGE_COMBINE_TEST;
                emit signalStageUpdate(m_stage);
                g_fpga_cmd.cmd0 = FPGA_CMD_COMBINE_TEST;
                g_fpga_cmd.cmd1 = m_step;
                qInfo() << "Enter COMBINE_TEST";
            }
            else {
                m_stage = STAGE_BAD;
                emit signalStageUpdate(m_stage, result);
            }
        }
        break;
    case STAGE_COMBINE_TEST:
        result = combineTest();
        if(result==RESULT_GOOD){
            m_stage = STAGE_GOOD;
            emit signalStageUpdate(m_stage);
        }
        else if(result!=RESULT_NONE){   //BAD
            m_stage = STAGE_BAD;
            emit signalStageUpdate(m_stage, result);
        }
        //else waiting...
        break;
    case STAGE_GOOD:
        m_final = RESULT_GOOD;
        g_fpga_cmd.cmd0 = FPGA_CMD_RESULT;
        g_fpga_cmd.cmd1 = m_final;
        emit signalStageChanged(m_stage);
        m_stage = STAGE_FINISHED;
        qInfo() << "Enter GOOD --> FINISH";
        break;
    case STAGE_BAD:
        m_final = RESULT_BAD;
        g_fpga_cmd.cmd0 = FPGA_CMD_RESULT;
        g_fpga_cmd.cmd1 = m_final;
        emit signalStageChanged(m_stage);
        m_stage = STAGE_FINISHED;
        qInfo() << "Enter BAD --> FINISH";
        break;
    case STAGE_FINISHED:
        //Waiting for the next test
        break;
    default:
        break;
    }
}

bool Tester::prepareArm()
{
    if(m_module == MODULE_RECEIVER)
        g_arm_cmd.working = 0;
    else if (m_module == MODULE_TEST){
        g_arm_cmd.working = 0;
        g_arm_cmd.codeTL1 = 2;
    }
    else if (m_module == MODULE_INTERCONNECT)
        g_arm_cmd.codeTL1 = 3;

    //else default

    if(g_arm_sta.mode != g_arm_cmd.mode)
        emit signalSendToArm(ARM_CMD_MODE);
    else if(g_arm_sta.working != g_arm_cmd.working)
        emit signalSendToArm(ARM_CMD_WORKING);
    else if(g_arm_sta.transmit != g_arm_cmd.transmit)
        emit signalSendToArm(ARM_CMD_TRANSMIT);
    //    else if(m_arm_sta.codeH1 != m_arm_cmd.codeH1)
    //        emit signalSendToArm(ARM_CMD_CODEH1);
    else if(g_arm_sta.codeTL1 != g_arm_cmd.codeTL1)
        emit signalSendToArm(ARM_CMD_CODETL1);
    //    else if(m_arm_sta.codeH3 != m_arm_cmd.codeH3)
    //        emit signalSendToArm(ARM_CMD_CODEH3);
    //    else if(m_arm_sta.codeTL3 != m_arm_cmd.codeTL3)
    //        emit signalSendToArm(ARM_CMD_CODETL3);
    else
        return true;

    return false;
}

const int mode_id[8] = {1, 2, 3, 4, 6, 7, 8, 9};  // MAX_STEP
const int max_step = 5;
int Tester::combineTest()
{
    if (m_module == MODULE_TRANSMITTER){
        if(g_fpga_sta.status == FPGA_STA_COMBINE_TEST){
            QThread::sleep(3);
            if (g_fpga_sta.result == RESULT_GOOD){
                int csphat = g_arm_sta.csphat;
                // int csphanxa = g_arm_sta.csphanxa;
                if (csphat<g_cs_offset)
                    return RESULT_BAD;  //MAX_ERROR + 1
                else
                    return RESULT_GOOD;
            }
            else {
                return g_fpga_sta.result;
            }
        }
    }
    else if(m_module == MODULE_RECEIVER){
        if (g_fpga_sta.status == FPGA_STA_COMBINE_TEST && g_fpga_sta.step == m_step){
            QThread::sleep(1);
            if (g_fpga_sta.result == RESULT_GOOD) {
                if(m_step == max_step-1) {
                    return RESULT_GOOD;
                }
                else {
                    g_arm_cmd.mode = mode_id[m_step+1];
                    if (g_arm_sta.mode != g_arm_cmd.mode)
                        emit signalSendToArm(ARM_CMD_MODE);
                    else{
                        m_step++;
                        g_fpga_cmd.cmd1 = m_step;
                    }
                }
            }
            else{
                return g_fpga_sta.result;
                // return m_step;
            }
        }
    }
    else {  //Các module khác
        if(g_fpga_sta.status == FPGA_STA_COMBINE_TEST){
            if(m_module == MODULE_INTERCONNECT){
                QThread::sleep(2);
                return m_interconnect_ok ? RESULT_GOOD : RESULT_BAD;
            }

            if(m_module == MODULE_PROCESS){
                if (m_process_ok)
                    return RESULT_GOOD;
                else if (++m_timeout>60)
                    return RESULT_BAD;
                else
                    return RESULT_NONE;
            }

            //Others
            QThread::sleep(2);
            return g_fpga_sta.result;
        }
    }

    return RESULT_NONE;
}
