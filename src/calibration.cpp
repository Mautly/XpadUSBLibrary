#include "calibration.h"


using namespace std;


Calibration::Calibration()
{
}

Calibration::Calibration(xpad_cmd *cmd)
{
    m_xpad_cmd = cmd;
}

/**
 * @brief Calibration::setDebugMode
 *      this function set the DEBUG MODE
 * @param value
 */
void Calibration::setDebugMode(bool value)
{
    DEBUG = value;
}

/**
 * @brief Calibration::calibrationOTN
 *      this function perform the OTN Calibration
 * @param moduleMask    : module mask
 * @param fpath         : path to save temp file
 * @param OTNconf       : 0 slow , 1 medium , 2 fast
 * @return              : -1 if ERROR else 0
 */
int Calibration::calibrationOTN(unsigned short moduleMask, char *fpath, unsigned short OTNconf){

    m_xpad_cmd->m_xpad_usb->setAbortFlag(false);
    //The number of modules depend of the detector (Needed to be change for another detector type)

#ifdef _WIN32
    mkdir(fpath);
#else
    mkdir(fpath, S_IRWXU | S_IRWXG | S_IRWXO);
#endif


    unsigned short regid = 62;
    unsigned short value = 0;

    unsigned short iterations = 20;

    cout << "\n\nStep 1. Configuring global registers" << endl;

    //Loading ConfigG Slow into detector
    for (unsigned short counter=0; counter<7; counter++){
        switch (OTNconf){
        case 0:switch(counter){
            case 0: regid=AMPTP;value=0;break;
            case 1: regid=IMFP;value=5;break;
            case 2: regid=IOTA;value=40;break;
            case 3: regid=IPRE;value=60;break;
            case 4: regid=ITHL;value=30;break;
            case 5: regid=ITUNE;value=120;break;
            case 6: regid=IBUFF;value=0;
            }; break;

        case 1:switch(counter){
            case 0: regid=AMPTP;value=0;break;
            case 1: regid=IMFP;value=25;break;
            case 2: regid=IOTA;value=40;break;
            case 3: regid=IPRE;value=60;break;
            case 4: regid=ITHL;value=30;break;
            case 5: regid=ITUNE;value=120;break;
            case 6: regid=IBUFF;value=0;
            }; break;

        case 2:switch(counter){
            case 0: regid=AMPTP;value=0;break;
            case 1: regid=IMFP;value=52;break;
            case 2: regid=IOTA;value=40;break;
            case 3: regid=IPRE;value=60;break;
            case 4: regid=ITHL;value=30;break;
            case 5: regid=ITUNE;value=120;break;
            case 6: regid=IBUFF;value=0;
            }; break;
        }
        //Each register value is read from the detector
        if (m_xpad_cmd->loadConfigG(moduleMask,regid,value,0x7F) == NULL)
            return -1;
        cout << "ConfigG Load finished for regid:" << regid << " Value: " << value << endl;
        
        if(m_xpad_cmd->getAbortFlag())
            return 1;
    }

    cout << "\n\nStep 2. ITHL scan." << endl;

    //****ITHL Scan
    //Create ITHL_scan folder
    char ithlscan_path[200];
    sprintf(ithlscan_path, "%s/ITHL_scan", fpath);

#ifdef _WIN32
    mkdir(ithlscan_path);
#else
    mkdir(ithlscan_path, S_IRWXU | S_IRWXG | S_IRWXO);
#endif

    //Image Acquisition and Scan of ITHL
    //    if (this->scanITHL_OTN(1000000, 20, 50, ithlscan_path,moduleMask) == -1)
    //        return -1;

    //Image Acquisition and Scan of ITHL
    if (scanITHL_v2(1000000, 20, 50, ithlscan_path,moduleMask) == -1)
        return -1;

    //This foncition allows to test the algorithm
    //this->scanITHLFromFile(20, 50, ithlscan_path);
    if(m_xpad_cmd->getAbortFlag()){
        m_xpad_cmd->m_xpad_usb->setAbortFlag(false);
        return 1;
    }
    cout << "ITHL values updated into detector" << endl;

    cout << "\n\nStep 3. DACL scan." << endl;

    //****DACL Scan
    //Create DACL_scan folder
    char daclscan_path[200];
    char dark_path[200];
    sprintf(daclscan_path, "%s/DACL_scan", fpath);
    sprintf(dark_path, "%s/adjustment", fpath);

#if defined(_WIN32)
    mkdir(daclscan_path);
#else
    mkdir(daclscan_path,S_IRWXU |  S_IRWXG |  S_IRWXO);
#endif

    //Image Acquisition and Scan of DACL
    if (scanDACL(moduleMask,1000000, daclscan_path) == -1)
        return -1;

    //This foncition allows to test the algorithm
    //this->scanDACLFromFile(daclscan_path);
    if(m_xpad_cmd->getAbortFlag()){
        m_xpad_cmd->m_xpad_usb->setAbortFlag(false);
        return 1;
    }
    cout << "DACL values updated into detector" << endl;

    cout << "\n\nStep 4. Uploading initial DACL matrix." << endl;
    cout << "\n\nStep 5. Adjusting calibration in " << iterations << " iterations." << endl;
    //cout << dark_path << endl;

    ithlIncrease(moduleMask);

    if (daclRefiningPulse(moduleMask,iterations) == -1)
        return -1;

    if(m_xpad_cmd->getAbortFlag()){
        m_xpad_cmd->m_xpad_usb->setAbortFlag(false);
        return 1;
    }
    //Increase ITHL value for one;
    //Uploading IHTL register in the detector with the ITHL values + 1
    if (ithlIncrease(moduleMask) == -1)
        return -1;

    m_xpad_cmd->m_xpad_usb->setAbortFlag(false);
    return 0;


}

/**
 * @brief Calibration::ithlIncrease
 *          this function increase the ITHL value + 1 save in the Detector
 * @param moduleMask    : module mask
 * @return              : -1 if ERROR else 0
 */

int Calibration::ithlIncrease(unsigned short moduleMask){
    //Increase ITHL value for one;
    //Uploading IHTL register in the detector with the ITHL values + 1
    unsigned short chipMask;
    int NbMod = m_tools.getNbModMask(moduleMask);
    unsigned short *ithlval = m_xpad_cmd->readConfigG(moduleMask,ITHL);
    if (ithlval!=NULL){


        unsigned short moduleMask_original = moduleMask;
        unsigned short chipMask_original = 0x7f;

        for(int mod=0;mod<NbMod;mod++){
            for(int chip=0;chip<7;chip++){
                chipMask = 0x01<<chip;
                if (m_xpad_cmd->loadConfigG(1<<mod,ITHL,ithlval[mod*8+chip+1]+1,chipMask) == NULL)
                    return -1;
            }
        }

        moduleMask = moduleMask_original;
        chipMask = chipMask_original;
        delete[] (ithlval);
        return 0;
    }
    else
        return -1;
}


/**
 * @brief Calibration::ithlDecrease
 *          this function Decrease the ITHL value -1 in the Detector
 * @param moduleMask        : module mask
 * @return                  : -1 if ERROR else 0
 */

int Calibration::ithlDecrease(unsigned short moduleMask){
    //Decrease ITHL value for one;
    //Uploading IHTL register in the detector with the ITHL values + 1
    unsigned short chipMask;
    int NbMod = m_tools.getNbModMask(moduleMask);
    unsigned short *ithlval = m_xpad_cmd->readConfigG(moduleMask,ITHL);
    if (ithlval!=NULL){


        unsigned short moduleMask_original = moduleMask;
        unsigned short chipMask_original = 0x7f;

        for(int mod=0;mod<NbMod;mod++){
            for(int chip=0;chip<7;chip++){
                chipMask = 0x01<<chip;
                if (m_xpad_cmd->loadConfigG(1<<mod,ITHL,ithlval[mod*8+chip+1]-1,chipMask) == NULL)
                    return -1;
            }
        }

        moduleMask = moduleMask_original;
        chipMask = chipMask_original;
        delete[] (ithlval);
        return 0;
    }
    else
        return -1;
}


/**
 * @brief Calibration::scanDACL
 *      Function to process DACL scan data
 *      For every pixel a DACL value is found for which pixel starts counting
 * @param moduleMask    : module mask
 * @param Texp          : exposure time
 * @param fpath         : path to save temp file
 * @return              : -1 if ERROR else 0
 */
