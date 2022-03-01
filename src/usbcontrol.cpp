#include "usbcontrol.h"
#include "defines.h"
#include <QThread>

using namespace std;

USBControl::USBControl()
{
    m_abort_flag = false;
    setAbortFlagUsbRead(false);
    setAbortFlagUsbWrite(false);
}

USBControl::~USBControl(){
//quick_exit(0);

#ifdef __linux__
    quick_exit(0);
#endif

}

/**
 * @brief USBControl::setAbortFlag
 * @param val       : set Abort flag for all functions
 */
void USBControl::setAbortFlag(bool val)
{
    m_abort_flag = val;
}

/**
 * @brief USBControl::getAbortFlag
 * @return : abort flag
 */
bool USBControl::getAbortFlag(void)
{
    return m_abort_flag;
}

/**
 * @brief USBControl::getUSBDeviceList
 * @return : get all Quick usb connected on the PC
 */

char * USBControl::getUSBDeviceList() {
    int idx = 0;
    char message[300];
    char *message_final = new char[900];

    for(int i =0;i<900;i++)
        message_final[i] = 0;

    for(int i =0;i<1024;i++)
        nameList[i] = 0;

    // strcpy(message_final,"");
    if (!QuickUsbFindModules(nameList, 1024))
        if (DEBUG && QuickUsbGetLastError(&lastError)) {
            cout << "Error find: " << lastError << endl;
            return NULL;
        }
    name = nameList;
    do {
        //cout << idx << " : " << name << endl;
        //cout << idx << " : nameList :" << nameList << endl;
        sprintf(message, "%d : %s \t",idx, name);
        strcat(message_final,message);
        name += strlen(name) + 1;
        idx++;
    } while (*name != 0);

    return message_final;
}


/**
 * @brief USBControl::setUSBDevice
 * @param i : index of usb device
 * @return  : -1 if error else 0
 */

int USBControl::setUSBDevice(int i) {
    name = nameList;
    for (int j=0; j<i; j++)
        name += strlen(name) + 1;
    cout << name << endl;
    if (!strncmp(name,"QUSB",4)){
        cout << "Setting active module to : " << name << endl;
        return 0;
    }
    else{
        cout << "ERROR: active module " << i << " not found" << endl;
        return -1;
    }
}

/**
 * @brief USBControl::setDebugMode
 * @param value     : true debug mode activate
 */

void USBControl::setDebugMode(bool value)
{
    DEBUG = value;
}

/**
 * @brief USBControl::getNbModMask
 * @param Mask      : module mask
 * @return          : numbers of module activate
 */

int USBControl::getNbModMask(unsigned short Mask)
{
    int number=0;

    for(int i=0;i<5;i++)
    {
        if((Mask>>i) & 0x01) number++;
    }
    return number;
}

/**
 * @brief USBControl::mask2ModNumber
 * @param Mask      : module mask
 * @return          : return the first module connected
 */
int USBControl::mask2ModNumber(unsigned short Mask)
{
    for(int i=0;i<5;i++) {
        if((Mask>>i) & 0x01)
            return i;
    }
    return -1;
}

/**
 * @brief USBControl::sendSpecialCommand
 *                    : Send command without response
 * @param cmd         :command buffer
 * @param size        : size of cmd in byte
 * @return
 */
