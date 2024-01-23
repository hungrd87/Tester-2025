#ifndef UDP_H
#define UDP_H

#include <QObject>
#include <QUdpSocket>
#include <QTimer>
#include <QProcess>
#include "udp_def.h"

class Udp : public QObject
{
    Q_OBJECT
public:
    explicit Udp(QObject *parent = nullptr);
    ~Udp();

    inline int module() {return m_module;}
    inline int result() {return m_final;}
    inline int stage() {return m_stage;}
    void resetTest();
    inline void prepare() {m_cmdPC = PC_CMD_PREPARE;}
    inline void test() {m_cmdPC = PC_CMD_TEST;}
    inline bool armOnline() {return m_armOnline;}
    inline bool fpgaOnline() {return m_fpgaOnline;}

public slots:
    void slotStartTest(int module);
    void slotResetTest();
    void slotStopTest();
    void slotUdpReadyRead();
    void slotPseudoProcessReadyRead();
    void slotPseudoInterConnectReadyRead();
    void slotIpChanged();
    void slotPingFinished(int exitCode, QProcess::ExitStatus exitStatus);

protected:
    void timerEvent(QTimerEvent *event) override;

signals:
    void signalStageChanged(int);
    void signalNetworkChanged();

private:
    void decodeFpga(char*);
    void decodeArm(char*, int len);
    void sendToArm(int cmd_id);
    void sendToFpga();
    void testLoop();
    bool prepareArm();
    int combineTest();

private:
    QUdpSocket *m_udp, *m_pseduo_process, *m_pseduo_interconnect;
    QProcess *m_ping;
    ARM_DATA_STRUCT m_arm_cmd, m_arm_sta;
    int m_module;
    bool m_requestStart, m_requestReset, m_requestStop;
    int m_cmdFpga, m_cmdStep;
    int m_step, m_timeout;
    int m_status;
    int m_result, m_final;
    bool m_armOnline, m_fpgaOnline, m_interconnect_ok, m_process_ok, m_ping_ok;
    int m_stage, m_cmdPC;
};

#endif // UDP_H
