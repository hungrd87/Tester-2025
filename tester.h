#ifndef TESTER_H
#define TESTER_H

#include <QObject>
#include "udp_def.h"

class Tester : public QObject
{
    Q_OBJECT
public:
    explicit Tester(QObject *parent = nullptr);

    inline int module() {return m_module;}
    inline int result() {return m_final;}
    inline int stage() {return m_stage;}
    void resetTest();

public slots:
    void slotStartTest(int module);
    void slotResetTest();
    void slotStopTest();
    void slotInterconnectOk();
    void slotProcessOk();

protected:
    void timerEvent(QTimerEvent *event) override;

signals:
    void signalStageChanged(int);
    void signalStageUpdate(int next_stage, int prev_result=0);
    void signalSendToArm(int cmd_id);
    void signalSendToInterconnect();

private:
    void testLoop();
    bool prepareArm();
    int combineTest();

    int m_module;
    bool m_requestStart, m_requestReset, m_requestStop;
    int m_step;
    int m_timeout;
    int m_final;
    bool m_interconnect_ok, m_process_ok;
    int m_stage;
};

#endif // TESTER_H