int USBControl::sendSpecialCommand(unsigned short *cmd,int size)
{
    unsigned char       c_buf[2];
    unsigned short      c_buf_len = 2;
    unsigned char *cmd_buf = new unsigned char[size];

    setAbortFlagUsbWrite(true);
    while(getAbortFlagUsbRead()>1) QThread::usleep(1);
    if(DEBUG){
        cout << "LibXpadUSB : " << __func__ <<"() \t"<< "USB to Detector =>>>" << endl;
    }
    for (int i=0;i<size/2;i++) {
        cmd_buf[2*i] = cmd[i]&0x00ff;
        cmd_buf[2*i+1] = (cmd[i]&0xff00)>>8;
        if(DEBUG){
            cout << hex << setw(2) << (int)cmd_buf[2*i+1] << setw(2) << (int)cmd_buf[2*i] << " " << dec;
        }
    }
    if(DEBUG){
        cout << endl;
    }

    //open the device
    if(getAbortFlagUsbRead()==0){
        if(!QuickUsbOpen(&hDevice, name)) {
            ok = QuickUsbGetLastError(&lastError);
            cout << "Error open: " << lastError << endl;
            setAbortFlagUsbWrite(false);
            return NULL;
        }
    }

    //    //Clear tx fifo
    //    c_buf[0] = VAL_CLR_TX; c_buf[1]=0;
    //    c_buf_len = 2;
    //    if(!QuickUsbWriteCommand(hDevice,  REG_CLR_FIFO_TX, c_buf, c_buf_len )){
    //        cout << "cannot clear tx register" << endl;
    //    }

    //set the tx address
    if(!QuickUsbWriteSetting(hDevice, SETTING_DATAADDRESS,REG_FIFO_TX)){
        cout<<"addressing error"<<endl;
        setAbortFlagUsbWrite(false);
        return -1;
    }

    if(!QuickUsbWriteData(hDevice, cmd_buf, size )){
        cout<<"cannot write data to buffer"<<endl;
        setAbortFlagUsbWrite(false);
        return -1;
    }
    //set tlk register
    c_buf[0] = VAL_SND_TLK; c_buf[1]=0;
    c_buf_len = 2;

    if(!QuickUsbWriteCommand(hDevice,  REG_SND_TLK, c_buf, c_buf_len )){
        cout<<"cannot redirect to tlk"<<endl;
        setAbortFlagUsbWrite(false);
        return -1;
    }


    //close device
    if(getAbortFlagUsbRead()==0){
        if(!QuickUsbClose(hDevice)) {
            ok = QuickUsbGetLastError(&lastError);
            cout << "Error close: " << lastError << endl;
        }
    }
    setAbortFlagUsbWrite(false);
   // fflush(stdout);
}


/**
 * @brief USBControl::sendCommand
 *                  : Send standart commande
 * @param modMask   : module mask
 * @param cmd       : data buffer
 * @param size      : sizeof cmd
 * @param timeout   : time before the command respond
 * @return          : the response from detector
 */
