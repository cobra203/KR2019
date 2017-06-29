#ifndef SPITHREAD_H
#define SPITHREAD_H

#include <QThread>
#include <QSemaphore>
#include "mcp2210.h"
#include <QVariant>

Q_DECLARE_METATYPE(VOCAL_SYS_STATUS_S)

class SpiThread : public QThread
{
    Q_OBJECT
public:
    SpiThread();
    ~SpiThread();
    void stop();

signals:
    void show_status(const QString &);
    void vocal_updata(QVariant);

protected:
    void run();

private:
    volatile bool stopped;
    unsigned int  status;

public:
    QSemaphore semaphore;
};

#endif // SPITHREAD_H