int Calibration::scanDACL(unsigned short moduleMask,unsigned int Texp, char *fpath){

    int nbMod = m_tools.getNbModMask(moduleMask);

    //Configure exposure parameters (impages read in 16bit format)
    if (m_xpad_cmd->exposeParam(moduleMask,1,Texp,4000,0,0,9000,0,0) == -1)
        return -1;


    //Buffer containting the 64 read images
    unsigned short *buffer = new unsigned short [nbMod * LineNumber*ColumnNumber*64];
    //    unsigned short buffer[LineNumber*ColumnNumber];
    unsigned short daclVal;

    //For each value of DACL
    for (daclVal=0; daclVal<64; daclVal++){

        //Send Flat DACL
        unsigned short value = daclVal*8+1;
        m_xpad_cmd->loadFlatConfigL(moduleMask,value);

        if(m_xpad_cmd->getAbortFlag())
            return 1;

        cout << "Acquiring image for DACL= " << daclVal << endl;

        //Expose and Read
        // unsigned int **img = this->Expose();
        unsigned int *img = m_xpad_cmd->getOneImage(moduleMask);

        if(img!=NULL){
            //Set name for DACL scan files
            char fname[200];
            sprintf (fname, "%s/DACL_%d.dat", fpath, daclVal);
            ofstream file(fname, ios::out);

            //Scan each image and count the pixels with noise
            for (unsigned int row=0;row<(LineNumber*nbMod);row++) {
                for (unsigned int column=0;column<(ColumnNumber);column++){

                    //Writting a buffer containting the 64 read images
                    // buffer[daclVal*LineNumber*ColumnNumber + (row*(ColumnNumber)+column)] = img[0][row*(ColumnNumber)+column];
                    buffer[(daclVal*LineNumber*ColumnNumber*nbMod) + (row*(ColumnNumber)+column)] = img[row*(ColumnNumber)+column];

                    //Writing to disk the ITHL scan image
                    // file << img[0][row*(ColumnNumber)+column] << " ";
                    file << img[row*(ColumnNumber)+column] << " ";
                }
                file << endl;
            }
            file.close();
            delete[] img;
        }
        else{
            delete[] buffer;
            return -1;
        }
    }// end of loop : scan DACL

    int counter = 0;
    bool max_detect=false;
    int counter_pix_32 =0;
    unsigned short *DACL = new unsigned short  [LineNumber*ColumnNumber*nbMod];

    for (unsigned int row=0;row<(LineNumber*nbMod);row++) {
        for (unsigned int column=0;column<(ColumnNumber);column++){
            unsigned short daclValue=0;

            unsigned short max=0;
            max_detect = false;
            //Detect the noise peak (only count above 5 in order not to be affected by cosmics)
            for (daclVal=0; daclVal<64; daclVal++){
                //cout << buffer[daclVal*LineNumber*ColumnNumber + (row*(ColumnNumber)+column)] << " ";
                if (buffer[(daclVal*LineNumber*ColumnNumber*nbMod) +(row*(ColumnNumber)+column)] > max){
                    //cout << "Detection au DACL = " << daclVal << "\n";
                    max = buffer[daclVal*LineNumber*ColumnNumber*nbMod +(row*(ColumnNumber)+column)];

                    if(max > 2) max_detect = true;

                    daclValue = daclVal;

                }
            }
            //  cout << endl;

            //Choose the DACL value
            // - in case noise peak could not be detected use a default value
            // - otherwise move threshold by two values from the last level above the noise

            if(!max_detect){
                daclValue = 31;
                counter_pix_32++;
            }
            else
                counter++;
            //cout << "DACL = " << daclVal << "\t";

            DACL[(row*(ColumnNumber)+column)] = ((daclValue *8)+1);

            //cout << "DACL = " << daclValue*8 + 1 << "\t";
        }
    }

    delete[] buffer;

    cout << "Counter no MAX detected pixels = " << counter_pix_32 << endl;
    cout << "Detected DACL count = " << counter << " of " << LineNumber*ColumnNumber*nbMod << endl;

    char fname[200];
    //Save result in file
    sprintf (fname, "%s/DACL_Matrix_Base_otn.dat",fpath);
    //cout << fname << endl;
    ofstream file(fname, ios::out);
    for (unsigned int row=0;row<(LineNumber*nbMod);row++){
        for (unsigned int column=0;column<(ColumnNumber);column++){
            file << DACL[row*(ColumnNumber)+column] << " ";
        }
        file << endl;
    }
    file.close();

    //Uploading Scanned DACL values into detector
    unsigned short values[80];
    for(int mod=0;mod<nbMod;mod++){
        for (unsigned int row=0; row<LineNumber; row++) {
            for (unsigned int column=0; column<(ColumnNumber); column++){
                unsigned short chip = column/80;

                if(m_xpad_cmd->getAbortFlag()){
                    delete[] DACL;
                    return 1;
                }

                //Values are being accumulated in packages of 80 data
                values[column - chip*80] = DACL[(mod*120*560) + row*(ColumnNumber)+column];

                //Once 80 values accumulated, they are sended to the SRAM of the detector
                if ((column - chip*80) == 79){
                    //Sending 80 values to the SRAM in the detector
                    if (m_xpad_cmd->saveConfigLToSRAM(1<<mod,chip,row,values) == -1){
                        if (DEBUG) cout<< "LibXpadUSB : " << __func__ <<"() \t"<< "ERROR: ConfigL data could not be send to module: " <<  mod << " chip:" << chip << " row: " << row << endl;
                        delete[] DACL;
                        return -1;
                    }
                }
            }
        }
    }

    if(m_xpad_cmd->getAbortFlag())
        return 1;

    //Uploading DACL to detector
    if (m_xpad_cmd->loadConfigLSRAMToDetector(moduleMask) == -1){
        delete[] DACL;
        return -1;
    }
    if(m_xpad_cmd->getAbortFlag()){
        delete[] DACL;
        return 1;
    }
    if(ithlIncrease(moduleMask) == -1){
        delete[] DACL;
        return -1;
    }

    delete[] DACL;
    return 0;
}



//Function to process ITHL data
/**
 * @brief Calibration::scanITHL
 *  *      Function to process ITHL scan data
 *      For every pixel a DACL value is found for which pixel starts counting
 * @param Texp
 * @param ithl_min
 * @param ithl_max
 * @param fpath
 * @param moduleMask
 * @return
 */
int Calibration::scanITHL(unsigned int Texp, unsigned short ithl_min, unsigned short ithl_max, char *fpath, unsigned short moduleMask){
    //unsigned short modNb = moduleNumber;
    //unsigned short chipNb = chipNumber;

    unsigned short moduleMask_original = moduleMask;
    unsigned short chipMask_original = 0x7f;
    unsigned short chipMask;

    unsigned short moduleNumber = m_tools.getNbModMask(moduleMask);

    //Configure exposure parameters (images read in 16bit format)
    if (m_xpad_cmd->exposeParam(moduleMask,1,Texp,4000,0,0,9000,0,0) == -1)
        return -1;

    //upload DACL=32 (32*8+1=257)
    if (m_xpad_cmd->loadFlatConfigL(moduleMask,257) == -1)
        return -1;

    //Buffer containting the NumFiles read images
    int NumFiles = ithl_max - ithl_min +1;
    unsigned short *buffer = new unsigned short [120*560*moduleNumber*NumFiles];
    unsigned short ithlVal;

    //For each value of ITHL we acquire one image
    for (ithlVal=ithl_max; ithlVal>=ithl_min; ithlVal--){

        if(m_xpad_cmd->getAbortFlag()){
            delete[] buffer;
            return 1;
        }

        //Upload Config G
        if (m_xpad_cmd->loadConfigG(moduleMask,ITHL,ithlVal,0x7F) == NULL){
            delete[] buffer;
            return -1;
        }

        cout << "Acquiring image for ITHL= " << ithlVal << endl;

        //Expose and Read
        //  unsigned int **img = this->Expose();
        unsigned int *img = m_xpad_cmd->getOneImage(moduleMask);

        int index = ithlVal - ithl_min;

        if(img!=NULL){

            //Set name for ITHL scan files
            char fname[200];
            sprintf (fname, "%s/ITHL_%d.dat", fpath, ithlVal);
            ofstream file(fname, ios::out);

            //Scan each image and count the pixels with noise
            for (unsigned int row=0;row<120*moduleNumber;row++) {
                for (unsigned int column=0;column<(560);column++){

                    //Writting a buffer containting the 64 read images
                    // buffer[index*LineNumber*ColumnNumber + (row*(ColumnNumber)+column)] = img[0][row*(ColumnNumber)+column];
                    buffer[index*120*560 + (row*(560)+column)] = img[row*(560)+column];

                    //Writing to disk the ITHL scan image
                    // file << img[0][row*(ColumnNumber)+column] << " ";
                    file << img[row*(120)+column] << " ";
                }
                file << endl;
            }
            file.close();
            // delete[] img[0];
            delete[] img;
        }
        else{
            delete[] buffer;
            return -1;
        }
    }// end of loop : scan ITHL

    int diff_cur=0, diff_max=0;
    unsigned short *ITH = new unsigned short[120*560];
    for (unsigned int row=0;row<120;row++) {
        for (unsigned int column=0;column<(560);column++){
            unsigned short ithlValue=ithl_min;
            diff_cur = 0;
            diff_max = 0;

            //Detect the noise peak (only count above 5 in order not to be affected by cosmics)
            for (unsigned short ithlVal=ithl_max; ithlVal>=ithl_min; ithlVal--){
                int index = ithlVal - ithl_min;

                if (index > 0 && index < NumFiles-1)
                    diff_cur = buffer[(index-1)*120*560 + (row*(560)+column)] - buffer[(index+1)*120*560 + (row*(560)+column)];
                else if (index == 0 || index == NumFiles-1)
                    diff_cur = 0;

                //if(column == 279 && row == 72)
                //cout << diff_cur << " ";
                //cout << buffer[(index)*LineNumber*ColumnNumber + (row*(ColumnNumber)+column)] << " ";

                if (diff_max < diff_cur){
                    ithlValue = ithlVal;
                    diff_max = diff_cur;
                }
            }
            //cout << endl;
            //Choose the DACL value
            //if(column == 279 && row == 72)
            //    cout << "\n x=" << column << " y=" << row << " ithlValue=" << ithlValue << " derive_value=" << diff_max << "\n";
            ITH[(row*(560)+column)] = ithlValue;
        }
    }

    delete[] buffer;



    char fname[200];
    //Save result in file
    sprintf (fname, "%s/ITHL_Matrix_Base.dat",fpath);
    cout << "Writting results in: " << fname << endl;
    ofstream file(fname, ios::out);
    for (unsigned short row=0;row<120;row++){
        for (unsigned short column=0;column<(560);column++)
            file << ITH[row*(560)+column] << " ";
        file << endl;
    }
    file.close();

    unsigned int sum=0, count=0;
    double mean;
    //Mean Value of ITHL for each chip of every module
    for (unsigned short i=0; i<moduleNumber; i++)
        for (unsigned short j=0; j<7; j++){
            //cout << "chip= " << j << endl;
            for (unsigned short row=i*120; row<(i+1)*120;row++){
                for (unsigned short column=j*80; column<(j+1)*80; column++){
                    //cout << "chip= " << j << " row= " << row << " column= " << column << endl;
                    if (ITH[row*(560)+column]>ithl_min){
                        sum += ITH[row*(560)+column];
                        count++;
                    }
                }
            }

            if (count > 0)
                mean = sum / count;
            else
                mean = 0;

            if(m_xpad_cmd->getAbortFlag()){
                delete[] ITH;
                return 1;
            }

            moduleMask = 0x01<<i;
            chipMask = 0x01<<j;
            cout << "module=" << i << " chip=" << j+1 << " ITHLmean=" << mean << endl;
            if (m_xpad_cmd->loadConfigG(moduleMask,ITHL,(unsigned short)mean,chipMask) == NULL){
                delete[] ITH;
                chipMask = chipMask_original;
                moduleMask = moduleMask_original;
                return -1;
            }
            sum = 0;
            count = 0;
        }
    delete[] ITH;
    chipMask = chipMask_original;
    moduleMask = moduleMask_original;
    return 0;

}


