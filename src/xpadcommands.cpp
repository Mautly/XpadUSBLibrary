#include "xpadcommands.h"

#include <QTime>

xpad_cmd::xpad_cmd()
{
    setDebugMode(false);
    m_expose_busy = 0;
    m_firmwareId = 0;
}

/**
 * @brief xpad_cmd::xpad_cmd
 *          Init the usb connection for all commands
 * @param USBdevice : set the usb object
 */

xpad_cmd::xpad_cmd(USBControl *USBdevice)
{
    this->setUSB(USBdevice);
    m_expose_busy = 0;
    m_firmwareId = 0;
}

/**
 * @brief xpad_cmd::setUSB
 *              set the usb object for all commands
 * @param usbDevice :
 */
void xpad_cmd ::setUSB(USBControl *usbDevice)
{
    m_xpad_usb = usbDevice;
}


/**
 * @brief xpad_cmd::setDebugMode
 * @param val : true debug mode activate
 */
void xpad_cmd:: setDebugMode(bool val)
{
    DEBUG = val;
}

/**
 * @brief xpad_cmd::getCurrentImageNumber
 *          numbers of last acquire image
 * @return : value of the last images acquire
 */
int xpad_cmd:: getCurrentImageNumber(void){
    return m_current_image_number;
}

/**
 * @brief xpad_cmd::clearCurrentImageNumber
 * set the last acquire image to 0
 */
void xpad_cmd:: clearCurrentImageNumber(void){
    m_current_image_number = 0;
}

/**
 * @brief xpad_cmd::getCurrentMask
 * @return      : return the current mask used
 */
unsigned short xpad_cmd:: getCurrentMask(void){
    return m_module_mask;
}

/**
 * @brief xpad_cmd::setDetectorType
 * @param detectorType      : type of detector example XPAD_S70 or XPAD_S140
 * @return
 */
int xpad_cmd:: setDetectorType(unsigned int detectorType){
    int ret=0;
    switch (detectorType){
    case XPAD_A10:
    case XPAD_C10:
    case XPAD_S10:
    case XPAD_C70:
    case XPAD_S70:
        m_module = 1;
        ret = 0;
        m_enableConfigureTransfertSize = false;
        break;
    case XPAD_S140:
        m_module = 2;
        ret = 0;
        m_enableConfigureTransfertSize = true;
        break;
    case XPAD_S210:
        m_module = 3;
        ret = 0;
        m_enableConfigureTransfertSize = true;
        break;
    case XPAD_C270:
    case XPAD_S270:
        m_module = 4;
        ret = 0;
        m_enableConfigureTransfertSize = true;
        break;
    case XPAD_S340:
        m_module = 5;
        ret = 0;
        m_enableConfigureTransfertSize = true;
        break;
    default :
        ret = -1;
        break;
    }

    if(ret == 0)
        m_detectorType = detectorType;

    return ret;
}

/**
 * @brief xpad_cmd::setModuleNumber
 * @param value : set numbers of modules for the detect choose
 */
void xpad_cmd::setModuleNumber(int value){
    m_module = value;
}

/**
 * @brief xpad_cmd::getExposeBusy
 * @return: 1 if the detector is busy else 0
 */
bool xpad_cmd:: getExposeBusy(void){
    return m_expose_busy;
}

/**
 * @brief xpad_cmd::getFirmwareId
 * @return : the firmware ID
 */
unsigned long xpad_cmd::getFirmwareId(void){
    return m_firmwareId;
}

/**
 * @brief xpad_cmd::askReady
 *          this command ask to each modules if ready
 * @return
 */

int xpad_cmd::askReady() {
    unsigned short tmp_mask =0;
    unsigned short moduleMask;
    unsigned short *ret;
    unsigned short  MOD_askReady[16]        = { HUB_HEADER, MOD_MESSAGE,
                                                0x0006,       //nb words
                                                0,      //modules masks
                                                MOD_HEADER,
                                                0x0003,       //nb words

                                                0,            //reserved
                                                CMD_ASKREADY, //0x0101,
                                                MSG_TRAILER,
                                                0,
                                                0,
                                                0, 0, 0, 0, 0};

    moduleMask = 0;
    //for(int i=0;i<m_xpad_usb->GetModuleNumber();i++){

    if(m_enableConfigureTransfertSize){
        modulesName();
        resetRegFifo();
        setDataTransfert(REG_CMD_SIZE);
    }

    for(int i=0;i<m_module;i++){
        tmp_mask = 1 << i;
        MOD_askReady[3] = tmp_mask;
        if(DEBUG)
            cout<< "LibXpadUSB : " << __func__ <<"() \t" << "Sending askready to " << "0x" << QString::number(tmp_mask,16).toStdString() << endl;

        ret = m_xpad_usb->sendCommand(tmp_mask,MOD_askReady,sizeof(MOD_askReady),100);
        if(ret!=NULL)
        {
            if(DEBUG)
            {
                cout<< "LibXpadUSB : " << __func__ <<"() \t" << "Detector to USB ==>"<< endl;
                for(int i=0;i<16;i++)
                    printf("0x%x ",ret[i]);
                cout << endl;
            }
            if (ret[0]==ACK_HEADER
                    &&ret[1]==(i+1)
                    &&ret[2]==ACK_SIZE
                    &&ret[3]==ACK_ASKREADY
                    &&ret[15]==ACK_TRAILER) {
                moduleMask |= tmp_mask;
                if(DEBUG){
                    cout<< "LibXpadUSB : " << __func__ <<"() \t"<<"Module "<<tmp_mask << " =>> Firmware ID = ";
                    printf("0x%x 0x%x 0x%x\n",ret[4],ret[5],ret[6]);
                }
                m_firmwareId = ((ret[4]&0xffff) << 24) | ((ret[5]&0xFF)<<16) | ret[6];
                delete[] ret;

            }
            else {
                if (DEBUG) cout<< "LibXpadUSB : " << __func__ <<"() \t"<<"Ask ready error on module "<<"0x" << QString::number(tmp_mask,16).toStdString()<<endl;
                delete[] ret;
                return 0;
            }
            //  delete ret;
        }
        //   QThread::sleep(2);
        //   cout << "wait new ask ready" << endl;

    }
    if(moduleMask == 0){
        cout << "LibXpadUSB : " << __func__ <<"() \t"<< "No module(s) detected" << endl;
        return 0;
    }
    m_module_mask = moduleMask;
    return moduleMask;
}




/**
 * @brief xpad_cmd::digitalTest
 *              test the XPAD digital part
 * @param moduleMask    : module mask
 * @param value         : numbers of digital hit
 * @param mode          : mode flat 0 , strip 1  or gradian 2
 * @return              : -1 if error else 0
 */


int xpad_cmd::digitalTest(unsigned short moduleMask, unsigned short value, unsigned short mode) {
    unsigned short *buf;
    if(globalAskReady(moduleMask)!=0) return -1;
    unsigned short  MOD_autoTest[16]        = { HUB_HEADER, MOD_MESSAGE,
                                                0x0008,       //nb words
                                                moduleMask,      //modules masks
                                                MOD_HEADER,
                                                0x0005,       //nb words

                                                0,            //reserved
                                                CMD_AUTOTEST, //0x0102, auto_test
                                                value,        //value to load in the detector
                                                mode,         //test_mode
                                                MSG_TRAILER,

                                                0, 0, 0, 0, 0};

    if (DEBUG) cout<< "LibXpadUSB : " << __func__ <<"() \t"<<"Sending digitaltest lib usb to "<<"0x" << QString::number(moduleMask,16).toStdString()<<endl;
    if(m_enableConfigureTransfertSize){
        resetRegFifo();
        setDataTransfert(REG_CMD_SIZE);
    }
    buf = m_xpad_usb->sendCommand(moduleMask,MOD_autoTest,sizeof(MOD_autoTest),15000);

    if(buf != NULL){
        if(checkModAcK(moduleMask,buf,ACK_AUTOTEST)==0){
            if (DEBUG) cout<< "LibXpadUSB : " << __func__ <<"() \t"<<"DigitalTest completed on module "<<"0x" << QString::number(moduleMask,16).toStdString()<<endl;
            delete[] buf;
            return 0;
        }
        else{
            if (DEBUG) cout<< "LibXpadUSB : " << __func__ <<"() \t"<<"DigitalTest error on module "<<"0x" << QString::number(moduleMask,16).toStdString()<<endl;
            delete[] buf;
            return -1;
        }
    }
    return -1;
}

/**
 * @brief xpad_cmd::resetModules
 *              reset the electronics XPAD board
 * @return
 */
int xpad_cmd::resetModules() {
    unsigned short  MOD_imxpadRstNIOS[16]   = { HUB_HEADER, HUB_MESSAGE, 0x0002,//0x02FF,
                                                0x0001, // bit(0) - reset NIOS
                                                MSG_TRAILER,
                                                0, 0, 0
                                                ,0};

    m_xpad_usb->sendSpecialCommand(MOD_imxpadRstNIOS,sizeof(MOD_imxpadRstNIOS));
    if (DEBUG) cout<< "LibXpadUSB : " << __func__ <<"() \t"<<"Module Resetted "<<endl;
    return 0;

}

/**
 * @brief xpad_cmd::modulesName
 *          set the hardware addr for S140 or more
 * @return  : -1 if error else 0
 */

int xpad_cmd::modulesName() {
    unsigned short  MOD_imxpadModuleName[6]   = { HUB_HEADER, HUB_MESSAGE, 0x0003,//0x02FF,
                                                  0x7FF0,0, // bit(0) - reset NIOS
                                                  MSG_TRAILER,
                                                };

    m_xpad_usb->sendSpecialCommand(MOD_imxpadModuleName,sizeof(MOD_imxpadModuleName));
    if (DEBUG) cout<< "LibXpadUSB : " << __func__ <<"() \t"<<"modulesName "<<endl;
    return 0;

}

/**
 * @brief xpad_cmd::resetRegFifo
 *          reset fifo memory in the reg board used for S140 or more
 * @return
 */

