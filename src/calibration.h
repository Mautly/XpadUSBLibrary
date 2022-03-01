#ifndef CALIBRATION_H
#define CALIBRATION_H

#include "xpadcommands.h"
#include "tools.h"
#include "defines.h"

class Calibration
{
public:
    Calibration();
    Calibration(xpad_cmd *xpad_cmd);

    // public functions
    void    setDebugMode(bool value);

    int     ithlIncrease(unsigned short moduleMask=1);
    int     ithlDecrease(unsigned short moduleMask=1);

    int     calibrationOTN_pulse(unsigned short moduleMask,char *fpath, unsigned short OTNconf);
    int     calibrationBEAM(unsigned short moduleMask,char *fpath, unsigned int Texp,
                            unsigned short ITHL_max,unsigned short itune, unsigned short imfp);

    int     calibrationOTN(unsigned short moduleMask, char *fpath, unsigned short OTNconf);

    int     processScanDACL_Beam(unsigned short moduleMask, char * fpath);

private:

    bool        DEBUG;
    xpad_cmd    *m_xpad_cmd;
    Tools        m_tools;



    // Scan Data
    int     scanITHL_v2(unsigned int Texp, unsigned short ithl_min, unsigned short ithl_max, char *fpath, unsigned short moduleMask);

    int     scanITHL(unsigned int Texp, unsigned short ithl_min,
                             unsigned short ithl_max, char *fpath, unsigned short moduleMask);
    int     scanITHL_beam(unsigned int Texp, unsigned short ithl_min,
                             unsigned short ithl_max, char *fpath, unsigned short moduleMask);

    int     scanITHL_OTN(unsigned int Texp, unsigned short ithl_min, unsigned short ithl_max,
                         char *fpath, unsigned short moduleMask);

    int     scanDACL_pulse(unsigned int nbpulse, char *fpath, unsigned short moduleMask);
    int     scanDACL_beam(unsigned short moduleMask,unsigned int Texp, char *fpath);
    int     scanDACL(unsigned short moduleMask,unsigned int Texp, char *fpath);


    // Process data
    int     daclRefiningPulse(unsigned short moduleMask,unsigned short iterations);

    int     Scurve_beam(unsigned int **data,int cord_x,int cord_y, int*count);
    int     BEAMCalibrationVerification(int *Buff);
    float   erf_f(int *x,int *par);
};

#endif // CALIBRATION_H
