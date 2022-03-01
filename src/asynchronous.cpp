#include "asynchronous.h"

/**
 * @brief asynchronous::asynchronous
 *  Asynchrone expose function
 * @param moduleMask    : module mask
 * @param cmd           : xpad_cmd object
 * @param burstNumber   : burst number
 */

asynchronous:: asynchronous(unsigned short moduleMask,xpad_cmd *cmd, unsigned int burstNumber):
m_module_mask(moduleMask), m_xpad_cmd(cmd), m_burst_number(burstNumber){
}

/**
 * @brief asynchronous::run
 * Thread expose async
 */

void asynchronous::run(){
   // m_xpad_cmd->expose(m_module_mask,m_burst_number);

    m_xpad_cmd->exposeWriteFileAs(m_module_mask,m_burst_number);
}