int xpad_cmd::resetRegFifo() {
    unsigned short  MOD_imxpadRstFifo[6]   = { HUB_HEADER, HUB_MESSAGE, 0x0003,//0x02FF,
                                               0x5FF0,0, // bit(0) - reset reg fifo
                                               MSG_TRAILER,
                                             };
    if(m_enableConfigureTransfertSize){
        m_xpad_usb->sendSpecialCommand(MOD_imxpadRstFifo,sizeof(MOD_imxpadRstFifo));
        if (DEBUG){
            cout<< "LibXpadUSB : " << __func__ <<"() \t"<<"Reset FiFo"<<endl;
        }
    }
    return 0;
}

/**
 * @brief xpad_cmd::setRegTrasfertSize
 * @param size : size of data transfert
 * @return
 */

int xpad_cmd::setRegTrasfertSize(unsigned short size) {
    unsigned short  MOD_imxpadSetTransfertSize[6]   = { HUB_HEADER, HUB_MESSAGE, 0x0003,//0x02FF,
                                                        0x8FF0,size, // bit(0) - set transfert size
                                                        MSG_TRAILER,
                                                      };
    if(m_enableConfigureTransfertSize){
        m_xpad_usb->sendSpecialCommand(MOD_imxpadSetTransfertSize,sizeof(MOD_imxpadSetTransfertSize));
        if (DEBUG){
            cout<< "LibXpadUSB : " << __func__ <<"() \t"<<"transfertSize = " << size <<endl;
        }
    }
    return 0;
}

/**
 * @brief xpad_cmd::setDataTransfert : give the numbers of 16 bits datas transfert from reg 2 usb
 * @param type :
 *              REG_CMD_SIZE  : use for command
 *              REG_IMG_2B    : use for images 16 bits
 *              REG_IMG_4B    : use for images 32 bits
 * @return -1 if ERROR type else 0
 */

int xpad_cmd :: setDataTransfert(int type){
    unsigned short size;

    switch(type){
    case REG_CMD_SIZE:
        size = 16;
        break;
    case REG_IMG_2B:
        size = 566;
        break;
    case REG_IMG_4B:
        size = 1126;
        break;
    default:
        return -1;
    }

    return setRegTrasfertSize(size);
}





/**
 * @brief xpad_cmd::testPulse
 *              send the analog hit to XPAD chip
 * @param moduleMask        : module mask
 * @param nbpulse           : numbers of pulse
 * @return                  : -1 if error else 1
 */

unsigned short* xpad_cmd::testPulse(unsigned short moduleMask, unsigned short nbpulse) {
    //unsigned short *buf;
    unsigned short *buffer;
    unsigned char *bufRaw=NULL;
    if(globalAskReady(moduleMask)!=0) return NULL;
    int NbMod = m_tools.getNbModMask(moduleMask);
    unsigned short  MOD_TestPulse[16]        = { HUB_HEADER, MOD_MESSAGE,
                                                 0x0008,        //nb words
                                                 moduleMask,    //modules masks
                                                 MOD_HEADER,
                                                 0x0005,        //nb words

                                                 0,             //reserved
                                                 CMD_TESTPULSE, //0x0106, auto_test
                                                 nbpulse,       //nb pulse
                                                 nbpulse,       //nb pulse
                                                 MSG_TRAILER,

                                                 0, 0, 0, 0, 0};


    int imgsize = 120*566*NbMod;
    bufRaw = new unsigned char[imgsize*2+2];

    if(m_enableConfigureTransfertSize){
        resetRegFifo();
        setDataTransfert(REG_IMG_2B);
    }
    if (DEBUG) cout<< "LibXpadUSB : " << __func__ <<"() \t"<<"Sending Test pulse to "<<"0x" << QString::number(moduleMask,16).toStdString()<<endl;
    if(m_xpad_usb->readOneImage(MOD_TestPulse,0,bufRaw,__func__,moduleMask)!=0)
    {
        if (DEBUG) cout<< "LibXpadUSB : " << __func__ <<"() \t"<<"Test Pulse error on module "<<"0x" << QString::number(moduleMask,16).toStdString()<<endl;
        delete [] bufRaw;
        // delete [] buf;
        return NULL;
    }
    if (DEBUG) cout<< "LibXpadUSB : " << __func__ <<"() \t"<<"Test Pulse on module "<<"0x" << QString::number(moduleMask,16).toStdString()<< " done !!!" << endl;

    buffer= new unsigned short [120*560*NbMod];
    m_tools.rawBuffer2Buffer(moduleMask,0,buffer,bufRaw,__func__);

    delete [] bufRaw;
    return buffer;

}



/**
 * @brief xpad_cmd::diagMemory
 *          test the electronics memory
 * @param moduleMask    : module mask
 * @param type          : type of test
 * @param value         : vaue set in the memory
 * @return              : -1 if error else 0
 */
unsigned int* xpad_cmd::diagMemory(unsigned short moduleMask, unsigned short type, unsigned short value) {
    // unsigned short *buf;
    unsigned short *buffer;
    unsigned char *bufRaw=NULL;
    unsigned int *dataOut;
    if(globalAskReady(moduleMask)!=0) return NULL;
    int NbMod = m_tools.getNbModMask(moduleMask);
    unsigned short  MOD_DiagMemory[16]        = { HUB_HEADER, MOD_MESSAGE,
                                                  0x0008,       //nb words
                                                  moduleMask,      //modules masks
                                                  MOD_HEADER,
                                                  0x0005,       //nb words

                                                  0,            //reserved
                                                  CMD_DIAG,     //0x0520, diag memory
                                                  type,              //not used
                                                  value,         //nb pulse
                                                  MSG_TRAILER,

                                                  0, 0, 0, 0, 0};


    int imgsize = 120*1126*NbMod;
    bufRaw = new unsigned char[imgsize*2+2];
    if(m_enableConfigureTransfertSize){
        resetRegFifo();
        setDataTransfert(REG_IMG_4B);
    }
    if (DEBUG) cout<< "LibXpadUSB : " << __func__ <<"() \t"<<"Sending Test pulse to "<<"0x" << QString::number(moduleMask,16).toStdString()<<endl;
    if(m_xpad_usb->readOneImage(MOD_DiagMemory,1,bufRaw,__func__,moduleMask)!=0)
    {
        if (DEBUG) cout<< "LibXpadUSB : " << __func__ <<"() \t"<<" error on module "<<"0x" << QString::number(moduleMask,16).toStdString()<<endl;
        delete [] bufRaw;
        //  delete [] buf;
        return NULL;
    }
    if (DEBUG) cout<< "LibXpadUSB : " << __func__ <<"() \t"<<"Diag memory on module "<<"0x" << QString::number(moduleMask,16).toStdString()<< " done !!!" << endl;

    buffer= new unsigned short [120*1126*NbMod];
    m_tools.rawBuffer2Buffer(moduleMask,1,buffer,bufRaw,__func__);
    delete [] bufRaw;

    dataOut = new unsigned int [120*560*NbMod];

    m_tools.bufferToIntBuffer(moduleMask,1,buffer,dataOut);
    delete [] buffer;
    return dataOut;

}
/**
 * @brief xpad_cmd::diagMemoryFileUpload
 *              load image from pc to electronic board memory
 * @param moduleMask: module mask
 * @param type      : type of test
 * @param fpath     : file path
 * @return          : image from electronic memory
 */

unsigned int* xpad_cmd::diagMemoryFileUpload(unsigned short moduleMask, unsigned short type, char* fpath) {
    //unsigned short *buf;
    unsigned short *buffer;
    unsigned char *bufRaw=NULL;
    unsigned int *dataOut;
    if(globalAskReady(moduleMask)!=0) return NULL;
    int NbMod = m_tools.getNbModMask(moduleMask);
    unsigned short  MOD_DiagMemory[16]        = { HUB_HEADER, MOD_MESSAGE,
                                                  0x0008,       //nb words
                                                  moduleMask,      //modules masks
                                                  MOD_HEADER,
                                                  0x0005,       //nb words

                                                  0,            //reserved
                                                  CMD_DIAG,     //0x0520, diag memory
                                                  type,              //not used
                                                  0,         //nb pulse
                                                  MSG_TRAILER,

                                                  0, 0, 0, 0, 0};

    /*   if(type == 4 || type == 5)
    {
        unsigned short *ret = loadConfigLFromFileToSRAM(moduleMask,fpath);
        if(ret != NULL)
                delete[]ret;
    }
*/

    int imgsize = 120*1126*NbMod;
    bufRaw = new unsigned char[imgsize*2+2];
    if(m_enableConfigureTransfertSize){
        resetRegFifo();
        setDataTransfert(REG_IMG_4B);
    }
    if (DEBUG) cout<< "LibXpadUSB : " << __func__ <<"() \t"<<"Sending Test pulse to "<<"0x" << QString::number(moduleMask,16).toStdString()<<endl;
    if(m_xpad_usb->readOneImage(MOD_DiagMemory,1,bufRaw,__func__,moduleMask)!=0)
    {
        if (DEBUG) cout<< "LibXpadUSB : " << __func__ <<"() \t"<<"Test Pulse error on module "<<"0x" << QString::number(moduleMask,16).toStdString()<<endl;
        delete [] bufRaw;
        //delete [] buf;
        return NULL;
    }
    if (DEBUG) cout<< "LibXpadUSB : " << __func__ <<"() \t"<<"Test Pulse on module "<<"0x" << QString::number(moduleMask,16).toStdString()<< " done !!!" << endl;

    buffer= new unsigned short [120*1126*NbMod];
    m_tools.rawBuffer2Buffer(moduleMask,1,buffer,bufRaw,__func__);
    delete [] bufRaw;

    dataOut = new unsigned int [120*560*NbMod];

    m_tools.bufferToIntBuffer(1,moduleMask,buffer,dataOut);
    delete [] buffer;
    return dataOut;

}


