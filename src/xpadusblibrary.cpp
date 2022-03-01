#include "xpadusblibrary.h"




XpadUSBLibrary::XpadUSBLibrary()
{
    m_usb   = new USBControl;
    command   = new xpad_cmd(m_usb);
    calibration = new Calibration(command);
    tools = new Tools;
    this->setLibStatus(0);

    cout << "XpadUSBLibrary version 3.0.11" << endl;
}

/**
 * @brief XpadUSBLibrary::getXpadUsbDetector
 *      the librairy create one string buffer, can not forget free buffer after using
 * @return : list of XPAD USB Connecxion else NULL
 */
char * XpadUSBLibrary::getXpadUsbDetector(void){
    return m_usb->getUSBDeviceList();
}
/**
 * @brief XpadUSBLibrary::init
 *          this function init the XPAD communication
 * @param USBdevice : Value of XPAD USB connexion
 * @return  : -1 if ERROR else 0
 */
int XpadUSBLibrary::init(int USBdevice){
    m_usb->getUSBDeviceList();
    return m_usb->setUSBDevice(USBdevice);
}
/**
 * @brief XpadUSBLibrary::init
 * this function init the XPAD communication
 * @param USBdevice         :   Value of XPAD USB connexion
 * @param detectorType      : type of detector
 * @return                  : -1 if ERROR else 0
 */
int XpadUSBLibrary::init(int USBdevice,unsigned short detectorType){
    m_usb->getUSBDeviceList();
    setDetectorModel(detectorType);
    return m_usb->setUSBDevice(USBdevice);

}
/**
 * @brief XpadUSBLibrary::getAbortStatus
 * @return : Abort flag
 */
bool XpadUSBLibrary::getAbortStatus(void){
    return  m_usb->getAbortFlag();
}
/**
 * @brief XpadUSBLibrary::getFirmwareId
 * @return  : the firmware id 0x10092010 for 10/09/2010
 */
unsigned long XpadUSBLibrary::getFirmwareId(void){
    return command->getFirmwareId();
}
/**
 * @brief XpadUSBLibrary::getImageType
 * @return : type of images using for transfert 2Bytes or 4Bytes;
 */
unsigned int XpadUSBLibrary::getImageType(void){
    return command->getImageType();
}
/**
 * @brief XpadUSBLibrary::getLibStatus
 * @return  : flag detector busy
 *            value BUSY , 0 FREE
 */
int XpadUSBLibrary::getLibStatus(void){
    return m_Libstatus;
}
/**
 * @brief XpadUSBLibrary::setLibStatus
 * @param status    : set lib busy value
 */
void XpadUSBLibrary:: setLibStatus(int status){
    m_Libstatus = status;
}
/**
 * @brief XpadUSBLibrary::startDebugMode
 *  this function set verbose library
 */
void XpadUSBLibrary::startDebugMode(){
    m_usb->setDebugMode(true);
    command->setDebugMode(true);
    calibration->setDebugMode(true);
    DEBUG = true;
}
/**
 * @brief XpadUSBLibrary::stopDebugMode
 *  this function quiet library
 */
void XpadUSBLibrary::stopDebugMode(){
    m_usb->setDebugMode(false);
    command->setDebugMode(false);
    calibration->setDebugMode(false);
    DEBUG = false;
}
/**
 * @brief XpadUSBLibrary::setDebugMode
 * @param val   : 1 verbose mode 0 quiet
 */
void XpadUSBLibrary::setDebugMode(bool val){
    m_usb->setDebugMode(val);
    command->setDebugMode(val);
    calibration->setDebugMode(val);
    DEBUG = val;
}

/**
 * @brief XpadUSBLibrary::getImageCounter
 * @return : the value of last aqcuire images durring the Aqcisition
 */

int XpadUSBLibrary::getImageCounter(void)
{
    return command->getCurrentImageNumber();
}

/**
 * @brief XpadUSBLibrary::exposeAsync
 *      this function launch expose function in Asynchrous mode
 * @param moduleMask    : module mask
 * @param burstNumber   : burst number
 * @return
 */

int XpadUSBLibrary::exposeAsync(unsigned short moduleMask, unsigned int burstNumber)
{
    command->clearCurrentImageNumber();
    asynchronous *async = new asynchronous(moduleMask,command,burstNumber);
    async->start();
    return 0;

}

/**
 * @brief XpadUSBLibrary::getModuleMask
 * @return  : return the current module mask
 */

unsigned short XpadUSBLibrary::getModuleMask(void){
    return command->getCurrentMask();
}
/**
 * @brief XpadUSBLibrary::getModuleNumber
 * @return  : the numbers of connected modules
 */
