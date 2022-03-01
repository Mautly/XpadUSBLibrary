#ifndef ASYNCHRONOUS_H
#define ASYNCHRONOUS_H

#include <QThread>

#include "xpadcommands.h"
#include "tools.h"
#include "defines.h"



class asynchronous : public QThread
{
    Q_OBJECT
public:
    asynchronous(unsigned short moduleMask, xpad_cmd *cmd, unsigned int burstNumber);
    void run();

private:


private:

     xpad_cmd           *m_xpad_cmd;
     unsigned int       m_burst_number;
     unsigned short     m_module_mask;
};

#endif // ASYNCHRONOUS_H