/**
 * @brief xpad_cmd::loadConfigG
 *              load global Config registers
 * @param moduleMask    : module mask
 * @param regid         : addr of register
 * @param value         : register value
 * @param chipMask      : chip mask
 * @return              : global config read after loaded
 */
unsigned short* xpad_cmd::loadConfigG(unsigned short moduleMask,unsigned short regid, unsigned short value, unsigned short chipMask) {
    unsigned short *buf;
    if(globalAskReady(moduleMask)!=0) return NULL;
    unsigned short  MOD_configG[16]         = { HUB_HEADER, MOD_MESSAGE,
                                                0x000C,         //nb words
                                                moduleMask,        //modules masks
                                                MOD_HEADER,
                                                0x0009,         //nb words

                                                0,              //reserved
                                                CMD_CFG_G,      //0x0103 configG
                                                chipMask,       //chipMask
                                                regid,          //register id code
                                                value,          //register value example
                                                0, 0, 0,
                                                MSG_TRAILER,
                                                0};




    unsigned short *data = new unsigned short [m_tools.getLastModule(moduleMask)];
    if(m_enableConfigureTransfertSize){
        resetRegFifo();
        setDataTransfert(REG_CMD_SIZE);
    }
    if (DEBUG){
        cout<< "LibXpadUSB : " << __func__ <<"() \t"<<"Sending LoadConfigG to "<<"0x" << QString::number(moduleMask,16).toStdString()<<endl;
    }

    buf = m_xpad_usb->sendCommand(moduleMask,MOD_configG,sizeof(MOD_configG),1000);

    if(buf != NULL){
        for(int mod=0;mod<m_tools.getNbModMask(moduleMask);mod++){
            if (buf[16*mod]==ACK_HEADER
                    //&&buf[16*mod+1]==moduleMask
                    &&buf[16*mod+2]==ACK_SIZE
                    &&buf[16*mod+3]==ACK_CFG_G
                    &&buf[16*mod+15]==ACK_TRAILER) {
                if((1 << buf[16*mod+1]-1) & moduleMask){
                    data[m_tools.mask2ModNumber(buf[16*mod+1])] = buf[mod*16+4];
                    if (DEBUG){
                        cout<< "LibXpadUSB : " << __func__ <<"() \t" << "LoadConfigG completed on module 0x" << QString::number(buf[16*mod+1],16).toStdString()  << endl;
                        fflush(stdout);
                    }
                }
                else{
                    if (DEBUG){
                        cout<< "LibXpadUSB : " << __func__ <<"() \t" << "LoadConfigG module Mask ERROR" << endl;
                        fflush(stdout);
                    }
                    delete [] buf;
                    return NULL;
                }

            }
            else {
                if (DEBUG){
                    cout<< "LibXpadUSB : " << __func__ <<"() \t" << "LoadConfigG error on module " << "0x" << QString::number(moduleMask,16).toStdString() << endl;
                }
                delete [] buf;
                return NULL;
            }
        }
        delete [] buf;
        return data;
    }
    else {
        if (DEBUG){
            cout<< "LibXpadUSB : " << __func__ <<"() \t" << "LoadConfigG error buffer return NULL " << endl;
        }
        return NULL;
    }
}

/**
 * @brief xpad_cmd::readConfigG
 * @param moduleMask : module mask
 * @param regid      : global register addr
 * @param chipMask   : chip mask for all chips 0x7F
 * @return           : the global register value redid
 */

unsigned short* xpad_cmd::readConfigG(unsigned short moduleMask,unsigned short regid,unsigned short chipMask) {
    unsigned short *buf;
    if(globalAskReady(moduleMask)!=0) return NULL;
    unsigned short  MOD_readConfigG[16]     = { HUB_HEADER, MOD_MESSAGE,
                                                0x000C,             //nb words
                                                moduleMask,            //modules masks
                                                MOD_HEADER,
                                                0x0009,             //nb words

                                                0,                  //reserved
                                                CMD_READ_CFG_G,     //0x0105, readConfigG
                                                chipMask,           //chipMask
                                                regid,              //register id code
                                                0,
                                                0, 0, 0,
                                                MSG_TRAILER,

                                                0};
    unsigned short *data = new unsigned short [8 *m_tools.getNbModMask(moduleMask)];

    if (DEBUG) cout<< "LibXpadUSB : " << __func__ <<"() \t"<<"Sending ReadConfigG to "<<"0x" << QString::number(moduleMask,16).toStdString()<<endl;
    if(m_enableConfigureTransfertSize){
        resetRegFifo();
        setDataTransfert(REG_CMD_SIZE);
    }
    // buf = sendCmd( MOD_readConfigG);
    buf = m_xpad_usb->sendCommand(moduleMask,MOD_readConfigG,sizeof(MOD_readConfigG), 1000);

    if(buf != NULL){
        for(int mod=0;mod<m_tools.getNbModMask(moduleMask);mod++){

            if (buf[16*mod]==ACK_HEADER
                    //&&buf[16*mod+1]==moduleMask
                    &&buf[16*mod+2]==ACK_SIZE
                    &&buf[16*mod+3]==ACK_READ_CFG_G
                    &&buf[16*mod+15]==ACK_TRAILER) {
                if (DEBUG) cout<< "LibXpadUSB : " << __func__ <<"() \t" << "ReadConfigG completed on module " << buf[16*mod+1] << endl;

                if(buf[16*mod+1] & moduleMask){
                    for(int i=0;i<8;i++)
                        data[m_tools.mask2ModNumber(buf[16*mod+1])*8+i] = buf[mod*16+4+i];
                }
                else{
                    delete [] buf;
                    delete [] data;
                    return NULL;
                }
            }
            else {
                if (DEBUG) cout<< "LibXpadUSB : " << __func__ <<"() \t" << "ReadConfigG error on module " << "0x" << QString::number(moduleMask,16).toStdString()<< endl;
                delete [] data;
                delete [] buf;
                return NULL;
            }
        }// end for mod
        delete [] buf;
        return data;
    }// end buf
}

/**
 * @brief xpad_cmd::readTempSensor : read the temperature sensor inside the XPAD chip
 * @param moduleMask : module mask
 * @return : one buffer with all temperature values
 */
float* xpad_cmd::readTempSensor(unsigned short moduleMask) {
    unsigned short *buf;
    if(globalAskReady(moduleMask)!=0) return NULL;
    unsigned short  MOD_readTempSensor[16]     = { HUB_HEADER, MOD_MESSAGE,
                                                   0x000C,             //nb words
                                                   moduleMask,            //modules masks
                                                   MOD_HEADER,
                                                   0x0009,             //nb words

                                                   0,                  //reserved
                                                   CMD_READ_TEMP,     //0x0105, readConfigG
                                                   0,           //chipMask
                                                   0,              //register id code
                                                   0,
                                                   0, 0, 0,
                                                   MSG_TRAILER,

                                                   0};
    float *data = new float [7 * m_tools.getNbModMask(moduleMask)];

    if (DEBUG) cout<< "LibXpadUSB : " << __func__ <<"() \t"<<"Sending ReadTempSensor to "<<"0x" << QString::number(moduleMask,16).toStdString()<<endl;
    if(m_enableConfigureTransfertSize){
        resetRegFifo();
        setDataTransfert(REG_CMD_SIZE);
    }
    buf = m_xpad_usb->sendCommand(moduleMask,MOD_readTempSensor,sizeof(MOD_readTempSensor), 10000);

    if(buf != NULL){
        for(int mod=0;mod<m_tools.getNbModMask(moduleMask);mod++){

            if (buf[16*mod]==ACK_HEADER
                    //&&buf[16*mod+1]==moduleMask
                    &&buf[16*mod+2]==ACK_SIZE
                    &&buf[16*mod+3]==ACK_READ_TEMP
                    &&buf[16*mod+15]==ACK_TRAILER) {
                if (DEBUG) cout<< "LibXpadUSB : " << __func__ <<"() \t" << "ReadTempSensor completed on module " << buf[16*mod+1] << endl;
                // return &buf[4];
                if(buf[16*mod+1] & moduleMask){
                    for(int i=0;i<7;i++)
                        data[m_tools.mask2ModNumber(buf[16*mod+1])*8+i] = (float)buf[mod*16+4+i]*2.5-465.0;
                }
                else{
                    delete [] buf;
                    delete [] data;
                    return NULL;
                }
            }
            else {
                if (DEBUG) cout<< "LibXpadUSB : " << __func__ <<"() \t" << "ReadTempSensor error on module " << "0x" << QString::number(moduleMask,16).toStdString()<< endl;
                delete [] data;
                delete [] buf;
                return NULL;
            }
        }// end for mod
        delete [] buf;
        return data;
    }// end buf
    return NULL;
}


/**
 * @brief xpad_cmd::loadConfigLSRAMToDetector : load all local dac from the electronic board to XPAD chips
 * @param moduleMask    : module mask
 * @return              : return -1 if error else 0
 */
int xpad_cmd::loadConfigLSRAMToDetector(unsigned short moduleMask) {
    unsigned short *buf;
    if(globalAskReady(moduleMask)!=0) return -1;
    unsigned short  MOD_detLoadConfig[16]   = { HUB_HEADER, MOD_MESSAGE,
                                                0x0007,         //nb words
                                                moduleMask,        // modules masks
                                                MOD_HEADER,
                                                0x0004,         //nb words

                                                0,              // reserved
                                                CMD_LOAD_CFG_L, // 0x0480 detector load config
                                                0,              //calib id
                                                MSG_TRAILER,

                                                0, 0, 0, 0, 0, 0};

    if(m_enableConfigureTransfertSize){
        resetRegFifo();
        setDataTransfert(REG_CMD_SIZE);
    }
    if (DEBUG) cout<< "LibXpadUSB : " << __func__ <<"() \t"<<"Sending LoadConfigLSRAMToDetector to "<<moduleMask<<endl;
    buf = m_xpad_usb->sendCommand(moduleMask,MOD_detLoadConfig,sizeof(MOD_detLoadConfig),1000);
    if(buf!=NULL){
        if(checkModAcK(moduleMask,buf,ACK_LOAD_CFG_L)==0){
            if (DEBUG) cout<< "LibXpadUSB : " << __func__ <<"()\t" << "LoadConfigLSRAMToDetector error on module " << "0x" << QString::number(moduleMask,16).toStdString() << endl;
            delete[] buf;
            return 0;
        }
        else{
            if (DEBUG) cout<< "LibXpadUSB : " << __func__ <<"()\t" << "LoadConfigLSRAMToDetector completed on module " << "0x" << QString::number(moduleMask,16).toStdString() << endl;
            delete[] buf;
            return -1;
        }
    }
    return -1;
}

