#ifndef SPITHREAD_H
#define SPITHREAD_H

#include <QThread>
#include <QSemaphore>
#include "mcp2210.h"

class SpiThread : public QThread
{
    Q_OBJECT
public:
    SpiThread();
    ~SpiThread();
    void stop();

signals:
    void prin(const QString &);
    void vocal_cmd(int, int);

protected:
    void run();

private:
    volatile bool stopped;
    unsigned int  status;

public:
    QSemaphore semaphore;
};

#endif // SPITHREAD_H
