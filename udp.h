#ifndef UDP_H
#define UDP_H

#include <QObject>
#include <QUdpSocket>
#include <QTimer>
#include "udp_def.h"

class Udp : public QObject
{
    Q_OBJECT
public:
    explicit Udp(QObject *parent = nullptr);
    ~Udp();

    inline int module() {return m_module;}
    inline int result() {return m_result;}
    void resetTest();
    inline void prepare() {m_cmdPC = PC_CMD_PREPARE;}
    inline void test() {m_cmdPC = PC_CMD_TEST;}
    inline bool armOnline() {return m_armOnline;}
    inline bool fpgaOnline() {return m_fpgaOnline;}

public slots:
    void slotStartTest(int module);
    void slotUdpReadyRead();

protected:
    void timerEvent(QTimerEvent *event) override;

private:
    void decodeFpga(char*);
    void decodeArm(char*, int len);
    void sendToArm(int cmd_id);
    void sendToFpga();
    void testLoop();
    bool prepareArm();
    int combineTest();

private:
    QUdpSocket *m_udp;
    ARM_DATA_STRUCT m_arm_cmd, m_arm_sta;
    int m_module;
    int m_cmdFpga, m_cmdStep;
    int m_step;
    int m_status;
    int m_result, m_final;
    bool m_enabled, m_armOnline, m_fpgaOnline, m_ready;
    int m_stage, m_cmdPC;
};

#endif // UDP_H