/**
 * @brief xpad_cmd::loadFlatConfigL
 * @param moduleMask    : module mask
 * @param value         : DACL value for all pixels (  DACLvalue * 8 + 1)
 * @param chipMask      : chip mask max value 0x7F
 * @return
 */

int xpad_cmd::loadFlatConfigL(unsigned short moduleMask,unsigned short value,unsigned short chipMask) {
    unsigned short *buf;
    if(globalAskReady(moduleMask)!=0) return -1;
    unsigned short  MOD_flatConfig[16]      = { HUB_HEADER, MOD_MESSAGE,
                                                0x0009,         //nb words
                                                moduleMask,        // modules masks
                                                MOD_HEADER,
                                                0x0006,         //nb words

                                                0,              // reserved
                                                CMD_FLAT_CFG,   // 0x0104 flatConfig
                                                0,              // unused
                                                chipMask,       // chipMask All the chips
                                                value,          //value example
                                                MSG_TRAILER,

                                                0, 0, 0, 0};



    if (DEBUG) cout<< "LibXpadUSB : " << __func__ <<"() \t" << "Sending LoadFlatConfigL to " << "0x" << QString::number(moduleMask,16).toStdString() << endl;

    if(m_enableConfigureTransfertSize){
        resetRegFifo();
        setDataTransfert(REG_CMD_SIZE);
    }
    buf = m_xpad_usb->sendCommand(moduleMask,MOD_flatConfig,sizeof(MOD_flatConfig),1000);

    if(buf!=NULL){
        if(checkModAcK(moduleMask,buf,ACK_FLAT_CFG)==0){
            if (DEBUG) cout << "LibXpadUSB : " << __func__ <<"()\t"<< "LoadFlatConfigL completed on module " << "0x" << QString::number(moduleMask,16).toStdString() << endl;
            delete[] buf;
            return 0;
        }
        else{
            if (DEBUG) cout << "LibXpadUSB : " << __func__ <<"()\t"<< "LoadFlatConfigL error on module " << "0x" << QString::number(moduleMask,16).toStdString() << endl;
            delete[] buf;
            return -1;
        }
    }
    else
        return -1;
}


int xpad_cmd::globalAskReady(unsigned short moduleMask) {
    unsigned short *buf;
    unsigned short  MOD_GlobalAskReady[16]      = { HUB_HEADER, MOD_MESSAGE,
                                                    0x0009,         //nb words
                                                    moduleMask,        // modules masks
                                                    MOD_HEADER,
                                                    0x0006,         //nb words

                                                    0,              // reserved
                                                    CMD_ASKREADY,   // 0x0101 Ask ready
                                                    0,              // unused
                                                    0,              // chipMask All the chips
                                                    0,              //value example
                                                    MSG_TRAILER,

                                                    0, 0, 0, 0};



    if (DEBUG) cout<< "LibXpadUSB : " << __func__ <<"() \t" << "0x" << QString::number(moduleMask,16).toStdString() << endl;
    //modulesName();
    if(m_enableConfigureTransfertSize){
        resetRegFifo();
        setDataTransfert(REG_CMD_SIZE);
    }
    buf = m_xpad_usb->sendCommand(moduleMask,MOD_GlobalAskReady,sizeof(MOD_GlobalAskReady),500);

    if(buf!=NULL){
        if(checkModAcK(moduleMask,buf,ACK_ASKREADY)==0){
            if (DEBUG) cout << "LibXpadUSB : " << __func__ <<"()\t"<< "globalAskReady completed on module " << moduleMask << endl;
            delete[] buf;
            return 0;
        }
        else{
            if (DEBUG) cout << "LibXpadUSB : " << __func__ <<"()\t"<< "globalAskReady error on module " << moduleMask << endl;
            delete[] buf;
            return -1;
        }
    }
    else
        return -1;
}



/**
 * @brief xpad_cmd::saveConfigLToSRAM
 *              save local config line of chip per line
 * @param moduleMask    : module mask
 * @param chip          : chip concerne by the config line
 * @param row           : row in the chip ( chip mask )
 * @param value         : 80 value for one line
 * @return              : -1 if error else 0
 */

int xpad_cmd::saveConfigLToSRAM(unsigned short moduleMask,unsigned short chip, unsigned short row, unsigned short* value) {
    unsigned short *buf;
    unsigned short  MOD_saveConfigL[96]     = { HUB_HEADER, MOD_MESSAGE,
                                                0x0059,         //nb words
                                                moduleMask,        // modules masks
                                                MOD_HEADER,
                                                0x0056,         //nb words

                                                0,              // reserved
                                                CMD_SAVE_CFG_L, // 0x0380, saveConfig
                                                0,              // calib id
                                                chip,           //current chip
                                                row,            // current row
                                                // 80 values
                                                0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                                0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                                0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                                0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                                0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                                0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                                0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                                0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                                MSG_TRAILER,

                                                0 , 0, 0, 0};



    for (int i = 0; i<80 ;i++) MOD_saveConfigL[11+i] = value[i];

    if (DEBUG) cout << "LibXpadUSB : Sending SaveConfigLToSRAM to " << "0x" << QString::number(moduleMask,16).toStdString() << endl;
    if(m_enableConfigureTransfertSize){
        resetRegFifo();
        setDataTransfert(REG_CMD_SIZE);
    }
    buf = m_xpad_usb->sendCommand(moduleMask,MOD_saveConfigL,sizeof(MOD_saveConfigL),1000);
    if(buf!=NULL){
        if(checkModAcK(moduleMask,buf,ACK_SAVE_CFG_L)==0){
            if (DEBUG) cout<< "LibXpadUSB : " << __func__ <<"() \t" << "SaveConfigLToSRAM completed on module " << "0x" << QString::number(moduleMask,16).toStdString() << endl;
            delete[] buf;
            return 0;
        }
        else{
            if (DEBUG) cout << "LibXpadUSB : " << __func__ <<"() \t"<< "SaveConfigLToSRAM error on module " << "0x" << QString::number(moduleMask,16).toStdString() << endl;
            delete[] buf;
            return -1;
        }
    }
    return -1;
}


/**
 * @brief xpad_cmd::readConfigL
 *              read local config from the XPAD chip
 * @param moduleMask        : module mask
 * @return                  : dacl matrix
 */

unsigned short* xpad_cmd::readConfigL(unsigned short moduleMask ) {
    unsigned short *buffer;
    if(globalAskReady(moduleMask)!=0) return NULL;
    unsigned char  *rawBuffer;
    int NbMod = m_tools.getNbModMask(moduleMask);
    unsigned short  MOD_readConfig[16]      = { HUB_HEADER, MOD_MESSAGE,
                                                0x0006,         //nb words
                                                moduleMask,        // modules masks
                                                MOD_HEADER,
                                                0x0003,         //nb words

                                                0,              // reserved
                                                CMD_READ_CFG_L, // 0x01C0, readConfig
                                                MSG_TRAILER,

                                                0, 0, 0, 0, 0, 0, 0};


    if (DEBUG) cout << "LibXpadUSB : " << __func__ <<"() \t"<< "Getting image ReadConfigL from " << "0x" << QString::number(moduleMask,16).toStdString() << endl;

    rawBuffer = new unsigned char [120*566*NbMod*2+2];
    if(rawBuffer==NULL){
        if (DEBUG) cout << "LibXpadUSB : " << __func__ <<"() \t"<< "Can Not created the temp buffer" << endl;
        return NULL;
    }
    buffer = new unsigned short [120*560*NbMod];
    if(buffer==NULL){
        delete [] rawBuffer;
        if (DEBUG) cout << "LibXpadUSB : " << __func__ <<"() \t"<< "Can Not created the output buffer" << endl;
        return NULL;
    }
    if(m_enableConfigureTransfertSize){
        resetRegFifo();
        setDataTransfert(REG_IMG_2B);
    }


    if(m_xpad_usb->readOneImage(MOD_readConfig,0,rawBuffer,__func__,moduleMask)!=0){
        if (DEBUG) cout << "LibXpadUSB : " << __func__ <<"() \t"<< "No ReadConfigL returned" << endl;
        delete [] rawBuffer;
        return NULL;
    }
    m_tools.rawBuffer2Buffer(moduleMask,0,buffer,rawBuffer,__func__);
    if (DEBUG) cout << "LibXpadUSB : " << __func__ <<"() \t"<< "ReadConfigL returned" << endl;
    delete [] rawBuffer;
    return buffer;
}

/**
 * @brief xpad_cmd::exposeParam
 *                  send expose parameters for acquisition
 * @param moduleMask        : module mask
 * @param numImages         : numbers of images
 * @param ExposureTime      : exposure time in µs
 * @param overFlowTime      : overflow refresh time µs
 * @param TriggerMode       :external trigger mode
 * @param BusySrc           : busy output signal
 * @param waitingTime       : time between tow images
 * @param AcqMode           : type of acquisition ex: burst or standart
 * @param StakingOrBunchNumber  : numbers of stack or numbers of bunch depending of AcqMode
 * @return                  : -1 if error else 0
 */