unsigned short XpadUSBLibrary::getModuleNumber(void){
    return m_module_number;
}

/**
 * @brief XpadUSBLibrary::getChipNumber
 * @return  : numbers of chip per module
 */
unsigned short XpadUSBLibrary::getChipNumber(void){
    return m_chip_number;
}
/**
 * @brief XpadUSBLibrary::getChipMask
 * @return  : return the chip mask
 */
unsigned int XpadUSBLibrary::getChipMask(void){
    return m_chip_mask;
}

// return true if the detector aquiere the images
/**
 * @brief XpadUSBLibrary::getFlagExposeBusy
 * @return  : 1 if the detector Aqcuire Xray else 0
 */
bool XpadUSBLibrary :: getFlagExposeBusy(void){
    return command->getExposeBusy();
}

/**
 * @brief XpadUSBLibrary::setDetectorModel
 * @param detectorModel : detector type ex: XPD_S70 or XPAD_S140
 * @return  : -1 if ERROR else 0
 */
int XpadUSBLibrary::setDetectorModel(unsigned short detectorModel){

    int ret;

    switch (detectorModel){
    case XPAD_S10:
    case XPAD_C10:
    case XPAD_A10:
    case XPAD_S70:
    case XPAD_C70:
        m_chip_mask = 0x7F;
        m_chip_number = 7;
        m_module_number = 1;
        ret = 0;
        break;
    case XPAD_S140:        
    case XPAD_C140:
        m_chip_mask = 0x7F;
        m_chip_number = 7;
        m_module_number = 2;
        ret = 0;
        break;
    case XPAD_S210:
        m_chip_mask = 0x7F;
        m_chip_number = 7;
        m_module_number = 3;
        ret = 0;
        break;


    case XPAD_S270:
    case XPAD_C270:
        m_chip_mask = 0x7F;
        m_chip_number = 7;
        m_module_number = 4;
        ret = 0;
        break;
    case XPAD_S340:
        m_chip_mask = 0x7F;
        m_chip_number = 7;
        m_module_number = 5;
        ret = 0;
        break;


    default:
        cout << "LibXpadUSB : " << __func__ << "ERROR ==>> Detector Model" << endl;
        m_module_number = 1;
        ret = -1;
        break;
    }

    if(ret == 0)
        command->setModuleNumber(m_module_number);

    return command->setDetectorType(detectorModel);

}
/**
 * @brief XpadUSBLibrary::getDetectorType
 * @return  : the type of detector selected
 */
unsigned int XpadUSBLibrary:: getDetectorType(void){
    return command->getDetectorType();
}



//*********************************COMPOSITE FUNCTIONS**************************************
/**
 * @brief XpadUSBLibrary::loadConfigLFromFileToSRAM
 *      This Function read the ConfigL values from file, and send them to the SRAM of the detector.
 *      The user receives a buffer containing the values readed from the file.
 * @param moduleMask    : module mask
 * @param fpath         : Local config file name
 * @return              : -1 if ERROR else 0
 */
unsigned int XpadUSBLibrary::loadConfigLFromFileToSRAM(unsigned short moduleMask,char *fpath){
    int NbMod = tools->getNbModMask(moduleMask);
    ifstream file(fpath, ios::in);
    //Opening the file to read
    if (file.is_open()){
        unsigned short n = 0;
        unsigned short values[80];

        int i=0;
        int ret ;

        //Reading values from columns for each row. The chip number is estimated depending on the column number
        int transfert = 0;
        for(int mod=0;mod<NbMod;mod++){
            transfert = 0;
            for (unsigned int row=0; row<LineNumber; row++) {
                for (unsigned int column=0; column<ColumnNumber; column++){
                    file >> n;
                    if(((1<<mod) & moduleMask) == 0 ) continue; // test dans le cas ou un module est manquant
                    unsigned short chip = column/80;
                    if(this->getAbortStatus()){
                        command->m_xpad_usb->setAbortFlag(false);
                        return 1;
                    }
                    //Values are being accumulated in packages of 80 data
                    values[column - chip*80] = n;

                    //Once 80 values accumulated, they are sended to the SRAM of the detector
                    if ((column - chip*80) == 79){
                        transfert++;
                        cout << "Loading module " << mod << " line "   << transfert << " of 840\r" << flush;
                       // fflush(stdout);
                        //Sending 80 values to the SRAM in the detector
                        ret = command->saveConfigLToSRAM(1<<mod,chip,row,values);

                        //Checking if the 80 values where stored correctly
                        if (ret == -1){
                            if (DEBUG) cout << "ERROR: SaveConfigLToSRAM data could not be send to module mask: 0x" << (1<<mod) << " chip:" << chip << " row: " << row << endl;
                            file.close();
                            return -1;
                        }
                    }
                    i++;
                }
            }
            cout << endl;
        }
        cout << endl;
        file.close();

        return command->loadConfigLSRAMToDetector(moduleMask); // load config L from SRAM to the detector

    }
    else {
        if (DEBUG) cout<< "LibXpadUSB : " << __func__ <<"() \t" << "ERROR: " << fpath << " - File not founded" << endl;
        return -1;
    }
}