//Function to process DACL scan data
//For every pixel a DACL value is found for which pixel starts counting
/**
 * @brief Calibration::scanDACL_pulse
 *      Function to process DACL scan data with internal chip pulse
 *      For every pixel a DACL value is found for which pixel starts counting
 * @param nbpulse           : numbers of pulses
 * @param fpath             : path for temp calibration file
 * @param moduleMask        : module mask
 * @return                  : -1 if ERROR else 0
 */
int Calibration::scanDACL_pulse(unsigned int nbpulse, char *fpath, unsigned short moduleMask){

    int nbMod = m_tools.getNbModMask(moduleMask);
    //Buffer containting the 64 read images
    unsigned short *buffer = new unsigned short [120*560*64*nbMod];
    //    unsigned short buffer[LineNumber*ColumnNumber];
    unsigned short daclVal;

    //For each value of DACL
    for (daclVal=0; daclVal<64; daclVal++){

        if(m_xpad_cmd->getAbortFlag()){
            delete [] buffer;
            return 1;
        }

        //Send Flat DACL
        unsigned short value = ((daclVal*8)+1);
        if( m_xpad_cmd->loadFlatConfigL(moduleMask,value) < 0){
            cout << "LoadFlatConfigL(" << value << ") doesn't work" << endl;
            delete [] buffer;
            return -1;
        }

        if(m_xpad_cmd->getAbortFlag()){
            delete [] buffer;
            return 1;
        }
        cout << "Acquiring image pulse for DACL= " << daclVal << endl;

        unsigned short *img = m_xpad_cmd->testPulse(moduleMask,nbpulse);
        if( img == NULL){
            delete [] buffer;
            return -1;
        }

        if(img!=NULL){
            //Set name for DACL scan files
            char fname[200];
            sprintf (fname, "%s/DACL_%d.dat", fpath, daclVal);
            ofstream file(fname, ios::out);

            //Scan each image and count the pixels with noise
            for (unsigned int row=0;row<LineNumber*nbMod;row++) {
                for (unsigned int column=0;column<ColumnNumber;column++){
                    //Writting a buffer containting the 64 read images
                    buffer[(daclVal*LineNumber*ColumnNumber*nbMod) + (row*ColumnNumber+column)] = img[row*(ColumnNumber)+column];
                    //Writing to disk the ITHL scan image
                    file << img[row*(ColumnNumber)+column] << " ";
                }
                file << endl;
            }
            file.close();
        }
        else{
            delete [] buffer;
            return -1;            
        }

        delete[] img;
    }// end of loop : scan DACL

    if (m_xpad_cmd->loadConfigG(moduleMask,AMPTP,0,0x7F) == NULL)
        return -1;

    cout << endl;

    int counter = 0;
    bool max_detect=false;
    int counter_pix_32 =0;
    unsigned short *DACL = new unsigned short  [LineNumber*ColumnNumber*nbMod];

    for (unsigned int row=0;row<(LineNumber*nbMod);row++) {
        for (unsigned int column=0;column<(ColumnNumber);column++){
            unsigned short daclValue=0;

            unsigned short max=0;
            max_detect = false;
            //Detect the noise peak (only count above 5 in order not to be affected by cosmics)
            for (daclVal=0; daclVal<64; daclVal++){
                //cout << buffer[daclVal*LineNumber*ColumnNumber + (row*(ColumnNumber)+column)] << " ";
                if (buffer[(daclVal*LineNumber*ColumnNumber*nbMod) +(row*(ColumnNumber)+column)] > max){
                    //cout << "Detection au DACL = " << daclVal << "\n";
                    max = buffer[daclVal*LineNumber*ColumnNumber*nbMod +(row*(ColumnNumber)+column)];

                    if(max > 2) max_detect = true;

                    daclValue = daclVal;

                }
            }
            //  cout << endl;

            //Choose the DACL value
            // - in case noise peak could not be detected use a default value
            // - otherwise move threshold by two values from the last level above the noise

            if(!max_detect){
                daclValue = 31;
                counter_pix_32++;
            }
            else
                counter++;
            //cout << "DACL = " << daclVal << "\t";

            DACL[(row*(ColumnNumber)+column)] = ((daclValue *8)+1);

            //cout << "DACL = " << daclValue*8 + 1 << "\t";
        }
    }

    delete[] buffer;

    cout << "Counter no MAX detected pixels = " << counter_pix_32 << endl;
    cout << "Detected DACL count = " << counter << " of " << LineNumber*ColumnNumber*nbMod << endl;

    char fname[200];
    //Save result in file
    sprintf (fname, "%s/DACL_Matrix_Base_otn.dat",fpath);
    //cout << fname << endl;
    ofstream file(fname, ios::out);
    for (unsigned int row=0;row<(LineNumber*nbMod);row++){
        for (unsigned int column=0;column<(ColumnNumber);column++){
            file << DACL[row*(ColumnNumber)+column] << " ";
        }
        file << endl;
    }
    file.close();

    //Uploading Scanned DACL values into detector
    unsigned short values[80];
    for(int mod=0;mod<nbMod;mod++){
        for (unsigned int row=0; row<LineNumber; row++) {
            for (unsigned int column=0; column<(ColumnNumber); column++){
                unsigned short chip = column/80;

                if(m_xpad_cmd->getAbortFlag()){
                    delete[] DACL;
                    return 1;
                }

                //Values are being accumulated in packages of 80 data
                values[column - chip*80] = DACL[(mod*120*560) + row*(ColumnNumber)+column];

                //Once 80 values accumulated, they are sended to the SRAM of the detector
                if ((column - chip*80) == 79){
                    //Sending 80 values to the SRAM in the detector
                    if (m_xpad_cmd->saveConfigLToSRAM(1<<mod,chip,row,values) == -1){
                        if (DEBUG) cout<< "LibXpadUSB : " << __func__ <<"() \t"<< "ERROR: ConfigL data could not be send to module: " <<  mod << " chip:" << chip << " row: " << row << endl;
                        delete[] DACL;
                        return -1;
                    }
                }
            }
        }
    }

    if(m_xpad_cmd->getAbortFlag()){
        delete[] DACL;
        return 1;
    }

    //Uploading DACL to detector
    if (m_xpad_cmd->loadConfigLSRAMToDetector(moduleMask) == -1){
        delete[] DACL;
        return -1;
    }
    if(m_xpad_cmd->getAbortFlag()){
        delete[] DACL;
        return 1;
    }
    if(ithlIncrease(moduleMask) == -1){
        delete[] DACL;
        return -1;
    }

    delete[] DACL;
    return 0;
}



