#ifndef TOOLS_H
#define TOOLS_H

#include "defines.h"

#include <QString>
#include <QDir>

class Tools
{
public:
    Tools();

    int bufferToIntBuffer( unsigned short moduleMask,unsigned short imageType,unsigned short *buffer, unsigned int *outBuffer);
    int rawBuffer2Buffer(unsigned short modMask,int type,unsigned short *buffer,unsigned char *rawBuffer,const char *funcCall);
    int getNbModMask(unsigned short Mask);
    int mask2ModNumber(unsigned short Mask);
    int cleanSSDImages(int burstNumber, int imagesNumber);

    void setDebugMode(bool value);
    int  getLastModule(unsigned short Mask);
    int  getFirstModule(unsigned short Mask);
private:

    bool DEBUG;

};

#endif // TOOLS_H
