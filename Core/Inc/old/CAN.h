
// uCAN.h

#include "C8051F580.h" // SFR & CAN regs declarations
#include "stdvmc.h"

// #include "compiler_defs.h"

#define _1ST_OUT_MSG (N_FILTERS + 1) // = 16
#define LAST_OUT_MSG 32
#define N_OUT_MSGS (LAST_OUT_MSG - N_FILTERS) // = 17
// #define N_OUT_MSGS		(3) // = 17

#define MID_TABLE_LENGTH (sizeof(midTable))          // 9
#define FAULT_TABLE_LENGTH (sizeof(searchIndex) / 4) // 264
#define GEN_FAULT_LENGTH (sizeof(genFault) - 1)      // 32

SBIT(TEST_PIN, P2, 7); // TX ENABLE ACTIVE HIGH//TEST+++

// set up the filters we use just the upper 8 bits of the 11 bit ID
// The ID's are commands in the Raymond protocol.

#define N_FILTERS 9 // # of filters
const unsigned int code CanFilters[] = {
    // TEST +++
    //+	0x00F0 << 5,	 // InitDBCmd
    //+	0x00F0 << 5,	// Download command

    //	0x00FF << 5,	 // InitDBCmd for test only
    //	0x0000 << 5,	 // InitDBCmd for test only
    // TEST ---

    //+	0x00F0 << 5,	 // End Database download command
    //+	0x00F0 << 5,	// display background command
    //+	0x00F0 << 5,	// display text message
    //+	0x00F0 << 5,	// display BitMap Command
    //+	0x00F0 << 5,	// display group command
    //+	0x00F0 << 5,	// send version command
    //+	0x00F0 << 5,	// system failure message
    //+	0x00F0 << 5,	// adjust brightness command
    //+	0x00F0 << 5	 //AlarmCmd	equ	08ah Must be 8bh for testing !!!
    // 5-5-98 Alarm output control command

    //	0x00FF << 5,	 // InitDBCmd
    //	0x00FF << 5,	// Download command

    //	0x00FF << 5,	 // InitDBCmd for test only
    //	0x0000 << 5,	 // InitDBCmd for test only
    // TEST ---

    //	0x00FF << 5,	 // End Database download command
    //	0x00FF << 5,	// display background command
    //	0x00FF << 5,	// display text message
    //	0x00FF << 5,	// display BitMap Command
    //	0x00FF << 5,	// display group command
    //	0x00FF << 5,	// send version command
    //	0x00FF << 5,	// system failure message
    //	0x00FF << 5,	// adjust brightness command
    //	0x00FF << 5	 //AlarmCmd	equ	08ah Must be 8bh for testing !!!
    // 5-5-98 Alarm output control command

    // 0x1FFC is the 11 bits of the identifier
    0x1FFC, // display background command
    0x1FFC, // display text message
    0x1FFC, // display BitMap Command
    0x1FFC, // display group command
    0x1FFC, // send version command
    0x1FFC, // system failure message
    0x1FFC, // adjust brightness command
    0x1FFC, // AlarmCmd	equ	08ah Must be 8bh for testing !!! 5-5-98 Alarm
            // output control command
    0x1FFC  // this is a command which is not used by the display but prevents
            // timeouts when no data is being displayed

};

const unsigned int code
    CanIds[] = // acceptable ID's (used in Arbitration Register)
    {
        //-	0x0080 << 5, 	// InitDBCmd
        //-	0x0081 << 5,	// Download command
        //-	0x0082 << 5, 	// End Database download command
        //-	0x0083 << 5,	// display background command
        //-	0x0084 << 5,	// display text message
        //-	0x0085 << 5,	// display BitMap Command
        //-	0x0086 << 5,	// display group command
        //-	0x0087 << 5,	// send version command
        //-	0x0088 << 5,	// system failure message
        //-	0x0089 << 5,	// adjust brightness command
        //-	0x008A << 5		//AlarmCmd	equ	08ah Must be 8bh
        // for testing !!! 5-5-98 Alarm output control command
        // To convert the Raymond command number to an 11 BIT ID:
        // High byte of message ID = 0x84 >> 3 | 0xA0 = 0xB0
        // Low byte of message ID = 0x84 << 5 = 0x80
        // 0xB080h
        // Upper 3 bits = MSGVal, Xtd, Dir, so they are not part of the address
        // 0x1080
        // Shift the result right by two to get the final alignment of the
        // address.
        // 0x0420h
        // 0x84 = 0x0420h

        //	0x0420 << 2, 	// InitDBCmd
        //	0x0420 << 2,	// Download command
        //	0x0420 << 2, 	// End Database download command
        0x0418 << 2, // 0x83 display background command
        0x0420 << 2, // 0x84 display text message
        0x0428 << 2, // 0x85 display BitMap Command
        0x0430 << 2, // 0x86 display group command
        0x0438 << 2, // 0x87 send version command
        0x0440 << 2, // 0x88 system failure message
        0x0448 << 2, // 0x89 adjust brightness command
        0x0450 << 2, // 0x8A AlarmCmd	equ	08ah Must be 8bh for testing !!!
                     // 5-5-98 Alarm output control command

        0x0416 << 2 // what is this?? - probably an idle msg
};

