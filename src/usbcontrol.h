#ifndef USBCONTROL_H
#define USBCONTROL_H

#ifdef __APPLE__
#include "/Applications/QuickUSB/Library/C/QuickUSB.h"
#elif _WIN64
#include "C:\Program Files (x86)\Bitwise Systems\QuickUsb\Library\C++ Builder\QuickUSB.h"
#include <direct.h>
#elif _WIN32
#include "C:\Program Files\Bitwise Systems\QuickUsb\Library\C++ Builder\QuickUSB.h"
#include <direct.h>
#elif __linux__
#include "QuickUSB.h"
#endif

#include "tools.h"

#include <cstdio>
#include <iostream>
#include <fstream>          //needed for ifstream
#include <string.h>         //needed for strlen
#include <iomanip>          //for printing hexadecimal numbers correctly
#include <cstdlib>          //needed for malloc
#include <stdint.h>         //needed for uint16_t
#include <sys/types.h>      //needed for stat()
#include <sys/stat.h>       //needed for stat()
#include <string.h>         //needed for strncpm()

class USBControl
{
public:
    USBControl();
    ~USBControl();

    void            setDebugMode(bool value);
    int             setUSBDevice(int i);
    char*           getUSBDeviceList();


    void            setAbortFlag(bool val);
    bool            getAbortFlag(void);

    int             getNbModMask(unsigned short Mask);
    int             mask2ModNumber(unsigned short Mask);

    unsigned short* sendCommand(unsigned short modMask, unsigned short *cmd,int size, unsigned long timeout);
    int             sendSpecialCommand(unsigned short *cmd,int size);

    int             readImgInit(unsigned short *cmd);
    int             readImgBuff(unsigned short type,unsigned char *buf,unsigned short modMask);
    int             readImgClose();

    int             readOneImage(unsigned short *cmd,unsigned int ImgType,unsigned char *rawBuffer, const char *funcCall,unsigned short moduleMask);
    unsigned int*   getOneImage(int imgType,unsigned short moduleMask);


private :

    //QuickUSB
   QCHAR   nameList[1024], *name;
   QHANDLE hDevice;
   QRESULT ok;
   QULONG  lastError;

   bool             firstAqcUSB;

   unsigned short   moduleMask;
   bool             m_abort_flag;
   bool             m_abort_flag_usb_write;
   int              m_abort_flag_usb_read;

   bool DEBUG;
   Tools m_tools;

   void inline setAbortFlagUsbWrite(bool val){
       m_abort_flag_usb_write = val;
   }
   bool inline getAbortFlagUsbWrite(){
       return m_abort_flag_usb_write;
   }
   void inline setAbortFlagUsbRead(int val){
       m_abort_flag_usb_read = val;
   }
   int inline getAbortFlagUsbRead(){
       return m_abort_flag_usb_read;
   }


};

#endif // USBCONTROL_H