unsigned short* USBControl::sendCommand(unsigned short modMask, unsigned short *cmd,int size, unsigned long timeout)
{
    unsigned short      buf_len = size*2;
    unsigned char       c_buf[2];
    unsigned short      c_buf_len = 2;
    unsigned short      *data_buf = NULL;
    unsigned char       *tmp_buf = NULL;
    unsigned int        NbMod = this->getNbModMask(modMask);
#ifdef _WIN32
    unsigned long tmp_buf_len = 0;
#elif __linux__
    unsigned long tmp_buf_len = 0;
#else
    unsigned int tmp_buf_len = 0;
#endif

    unsigned char *cmd_buf = new unsigned char[2*(6+cmd[5])];
    unsigned short datatmp;

    if(DEBUG){
        cout << "LibXpadUSB : " << __func__ <<"() \t"<< "USB to Detector =>>>" << endl;
    }
    for (int i=0;i<6+cmd[5];i++) {
        cmd_buf[2*i] = cmd[i]&0x00ff;
        cmd_buf[2*i+1] = (cmd[i]&0xff00)>>8;
        if(DEBUG){
            datatmp = (cmd_buf[2*i+1] << 8) + cmd_buf[2*i];
            cout << "0x" << hex << datatmp << " " << dec;
        }
    }
    if(DEBUG){
        cout << endl;
    }

    data_buf = new unsigned short [size*NbMod];
    buf_len = size*2*NbMod;



    //open the device
    if(!QuickUsbOpen(&hDevice, name)) {
        ok = QuickUsbGetLastError(&lastError);
        cout << "Error open: " << lastError << endl;
        return NULL;
    }
    setAbortFlagUsbRead(1);
    //Clear tx fifo
    c_buf[0] = VAL_CLR_TX; c_buf[1]=0;
    c_buf_len = 2;
    if(!QuickUsbWriteCommand(hDevice,  REG_CLR_FIFO_TX, c_buf, c_buf_len ))
        cout << "cannot clear tx register" << endl;


    //Clear rx fifo
    c_buf[0] = VAL_CLR_RX; c_buf[1]=0;
    c_buf_len = 2;
    if(!QuickUsbWriteCommand(hDevice,  REG_CLR_FIFO_RX, c_buf, c_buf_len ))
        cout << "cannot clear rx register" << endl;


    //set the tx address
    if(!QuickUsbWriteSetting(hDevice, SETTING_DATAADDRESS,REG_FIFO_TX)){
        cout<<"addressing error"<<endl;
        return NULL;
    }

    if(!QuickUsbWriteData(hDevice, cmd_buf, 2*(6+cmd[5]) )){
        cout<<"cannot write data to buffer"<<endl;
        return NULL;
    }
    //set tlk register
    c_buf[0] = VAL_SND_TLK; c_buf[1]=0;
    c_buf_len = 2;

    if(!QuickUsbWriteCommand(hDevice,  REG_SND_TLK, c_buf, c_buf_len )){
        cout<<"cannot redirect to tlk"<<endl;
        return NULL;
    }

    unsigned long time=0;

    while ((QuickUsbReadCommand(hDevice,  REG_RX_COUNT, c_buf, &c_buf_len)) && (((NbMod-1)*16+1) >= (int)(c_buf[0]|(c_buf[1]<<8)))){
        if(timeout != 0){
            if(!getAbortFlagUsbWrite()){
                QThread :: msleep(1);
                time++;
                if(time > timeout){
                    //close device
                    if(!QuickUsbClose(hDevice)) {
                        ok = QuickUsbGetLastError(&lastError);
                        cout << "Error close: " << lastError << endl;
                    }
                    setAbortFlagUsbRead(0);
                    //cout << "LibXpadUSB : " << __func__ <<"() \t"<< "ERROR => TimeOUT USB to Detector =>>>" << endl;
                    return NULL;
                }
            }
            else{
                setAbortFlagUsbRead(1);
                QThread::usleep(1);
            }
        }
    }

    //set register address for rx
    if(!QuickUsbWriteSetting(hDevice, SETTING_DATAADDRESS,REG_FIFO_RX)){
        cout<<"addressing error"<<endl;
        return NULL;
    }
    tmp_buf_len = buf_len + 2;

    tmp_buf = new unsigned char [tmp_buf_len];
    //read from buffer
    if(!QuickUsbReadData(hDevice,  tmp_buf, &tmp_buf_len ))
        cout << "cannot read data to buffer here" << endl;

    unsigned int i = 1;
    unsigned int j = 0;

    //convert 0xcd 0xab to 0xabcd
    while(i<(tmp_buf_len/2)) {
        data_buf[(j)] = tmp_buf[2*i]|((tmp_buf[2*i+1]) << 8);
        //  if(DEBUG)  cout << hex << setw(4) << data_buf[(buf_len/2+j)] << " " << dec;
        i++;
        j++;
    }


    if (DEBUG){
        cout << endl;
    }
    delete[] tmp_buf;

    //Clear rx fifo
    c_buf[0] = VAL_CLR_RX; c_buf[1]=0;
    c_buf_len = 2;
    if(!QuickUsbWriteCommand(hDevice,  REG_CLR_FIFO_RX, c_buf, c_buf_len ))
        cout << "cannot clear rx register" << endl;

    //close device
    if(!QuickUsbClose(hDevice)) {
        ok = QuickUsbGetLastError(&lastError);
        cout << "Error close: " << lastError << endl;
    }
    setAbortFlagUsbRead(0);
    delete[] cmd_buf;
    return data_buf;
}




/**
 * @brief USBControl::readImgInit
 *              : prepare image acquisition from detector
 * @param cmd   : data cmd buffer
 * @return
 */

int USBControl::readImgInit(unsigned short *cmd){
    unsigned char       c_buf[2];
    unsigned short      c_buf_len = 2;
    unsigned char *cmd_buf = new unsigned char[2*(6+cmd[5])];

    for (int i=0;i<6+cmd[5];i++) {
        cmd_buf[2*i] = cmd[i]&0x00ff;
        cmd_buf[2*i+1] = (cmd[i]&0xff00)>>8;
        if(DEBUG) cout << hex << setw(2) << (int)cmd_buf[2*i+1] << setw(2) << (int)cmd_buf[2*i] << " " << dec;
    }
    //open the device
    if(!QuickUsbOpen(&hDevice, name)) {
        ok = QuickUsbGetLastError(&lastError);
        cout << "Error open: " << lastError << endl;
        return -1;
    }
    setAbortFlagUsbRead(1);
    //set the tx address

    //Clear tx fifo
    c_buf[0] = VAL_CLR_TX; c_buf[1]=0;
    c_buf_len = 2;
    if(!QuickUsbWriteCommand(hDevice,  REG_CLR_FIFO_TX, c_buf, c_buf_len ))
        cout << "cannot clear tx register" << endl;

    //Clear rx fifo
    c_buf[0] = VAL_CLR_RX; c_buf[1]=0;
    c_buf_len = 2;
    if(!QuickUsbWriteCommand(hDevice,  REG_CLR_FIFO_RX, c_buf, c_buf_len ))
        cout << "cannot clear rx register" << endl;

    if(!QuickUsbWriteSetting(hDevice, SETTING_DATAADDRESS,REG_FIFO_TX)){
        cout<<"addressing error"<<endl;
        return -1;
    }


    if(!QuickUsbWriteData(hDevice, cmd_buf, 2*(6+cmd[5]) )){
        cout<<"cannot write data to buffer"<<endl;
        return -1;
    }
    //set tlk register
    c_buf[0] = VAL_SND_TLK; c_buf[1]=0;
    c_buf_len = 2;

    firstAqcUSB = true;
    if(!QuickUsbWriteCommand(hDevice,  REG_SND_TLK, c_buf, c_buf_len )){
        cout<<"cannot redirect to tlk"<<endl;
        return -1;
    }
    return 0;

}

