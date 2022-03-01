#ifndef DEFINES_H
#define DEFINES_H

#if defined ( WIN32 )
#define __func__ __FUNCTION__
#endif

/***********************************************************************************
// FOR LIMA
************************************************************************************/

/*     TYPE OF SYTEM     */
#define XPAD_S10                    1
#define XPAD_C10                    2
#define XPAD_A10                    3
#define XPAD_S70                    4
#define XPAD_C70                    5

#define XPAD_S140                   6
#define XPAD_C140                   7

#define XPAD_S210                   8


#define XPAD_S270                   9
#define XPAD_C270                   10

#define XPAD_S340                   11



#define RAWFILE "/opt/imXPAD/tmp/burst_%d_img_%d.bin"


/*     TYPE OF CHIPS     */
#define XPAD_31                     0
#define XPAD_32                     1

/* IMAGE POST PROCESSING */
#define IMG_POSTPROC_GEOM           0x00000001
#define IMG_POSTPROC_DEAD           0x00000002

enum DATA_TYPE {IMG, CONFIG};
enum IMG_TYPE  {B2,B4};

#define LineNumber 120
#define ColumnNumber 560

/***********************************************************************************
// USB Constants definition
************************************************************************************/
//register address
#define REG_FIFO_TX		0x8080
#define REG_CLR_FIFO_RX	0x8091
#define REG_CLR_FIFO_TX	0x8091
#define REG_SND_TLK		0x8091
#define REG_FIFO_RX		0x8000
#define REG_RX_COUNT	0x8008
#define REG_RX_EMPTY    0x8010
#define REG_TX_COUNT	0x8009
#define REG_3			0x800A
#define REG_4			0x800B

//register values
#define VAL_CLR_RX		0x02
#define VAL_CLR_TX		0x04
#define VAL_SND_TLK		0x08

//definition of different receiving message length
#define ACK_COUNT_NUMBER	16
//#define IMG2B_COUNT_NUMBER	566
//#define IMG4B_COUNT_NUMBER	1126

//definition of number of line for different message type
#define NO_LINE		0
#define ACK_LINE	1
#define IMG_LINE	120
#define IMG_COLUMN 	80
//#define IMG_NUMBER 	560

/***********************************************************************************
// USB Commands definition
************************************************************************************/

//definition of ack components
#define ACK_HEADER	0xAA55
#define ACK_TRAILER	0xF0F0
#define ACK_SIZE	0x000D

//definition of command
#define CMD_RESET		0x0001
#define CMD_ABORT		0x0002
#define CMD_ASKREADY	0x0101
#define CMD_AUTOTEST	0x0102
#define CMD_CFG_G		0x0103
#define CMD_FLAT_CFG	0x0104
#define CMD_READ_CFG_G	0x0105
#define CMD_READ_TEMP   0x0108
#define CMD_A_CFG_G		0x0203
#define CMD_IMG_2B		0x0182
#define CMD_IMG_4B		0x0183
#define CMD_READ_CFG_L	0x01C0
#define CMD_SAVE_CFG_L	0x0380
#define CMD_SAVE_CFG_G	0x0381
#define CMD_LOAD_CFG_L	0x0480
#define CMD_EXPOSE		0x0140
#define CMD_EXPOSE_PAR	0x0190
#define CMD_ACQUIRE		0x0141
#define CMD_READ_ADC    0x0500
#define CMD_SET_HV      0x0510
#define CMD_SET_HVPARAM 0x0511
#define CMD_DIAG        0x0520
#define CMD_TESTPULSE   0x0106

//definition of command ack
#define ACK_ASKREADY	0x1101
#define ACK_AUTOTEST	0x1102
#define ACK_CFG_G		0x1103
#define ACK_FLAT_CFG	0x1104
#define ACK_READ_CFG_G	0x1105
//#define ACK_A_CFG_G		0x1203
#define ACK_SAVE_CFG_L	0x1380
//#define ACK_SAVE_CFG_G		0x1381
#define ACK_LOAD_CFG_L	0x1480
#define ACK_EXPOSE		0x1140
#define ACK_EXPOSE_PAR	0x1190
#define ACK_READ_ADC    0x1500
#define ACK_SET_HV      0x1510
#define ACK_SET_HVPARAM 0x1511
#define ACK_TESTPULSE   0x1106
#define ACK_READ_TEMP   0x1108


/***********************************/
// REG REGISTER

#define REG_CMD_SIZE    0
#define REG_IMG_2B      1
#define REG_IMG_4B      2


/***********************************************************************************
// Constants definition
************************************************************************************/
//#define NB_WORDS_REPLY           0xD

/* messages tags */
#define HUB_HEADER               0xBB44
#define HUB_MESSAGE              0xCCCC
#define MOD_MESSAGE              0x3333
#define MOD_HEADER               0xAA55
#define MSG_TRAILER              0xF0F0

/* Global Registers */
#define AMPTP                   31
#define IMFP                    59
#define IOTA                    60
#define IPRE                    61
#define ITHL                    62
#define ITUNE                   63
#define IBUFF                   64

#define SLOW                    0
#define MEDIUM                  1
#define FAST                    2

#define TRIG_SINGLE_BUNCH       6

/*  Acquisition Mode    */
#define EXPOSE_STANDARD                0    // image 16 or 32 bits          AQC mode = 0
#define EXPOSE_LOCALBURST              1    // image 12 bits 980 images max AQC mode = 1
#define EXPOSE_BURST                   2    // image 12 bits                AQC mode = 2
#define EXPOSE_STACKING_16             3    // image 16 bits                AQC mode = 3 and Format =0
#define EXPOSE_STACKING_32             4    //image  32 bits                AQC mode = 3 and Format =1
#define EXPOSE_SINGLE_BUNCH_16         5    // image 16 bits                AQC mode = 4 and Format =0
#define EXPOSE_SINGLE_BUNCH_32         6    // image 32 bits                AQC mode = 4 and Format =1
#endif // DEFINES_H