int xpad_cmd::exposeParam(unsigned short moduleMask ,unsigned int numImages, unsigned int ExposureTime,
                          unsigned int overFlowTime, unsigned short TriggerMode, unsigned short BusySrc,
                          unsigned int waitingTime, unsigned short AcqMode, unsigned int StakingOrBunchNumber) {
    if(globalAskReady(moduleMask)!=0) return -1;
    unsigned short *buf;
    unsigned short time0 = (ExposureTime&0xffff0000)>>16;
    unsigned short time1 = ExposureTime&0x0000ffff;
    unsigned short overFlow0 = (overFlowTime&0xffff0000)>>16;
    unsigned short overFlow1 = overFlowTime&0x0000ffff;
    unsigned short waitingTime0 = 0;
    unsigned short waitingTime1 = 0;
    unsigned int   waitingTime_real;

    unsigned short StackBunch0 = (StakingOrBunchNumber>> 16) & 0xffff;
    unsigned short StackBunch1 = (StakingOrBunchNumber) & 0xffff;

    unsigned short AqcMode_internal;
    unsigned short ImageFormat;


    if(waitingTime < 10000 && AcqMode == EXPOSE_STANDARD)
        waitingTime_real = 10000;
    else
        waitingTime_real = waitingTime;

    if(waitingTime > 60){
        waitingTime0 = ((waitingTime_real-60)&0xffff0000)>>16;
        waitingTime1 = (waitingTime_real-60)&0x0000ffff;
    }

    //2Byte or 4Byte data output Automatic Selection Format
    if(AcqMode == EXPOSE_STANDARD){ // standart mode
        AqcMode_internal = 0;
        if (ExposureTime > 16*overFlowTime)
            ImageFormat = 0x1;        // Image data format 4 Bytes
        else
            ImageFormat = 0x0;        // Image data format 2 Bytes
    }
    else  if(AcqMode == EXPOSE_STACKING_16) {  // Stacking Mode
        AqcMode_internal = 3;
        ImageFormat = 0x0;        // Image data format 2 Bytes
        if(numImages > 980){
            cout << "LibXpadUSB => " << __func__ << "() ERROR :: Cannot set Numbers of images value = " << numImages << endl;
            return -1;
        }
    }
    else  if(AcqMode == EXPOSE_STACKING_32) {  // Stacking Mode
        AqcMode_internal = 3;
        ImageFormat = 0x1;        // Image data format 4 Bytes
        if(numImages > 480){
            cout << "LibXpadUSB => " << __func__ << "() ERROR :: Cannot set Numbers of images value = " << numImages << endl;
            return -1;
        }
    }
    else if(AcqMode == EXPOSE_SINGLE_BUNCH_16){
        AqcMode_internal = 4;
        ImageFormat=0x0;
    }
    else if(AcqMode == EXPOSE_SINGLE_BUNCH_32){
        AqcMode_internal = 4;
        ImageFormat=0x1;
    }
    else if(AcqMode == EXPOSE_LOCALBURST) {       // mode DDR and film
        AqcMode_internal = 1;
        ImageFormat = 0x0;        // Image data format 2 Bytes
        if(numImages > 980){
            cout << "LibXpadUSB => " << __func__ << "() ERROR :: Cannot set Numbers of images value = " << numImages << endl;
            return -1;
        }
    }
    else if(AcqMode == EXPOSE_BURST) {       // mode DDR and film
        AqcMode_internal = 2;
        ImageFormat = 0x0;        // Image data format 2 Bytes
    }
    else{
        cout << "LibXpadUSB => " << __func__ << "() ERROR :: Cannot set AQC_MODE value = " << AcqMode << endl;
        return -1;
    }

    if(TriggerMode == TRIG_SINGLE_BUNCH){
        cout << "LibXpadUSB => " << __func__ << "() ERROR :: Cannot set TriggerMode value = " << TriggerMode << endl;
        return -1;
    }
    globalAskReady(moduleMask);
    unsigned short  MOD_exposureParam[30]   = { HUB_HEADER, MOD_MESSAGE,
                                                0x001B,         //nb words
                                                moduleMask,        // modules masks
                                                MOD_HEADER,
                                                0x0018,         //nb words

                                                0,              // reserved
                                                CMD_EXPOSE_PAR, // 0x190 exposure parameters command
                                                time0,          // exposure time H
                                                time1,          // exposure time L
                                                waitingTime0,   //0x0,            // dead time H
                                                waitingTime1,   //0x1388,         // dead time L
                                                0x0,            // init time H
                                                0x0,            // init time L
                                                0x000f,         // shutter time H
                                                0x3e58,         // shutter time L
                                                overFlow0,      // ovf scan period H
                                                overFlow1,      // ovf scan period L
                                                TriggerMode,           // exposure mode
                                                0x0,            // N param
                                                0x0,            // P param
                                                numImages,            // Number of images
                                                BusySrc,            // Busy out select
                                                0x1,            // Image data format
                                                0x0,            // Post-processing enabled
                                                0x0,            //
                                                AqcMode_internal,            // 0 mode standart | 1 mode DDR | 2 mode film | 3 staking Mode
                                                StackBunch0,      // Number of the loop in the staking mode or Bunch Number
                                                StackBunch1,
                                                MSG_TRAILER
                                              };


    if (DEBUG) cout<< "LibXpadUSB : " << __func__ <<"() \t"<<"Sending ExposeParam to "<<"0x" << QString::number(moduleMask,16).toStdString()<<endl;

    MOD_exposureParam[23] = ImageFormat;
    m_image_type = MOD_exposureParam[23];
    m_image_numbers = numImages;
    if(m_enableConfigureTransfertSize){
        resetRegFifo();
        setDataTransfert(REG_CMD_SIZE);
    }
    buf = m_xpad_usb->sendCommand(moduleMask,MOD_exposureParam,sizeof(MOD_exposureParam),1000);
    if(buf!=NULL){
        if(checkModAcK(moduleMask,buf,ACK_EXPOSE_PAR)==0){
            if (DEBUG) cout << "LibXpadUSB : " << __func__ <<"() \t" << "ExposeParam completed on module " << moduleMask << endl;
            delete[] buf;
            return -0;
        }
        else{
            if (DEBUG) cout << "LibXpadUSB : " << __func__ <<"() \t"<< "ExposeParam error on module " << moduleMask << endl;
            delete[] buf;
            return -1;
        }
    }
    return -1;
}


/**
 * @brief xpad_cmd::expose
 *          start detector exposition synchrone
 * @param moduleMask    : module mask
 * @param burstNumber   : burst number
 * @return              : - 1 if error else 0
 */

int xpad_cmd::expose(unsigned short moduleMask,unsigned int burstNumber) {
    FILE *fd_img =NULL;
    char fileName[200];
    int imgsize=0;
    if(globalAskReady(moduleMask)!=0) return -1;
    int NbMod = m_tools.getNbModMask(moduleMask);
    unsigned char *bufRaw=NULL;
    unsigned short  MOD_expose[16]          = { HUB_HEADER, MOD_MESSAGE,
                                                0x0009,         //nb words
                                                moduleMask,        // modules masks
                                                MOD_HEADER,
                                                0x0006,         //nb words

                                                0,              // reserved
                                                CMD_EXPOSE,     // 0x0140
                                                0,              // gate mode
                                                0,              // gate length
                                                0,              // time unit
                                                MSG_TRAILER,

                                                0, 0, 0, 0};

    m_expose_busy = 1;
    if (DEBUG) cout << "LibXpadUSB : " << __func__ <<"() \t"<< "Sending Expose to " << "0x" << QString::number(moduleMask,16).toStdString() << endl;

    if(m_enableConfigureTransfertSize){
        resetRegFifo();
        if(m_image_type == 0){
            setDataTransfert(REG_IMG_2B);
        }
        else{
            setDataTransfert(REG_IMG_4B);
        }
    }

    if(m_image_type == 0){
        imgsize = 120*566*NbMod;

    }
    else{
        imgsize = 120*1126*NbMod;
    }

    bufRaw = new unsigned char[imgsize*2+2];
    if(bufRaw == NULL)
        return -1;

    m_xpad_usb->setAbortFlag(false);



    if(m_xpad_usb->readImgInit(MOD_expose) != 0){
        cout << "LibXpadUSB : " << __func__ <<"() /opt/imXPAD/tmp/burst_%d_img_%d.bin\t"<< "ERROR => readImgInit(MOD_expose)" << endl;
    }

    m_current_image_number = 0;
    for(unsigned int i = 0;i<m_image_numbers;i++)
    {
        if(DEBUG)
            cout << "LibXpadUSB : " << __func__ <<"() \t"<< "Image Number = " << i << endl;

        sprintf(fileName,RAWFILE,burstNumber,i);
        fd_img = fopen(fileName,"wb");
        if(fd_img == NULL){
            cout << "LibXpadUSB : " << __func__ <<"() \t"<< "Can not open " << fileName << endl;
            return -1;
        }

        if(m_xpad_usb->readImgBuff(m_image_type,bufRaw,moduleMask)!=0){
            cout << "LibXpadUSB : " << __func__ <<"() \t"<< "ERROR => ReadImgBuff(" << i <<")" << endl;
        }

        fwrite(bufRaw,imgsize*2+2,1,fd_img);
        fclose(fd_img);

        if(m_xpad_usb->getAbortFlag())
            break;

        m_current_image_number++;
    }

    if(m_xpad_usb->readImgClose()!=0){
        cout << "LibXpadUSB : " << __func__ <<"() \t"<< "ERROR => readImgClose" << endl;
    }
    delete[] bufRaw;
    cout << "LibXpadUSB : " << __func__ <<"() \t"<< "Expose finish !!!!" << endl;

    m_xpad_usb->setAbortFlag(false);
    m_expose_busy = 0;
    return 0;
}

/**
 * @brief xpad_cmd::readImage
 *              this function read the image in the detector memory in 4 bytes
 * @param moduleMask    : module mask
 * @return image buffer
 */
unsigned int * xpad_cmd::readImage(unsigned short moduleMask){
    if(globalAskReady(moduleMask)!=0) return NULL;
    return this->readImage4B(moduleMask);
}


/**
 * @brief xpad_cmd::readImage2B
 *                  this function read the image in the detector memory in 2 bytes
 * @param moduleMask
 * @return image buffer
 */