/**
 * @brief Calibration::daclRefiningPulse
 *          this function find the noisy pick after DACL Scan for each pixels and calibrate all pixels
 *          at the best DACL value over the noise
 * @param moduleMask    : module mask
 * @param iterations    : numbers of time you restart the process
 * @return              : -1 if ERROR else 0
 */


int Calibration::daclRefiningPulse(unsigned short moduleMask,unsigned short iterations){

    unsigned int noisyPixels = 0;
    unsigned int blockedPixels = 0;
    unsigned int Pixels_0 = 0;
    unsigned short *daclVal=NULL;
    // unsigned int **img = NULL;

    unsigned int *img = NULL;
    int Nbmod = m_tools.getNbModMask(moduleMask);

    //unsigned short *img = new unsigned short [LineNumber*ColumnNumber];

    //Send Expose Pameters
    if (m_xpad_cmd->exposeParam(moduleMask,1,2000000,4000,0,0,9000,0,0) == -1)
        return -1;

    daclVal =  m_xpad_cmd->readConfigL(moduleMask);
    if (daclVal == NULL)
        return -1;

    int tmp=0;
    for (unsigned int pixel=0; pixel<LineNumber*ColumnNumber*Nbmod; pixel++){
        daclVal[pixel] -= 1;
        daclVal[pixel] /= 8;
        if( daclVal[pixel]==0) tmp++;
    }

    cout << "nb pixel read dacl = 0 is < " << tmp << " > " << endl;
    for (unsigned short i=0; i<iterations; i++){
        if(m_xpad_cmd->getAbortFlag())
            return 1;

        //   img = this->Expose();
        img = m_xpad_cmd->getOneImage(moduleMask);

        if (img == NULL)
            return -1;

        blockedPixels = 0;
        noisyPixels   = 0;
        Pixels_0      = 0;

        //Counting noisyPixels
        for (unsigned int pixel=0; pixel<LineNumber*ColumnNumber*Nbmod; pixel++){
            if (img[pixel] > 0){
                noisyPixels++;
                if (daclVal[pixel] > 0)
                    daclVal[pixel] = daclVal[pixel] - 1;
                else
                    blockedPixels++;
            }
            if(daclVal[pixel] == 0)
                Pixels_0++;
        }

        cout << "Iteration : " << i << endl;
        cout << "Number of noisy pixels = " << noisyPixels << " of " << LineNumber*ColumnNumber*Nbmod << endl;
        cout << "Number of blocked pixels = " << blockedPixels << " of " << noisyPixels << endl;
        cout << "Number of corrected pixels = " << noisyPixels-blockedPixels << " of " << noisyPixels << endl;
        cout << "Number of pixels DACL = 0 = " << Pixels_0 << " of " << LineNumber*ColumnNumber*Nbmod << endl;

        if(noisyPixels==0){
            cout << "Finished calibration optimization, any noisy pixel was found after " << i << " iterations." << endl;
            break;
        }
        int transfert = 0;
        //Uploading Scanned DACL values into detector
        unsigned short values[80];
        for(int mod=0;mod<Nbmod;mod++){
            transfert = 0;
            for (unsigned int row=0; row<LineNumber; row++) {
                for (int column=0; column<(ColumnNumber); column++){
                    unsigned short chip = column/80;
                    if(m_xpad_cmd->m_xpad_usb->getAbortFlag())
                        return 1;
                    if(((1<<mod) & moduleMask) == 0 ) continue; // test dans le cas ou un module est manquant
                    //Values are being accumulated in packages of 80 data
                    values[column - chip*80] = ((daclVal[(mod*560*120)+ row*(ColumnNumber)+column] * 8 ) + 1) ;
                    //Once 80 values accumulated, they are sended to the SRAM of the detector
                    if ((column - chip*80) == 79){

                        //Sending 80 values to the SRAM in the detector
                        if (m_xpad_cmd->saveConfigLToSRAM(1<<mod,chip,row,values) == -1){
                            if (DEBUG) cout<< "LibXpadUSB : " << __func__ <<"() \t" << "ERROR: ConfigL data could not be send to module: " << moduleMask << " chip:" << chip << " row: " << row << endl;
                            return -1;
                        }
                        if(m_xpad_cmd->getAbortFlag())
                            return 1;
                        transfert++;
                        cout << "Loading module " << mod << " line "   << transfert << " of 840\r" << flush;
                    }
                }

            }
            cout << endl;
        }

        //Uploading DACL to detector
        if (m_xpad_cmd->loadConfigLSRAMToDetector(moduleMask) == -1)
            return -1;

        //Reset of noisyPixels counter
        noisyPixels = 0;
        delete [] img;
        img = NULL;
    }
    delete[] daclVal;
    return 0;
}

/**
 * @brief Calibration::calibrationOTN_pulse
 *              this function calibrate all pixels over the noise
 * @param moduleMask    : module mask
 * @param fpath         : ptah for temp calibration file
 * @param OTNconf       : OTN config SLOW,MEDIUM or FAST
 * @return
 */

int Calibration::calibrationOTN_pulse(unsigned short moduleMask,char *fpath, unsigned short OTNconf){

    //The number of modules depend of the detector (Needed to be change for another detector type)

    m_xpad_cmd->m_xpad_usb->setAbortFlag(false);
    unsigned short regid = 62;
    unsigned short value = 0;

    unsigned short iterations = 20;

#if defined(_WIN32)
    mkdir(fpath);
#else
    mkdir(fpath, S_IRWXU | S_IRWXG | S_IRWXO);
#endif

    cout << "\n\nStep 1. Configuring global registers" << endl;

    //Loading ConfigG Slow into detector
    for (unsigned short counter=0; counter<7; counter++){
        if(m_xpad_cmd->getAbortFlag()){
            m_xpad_cmd->m_xpad_usb->setAbortFlag(false);
            return 1;
        }
        switch (OTNconf){
        case 0:switch(counter){
            case 0: regid=AMPTP;value=30;break;
            case 1: regid=IMFP;value=5;break;
            case 2: regid=IOTA;value=40;break;
            case 3: regid=IPRE;value=60;break;
            case 4: regid=ITHL;value=35;break;
            case 5: regid=ITUNE;value=120;break;
            case 6: regid=IBUFF;value=0;
            }; break;

        case 1:switch(counter){
            case 0: regid=AMPTP;value=30;break;
            case 1: regid=IMFP;value=25;break;
            case 2: regid=IOTA;value=40;break;
            case 3: regid=IPRE;value=60;break;
            case 4: regid=ITHL;value=30;break;
            case 5: regid=ITUNE;value=120;break;
            case 6: regid=IBUFF;value=0;
            }; break;

        case 2:switch(counter){
            case 0: regid=AMPTP;value=30;break;
            case 1: regid=IMFP;value=52;break;
            case 2: regid=IOTA;value=40;break;
            case 3: regid=IPRE;value=60;break;
            case 4: regid=ITHL;value=30;break;
            case 5: regid=ITUNE;value=120;break;
            case 6: regid=IBUFF;value=0;
            }; break;
        }
        if(m_xpad_cmd->getAbortFlag()){
            m_xpad_cmd->m_xpad_usb->setAbortFlag(false);
            return 1;
        }
        //Each register value is read from the detector
        if (m_xpad_cmd->loadConfigG(moduleMask,regid,value,0x7F) == NULL)
            return -1;
        cout << "ConfigG Load finished for regid:" << regid << " Value: " << value << endl;
    }


    cout << "\n\nStep 2. ITHL scan." << endl;

    //****ITHL Scan
    //Create ITHL_scan folder
    char ithlscan_path[200];
    sprintf(ithlscan_path, "%s/ITHL_scan", fpath);

#if defined(_WIN32)
    mkdir(ithlscan_path);
#else
    mkdir(ithlscan_path, S_IRWXU | S_IRWXG | S_IRWXO);
#endif

    //Image Acquisition and Scan of ITHL
    if (scanITHL_v2(1000000, 20, 50, ithlscan_path,moduleMask) == -1)
        return -1;

    if(m_xpad_cmd->getAbortFlag()){
        m_xpad_cmd->m_xpad_usb->setAbortFlag(false);
        return 1;
    }

    cout << "ITHL values updated into detector" << endl;

    cout << "\n\nStep 3. DACL scan." << endl;

    //****DACL Scan
    //Create DACL_scan folder
    char daclscan_path[200];
    char dark_path[200];
    sprintf(daclscan_path, "%s/DACL_scan", fpath);
    sprintf(dark_path, "%s/adjustment", fpath);

#if defined(_WIN32)
    mkdir(daclscan_path);
#else
    mkdir(daclscan_path,S_IRWXU |  S_IRWXG |  S_IRWXO);
#endif

    cout << "Image Acquisition and Scan of DACL" << endl;
    //Image Acquisition and Scan of DACL
    if (scanDACL_pulse(100, daclscan_path,moduleMask) == -1)
        return -1;

    if(m_xpad_cmd->getAbortFlag()){
        m_xpad_cmd->m_xpad_usb->setAbortFlag(false);
        return 1;
    }

    cout << "Scan of DACL DONE......" << endl;
    if (m_xpad_cmd->loadConfigG(moduleMask,AMPTP,0,0x7F) == NULL)
        return -1;
    cout << "ConfigG Load regid AMPTP = 0"<< endl;

    cout << "DACL values updated into detector" << endl;

    cout << "\n\nStep 4. Uploading initial DACL matrix." << endl;
    cout << "\n\nStep 5. Adjusting calibration in " << iterations << " iterations." << endl;
    cout << dark_path << endl;

    if (ithlIncrease() == -1)
        return -1;

    if (daclRefiningPulse(moduleMask,iterations) == -1)
        return -1;

    if(m_xpad_cmd->getAbortFlag()){
        m_xpad_cmd->m_xpad_usb->setAbortFlag(false);
        return 1;
    }
    //Increase ITHL value for one;
    //Uploading IHTL register in the detector with the ITHL values + 1
    if (ithlIncrease() == -1)
        return -1;
    //This foncition allows to test the algorithm

    m_xpad_cmd->m_xpad_usb->setAbortFlag(false);
    return 0;
}



