#include <QApplication>
#include "udp_def.h"

#include "view.h"
#include "dialog.h"

void myMessageOutput(QtMsgType type, const QMessageLogContext&, const QString &msg);
void getParams();

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setWindowIcon(QIcon(qApp->applicationDirPath() + "/Tester.ico"));

    //Check Arguments
    bool is_log = false, is_admin = false;
    for(int i=0; i<argc; i++){
        if(QString::fromLatin1(argv[i]).contains("--log") || QString::fromLatin1(argv[i]).contains("-l"))
            is_log = true;
        if(QString::fromLatin1(argv[i]).contains("--admin") || QString::fromLatin1(argv[i]).contains("-a"))
            is_admin = true;
    }

    if(is_log)
        qInstallMessageHandler(myMessageOutput);

    getParams();

    View w;
    if(is_admin) {w.setAdmin(is_admin);}

    // Dialog w;
    w.show();

    return a.exec();
}

void myMessageOutput(QtMsgType type, const QMessageLogContext&, const QString &msg)
{
    static bool started = false;
    QString txt = msg;
    static QString file = "";
    QString sType = "";

    switch (type)
    {
    case QtDebugMsg:
        sType = "Debug";
        break;
    case QtInfoMsg:
        sType = "Info ";
        break;
    case QtWarningMsg:
        sType = "Warn ";
        break;
    case QtCriticalMsg:
        sType = "Criti";
        break;
    case QtFatalMsg:
        sType = "Fatal";
        //            abort();
    }

    txt.prepend(" - ");
    txt.prepend(sType);

    txt.prepend(" - ");
    txt.prepend(QDateTime::currentDateTime().toString("[hh:mm:ss]"));

    QString path = qApp->applicationDirPath() + "/log";
    QDir().mkpath(path);
    if(!started)
        file = path + "/Tester_" + QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss") + ".log";

    QFile outFile(file);
    outFile.open(QIODevice::WriteOnly | QIODevice::Append);
    QTextStream ts(&outFile);
    ts << txt << Qt::endl;

    started = true;
}

void getParams()
{
    //Restore values of controls
    QSettings settings(qApp->applicationDirPath() + "/Config.ini", QSettings::IniFormat);
    g_cs_offset = settings.value("cs_offset", 1999).toInt();
    qDebug() << "cs_offset" << g_cs_offset;
}
