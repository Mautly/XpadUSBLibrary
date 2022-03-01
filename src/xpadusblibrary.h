#ifndef XPADUSBLIBRARY_H
#define XPADUSBLIBRARY_H

#include "xpadusblibrary_global.h"


#include "defines.h"
#include "xpadcommands.h"
#include "calibration.h"
#include "asynchronous.h"


class XPADUSBLIBRARYSHARED_EXPORT XpadUSBLibrary
{
public:
    XpadUSBLibrary();
    ~XpadUSBLibrary();

    char *        getXpadUsbDetector(void);
    int           init(int USBdevice=0);
    int           init(int USBdevice,unsigned short detectorType);

    void           startDebugMode();
    void           stopDebugMode();
    void           setDebugMode(bool val);

    int            getLibStatus(void);
    void           setLibStatus(int status);

    bool           getAbortStatus(void);
    int            getImageCounter(void);

    bool           getFlagExposeBusy(void);
    unsigned long   getFirmwareId(void);

    unsigned short getModuleMask(void);
    int            setDetectorModel(unsigned short detectorModel);
    unsigned short getModuleNumber(void);
    unsigned short getChipNumber(void);
    unsigned int   getChipMask(void);

    int            exposeAsync(unsigned short moduleMask, unsigned int burstNumber);

    unsigned int   loadConfigLFromFileToSRAM(unsigned short moduleMask,char *fpath);
    int            loadConfigGFromFile(unsigned short moduleMask,char *fpath);
    int            saveConfigGToFile(unsigned short moduleMask,char *fpath);

    unsigned int  getImageType(void);


    /**
     * @brief getDetectorType
     * @return return the type of detector else -1
     */
    unsigned int getDetectorType(void);
    /**
     * @brief setDetectorType
     * @param detectorType : type of detector ex: S70 or S140
     * @return -1 if the detector isn't compatible else 0
     */
    int setDetectorType(unsigned int detectorType);





public:

    xpad_cmd        *command;
    Calibration     *calibration;
    Tools           *tools;

private:

    USBControl     *m_usb;

    unsigned short  m_module_number;
    unsigned short  m_chip_number;
    unsigned int    m_chip_mask;

    int             m_Libstatus;

    bool            DEBUG;

};
#endif // XPADUSBLIBRARY_H