/**
 * @brief Calibration::scanDACL_beam
 *              Function to process DACL scan data
 *              For every pixel a DACL value is found for which pixel starts counting
 * @param moduleMask    : module mask
 * @param Texp          : Exposure time in µs
 * @param fpath         : path for temp file
 * @return              : -1 if ERROR else 0
 */
int Calibration::scanDACL_beam(unsigned short moduleMask,unsigned int Texp, char *fpath){

    //Configure exposure parameters (impages read in 16bit format)
    if (m_xpad_cmd->exposeParam(moduleMask,1,Texp,4000,0,0,10000,0,0) == -1)
        return -1;

    int nbMod = m_tools.getNbModMask(moduleMask);

    //Buffer containting the 64 read images
    //    unsigned short *buffer = new unsigned short [120*nbMod*ColumnNumber*64];
    //    unsigned short buffer[LineNumber*ColumnNumber];
    unsigned short daclVal;

    //For each value of DACL
    for (daclVal=0; daclVal<64; daclVal++){
        if(m_xpad_cmd->getAbortFlag())
            return 1;

        //Send Flat DACL
        unsigned short value = daclVal*8+1;
        m_xpad_cmd->loadFlatConfigL(moduleMask,value);


        cout << "Acquiring image for DACL= " << daclVal << endl;

        //Expose and Read
        // unsigned int **img = this->Expose();
        unsigned int *img = m_xpad_cmd->getOneImage(moduleMask);

        if(img!=NULL){
            //Set name for DACL scan files
            char fname[200];
            sprintf (fname, "%s/DACL_%d.dat", fpath, daclVal);
            ofstream file(fname, ios::out);

            //Scan each image and count the pixels with noise
            for (int row=0;row<120*nbMod;row++) {
                for (int column=0;column<(ColumnNumber);column++){
                    file << img[row*(ColumnNumber)+column] << " ";
                }
                file << endl;
            }
            file.close();
            delete[] img;
        }
        else{
            //delete[] buffer;
            return -1;
        }
    }// end of loop : scan DACL

    return processScanDACL_Beam(moduleMask,fpath);
}

/**
 * @brief Calibration::calibrationBEAM
 * @param moduleMask
 * @param fpath
 * @param Texp
 * @param ITHL_max
 * @param itune
 * @param imfp
 * @return
 */

int Calibration::calibrationBEAM(unsigned short moduleMask,char *fpath, unsigned int Texp, unsigned short ITHL_max,unsigned short itune, unsigned short imfp){

    //The number of modules depend of the detector (Needed to be change for another detector type)

    m_xpad_cmd->m_xpad_usb->setAbortFlag(false);

    unsigned short regid = 62;
    unsigned short value = 0;

#ifdef _WIN32
    mkdir(fpath);
#else
    mkdir(fpath, S_IRWXU | S_IRWXG | S_IRWXO);
#endif

    //unsigned short iterations = 20;

    cout << "\n\nStep 1. Configuring global registers" << endl;

    //Loading ConfigG Slow into detector
    for (unsigned short counter=0; counter<7; counter++){
        if(m_xpad_cmd->getAbortFlag()){
            m_xpad_cmd->m_xpad_usb->setAbortFlag(false);
            return 1;
        }

        switch(counter){
        case 0: regid=AMPTP;value=0;break;
        case 1: regid=IMFP;value=imfp;break;
        case 2: regid=IOTA;value=40;break;
        case 3: regid=IPRE;value=60;break;
        case 4: regid=ITHL;value=35;break;
        case 5: regid=ITUNE;value=itune;break;
        case 6: regid=IBUFF;value=0;
        };

        //Each register value is read from the detector
        if (m_xpad_cmd->loadConfigG(moduleMask,regid,value,0x7F) == NULL)
            return -1;
        cout << "ConfigG Load finished for regid:" << regid << " Value: " << value << endl;
    }
    cout << "\n\nStep 2. ITHL scan." << endl;

    //****ITHL Scan
    //Create ITHL_scan folder
    char ithlscan_path[200];
    sprintf(ithlscan_path, "%s/ITHL_scan", fpath);

#if defined(_WIN32)
    mkdir(ithlscan_path);
#else
    mkdir(ithlscan_path, S_IRWXU | S_IRWXG | S_IRWXO);
#endif

    //Image Acquisition and Scan of ITHL
    if (scanITHL_beam(Texp, 20, ITHL_max, ithlscan_path,moduleMask) == -1)
        return -1;

    if(m_xpad_cmd->getAbortFlag()){
        m_xpad_cmd->m_xpad_usb->setAbortFlag(false);
        return 1;
    }

    cout << "ITHL values updated into detector" << endl;

    cout << "\n\nStep 3. DACL scan." << endl;

    //****DACL Scan
    //Create DACL_scan folder
    char daclscan_path[200];
    char dark_path[200];
    sprintf(daclscan_path, "%s/DACL_scan", fpath);
    sprintf(dark_path, "%s/adjustment", fpath);

#if defined(_WIN32)
    mkdir(daclscan_path);
#else
    mkdir(daclscan_path,S_IRWXU |  S_IRWXG |  S_IRWXO);
#endif

    cout << "Image Acquisition and Scan of DACL" << endl;
    //Image Acquisition and Scan of DACL
    if (scanDACL_beam(moduleMask,Texp, daclscan_path) == -1)
        return -1;

    if(m_xpad_cmd->getAbortFlag()){
        m_xpad_cmd->m_xpad_usb->setAbortFlag(false);
        return 1;
    }


    cout << "DACL values updated into detector" << endl;

    m_xpad_cmd->m_xpad_usb->setAbortFlag(false);
    return 0;
}



//Function to process ITHL data
/**
 * @brief Calibration::scanITHL_OTN
 * @param Texp
 * @param ithl_min
 * @param ithl_max
 * @param fpath
 * @param moduleMask
 * @return
 */
