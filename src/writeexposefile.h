#ifndef WRITEEXPOSEFILE_H
#define WRITEEXPOSEFILE_H

#include <QThread>
#include "defines.h"
#include "tools.h"

#include <iostream>
#include <stdio.h>


using namespace std;

class WriteExposeFile : public QThread
{
public:
    WriteExposeFile(unsigned moduleMask,
                      int nImage,
                      unsigned short imgType,
                      bool* abortFlag,
                      int burstNumber,
                      int *currentImage,
                      int *currentImageFromDetector,
                      unsigned char ** buffer);
    void run();
private:
    unsigned char     ** m_buffer;
    int                * m_currentImage;
    int                * m_currentImageFromDetector;
    int                  m_nImage;
    unsigned short       m_imgType;
    int                  m_burstNumber;
    unsigned short       m_moduleMask;
    Tools                m_tools;
    bool               * m_abortFlag;

};

#endif // WRITEEXPOSEFILE_H
