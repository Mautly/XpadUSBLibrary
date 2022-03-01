#include "writeexposefile.h"

/**
 * @brief WriteExposeFile::WriteExposeFile
 *      Asynchrone Expose function
 * @param moduleMask    : modulke mask
 * @param nImage        : numbers of images
 * @param imgType       : image format 2 bytes or 4 bytes
 * @param abortFlag     : pointer on abort flag
 * @param burstNumber   : numbers of Acquisition burst
 * @param currentImage  : curent images acquire the detector
 * @param currentImageFromDetector  : current image use for write raw file
 * @param buffer        : ring buffer
 */

WriteExposeFile::WriteExposeFile(unsigned moduleMask,
                                     int nImage,
                                     unsigned short imgType,
                                     bool* abortFlag,
                                     int burstNumber,
                                     int *currentImage,
                                     int *currentImageFromDetector,
                                     unsigned char ** buffer){
    m_moduleMask = moduleMask;
    m_nImage= nImage;
    m_imgType = imgType;
    m_burstNumber = burstNumber;
    m_currentImage = currentImage;
    m_currentImageFromDetector = currentImageFromDetector;
    m_buffer = buffer;
    m_abortFlag = abortFlag;
}


/**
 * @brief WriteExposeFile::run
 *  Thread write raw images file
 */

void WriteExposeFile::run(){

    int imgsize =0;
    char fileName[200];
    FILE *fd_img=NULL;

    if(m_imgType == 0)
        imgsize = 120*566*m_tools.getNbModMask(m_moduleMask);
    else
        imgsize = 120*1126*m_tools.getNbModMask(m_moduleMask);


    for(int i=0;i<m_nImage;i++){
    //    cout << "QThread : " << __func__ <<"() \t"<< "Image Number = " << i << endl;

       //while(*m_currentImageFromDetector <= i && (*m_abortFlag)==0) QThread::usleep(1);
        while(*m_currentImageFromDetector <= i ) QThread::usleep(1);


        sprintf(fileName,RAWFILE,m_burstNumber,i);
        fd_img = fopen(fileName,"wb");
        if(fd_img == NULL){
            cout << "LibXpadUSB : " << __func__ <<"() \t"<< "Can not open " << fileName << endl;
            break;
        }

        fwrite(m_buffer[i%2],imgsize*2+2,1,fd_img);
        fclose(fd_img);
        fd_img = NULL;
        *m_currentImage = (*m_currentImage) + 1;
        if(*m_abortFlag){
            cout << "LibXpadUSB : " << __func__ <<"() \t"<< "Abort detected " << endl;
            fflush(stdout);
            break;
        }
    }
}