int Calibration::scanITHL_OTN(unsigned int Texp, unsigned short ithl_min, unsigned short ithl_max, char *fpath, unsigned short moduleMask){
    //unsigned short modNb = moduleNumber;
    //unsigned short chipNb = chipNumber;

    unsigned short moduleMask_original = moduleMask;
    unsigned short chipMask_original = 0x7f;
    unsigned short chipMask;

    unsigned short moduleNumber = m_tools.getNbModMask(moduleMask);

    //Configure exposure parameters (images read in 16bit format)
    if (m_xpad_cmd->exposeParam(moduleMask,1,Texp,4000,0,0,9000,0,0) == -1)
        return -1;

    //upload DACL=32 (32*8+1=257)
    if (m_xpad_cmd->loadFlatConfigL(moduleMask,257) == -1)
        return -1;

    //Buffer containting the NumFiles read images
    int NumFiles = ithl_max - ithl_min +1;
    unsigned short *buffer = new unsigned short [120*560*moduleNumber*NumFiles];
    unsigned short ithlVal;

    //For each value of ITHL we acquire one image
    for (ithlVal=ithl_max; ithlVal>=ithl_min; ithlVal--){

        if(m_xpad_cmd->getAbortFlag())
            return 1;

        //Upload Config G
        if (m_xpad_cmd->loadConfigG(moduleMask,ITHL,ithlVal,0x7F) == NULL){
            return -1;
        }

        cout << "Acquiring image for ITHL= " << ithlVal << endl;

        //Expose and Read
        //  unsigned int **img = this->Expose();
        unsigned int *img = m_xpad_cmd->getOneImage(moduleMask);

        int index = ithlVal - ithl_min;

        if(img!=NULL){

            //Set name for ITHL scan files
            char fname[200];
            sprintf (fname, "%s/ITHL_%d.dat", fpath, ithlVal);
            ofstream file(fname, ios::out);

            //Scan each image and count the pixels with noise
            for (unsigned int row=0;row<120*moduleNumber;row++) {
                for (unsigned int column=0;column<(560);column++){

                    //Writting a buffer containting the 64 read images
                    // buffer[index*LineNumber*ColumnNumber + (row*(ColumnNumber)+column)] = img[0][row*(ColumnNumber)+column];
                    buffer[index*120*560 + (row*(560)+column)] = img[row*(560)+column];

                    //Writing to disk the ITHL scan image
                    // file << img[0][row*(ColumnNumber)+column] << " ";
                    file << img[row*(120)+column] << " ";
                }
                file << endl;
            }
            file.close();
            // delete[] img[0];
            delete[] img;
        }
        else{
            delete[] buffer;
            return -1;
        }
    }// end of loop : scan ITHL

    unsigned short *ITH = new unsigned short[120*560*moduleNumber];
    for(int i = 0;i<120*560*moduleNumber;i++)
        ITH[i] = ithl_min;

    for (unsigned int row=0;row<120;row++) {
        for (unsigned int column=0;column<(560);column++){
            //Detect the noise peak (only count above 5 in order not to be affected by cosmics)
            for (unsigned short ithlVal=ithl_max; ithlVal>=ithl_min; ithlVal--){
                int index = ithlVal - ithl_min;

                if(buffer[index*120*560 + (row*(560)+column)] > 100){
                    ITH[(row*(560)+column)] = ithlVal;
                    break;
                }
            }
            //cout << endl;
            //Choose the DACL value
            //if(column == 279 && row == 72)
            //    cout << "\n x=" << column << " y=" << row << " ithlValue=" << ithlValue << " derive_value=" << diff_max << "\n";

        }
    }

    delete[] buffer;

    char fname[200];
    //Save result in file
    sprintf (fname, "%s/ITHL_Matrix_Base.dat",fpath);
    cout << "Writting results in: " << fname << endl;
    ofstream file(fname, ios::out);
    for (unsigned short row=0;row<120;row++){
        for (unsigned short column=0;column<(560);column++)
            file << ITH[row*(560)+column] << " ";
        file << endl;
    }
    file.close();

    unsigned int sum=0, count=0;
    double mean;
    //Mean Value of ITHL for each chip of every module
    for (unsigned short i=0; i<moduleNumber; i++)
        for (unsigned short j=0; j<7; j++){
            //cout << "chip= " << j << endl;
            for (unsigned short row=i*120; row<(i+1)*120;row++){
                for (unsigned short column=j*80; column<(j+1)*80; column++){
                    //cout << "chip= " << j << " row= " << row << " column= " << column << endl;
                    if (ITH[row*(560)+column]>ithl_min){
                        sum += ITH[row*(560)+column];
                        count++;
                    }
                }
            }

            if (count > 0)
                mean = sum / count;
            else
                mean = 0;
            if(m_xpad_cmd->getAbortFlag())
                return 1;

            moduleMask = 0x01<<i;
            chipMask = 0x01<<j;
            cout << "module=" << i << " chip=" << j+1 << " ITHLmean=" << mean << endl;
            if (m_xpad_cmd->loadConfigG(moduleMask,ITHL,(unsigned short)mean,chipMask) == NULL){
                delete[] ITH;
                chipMask = chipMask_original;
                moduleMask = moduleMask_original;
                return -1;
            }
            sum = 0;
            count = 0;
        }
    delete[] ITH;
    chipMask = chipMask_original;
    moduleMask = moduleMask_original;
    return 0;

}





//Function to process ITHL data
/**
 * @brief Calibration::scanITHL_beam
 * @param Texp
 * @param ithl_min
 * @param ithl_max
 * @param fpath
 * @param moduleMask
 * @return
 */
int Calibration::scanITHL_beam(unsigned int Texp, unsigned short ithl_min, unsigned short ithl_max, char *fpath, unsigned short moduleMask){
    //unsigned short modNb = moduleNumber;
    //unsigned short chipNb = chipNumber;

    unsigned short moduleMask_original = moduleMask;
    unsigned short chipMask_original = 0x7f;
    unsigned short chipMask;

    unsigned short moduleNumber = m_tools.getNbModMask(moduleMask);

    //Configure exposure parameters (images read in 16bit format)
    if (m_xpad_cmd->exposeParam(moduleMask,1,Texp,4000,0,0,9000,0,0) == -1)
        return -1;

    //upload DACL=32 (32*8+1=257)
    if (m_xpad_cmd->loadFlatConfigL(moduleMask,257) == -1)
        return -1;

    //Buffer containting the NumFiles read images
    int NumFiles = ithl_max - ithl_min + 1;
    unsigned short *buffer = new unsigned short [120*560*moduleNumber*NumFiles];
    unsigned short ithlVal;

    //For each value of ITHL we acquire one image
    for (ithlVal=ithl_max; ithlVal>=ithl_min; ithlVal--){

        if(m_xpad_cmd->getAbortFlag())
            return 1;

        //Upload Config G
        if (m_xpad_cmd->loadConfigG(moduleMask,ITHL,ithlVal,0x7F) == NULL){
            return -1;
        }

        cout << "Acquiring image for ITHL= " << ithlVal << endl;

        //Expose and Read
        //  unsigned int **img = this->Expose();
        unsigned int *img = m_xpad_cmd->getOneImage(moduleMask);

        int index = ithlVal - ithl_min;

        if(img!=NULL){

            //Set name for ITHL scan files
            char fname[200];
            sprintf (fname, "%s/ITHL_%d.dat", fpath, ithlVal);
            ofstream file(fname, ios::out);

            //Scan each image and count the pixels with noise
            for (unsigned int row=0;row<120*moduleNumber;row++) {
                for (unsigned int column=0;column<(560);column++){

                    //Writting a buffer containting the 64 read images
                    // buffer[index*LineNumber*ColumnNumber + (row*(ColumnNumber)+column)] = img[0][row*(ColumnNumber)+column];
                    buffer[index*120*560*moduleNumber + (row*(560)+column)] = img[row*(560)+column];

                    //Writing to disk the ITHL scan image
                    // file << img[0][row*(ColumnNumber)+column] << " ";
                    file << img[row*(120)+column] << " ";
                }
                file << endl;
            }
            file.close();
            // delete[] img[0];
            delete[] img;
        }
        else{
            fflush(stdout);
            delete[] buffer;
            return -1;
        }
    }// end of loop : scan ITHL


    int derive1 [ithl_max-ithl_min];
    int derive2 [ithl_max-ithl_min];

    unsigned short *ITH = new unsigned short[120*560*moduleNumber];
    for (unsigned int row=0;row<120*moduleNumber;row++) {
        for (unsigned int column=0;column<(560);column++){
            unsigned short ithlValue=ithl_min;

            //Detect the noise peak (only count above 5 in order not to be affected by cosmics)
            for (unsigned short ithlVal=ithl_max; ithlVal>=ithl_min; ithlVal--){
                int index = ithlVal - ithl_min;
                //  cout << "_FRED__   ithl_max-ithl_min-index = " << ithl_max-ithl_min-index << endl;
                if (buffer[(index)*120*560*moduleNumber + (row*(560)+column)] < 50)
                    buffer[(index)*120*560*moduleNumber + (row*(560)+column)] = 0;
            }

            //Detect the noise peak (only count above 5 in order not to be affected by cosmics)
            for (unsigned short ithlVal=ithl_max; ithlVal>=ithl_min; ithlVal--){
                int index = ithlVal - ithl_min;
                //  cout << "_FRED__   ithl_max-ithl_min-index = " << ithl_max-ithl_min-index << endl;
                if (index > 0 && index < NumFiles-1)
                    derive1[ithl_max-ithl_min-index] = buffer[(index-1)*120*560*moduleNumber + (row*(560)+column)] - buffer[(index+1)*120*560*moduleNumber + (row*(560)+column)];
                else// if (index == 0 || index == NumFiles-1)
                    derive1[ithl_max-ithl_min-index] = 0;
            }

            //Detect the noise peak (only count above 5 in order not to be affected by cosmics)
            for (unsigned short index=0; index<ithl_max - ithl_min; index++){
                if (index > 0 && index <  NumFiles-1)
                    derive2[index] = derive1[index + 1] - derive1[index - 1];
                else //if (index == 0 || index == ithl_max-ithl_min-1)
                    derive2[index] = 0;
            }

            for (unsigned short index=0; index<ithl_max - ithl_min; index++){
                if(derive2[index] < -1){
                    ithlValue = ithl_max - index;
                    break;
                }
            }
            //cout << endl;
            //Choose the DACL value
            //if(column == 279 && row == 72)
            //    cout << "\n x=" << column << " y=" << row << " ithlValue=" << ithlValue << " derive_value=" << diff_max << "\n";
            ITH[(row*(560)+column)] = ithlValue;
        }
    }



    if(buffer != NULL)
        delete[] buffer;

    char fname[200];
    //Save result in file
    sprintf (fname, "%s/ITHL_Matrix_Base.dat",fpath);
    cout << "Writting results in: " << fname << endl;
    ofstream file(fname, ios::out);
    for (unsigned short row=0;row<120*moduleNumber;row++){
        for (unsigned short column=0;column<(560);column++)
            file << ITH[row*(560)+column] << " ";
        file << endl;
    }
    file.close();

    unsigned int sum=0, count=0;
    double mean;
    //Mean Value of ITHL for each chip of every module
    for (unsigned short i=0; i<moduleNumber; i++)
    {
        for (unsigned short j=0; j<7; j++){
            //cout << "chip= " << j << endl;
//            for (unsigned short row=i*120; row<(i+1)*120;row++){
//                for (unsigned short column=j*80; column<(j+1)*80; column++){
//                    //cout << "chip= " << j << " row= " << row << " column= " << column << endl;
//                    if (ITH[i*560*120+ row*(560)+column]>ithl_min){
//                        sum += ITH[i*560*120 + row*(560)+column];
//                        count++;
//                    }
//                }
//            }
            for(unsigned short row=0;row<120;row++){
                for(unsigned short col=0;col<80;col++){
                    if (ITH[i*560*120 +j*80 + row*(560)+col]>ithl_min){
                        sum += ITH[i*560*120 + j*80 + row*(560)+col];
                        count++;
                    }
                }
            }

            if (count > 0)
                mean = sum / count;
            else
                mean = 0;
            if(m_xpad_cmd->getAbortFlag())
                return 1;

            moduleMask = 0x01<<i;
            chipMask = 0x01<<j;
            cout << "module=" << i << " chip=" << j+1 << " ITHLmean=" << mean << endl;
            if (m_xpad_cmd->loadConfigG(0x01<<i,ITHL,(unsigned short)mean,chipMask) == NULL){
                delete[] ITH;
                chipMask = chipMask_original;
                moduleMask = moduleMask_original;
                return -1;
            }
            sum = 0;
            count = 0;
        }
    }
    delete[] ITH;
    chipMask = chipMask_original;
    moduleMask = moduleMask_original;
    return 0;

}







