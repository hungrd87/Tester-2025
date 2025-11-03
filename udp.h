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

    inline bool armOnline() {return m_armOnline;}
    inline bool fpgaOnline() {return m_fpgaOnline;}

public slots:
    void slotUdpReadyRead();
    void slotPseudoProcessReadyRead();
    void slotPseudoInterConnectReadyRead();
    void slotIpChanged();
    void slotSendToArm(int cmd_id);
    void slotSendToInterconnect();

protected:
    void timerEvent(QTimerEvent *event) override;

signals:
    void signalNetworkChanged();
    void signalInterconnectOk();
    void signalProcessOk();

private:
    void decodeFpga(char*);
    void decodeArm(char*, int len);
    void sendToArm(int cmd_id);
    void sendToFpga();

private:
    QUdpSocket *m_udp, *m_pseduo_process, *m_pseduo_interconnect;
    bool m_armOnline, m_fpgaOnline;
};

#endif // UDP_H