/**
 * @brief USBControl::readImgBuff
 *              read one image from detector
 *              used after readImgInit()
 * @param type  : type of images 2bytes or 4 bytes per pixels
 * @param buf   : output buffer
 * @param modMask   : module mask
 * @return          : -1 if error else 0
 */

int USBControl::readImgBuff(unsigned short type,unsigned char *buf,unsigned short modMask)
{

    unsigned char       c_buf[2];
    unsigned short      c_buf_len = 2;
    int                 numberLoop;
    unsigned int tmp_buf_len;
    //    int size=0;
    int sizeLine=0;

#ifdef _WIN32
    unsigned long buf_len = 0;
#elif __linux__
    unsigned long buf_len = 0;
#else
    unsigned int buf_len = 0;
#endif
    if(type == 0){
        tmp_buf_len = 2*566*120/2;
        numberLoop = getNbModMask(modMask)*2;
        sizeLine = 566;
        firstAqcUSB = true;
    }
    else{
        tmp_buf_len = (2*1126*120)/4;
        numberLoop = getNbModMask(modMask)*4;
        sizeLine = 1126;
        firstAqcUSB = true;
    }

    //allocate buffer
    if (buf == NULL){
        cout << "LibXpadUSB : " << __func__ <<"() \t"<< "ERROR buf = NULL" << endl;
        return -1;
    }
    unsigned short dataFifoSize=0;
    if(DEBUG) {
        cout  << endl<< "LibXpadUSB : " << __func__ <<"() \t" << "Total transfert = "  << numberLoop << endl;
        cout  << "LibXpadUSB : " << __func__ <<"() \tData Transfert =>>" << endl << endl;
    }

    for(int loop=0;loop<numberLoop;loop++){
        setAbortFlagUsbRead(2);
        while (dataFifoSize < c_buf_len/2){
            if(!getAbortFlagUsbWrite()){
                QuickUsbReadCommand(hDevice,  REG_RX_COUNT, c_buf, &c_buf_len);
                dataFifoSize = (c_buf[1] << 8) & 0xFF00 | c_buf[0] & 0xFF;
            }
            else{
                setAbortFlagUsbRead(1);
                QThread::usleep(1);
            }
        }
        setAbortFlagUsbRead(2);
        dataFifoSize = 0;
        //set register address for rx
        if(!QuickUsbWriteSetting(hDevice, SETTING_DATAADDRESS,REG_FIFO_RX))
            cout<< "LibXpadUSB : " << __func__ <<"() \t"<<"addressing error"<<endl;
        //read from buffer

        if (firstAqcUSB && loop==0 && type == 1){
            buf_len = tmp_buf_len + 2;
        }
        else if(firstAqcUSB && loop == 0 && type == 0)
            buf_len = tmp_buf_len + 2;
        else
            buf_len = tmp_buf_len;

        if(loop == 0 && firstAqcUSB){
            if(!QuickUsbReadData(hDevice,  &buf[buf_len*loop], &buf_len ))
                cout << "LibXpadUSB : " << __func__ <<"() \t"<< "cannot read data to buffer" << endl;
        }
        else{
            //   cout << "__FRED__ firstAqcUSB and loop != 0" << endl;
            if(!QuickUsbReadData(hDevice,  &buf[buf_len*loop+2], &buf_len ))
                cout << "LibXpadUSB : " << __func__ <<"() \t"<< "cannot read data to buffer" << endl;
        }

        if(DEBUG){
            cout << "LibXpadUSB : Transfert " << loop <<"\t" << endl;
            for(int j=0;j<5;j++){
                for(int i=0;i<6;i++){
                    printf("0x%x ",buf[2+i*2+(j*sizeLine*2)+buf_len*loop] | buf[3+i*2+(j*sizeLine*2)+buf_len*loop] << 8);
                }
                cout << "\t";
                if(type == 0 ){
                    for(int i=563;i<566;i++){
                        printf("0x%x ",buf[2+i*2+(j*sizeLine*2)+buf_len*loop] | buf[3+i*2+(j*sizeLine*2)+buf_len*loop] << 8);
                    }
                }
                else{
                    for(int i=1123;i<1126;i++){
                        printf("0x%x ",buf[2+i*2+(j*sizeLine*2)+buf_len*loop] | buf[3+i*2+(j*sizeLine*2)+buf_len*loop] << 8);
                    }
                }
                cout << endl;
            }
            cout << endl;
        }

    }
    firstAqcUSB = false;
    return 0;

}

