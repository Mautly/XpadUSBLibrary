#include "tools.h"
#include <iostream>
using namespace std;

Tools::Tools()
{
}

/**
 * @brief Tools::setDebugMode
 * @param value : true debug mode activate
 */

void Tools::setDebugMode(bool value)
{
    DEBUG = value;
}

/**
 * @brief Tools::bufferToIntBuffer : convert buffer to 32 bits buffer
 * @param moduleMask : Module mask of detector S70 or S10 value = 0x1
 * @param imageType  :
 *                      : 1 => image 32 bits
 *                      : 0 => image 16 bits
 * @param buffer        : input buffer 16 bits
 * @param outBuffer     : output buffer 32 bits
 * @return              :
 *                      : -1 if the module mask = 0 or buffer = NULL or outBuffer = NULL
 *                      : 0 succes
 */

int Tools::bufferToIntBuffer(unsigned short moduleMask,unsigned short imageType,unsigned short *buffer, unsigned int *outBuffer)
{
    int NbMod  = getNbModMask(moduleMask);

    if(moduleMask == 0)
    {
        cout << "LibXpadUSB : " << __func__ <<"()\t" << "ERROR => ModuleMask = 0" << endl;
        return -1;
    }

    if(buffer == NULL)
    {
        cout << "LibXpadUSB : " << __func__ <<"()\t" << "ERROR => buffer in == NULL = 0" << endl;
        return -1;
    }
    if(outBuffer == NULL)
    {
        cout << "LibXpadUSB : " << __func__ <<"()\t" << "ERROR => Buffer out == NULL" << endl;
        return -1;
    }

    if (imageType == 0){
        int k = 0;
        for (int i = 0; i<120*NbMod; i++) {
            for (unsigned int j = 0; j<560; j++) {
                outBuffer[k]=buffer[j+i*560];
                k++;
            }
        }
    }
    else{
        for (unsigned int i = 0; i<120*NbMod; i++){
            for (unsigned int j = 0; j<560;j++){
                outBuffer[j+i*560]=(buffer[i*1120+j*2]) | (buffer[i*1120+j*2+1] << 16) ;
            }
        }
    }
    return 0;
}

/**
 * @brief Tools::getNbModMask
 * @param Mask  : module mask
 * @return
 */

int Tools::getNbModMask(unsigned short Mask)
{
    int number=0;
    for(int i=0;i<5;i++)
    {
        if(Mask & (0x01<<i)) number++;
    }
    return number;
}

/**
 * @brief Tools::rawBuffer2Buffer
 *              this function change the raw buffer from detector to the matrix image
 * @param modMask   : module mask
 * @param type      : 2 byte or 4 bytes
 * @param buffer    : outpout buffer
 * @param rawBuffer : raw buffer from detector
 * @param funcCall  : name of function call this
 * @return          : -1 if error else 0
 */


int Tools:: rawBuffer2Buffer(unsigned short modMask,int type,unsigned short *buffer,unsigned char *rawBuffer,const char *funcCall){

    int imgSize =0;

    unsigned short *buf_tmp=NULL;
    int NbMod = getNbModMask(modMask);
    int sizeLine=0;
    int module =0;
    int line   =0;
    int ret =0;

    if(type==0){
        imgSize = 120*566*2*NbMod;
        sizeLine = 566;
    }
    else
    {
        imgSize = 120*1126*2*NbMod;
        sizeLine = 1126;
    }

    buf_tmp     = new unsigned short[imgSize/2];
    if(rawBuffer == NULL || buf_tmp==NULL){
        if(DEBUG)
            cout << "LibXpadUSB : " << __func__ <<"() in " << funcCall << "() \t"<< "Can not create tmp buffer" << endl;
        return -1;
    }

    for(int j=0;j<120*NbMod;j++){
        for(int i=0;i<sizeLine;i++){
            buf_tmp[i+j*sizeLine]= rawBuffer[2+i*2+(j*sizeLine*2)] | rawBuffer[3+i*2+(j*sizeLine*2)] << 8;
        }
    }


    for(int j=0;j<120*NbMod;j++){
        if(buf_tmp[j*sizeLine]!=0xAA55){
           if(DEBUG)
                cout << "LibXpadUSB : " << __func__ <<"() in " << funcCall <<"() \t"<< "ERROR 0xAA55 not found line = "<< j << endl;
            ret = -1;
            continue;
        }
        if(buf_tmp[j*sizeLine+sizeLine-1]!=0xF0F0){
            if(DEBUG)
                cout << "LibXpadUSB : " << __func__ <<"() in " << funcCall <<"() \t"<< "ERROR 0xF0F0 not found line = "<< j << endl;
            ret = -1;
            continue;
        }
        module = buf_tmp[j*sizeLine+1]-1;
        line   = buf_tmp[j*sizeLine+4]-1;

        for(int i=5;i<(sizeLine-1);i++){
            buffer[module*120*(sizeLine-6) + line*(sizeLine-6)+i-5] = buf_tmp[i+j*sizeLine] & 0xFFFF;
        }
    }

    delete[] buf_tmp;
    return ret;
}

/**
 * @brief Tools::mask2ModNumber
 *          find the first module
 * @param Mask : module mask
 * @return  : return the value of first module
 */

int Tools::mask2ModNumber(unsigned short Mask)
{

    for(int i=0;i<5;i++) {
        if((Mask>>i) & 0x01)
            return i;
    }
    return -1;
}

/**
 * @brief Tools::cleanSSDImages
 *              delete all raw file from burst
 * @param burstNumber   : burst number
 * @param imagesNumber  : numbers of images deteted
 * @return              : -1 if error else 0
 */

int Tools::cleanSSDImages(int burstNumber, int imagesNumber){

    QString fileName;
    QDir file;

    for (int i=0; i<imagesNumber; i++){
        fileName = "/opt/imXPAD/tmp/burst_" + QString::number(burstNumber) + "_img_" + QString::number(i) + ".bin";
        bool ret = file.remove(fileName);
        if (ret == false)
            return -1;
    }
    return 0;
}

/**
 * @brief Tools::getFirstModule
 *          give the first module for the mask
 * @param Mask : module mask
 * @return     : the first module
 */

int Tools::getFirstModule(unsigned short Mask)
{

    for(int i=0;i<5;i++) {
        if((Mask>>i) & 0x01)
            return i;
    }
    return -1;
}

/**
 * @brief Tools::getLastModule
 *              return the last module
 * @param Mask  : module mask
 * @return      : the last module
 */

int Tools::getLastModule(unsigned short Mask)
{
    int last = -1;
    for(int i=0;i<5;i++) {
        if((Mask>>i) & 0x01)
            last = i+1;
    }
    return last;
}