/**
 * @brief XpadUSBLibrary::loadConfigGFromFile
 *  function to read a configg file and upload values to the detector
 * @param moduleMask    : module mask
 * @param fpath         : Global config file name
 * @return              : -1 if ERROR else 0
 */
int XpadUSBLibrary::loadConfigGFromFile(unsigned short moduleMask,char *fpath){

    //chipMask is being fixed to 7 Chips (This must be changed for different Model of detector)

    unsigned short mod = 0;
    unsigned short chip = 0;
    unsigned short reg = 0;
    unsigned short chipMask= getChipMask();
    unsigned short chipNumber = 7;

#ifdef _WIN32
    unsigned short *regVal = new unsigned short[chipNumber];
#else
    unsigned short regVal[chipNumber];
#endif

    unsigned short moduleMask_original = moduleMask;
    unsigned short chipMask_original = chipMask;


    ifstream file(fpath, ios::in);
    if (DEBUG) cout << fpath << endl;
    //Opening File to read
    if (file.is_open()){
        while(!file.eof()){
            file >> moduleMask;
            if (DEBUG) cout << moduleMask << " ";
            file >> reg;
            if (DEBUG) cout << reg << " ";
            for (int i=0; i<chipNumber ; i++){
                file >> regVal[i];
                if (DEBUG) cout << regVal[i] << " ";
                if(this->getAbortStatus()){
                    command->m_xpad_usb->setAbortFlag(false);
                    return 1;
                }
            }
            if (DEBUG) cout << endl;

            for(chip=0;chip<chipNumber;chip++){
                chipMask = 0x01<<chip;

                if(this->getAbortStatus()){
                    command->m_xpad_usb->setAbortFlag(false);
                    return 1;
                }
                cout << ".";
                //Global Configuration values are send to detector;
                unsigned short *ret=command->loadConfigG(moduleMask,reg,regVal[chip],chipMask);
                if (ret == NULL){
                    if (DEBUG) cout<< "LibXpadUSB : " << __func__ <<"() \t" << "ERROR: ConfigG data could not be loaded to module:" << mod << " register: " << reg << endl;
                    file.close();
                    return -1;
                }
                delete[] ret;
            }
            cout << endl;
        }
        file.close();
        moduleMask = moduleMask_original;
        chipMask = chipMask_original;
        return 0;
    }
    else {
        if (DEBUG) cout<< "LibXpadUSB : " << __func__ <<"() \t"<< "ERROR: " << fpath << " - File not founded" << endl;
        return -1;
    }
}


/**
 * @brief XpadUSBLibrary::saveConfigGToFile
 *      this function read all Global config and write calibration file
 * @param moduleMask    : module mask
 * @param fpath         : path for output file
 * @return              : -1 if ERROR else 0
 */
int XpadUSBLibrary::saveConfigGToFile(unsigned short moduleMask,char *fpath){

    unsigned short regid;

    //File is being open to be writen
    ofstream file(fpath, ios::out);
    if (file.is_open()){

        //All registers are being read
        for (unsigned short counter=0; counter<7; counter++){
            switch(counter){
            case 0: regid=AMPTP;break;
            case 1: regid=IMFP;break;
            case 2: regid=IOTA;break;
            case 3: regid=IPRE;break;
            case 4: regid=ITHL;break;
            case 5: regid=ITUNE;break;
            case 6: regid=IBUFF;
            }

            //Each register value is read from the detector
            unsigned short *regVal=command->readConfigG(moduleMask,regid);
            if (regVal == NULL){
                if (DEBUG) cout << "ERROR: ConfigG could not be readed from module :" << moduleMask <<  " register: " << regid << endl;
                file.close();
                return -1;
            }

            //Register values are being stored in the file
            file << moduleMask << " ";
            for (int i=0; i<8 ; i++)
                file << regVal[i] << " ";
            file << endl;
            delete[] (regVal);
        }
        file.close();
        return 0;
    }
    else
        if (DEBUG) cout<< "LibXpadUSB : " << __func__ <<"() \t" << "ERROR: " << fpath << " - File could not be opened" << endl;
    return -1;
}



XpadUSBLibrary::~XpadUSBLibrary()
{
    delete calibration;
    delete command;
    delete m_usb;
    delete tools;
}

