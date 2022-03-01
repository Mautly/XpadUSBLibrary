#ifndef XPADCOMMANDS_H
#define XPADCOMMANDS_H

#include "defines.h"
#include "usbcontrol.h"
#include "tools.h"
#include "writeexposefile.h"


using namespace std;

class xpad_cmd
{
public:
    xpad_cmd();
    xpad_cmd(USBControl *USBdevice);

     USBControl *m_xpad_usb;
     Tools m_tools;

     void           setDebugMode(bool val);
     void           setUSB(USBControl *usbDevice);

     int            getCurrentImageNumber(void);
     void           clearCurrentImageNumber(void);
     unsigned short getCurrentMask(void);
     bool           getExposeBusy(void);
     bool           getAbortFlag(void);
     unsigned long  getFirmwareId(void);
     void           setModuleNumber(int value);


    // *********************************************
    //                Detector CMD
    // *********************************************
    int             askReady();
    int             digitalTest(unsigned short moduleMask,unsigned short value, unsigned short mode);
    int             resetModules() ;
    unsigned short* testPulse(unsigned short moduleMask,unsigned short nbpulse);
    unsigned int*   diagMemory(unsigned short moduleMask,unsigned short type, unsigned short value);
    unsigned int*   diagMemoryFileUpload(unsigned short moduleMask, unsigned short type, char* fpath);
    unsigned short* loadConfigG(unsigned short moduleMask,unsigned short regid, unsigned short value, unsigned short chipMask);
    unsigned short* readConfigG(unsigned short moduleMask,unsigned short regid,unsigned short chipMask=0x7f);
    float*          readTempSensor(unsigned short moduleMask=1);
    int             loadConfigLSRAMToDetector(unsigned short moduleMask);
    int             loadFlatConfigL(unsigned short moduleMask,unsigned short value,unsigned short chipMask=0x7F);
    int             saveConfigLToSRAM(unsigned short moduleMask,unsigned short chip, unsigned short row, unsigned short* value);
    unsigned short* readConfigL(unsigned short moduleMask);
    int             loadAllConfigG(unsigned short moduleMask, unsigned amptpVal, unsigned imfpVal,
                                   unsigned iotaVal,unsigned ipreVal,unsigned ithlVal,unsigned ituneVal,
                                   unsigned iBuffer);
    int             exposeParam(unsigned short moduleMask ,unsigned int numImages, unsigned int ExposureTime,
                                unsigned int overFlowTime, unsigned short TriggerMode,
                                unsigned short BusySrc,unsigned int waitingTime,
                                unsigned short AcqMode,unsigned int StakingOrBunchNumber);
    int             expose(unsigned short moduleMask,unsigned int burstNumber);
    unsigned int *  readImage(unsigned short moduleMask);
    unsigned short* readImage2B(unsigned short moduleMask);
    unsigned int*   readImage4B(unsigned short moduleMask);
    unsigned int*   getOneImage(void);
    int             abortExposure();
    int             readOneImage(unsigned short moduleMask,unsigned short *cmd,unsigned int ImgType,unsigned char *rawBuffer, const char *funcCall);
    unsigned int*   getOneImage(unsigned short moduleMask);

    int             setHV(unsigned short moduleMask,unsigned char DAC_HV);
    int             setHV_param(unsigned short moduleMask,unsigned int hvCutTime, unsigned int hvDeadTime=1000000);
    int             readADC( unsigned short moduleMask,float Voltage_value[4]);

    int             exposeWriteFileAs(unsigned short moduleMask,unsigned int burstNumber);

    int             readImgFromDisk(unsigned short modMask,int type,unsigned short *buff,int imageNumber, int burstNumber);


    unsigned short * loadConfigLFromFileToSRAM(unsigned short moduleMask,char *fpath);
    int              loadConfigGFromFile(unsigned short moduleMask,char *fpath);
    int              saveConfigGToFile(unsigned short moduleMask,char *fpath);

    inline unsigned int getImageType(void){
        return m_image_type;
    }
    inline unsigned int getDetectorType(void){
        return m_detectorType;
    }
    int setDetectorType(unsigned int detectorType);

 private:

    bool DEBUG;
    unsigned int    m_image_type;
    unsigned int    m_image_numbers;
    int             m_current_image_number;
    unsigned short  m_module_mask;
    int             m_module;
    bool            m_expose_busy;
    unsigned long   m_firmwareId;
    unsigned int    m_detectorType;

    bool            m_enableConfigureTransfertSize;

    int             checkModAcK(unsigned short Mask, unsigned short *data, unsigned short Ack);
    int             resetRegFifo();     /// reset reg fifo before transfert
    int             modulesName();      /// set the hardware address of module
    int             setRegTrasfertSize(unsigned short size);
    int             setDataTransfert(int type);

    int             globalAskReady(unsigned short moduleMask); // check all modules



};

#endif // XPADCOMMANDS_H