//Function to process ITHL data
/**
 * @brief Calibration::scanITHL_v2
 * @param Texp
 * @param ithl_min
 * @param ithl_max
 * @param fpath
 * @param moduleMask
 * @return
 */
int Calibration::scanITHL_v2(unsigned int Texp, unsigned short ithl_min, unsigned short ithl_max, char *fpath, unsigned short moduleMask){
    //Function to process ITHL data
    //unsigned short modNb = moduleNumber;
    //unsigned short chipNb = chipNumber;

    unsigned short moduleMask_original = moduleMask;
    unsigned short chipMask_original = 0x7f;
    unsigned short chipMask;

    unsigned short moduleNumber = m_tools.getNbModMask(moduleMask);

    //Configure exposure parameters (images read in 16bit format)
    if (m_xpad_cmd->exposeParam(moduleMask,1,Texp,4000,0,0,9000,0,0) == -1)
        return -1;

    //upload DACL=32 (32*8+1=257)
    if (m_xpad_cmd->loadFlatConfigL(moduleMask,257) == -1)
        return -1;

    //Buffer containting the NumFiles read images
    int NumFiles = ithl_max - ithl_min + 1;
    unsigned short *buffer = new unsigned short [120*560*moduleNumber*NumFiles];
    unsigned short ithlVal;

    //For each value of ITHL we acquire one image
    for (ithlVal=ithl_max; ithlVal>=ithl_min; ithlVal--){

        if(m_xpad_cmd->getAbortFlag())
            return 1;

        //Upload Config G
        if (m_xpad_cmd->loadConfigG(moduleMask,ITHL,ithlVal,0x7F) == NULL){
            return -1;
        }

        cout << "Acquiring image for ITHL= " << ithlVal << endl;

        //Expose and Read
        //  unsigned int **img = this->Expose();
        unsigned int *img = m_xpad_cmd->getOneImage(moduleMask);

        int index = ithlVal - ithl_min;

        if(img!=NULL){

            //Set name for ITHL scan files
            char fname[200];
            sprintf (fname, "%s/ITHL_%d.dat", fpath, ithlVal);
            ofstream file(fname, ios::out);

            //Scan each image and count the pixels with noise
            for (unsigned int row=0;row<120*moduleNumber;row++) {
                for (unsigned int column=0;column<(560);column++){

                    //Writting a buffer containting the 64 read images
                    // buffer[index*LineNumber*ColumnNumber + (row*(ColumnNumber)+column)] = img[0][row*(ColumnNumber)+column];
                    buffer[index*120*560*moduleNumber + (row*(560)+column)] = img[row*(560)+column];

                    //Writing to disk the ITHL scan image
                    // file << img[0][row*(ColumnNumber)+column] << " ";
                    file << img[row*(560)+column] << " ";
                }
                file << endl;
            }
            file.close();
            // delete[] img[0];
            delete[] img;
        }
        else{
            delete[] buffer;
            return -1;
        }
    }// end of loop : scan ITHL


    unsigned short *ITH = new unsigned short[120*560*moduleNumber];
    unsigned short ITHL_Value[7*moduleNumber];
    bool           ITHL_flag[7*moduleNumber];

    for(int i =0;i<7*moduleNumber;i++){
        ITHL_Value[i] = ithl_min;
        ITHL_flag[i] = true;
    }

    for (unsigned int row=0;row<120*moduleNumber;row++) {
        for (unsigned int column=0;column<(560);column++){
            ITH[row*560+column] = 0;
        }

    }

    for (unsigned short ithlVal=ithl_max; ithlVal>=ithl_min; ithlVal--){
        int index = ithlVal - ithl_min;
        int sum;

        for (unsigned int row=0;row<120*moduleNumber;row++) {
            for (unsigned int column=0;column<(560);column++){
                ITH[(row*(560)+column)] += buffer[(index)*120*560*moduleNumber + (row*(560)+column)];
            }
        }
        for(int mod=0;mod<moduleNumber;mod++){
            for(int chip=0;chip<7;chip++){
                sum=0;
                for (unsigned int row=0;row<120;row++) {
                    for (unsigned int column=0;column<(80);column++){
                        if(ITH[(mod*560*120+ row*(560)+chip*80+column)]> 50)
                            sum++;
                    }
                }
                if(ITHL_flag[7*mod + chip]){
                    if(sum > 4800){
                        ITHL_flag[7*mod + chip] = false;
                        ITHL_Value[7*mod + chip] = ithlVal;
                    }
                }
            }
        }
    }

    delete[] buffer;

    //Mean Value of ITHL for each chip of every module
    for (unsigned short i=0; i<moduleNumber; i++){
        cout << "ITHL Module < " << i+1 << " > " ;
        for (unsigned short j=0; j<7; j++){
            unsigned short value = ITHL_Value[j+i*7];
            moduleMask = 0x01<<i;
            chipMask = 0x01<<j;
            unsigned short *ret = m_xpad_cmd->loadConfigG(moduleMask,ITHL,value,chipMask);
            if (ret == NULL){
                delete[] ITH;
                chipMask = chipMask_original;
                moduleMask = moduleMask_original;
                return -1;
            }
            else delete[] ret;
            printf("%d ",value);
        }
        cout << endl;
    }
    delete[] ITH;
    chipMask = chipMask_original;
    moduleMask = moduleMask_original;
    return 0;
}


/**
 * @brief Calibration::processScanDACL_Beam
 * @param moduleMask
 * @param fpath
 * @return
 */
int Calibration :: processScanDACL_Beam(unsigned short moduleMask, char * fpath){

    int nbMod = m_tools.getNbModMask(moduleMask);
    unsigned int **images = new unsigned int*[64];
    int otnCalibration = 0;
    int retScanf=0;
    for(int i =0;i<64;i++)
        images[i] = new unsigned int [560*120*nbMod];

    unsigned short *daclvalue= new unsigned short[560*120*nbMod];

    for(int i =0;i<64;i++){
        QString fname = fpath;
        fname += "/DACL_" + QString::number(i) + ".dat";

        FILE *fd = fopen(fname.toStdString().c_str(),"r");
        if(fd == NULL){
            printf("Can not open file < %s > \n",fname.toStdString().c_str());
            break;
        }
 //       cout<< "open file = " << fname.toStdString() << endl;
        for(int j=0;j<560*120*nbMod;j++){
            retScanf = fscanf(fd,"%u",&images[i][j]);
        }
        fclose(fd);
    }

    for(int row=0;row<120*nbMod;row++){
        for(int col=0;col<560;col++){
            daclvalue[row*560+col] = Scurve_beam(images,row,col,&otnCalibration);
            if(m_xpad_cmd->getAbortFlag())
                return 1;
        }
    }

    cout<< "LibXpadUSB : " << __func__ <<"() \t"<<"Numbers of OTN Calibration =  "<< otnCalibration <<endl;

    for(int i =0;i<64;i++)
        delete [] images[i];
    delete[] images;

    char fname[200];
    //Save result in file
    sprintf (fname, "%s/DACL_Matrix_Base.dat",fpath);
    cout << fname << endl;
    ofstream file(fname, ios::out);
    for (int row=0;row<120*nbMod;row++){
        for (int column=0;column<560;column++){
            daclvalue[row*(560)+column]= daclvalue[row*560+column]*8+1;
            file << daclvalue[row*(560)+column] << " ";
        }
        file << endl;
    }
    file.close();

    //Uploading Scanned DACL values into detector
    unsigned short values[80];
    for(int mod=0;mod<nbMod;mod++){
        for (int row=0; row<120; row++) {
            for (int column=0; column<560; column++){
                unsigned short chip = column/80;

                if(m_xpad_cmd->getAbortFlag()){
                    delete[] daclvalue;
                    return 1;
                }

                //Values are being accumulated in packages of 80 data
                values[column - chip*80] = daclvalue[mod*120*560 + row*560+column];

                //Once 80 values accumulated, they are sended to the SRAM of the detector
                if ((column - chip*80) == 79){

                    //Sending 80 values to the SRAM in the detector
                    if (m_xpad_cmd->saveConfigLToSRAM(1<<mod,chip,row,values) == -1){
                        if (DEBUG) cout<< "LibXpadUSB : " << __func__ <<"() \t" << "ERROR: ConfigL data could not be send to module: " << moduleMask << " chip:" << chip << " row: " << row << endl;
                        delete[] daclvalue;
                        return -1;
                    }
                }
            }
        }
    }
    //Uploading DACL to detector
    if (m_xpad_cmd->loadConfigLSRAMToDetector(moduleMask) == -1){
        delete[] daclvalue;
        return -1;
    }
    delete[] daclvalue;
    return 0;
}