unsigned short* xpad_cmd::readImage2B(unsigned short moduleMask) {
    //  unsigned short *buf;
    unsigned char *rawBuffer;
    if(globalAskReady(moduleMask)!=0) return NULL;
    int NbMod = m_tools.getNbModMask(moduleMask);
    unsigned short *buffer;
    unsigned short  MOD_img2B[16]           = { HUB_HEADER, MOD_MESSAGE,
                                                0x0006,         //nb words
                                                moduleMask,        //modules masks
                                                MOD_HEADER,
                                                0x0003,         //nb words

                                                0,              //reserved
                                                CMD_IMG_2B,     //0x0182 request img 2B
                                                MSG_TRAILER,

                                                0, 0, 0, 0, 0, 0, 0};



    if (DEBUG) cout<< "LibXpadUSB : " << __func__ <<"() \t" << "Getting 2B image from " << moduleMask << endl;

    rawBuffer = new unsigned char [120*566*NbMod*2+2];
    if(rawBuffer==NULL){
        if (DEBUG) cout << "LibXpadUSB : " << __func__ <<"() \t"<< "Can Not created the temp buffer" << endl;
        return NULL;
    }
    buffer = new unsigned short [120*560*NbMod];
    if(buffer==NULL){
        delete [] rawBuffer;
        if (DEBUG) cout << "LibXpadUSB : " << __func__ <<"() \t"<< "Can Not created the output buffer" << endl;
        return NULL;
    }

    if(m_enableConfigureTransfertSize){
        resetRegFifo();
        setDataTransfert(REG_IMG_2B);
    }


    if(m_xpad_usb->readOneImage(MOD_img2B,0,rawBuffer,__func__,moduleMask)!=0){
        delete [] rawBuffer;
        delete [] buffer;
        if (DEBUG) cout << "LibXpadUSB : " << __func__ <<"() \t"<< "Any 16 bits image was returned" << endl;
        return NULL;

    }
    if (DEBUG) cout << "LibXpadUSB : " << __func__ <<"() \t"<< "16 bits image was returned" << endl;
    m_tools.rawBuffer2Buffer(moduleMask,0,buffer,rawBuffer,__func__);
    delete [] rawBuffer;
    return buffer;

}

/**
 * @brief xpad_cmd::readImage4B
 *              this function read the image in the detector memory in 4 bytes
 * @param moduleMask
 * @return image buffer
 */
unsigned int* xpad_cmd::readImage4B(unsigned short moduleMask) {
    //unsigned short *buf;
    unsigned char *RawBuffer;
    if(globalAskReady(moduleMask)!=0) return NULL;
    int NbMod = m_tools.getNbModMask(moduleMask);
    unsigned int *buffer;
    unsigned short  MOD_img4B[16]           = { HUB_HEADER, MOD_MESSAGE,
                                                0x0006,         //nb words
                                                moduleMask,        //modules masks
                                                MOD_HEADER,
                                                0x0003,         //nb words

                                                0,              //reserved
                                                CMD_IMG_4B,     //0x0183 request img 4B
                                                MSG_TRAILER,

                                                0, 0, 0, 0, 0, 0, 0};


    if (DEBUG) cout<< "LibXpadUSB : " << __func__ <<"() \t"<<"Getting 4B image from "<<"0x" << QString::number(moduleMask,16).toStdString()<<endl;

    RawBuffer = new unsigned char[NbMod*1126*120*2+2];
    unsigned short *buf16 = new unsigned short [NbMod*1126*120];

    if(RawBuffer == NULL){
        if (DEBUG) cout<< "LibXpadUSB : " << __func__ <<"() \t" << "Can not created the temp buffer " << endl;
        return NULL;
    }
    if(buf16 == NULL){
        if (DEBUG) cout<< "LibXpadUSB : " << __func__ <<"() \t" << "Can not created the temp buffer " << endl;
        return NULL;
    }
    if(m_enableConfigureTransfertSize){
        resetRegFifo();
        setDataTransfert(REG_IMG_4B);
    }
    if(m_xpad_usb->readOneImage(MOD_img4B,1,RawBuffer,__func__,moduleMask)!=0)
    {
        if (DEBUG) cout<< "LibXpadUSB : " << __func__ <<"() \t" << "No 4B image returned" << endl;
        return NULL;
    }

    m_tools.rawBuffer2Buffer(moduleMask,1,buf16,RawBuffer,__func__);

    delete[] RawBuffer;
    buffer = new unsigned int [120*560*NbMod];
    if(buffer == NULL){
        if (DEBUG) cout<< "LibXpadUSB : " << __func__ <<"() \t" << "Can not created the output buffer " << endl;
        delete[] buf16;
        return NULL;
    }
    for (int i = 0; i<120*NbMod; i++) {
        for (unsigned int j = 0; j<560;j++) {
            buffer[i*560+j]=buf16[i*1120+j*2]|(buf16[i*1120+j*2+1]<<16);
        }
    }
    delete[] buf16;
    return buffer;
}

/**
 * @brief xpad_cmd::abortExposure
 *          this function abort all command send to the detector
 * @return 0
 */

int xpad_cmd::abortExposure() {
    unsigned short  MOD_imxpadAbortExp[16]  = { HUB_HEADER, HUB_MESSAGE, 0x0002,//0x02FF,
                                                CMD_ABORT, // bit(1) - abort exposure
                                                MSG_TRAILER,
                                                0, 0, 0
                                                ,0};
    m_xpad_usb->setAbortFlag(true);
    m_xpad_usb->sendSpecialCommand(MOD_imxpadAbortExp,sizeof(MOD_imxpadAbortExp));
    return 0;

}


/**
 * @brief xpad_cmd::checkModAcK
 *          this function check if the detector return good data
 * @param Mask          : module mask
 * @param data          : data receive by the detector
 * @param Ack           : test with ack
 * @return
 */
int xpad_cmd::checkModAcK(unsigned short Mask, unsigned short *data, unsigned short Ack)
{
    int nbMod = m_tools.getNbModMask(Mask);
    if(data != NULL){
        if(DEBUG) cout<< "LibXpadUSB : " << __func__ <<"() \t" << "Detector to USB ==>"<< endl;
        for(int mod=0;mod<nbMod;mod++)
        {
            if(DEBUG){
                for(int i=0;i<16;i++)
                    printf("0x%x ",data[mod*16 + i]);
                cout << endl;
            }
            if (data[mod*16 + 0]==ACK_HEADER
                    &&data[mod*16 +2]==ACK_SIZE
                    &&data[mod*16 +3]==Ack
                    &&data[mod*16 +15]==ACK_TRAILER) {
                if((1<<(data[mod*16 +1]-1) & Mask)==0)
                    return -1;
            }
            else {
                if (DEBUG) cout<<"ACK error " <<endl;
                return -1;
            }
        }// for mod
    }
    else // date == NULL
        return -1;

    return 0;
}

/**
 * @brief xpad_cmd::readADC
 *  This function read All ADC on C4 board VA,VD,VT and HV
 * @param moduleMask : module mask
 * @param Voltage_value : output buffer with all values,
 *               val[0] = HV
 *               val[1] = VA
 *               val[2] = VT
 *               val[3] = VD
 * @return -1 if ERROR else 0
 */

int xpad_cmd::readADC(unsigned short moduleMask,float Voltage_value[]) {
    int ret=0;
    unsigned short *buf;
    if(globalAskReady(moduleMask)!=0) return NULL;
    const float bin = (3.3/256);
    int nbMod = m_tools.getNbModMask(moduleMask);
    const float binHV = (bin*(1000000+20000)/20000);
    unsigned short modAddr=0;
    unsigned short  MOD_readADC[16]        = { HUB_HEADER, MOD_MESSAGE,
                                               0x0006,       //nb words
                                               moduleMask,      //modules masks
                                               MOD_HEADER,
                                               0x0003,       //nb words

                                               0,            //reserved
                                               CMD_READ_ADC, //0x0500, Read ADC ( Channel_0 : HV, Channel_1 : VA, Channel_2 : VD, Channel_3 : VT )
                                               MSG_TRAILER,

                                               0,0,0,0,0,0,0};


    if (DEBUG) cout<< "LibXpadUSB : " << __func__ <<"() \t" << "Sending Read ADC to " << "0x" << QString::number(moduleMask,16).toStdString() << endl;
    if(m_enableConfigureTransfertSize){
        resetRegFifo();
        setDataTransfert(REG_CMD_SIZE);
    }
    buf = m_xpad_usb->sendCommand(moduleMask,MOD_readADC,sizeof(MOD_readADC),1000);
    if(buf!=NULL){
        for(int mod = 0;mod<nbMod;mod++)
        {
            if (buf[0 + mod*16]==ACK_HEADER
                    //&&buf[1]==moduleMask
                    &&buf[2 + mod*16]==ACK_SIZE
                    &&buf[3 + mod*16]==ACK_READ_ADC
                    &&buf[15 + mod*16]==ACK_TRAILER) {

                modAddr = m_tools.mask2ModNumber(buf[1 + mod*16]);
                if (DEBUG) cout<< "LibXpadUSB : " << __func__ <<"() \t" << "Module " << moduleMask << " is ready" << endl;
                // read value
                if(buf[4 + (16*mod)]==0x8000) ret = -1; else Voltage_value[0 + (16*modAddr)] = (unsigned short)buf[4 + mod*16]*binHV;
                if(buf[5 + (16*mod)]==0x8000) ret = -1; else Voltage_value[1 + (16*modAddr)] = (unsigned short)buf[5 + mod*16]*bin;
                if(buf[6 + (16*mod)]==0x8000) ret = -1; else Voltage_value[2 + (16*modAddr)] = (unsigned short)buf[6 + mod*16]*bin;
                if(buf[7 + (16*mod)]==0x8000) ret = -1; else Voltage_value[3 + (16*modAddr)] = (unsigned short)buf[7 + mod*16]*bin;
                delete[] buf;
                return ret;
            }
            else {
                if (DEBUG) cout<< "LibXpadUSB : " << __func__ <<"() \t" << "Module " << "0x" << QString::number(moduleMask,16).toStdString() << " is not ready" << endl;
                delete[] buf;
                return -1;
            }
        }
    }
    return -1;
}