///
extern long sysTime;
//
extern bool bCanOn;
extern bool bCanError;
extern bool bAddrsRequested;
extern bool bSendAddrsRequest;
extern long lSNndMC;
extern int8 nLastSentTxMsg;
extern pdata BOOL bPendTxMsg[N_OUT_MSGS];
extern pdata brp;
extern pdata unsigned char BdRate;
extern xdata unsigned char CanTxQ[10]; // ID, number of bytes and 8 data bytes.
extern xdata CanRxQ;
extern pdata CanRxInPtr;

void readConfig();
void writeConfig();
void initConfig();
void sendConfig();
void parseCommands();

//;;void setCanBitrate( int8 iBrp);
void setCanBitrate();
void clearCanMsgObjects();
void initCanFilter(int8 iMsg);
void startCan();
void SaveCanMsg(int8 MsgNum);
void sendCanMsg();
void CANTX();

// for 250k-1M bps:
// setBitrate(BRP-1);//(1000/BRP) kbps
//
// #define SET_BITRATE(iBR)  setCanBitrate( BRPS[iBR] )
//
// SET_BITRATE(0) = setBitrate(3) // 250K = std j1939
// SET_BITRATE(1) = setBitrate(2) // 333K
// SET_BITRATE(2) = setBitrate(1) // 500K
// SET_BITRATE(3) = setBitrate(0) // 1M

/* CAN0CN 0xC0 availible only when SFRPAGE = CAN0_PAGE.  CAN Control Register
 * Page 14 of Bosch CAN manual*/
sbit CANINIT = CAN0CN ^ 0; /* CAN Initialization bit */
sbit CANIE = CAN0CN ^ 1;   /* CAN Module Interrupt Enable Bit */
sbit CANSIE = CAN0CN ^ 2;  /* CAN Status change Interrupt Enable Bit */
sbit CANEIE = CAN0CN ^ 3;  /* CAN Module Error Interrupt Enable Bit */
sbit CANIF = CAN0CN ^ 4;   /* CAN Module Interrupt Flag */
sbit CANDAR = CAN0CN ^ 5;  /* CAN Disable Automatic Retransmission bit */
sbit CANCCE = CAN0CN ^ 6;  /* CAN Configuration Change Enable bit */
sbit CANTEST = CAN0CN ^ 7; /* CAN Test Mode Enable bit */

// In the C8051F047 (original code base used) the CAN0STAT register was bit
// accessable this is how the bits were mapped which is the same as in the
// C8051F580 which is not bit addressable Here is are the masks used to check
// each bit inthe CAN0STAT register.

#define BOFF_MASK                                                              \
  0x80 // sbit BOFF   = CAN0STA ^ 7;          /* Bus Off Status */
#define EWARN_MASK                                                             \
  0x40 // sbit EWARN  = CAN0STA ^ 6;          /* Warning Status */
#define EPASS_MASK                                                             \
  0x20 // sbit EPASS  = CAN0STA ^ 5;          /* Error Passive */
#define RXOK_MASK                                                              \
  0x10 // sbit RXOK   = CAN0STA ^ 4;          /* Received Message Successfully
       // */
#define TXOK_MASK                                                              \
  0x08 // sbit TXOK   = CAN0STA ^ 3;          /* Transmit a Message Successfully
       // */
#define LEC2_MASK                                                              \
  0x04 // sbit LEC2   = CAN0STA ^ 2;          /* Last error code bit 2 */
#define LEC1_MASK                                                              \
  0x02 // sbit LEC1   = CAN0STA ^ 1;          /* Last error code bit 1 */
#define LEC0_MASK                                                              \
  0x01 // sbit LEC0   = CAN0STA ^ 0;          /* Last error code bit */
#define RxOk 0x10 // Receive Message Successfully
#define TxOk 0x08 // Transmitted Message Successfully