/**
 * @brief USBControl::readImgClose
 *          close the images acquisition
 *          used after all images downloaded
 * @return
 */
int USBControl::readImgClose()
{
    unsigned char       c_buf[2];
    unsigned short      c_buf_len = 2;
    //Clear rx fifo
    c_buf[0] = VAL_CLR_RX; c_buf[1]=0;
    c_buf_len = 2;
    if(!QuickUsbWriteCommand(hDevice,  REG_CLR_FIFO_RX, c_buf, c_buf_len ))
        if(DEBUG) cout<< "LibXpadUSB : " << __func__ <<"() \t" << "cannot clear rx register" << endl;

    //close device
    if(!QuickUsbClose(hDevice)) {
        ok = QuickUsbGetLastError(&lastError);
        if(DEBUG) cout<< "LibXpadUSB : " << __func__ <<"() \t" << "Error close: " << lastError << endl;
    }
    setAbortFlagUsbRead(0);
    return 0;
}


/**
 * @brief USBControl::readOneImage
 *              read one image from detector for example DACl config
 * @param cmd       : cmd data buffer
 * @param ImgType   : type of image 2 bytes or 4 bytes
 * @param rawBuffer : raw buffer from detector
 * @param funcCall  : name of function call this
 * @param moduleMask    : module mask
 * @return              : -1 if error else 0
 */

int USBControl::readOneImage(unsigned short *cmd,unsigned int ImgType,unsigned char *rawBuffer, const char *funcCall,unsigned short moduleMask)
{

    if(readImgInit(cmd)!=0){
        if(DEBUG) cout << "LibXpadUSB : " << __func__ <<"()\tin " << funcCall << "()\t" << "ERROR => readImgInit" << endl;
        return -1;
    }
    if(readImgBuff(ImgType,rawBuffer,moduleMask)!=0)
    {
        if(DEBUG) cout << "LibXpadUSB : " << __func__ <<"()\tin " << funcCall << "()\t" << "ERROR => readImgBuff" << endl;
        return -1;
    }
    if(readImgClose()!=0){
        if(DEBUG) cout << "LibXpadUSB : " << __func__ <<"()\tin " << funcCall << "()\t"<< "ERROR => readImgClose" << endl;
        return -1;
    }
    return 0;
}

/**
 * @brief USBControl::getOneImage
 *                  take one image with the detector
 * @param imgType   : type of data output 2 bytes or 4 bytes
 * @param moduleMask    : module mask
 * @return          : -1 if error else 0
 */

unsigned int* USBControl::getOneImage(int imgType,unsigned short moduleMask)
{
    unsigned int *buffer;
    unsigned char *RawBuffer;
    unsigned short *buf16;
    int NbMod = getNbModMask(moduleMask);
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

    if(imgType==0)
        imageSize = 120*566*NbMod;
    else
        imageSize = 120*1126*NbMod;


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

    if(readOneImage(MOD_expose,imgType,RawBuffer,__func__,moduleMask)!=0){
        if(DEBUG) cout << "LibXpadUSB : " << __func__ <<"()\t" << "ERROR => ReadOneImage" << endl;
        delete [] RawBuffer;
        delete [] buf16;
        return NULL;
    }
    if(m_tools.rawBuffer2Buffer(moduleMask,imgType,buf16,RawBuffer,__func__)!=0){
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
    if(imgType==0)
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
    return buffer;
}