/**
 * @brief xpad_cmd::setHV
 *      this function set HV DAC value
 * @param moduleMask    : module mask
 * @param DAC_HV        : DAC VALUE 0 to 255<br>
 *                      CdTe DAC values:<br>
 *                      160 = -400V<br>
 *                      120 = -300V<br>
 *                      100 = -200V<br>
 *                      Si DAC value:<br>
 *                      160 = 80V
 * @return              : -1 if ERROR esle 0
 */

int xpad_cmd::setHV(unsigned short moduleMask,unsigned char DAC_HV ) {
    unsigned short *buf;
    // unsigned char DAC_HV=HV;
    unsigned short modAddr=0;
    if(globalAskReady(moduleMask)!=0) return -1;
    int ret = 0;
    int nbMod = m_tools.getNbModMask(moduleMask);
    unsigned short  MOD_setHV[16]        = { HUB_HEADER, MOD_MESSAGE,
                                             0x0007,       //nb words
                                             moduleMask,      //modules masks
                                             MOD_HEADER,
                                             0x0004,       //nb words

                                             0,            //reserved
                                             CMD_SET_HV, //0x0510,
                                             (unsigned short) DAC_HV,
                                             MSG_TRAILER,

                                             0,0,0,0,0,0};



    if (DEBUG) cout << "LibXpadUSB : " << __func__ <<"() \t"<< "Sending Set HV to " << moduleMask << endl;
    if (DEBUG) cout << "LibXpadUSB : " << __func__ <<"() \t"<< "Sending DAC HV = " << (unsigned int) DAC_HV << " to " << moduleMask << endl;

    if(m_enableConfigureTransfertSize){
        resetRegFifo();
        setDataTransfert(REG_CMD_SIZE);
    }
    buf = m_xpad_usb->sendCommand(moduleMask,MOD_setHV, sizeof(MOD_setHV),1000);
    if(buf != NULL){
        for (int mod = 0 ; mod < nbMod ; mod++){
            if (buf[0+mod*16]==ACK_HEADER
                    //  &&buf[1]==moduleMask
                    &&buf[2+mod*16]==ACK_SIZE
                    &&buf[3+mod*16]==ACK_SET_HV
                    &&buf[15+mod*16]==ACK_TRAILER) {
                if (DEBUG) cout<< "LibXpadUSB : " << __func__ <<"() \t" << "Module " << "0x" << QString::number(moduleMask,16).toStdString() << " is ready" << endl;
                modAddr = m_tools.mask2ModNumber(buf[1+mod*16]);
                if(buf[4+mod*16]==0x8000){
                    ret =  -1;
                }
            }
            else {
                if (DEBUG) cout<< "LibXpadUSB : " << __func__ <<"() \t" << "Module " << "0x" << QString::number(moduleMask,16).toStdString() << " is not ready" << endl;
                ret = -1;
            }
        }
        delete[] buf;
    }
    else
        ret = -1;

    return ret;
}

/**
 * @brief xpad_cmd::setHV_param
 *              this function is used for CdTe Detectors,<br>
 *              it set the time hvCut in µs, each hVcurt time the HV is off durring hvDeadTime.<br>
 *              by default the hvCut = 120000000 µs (2min) and hvDead = 1000000 (1s)<br>
 * @param moduleMask        : module mask
 * @param hvCutTime         : periode time for cut the HV
 * @param hvDeadTime        : time cut the HV between 2 hvCutTime
 * @return                  : -1 if ERROR else 0
 */


int xpad_cmd::setHV_param(unsigned short moduleMask,unsigned int hvCutTime, unsigned int hvDeadTime ) {
    unsigned short *buf;
    // unsigned char DAC_HV=HV;
    //  unsigned short modAddr = 0;
    if(globalAskReady(moduleMask)!=0) return -1;

    unsigned short hvCut0 = (hvCutTime&0xffff0000)>>16;
    unsigned short hvCut1 = hvCutTime&0x0000ffff;

    unsigned short hvDead0 = (hvDeadTime&0xffff0000)>>16;
    unsigned short hvDead1 = hvDeadTime&0x0000ffff;
    int ret = 0;
    int nbMod = m_tools.mask2ModNumber(moduleMask);
    unsigned short modAddr = 0;


    unsigned short  MOD_setHVParam[16]        = { HUB_HEADER, MOD_MESSAGE,
                                                  0x0007,       //nb words
                                                  moduleMask,      //modules masks
                                                  MOD_HEADER,
                                                  0x0007,       //nb words

                                                  0,            //reserved
                                                  CMD_SET_HVPARAM, //0x0510,
                                                  hvCut0,
                                                  hvCut1,
                                                  hvDead0,
                                                  hvDead1,
                                                  MSG_TRAILER,
                                                  0,0,0};



    if (DEBUG) cout << "LibXpadUSB : " << __func__ <<"() \t"<< "Sending Set HV_CUT and HV_DEAD to " << "0x" << QString::number(moduleMask,16).toStdString() << endl;
    if (DEBUG) cout << "LibXpadUSB : " << __func__ <<"() \t"<< "Sending HV_CUT = " << (unsigned int) hvCutTime << " and HV_DEAD = " << (unsigned int) hvDeadTime  << " to " << "0x" << QString::number(moduleMask,16).toStdString() << endl;

    if(m_enableConfigureTransfertSize){
        resetRegFifo();
        setDataTransfert(REG_CMD_SIZE);
    }
    buf = m_xpad_usb->sendCommand(moduleMask,MOD_setHVParam, sizeof(MOD_setHVParam),1000);

    if(buf != NULL){
        for (int mod = 0 ; mod < nbMod ; mod++){
            if (buf[0+mod*16]==ACK_HEADER
                    &&buf[2+mod*16]==ACK_SIZE
                    &&buf[3+mod*16]==CMD_SET_HVPARAM
                    &&buf[15+mod*16]==ACK_TRAILER) {
                if (DEBUG) cout<< "LibXpadUSB : " << __func__ <<"() \t" << "Module " << "0x" << QString::number(moduleMask,16).toStdString() << " is ready" << endl;
                modAddr = m_tools.mask2ModNumber(buf[1+mod*16]);
                if(buf[4+mod*16]==0x8000){
                    ret =  -1;
                }
            }
            else {
                if (DEBUG) cout<< "LibXpadUSB : " << __func__ <<"() \t" << "Module " << "0x" << QString::number(moduleMask,16).toStdString() << " is not ready" << endl;
                ret = -1;
            }
        }
        delete[] buf;
    }
    else
        ret = -1;

    return ret;


}

/**
 * @brief xpad_cmd::readOneImage<br>
 *          This function take one image or dacl with the detector
 * @param moduleMask : module mask
 * @param cmd        : command expose or read config
 * @param ImgType    : image type 2Bytes or 4Bytes
 * @param rawBuffer  : raw data from detector
 * @param funcCall   : string used for print
 * @return           : -1 if ERROR else 0
 */


int xpad_cmd:: readOneImage(unsigned short moduleMask,unsigned short *cmd,unsigned int ImgType,unsigned char *rawBuffer, const char *funcCall)
{
    if(globalAskReady(moduleMask)!=0) return -1;

    if(m_enableConfigureTransfertSize){
        resetRegFifo();
        if(ImgType == 0){
            setDataTransfert(REG_IMG_2B);
        }
        else{
            setDataTransfert(REG_IMG_4B);
        }
    }

    if(m_xpad_usb->readImgInit(cmd)!=0){
        if(DEBUG) cout << "LibXpadUSB : " << __func__ <<"()\tin " << funcCall << "()\t" << "ERROR => readImgInit" << endl;
        return -1;
    }
    if(m_xpad_usb->readImgBuff(ImgType,rawBuffer,moduleMask)!=0)
    {
        if(DEBUG) cout << "LibXpadUSB : " << __func__ <<"()\tin " << funcCall << "()\t" << "ERROR => readImgBuff" << endl;
        return -1;
    }
    if(m_xpad_usb->readImgClose()!=0){
        if(DEBUG) cout << "LibXpadUSB : " << __func__ <<"()\tin " << funcCall << "()\t"<< "ERROR => readImgClose" << endl;
        return -1;
    }
    return 0;
}

/**
 * @brief xpad_cmd::getOneImage<br>
 *              this function take one image with the detector
 * @param moduleMask    : module mask
 * @return              : return 4Bytes buffer with correct data. IMPORTANT DELETE THE OUTPUT BUFFER
 */