/**
 * @brief Calibration::Scurve_beam
 * @param data
 * @param cord_x
 * @param cord_y
 * @param count
 * @return
 */

//int Calibration:: Scurve_beam(unsigned int **data,int cord_x,int cord_y, int* count){
//    int max=0;
//    int min_derive=0;
//    int max_derive=0;
//    // generate some data:

//    int val;

//    int derive[64];
//    int derive2[64];
//    int cordo = cord_x*560 + cord_y;
//    int firstval=0;
//    int flag=0;
//    int img[64];

//    for(int ii=0;ii<64;ii++)
//        img[ii]=data[ii][cordo];

//    for (int i=0; i<64; ++i)
//    {
//        if(img[i] <= 50)
//            img[i] = 0;

//        if(data[i][cordo] > 5 && flag == 0){
//            flag = 1;
//            if(i>4)
//                firstval = i-4;
//            else firstval = 0;
//        }

//        if(img[i] > max)
//            max = img[i];


//        if(i==0)derive[i] = 0;
//        else if(i==63) derive[i] = 0;
//        else{
//            //derive[i] = images[i+1][cordo] - images[i][cordo];
//            derive[i] = 0;
//            derive[i] = img[i+1] - img[i-1];
//        }

//        if(derive[i] < min_derive)
//            min_derive = derive[i];
//        if(derive[i] > max_derive)
//            max_derive = derive[i];
//    }


//    derive2[0]=0;
//    derive2[1]=0;
//    derive2[62]=0;
//    derive2[63]=0;
//    for(int i=2;i<62;i++){
//        derive2[i]=0;
//        derive2[i] = derive[i+1]-derive[i-1];

//    }

//    val = 0;
//    for(int dacl=0;dacl<63;dacl++){
//        if(derive2[dacl]<-10){
//            val = dacl-1;
//            break;
//        }
//    }

//    if(BEAMCalibrationVerification(img) == 0){
//        *count = (*count) + 1;
//        // cout << "Val = " << val << " firstval = " << firstval << endl;
//        for(int j = 0;j<15;j++){
//            if(firstval > 1){
//                if(derive[firstval] != 0){
//                    val= firstval;
//                    firstval--;
//                }
//                else{
//                    val= (firstval-2);
//                    break;
//                }
//            }
//            else{
//                val = 0;
//                break;
//            }
//        }
//    }
//    return val;
//}


int Calibration:: Scurve_beam(unsigned int **data,int cord_x,int cord_y, int* count){
    int cordo = cord_x*560 + cord_y;
    int img[64];




    int max=0;
    int min_derive=0;
    int max_derive=0;
    // generate some data:

    int val;

    int derive[64];
    int derive2[64];
    //int cordo = cord_x*560 + cord_y;
    int firstval=0;
    int flag=0;
    int scandacl[64];
    int dacl=0;

    int TH = 3;


    for(int ii=0;ii<64;ii++)
        img[ii]=data[ii][cordo];

    for(int ii=0;ii<64;ii++)
        scandacl[ii]=img[ii];

    for (int i=0; i<64; ++i)
    {
        if(img[i] < 50)
            img[i] = 0;

        if(img[i] > 5 && flag == 0){
            flag = 1;
            if(i>4)
                firstval = i-2;
            else firstval = 0;
        }

        if(img[i] > max)
            max = img[i];


        if(i==0)derive[i] = 0;
        else if(i==63) derive[i] = 0;
        else{
            //derive[i] = images[i+1][cordo] - images[i][cordo];
            derive[i] = 0;
            derive[i] = img[i+1] - img[i-1];
        }

        if(derive[i] < min_derive)
            min_derive = derive[i];
        if(derive[i] > max_derive)
            max_derive = derive[i];
    }


    derive2[0]=0;
    derive2[1]=0;
    derive2[62]=0;
    derive2[63]=0;
    for(int i=2;i<62;i++){
        derive2[i]=0;
        derive2[i] = derive[i+1]-derive[i-1];

    }

    val = 0;
    int flagDacl=1;
    int maxSCurve=0;
    int midSCurve=0;
    for(dacl=0;dacl<63;dacl++){
        if(flagDacl==1){
            if(derive2[dacl]<-10){
                val = dacl-1;
                flagDacl= 0;
            }
        }
        else{
            if(derive2[dacl]>10){
                maxSCurve = img[dacl];
                midSCurve = img[val+1];
                break;
            }
        }
    }

    int par[3];
    par[0]=maxSCurve;
    par[1]=midSCurve;
    par[2]=midSCurve/2.8;

    float fitSCurve[64];

    for(dacl=0;dacl<64;dacl++){
        if((par[1] == 0)  || (par[0] == 0)){
            fitSCurve[dacl] = 0;
        }
        else
            fitSCurve[dacl] = erf_f(&img[dacl],par);;
    }

    int maxPla=0;
    int plat = 0;


    for(int dac=0;dac<61;dac++){
        if(fitSCurve[dac]>100 )
        {
            if((fitSCurve[dac] - fitSCurve[dac+1]) < 3)
                plat++;
            else
                plat = 0;
        }
        if(plat > maxPla)
            maxPla = plat;
    }

    if(maxPla>TH){
        val = val;
    }
    else{
        *count = (*count)+1;
        for(int j = 0;j<15;j++){
            if(firstval > 3){
                if(derive[firstval] != 0){
                    val= firstval;
                    firstval--;
                }
                else{
                    val= firstval-4;
                    break;
                }
            }
            else{
                val = 0;
                break;
            }
        }
    }
    return (val);
}


/**
 * @brief Calibration::BEAMCalibrationVerification
 * @param Buff
 * @return
 */
int Calibration:: BEAMCalibrationVerification(int *Buff){

    int SmoothHeight = 0;
    int SmoothWidth = 0;
    int SmoothArea = 0;
    int SmoothXmin = 64;
    int SmoothXmax = 0;
    float Mean = 0;
    float threshold = 0.6;
    int count = 0;

    int pointsNumber = 10;

    int limit = pointsNumber/2;

    for (int DACL=0; DACL<64; DACL++){
        Mean = 0;
        count = 0;
        for (int i=-limit; i<limit+1; i++){
            if ((DACL+i) >= 0 && (DACL+i) < 64){
                Mean += Buff[DACL+i];
                count ++;
            }
        }
        if (count > 0)
            Mean = Mean/count;
        else
            Mean = 0;

        if (Mean > SmoothHeight)
            SmoothHeight = Mean;

        if (Mean > 50){
            SmoothArea += Mean;
            if (DACL < SmoothXmin)
                SmoothXmin = DACL;
            if (DACL >= SmoothXmax)
                SmoothXmax = DACL;
        }
    }

    SmoothWidth = SmoothXmax - SmoothXmin;

    //    cout << "Estimating Rectangularity" << endl;
    //    cout << "Range[" << SmoothXmin << ", " << SmoothXmax << "]" << endl;
    //    cout << "Width = " << SmoothWidth << " Height = " << SmoothHeight << endl;

    float Rect;

    if ((SmoothHeight*SmoothWidth) > 0)
        Rect = (float)SmoothArea /(SmoothHeight*SmoothWidth);
    else
        Rect = 0;

    cout << "Rectangularity = "  << Rect << endl;;

    if (Rect <= threshold)
        return 1;
    else
        return 0;
}


float Calibration :: erf_f(int *x,int *par)
{
    float arg = 0,arg0,denom;
    float fitval;
    // par[0]= valeur du max
    // par[1]= valeur du seuil
    // par[2]= ecart type10000
    arg = (x[0] - par[1])/par[2]; // arg est la valeur de la variable rÃ©duite
    arg0=arg;
    if(arg0<0) arg=-arg;
    denom =(1+ 0.278393*arg+0.230389*arg*arg+ 0.000972*arg*arg*arg+0.078108*arg*arg*arg*arg);
    fitval=1/(denom*denom*denom*denom);
    fitval=1-fitval;
    if(arg0<0)fitval=-fitval;
    fitval=(1+fitval)*par[0]/2;
    return fitval;
}