unsigned int* xpad_cmd::getOneImage(unsigned short moduleMask)
{
    unsigned int *buffer;
    unsigned char *RawBuffer;
    unsigned short *buf16;
    if(globalAskReady(moduleMask)!=0) return NULL;
    int NbMod = m_tools.getNbModMask(moduleMask);
    int imageSize;
    unsigned short  MOD_expose[16]          = { HUB_HEADER, MOD_MESSAGE,
                                                0x0009,         //nb words
                                                moduleMask,        // modules masks
                                                MOD_HEADER,
                                                0x0006,         //nb words

                                                0,              // reserved
                                                CMD_EXPOSE,     // 0x0140
                                                0,              // gate mode
                                                0,              // gate length
                                                0,              // time unit
                                                MSG_TRAILER,

                                                0, 0, 0, 0};
    if(m_enableConfigureTransfertSize){
        resetRegFifo();
        if(m_image_type == 0){
            setDataTransfert(REG_IMG_2B);
        }
        else{
            setDataTransfert(REG_IMG_4B);
        }

    }
    if(m_image_type==0){
        imageSize = 120*566*NbMod;
    }
    else{
        imageSize = 120*1126*NbMod;
    }

    RawBuffer = new unsigned char[imageSize*2+2];
    if(RawBuffer == NULL){
        if(DEBUG) cout << "LibXpadUSB : " << __func__ <<"()\t" << "ERROR => Can not create temp buffer" << endl;
        return NULL;
    }
    buf16 = new unsigned short [imageSize];
    if(buf16 == NULL){
        if(DEBUG) cout << "LibXpadUSB : " << __func__ <<"()\t" << "ERROR => Can not create temp buffer" << endl;
        delete[] RawBuffer;
        return NULL;
    }

    if(readOneImage(moduleMask,MOD_expose,m_image_type,RawBuffer,__func__)!=0){
        if(DEBUG) cout << "LibXpadUSB : " << __func__ <<"()\t" << "ERROR => ReadOneImage" << endl;
        delete [] RawBuffer;
        delete [] buf16;
        return NULL;
    }

    if(m_tools.rawBuffer2Buffer(moduleMask,m_image_type,buf16,RawBuffer,__func__)!=0){
        if(DEBUG) cout << "LibXpadUSB : " << __func__ <<"()\t" << "ERROR => RawBuffer2Buffer" << endl;
        delete [] RawBuffer;
        delete [] buf16;
        return NULL;
    }

    delete [] RawBuffer;

    buffer = new unsigned int [120*560*NbMod];
    if(buffer == NULL){
        if(DEBUG) cout << "LibXpadUSB : " << __func__ <<"()\t" << "ERROR => Can not create output buffer" << endl;
        delete[] buf16;
        return NULL;
    }
    if(m_image_type==0)
    {
        for(int row=0;row<120*NbMod;row++)
            for(int col=0;col<560;col++)
                buffer[row*560+col] = buf16[row*560+col];
    }
    else
    {
        for(int row=0;row<120*NbMod;row++)
            for(int col=0;col<560;col++)
                buffer[row*560+col] = (buf16[row*1120+col*2+1] << 16) | (buf16[row*1120+col*2]);
    }
    delete [] buf16;
    return buffer;
}


/**
 * @brief xpad_cmd::loadAllConfigG<br>
 *          this function load all global config to all XPAD chip selected
 * @param moduleMask    : module mask
 * @param amptpVal
 * @param imfpVal
 * @param iotaVal
 * @param ipreVal
 * @param ithlVal
 * @param ituneVal
 * @param iBuffer
 * @return              : -1 if ERROR else 0
 */
int xpad_cmd::loadAllConfigG(unsigned short moduleMask,
                             unsigned amptpVal,
                             unsigned imfpVal,
                             unsigned iotaVal,
                             unsigned ipreVal,
                             unsigned ithlVal,
                             unsigned ituneVal,
                             unsigned iBuffer
                             ){
    unsigned short reg[7];
    unsigned short regVal[7];
    if(globalAskReady(moduleMask)!=0) return -1;
    if(m_enableConfigureTransfertSize){
        resetRegFifo();
        setDataTransfert(REG_CMD_SIZE);
    }
    for (int i=0; i<7; i++){
        switch(i){
        case 0: reg[i]=AMPTP; regVal[i]=amptpVal;break;
        case 1: reg[i]=IMFP; regVal[i]=imfpVal;break;
        case 2: reg[i]=IOTA; regVal[i]=iotaVal;break;
        case 3: reg[i]=IPRE; regVal[i]=ipreVal;break;
        case 4: reg[i]=ITHL; regVal[i]=ithlVal;break;
        case 5: reg[i]=ITUNE; regVal[i]=ituneVal;break;
        case 6: reg[i]=IBUFF; regVal[i]=iBuffer;
        }

        //Global Configuration values are send to detector;
        unsigned short *ret=loadConfigG(moduleMask,reg[i],regVal[i],0x7F);
        if (ret == NULL){
            if (DEBUG) cout << "LibXpadUSB : " << __func__ <<"() \t"<< "ERROR: ConfigG data could not be loaded to module:" << "0x" << QString::number(moduleMask,16).toStdString() << " register: " << reg << endl;
            return -1;
        }
        delete[] ret;
    }
    return 0;
}


/**
 * @brief xpad_cmd::readImgFromDisk
 *          this function read raw data from disk and put in the raw buffer
 * @param modMask       : module mask
 * @param type          : type of images 2Bytes or 4Bytes
 * @param buff          : buuffer containe the image
 * @param imageNumber   : image number
 * @param burstNumber   : burst numbers
 * @return              : - 1 if ERROR else 0
 */
int xpad_cmd:: readImgFromDisk(unsigned short modMask,int type,unsigned short *buff,int imageNumber, int burstNumber){

    FILE *fd=NULL;
    char fileName[200];
    int imgSize =0;
    unsigned char *buf_tmp_raw=NULL;
    int NbMod =  m_tools.getNbModMask(modMask);
    int sizeLine=0;

    if(buff == NULL){
        cout << "LibXpadUSB : " << "input buffer buff = NULL" << endl;
        return -1;
    }

    if(type==0){
        imgSize = 120*566*2*NbMod;
        sizeLine = 566;
    }
    else
    {
        imgSize = 120*1126*2*NbMod;
        sizeLine = 1126;
    }

    buf_tmp_raw = new unsigned char[imgSize+2];
    if(buf_tmp_raw == NULL){
        cout << "LibXpadUSB : " << __func__ <<"() \t"<< "Can not create tmp buffer" << endl;
        return -1;
    }

    sprintf(fileName,RAWFILE,burstNumber,imageNumber);
    fd=fopen(fileName,"r+");
    if(fd == NULL)
    {
        cout << "LibXpadUSB : " << __func__ <<"() \t"<< "Can not open burst = " << burstNumber << " and imageNumber = " << imageNumber << " file" << endl;
        return -1;
    }

    fread(buf_tmp_raw,imgSize+2,1,fd);
    fclose(fd);

    m_tools.rawBuffer2Buffer(modMask,type,buff,buf_tmp_raw,__func__);
    delete[] buf_tmp_raw;
    return 0;
}


/**
 * @brief xpad_cmd::exposeWriteFileAs
 *          this function take one or more images and write the raw data file in parrall
 * @param moduleMask    : module mask
 * @param burstNumber   : burst numbers
 * @return              : -1 if ERROR else 0
 */


int xpad_cmd::exposeWriteFileAs(unsigned short moduleMask,unsigned int burstNumber) {

    int imgsize=0;
    bool abort_flag = false;
    if(globalAskReady(moduleMask)!=0) return -1;
    m_xpad_usb->setAbortFlag(false);
    int  current_image = 0;
    int NbMod = m_tools.getNbModMask(moduleMask);
    unsigned char **bufRaw=NULL;
    unsigned short  MOD_expose[16]          = { HUB_HEADER, MOD_MESSAGE,
                                                0x0009,         //nb words
                                                moduleMask,        // modules masks
                                                MOD_HEADER,
                                                0x0006,         //nb words

                                                0,              // reserved
                                                CMD_EXPOSE,     // 0x0140
                                                0,              // gate mode
                                                0,              // gate length
                                                0,              // time unit
                                                MSG_TRAILER,

                                                0, 0, 0, 0};

    m_expose_busy = 1;
    cout << "LibXpadUSB :\tStart expose"  << endl;
    if (DEBUG)
        cout << "LibXpadUSB : " << __func__ <<"() \t"<< "Sending Expose to " << "0x" << QString::number(moduleMask,16).toStdString() << endl;

    if(m_enableConfigureTransfertSize){
        resetRegFifo();
        if(m_image_type == 0){
            setDataTransfert(REG_IMG_2B);
        }
        else{
            setDataTransfert(REG_IMG_4B);
        }
    }

    if(m_image_type == 0){
        imgsize = 120*566*NbMod;
    }
    else{
        imgsize = 120*1126*NbMod;
    }


    bufRaw = new unsigned char* [2];
    bufRaw[0] = new unsigned char[imgsize*2+2];
    bufRaw[1] = new unsigned char[imgsize*2+2];
    if(bufRaw == NULL)
        return -1;

    m_xpad_usb->setAbortFlag(false);
    m_current_image_number = 0;
    current_image = 0;

    WriteExposeFile *write_file = new WriteExposeFile(m_module_mask,m_image_numbers,m_image_type,&abort_flag,burstNumber,&m_current_image_number,&current_image,bufRaw);
    write_file->start();

    if(m_xpad_usb->readImgInit(MOD_expose) != 0){
        cout << "LibXpadUSB : " << __func__ <<"() /opt/imXPAD/tmp/burst_%d_img_%d.bint"<< "ERROR => readImgInit(MOD_expose)" << endl;
        return -1;
    }

    for(unsigned int i = 0;i<m_image_numbers;i++)
    {
        if(DEBUG)
            cout << "LibXpadUSB : " << __func__ <<"() \t"<< "Image Number = " << i << endl;

        if(m_xpad_usb->readImgBuff(m_image_type,bufRaw[i%2],moduleMask)!=0){
            cout << "LibXpadUSB : " << __func__ <<"() \t"<< "ERROR => ReadImgBuff(" << i <<")" << endl;
        }

        // m_current_image_number++;
        current_image++;
        abort_flag = m_xpad_usb->getAbortFlag();
        if(abort_flag) break;
    }

    if(m_xpad_usb->readImgClose()!=0){
        cout << "LibXpadUSB : " << __func__ <<"() \t"<< "ERROR => readImgClose" << endl;
    }


    write_file->wait();
    delete [] bufRaw[0];
    delete [] bufRaw[1];
    delete[] bufRaw;
    cout << "LibXpadUSB : " << __func__ <<"() \t"<< "Expose finish !!!!" << endl;

    globalAskReady(moduleMask);
    m_xpad_usb->setAbortFlag(false);
    m_expose_busy = 0;

    return 0;
}


/**
 * @brief xpad_cmd::getAbortFlag
 *          this function get une ABORT flag
 * @return  : abort flag
 */

bool xpad_cmd::getAbortFlag(void)
{
    return m_xpad_usb->getAbortFlag();
}


