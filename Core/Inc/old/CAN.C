// uCAN.c

//#define IMPLEMENT_GLOBAL_VARS

#include "CAN.h"

// defaults:


//
// global vars:
//

// In the C8051F047 the CAN0STAT register was bit accessable
// this is how the bits were mapped which is the same as in the C8051F580 which is not bit addressable

/* CAN0STAT NOT BIT ADDRESSABLE*/
//sbit BOFF   = CAN0STA ^ 7;          /* Bus Off Status                  */
//sbit EWARN  = CAN0STA ^ 6;          /* Warning Status                  */
//sbit EPASS  = CAN0STA ^ 5;          /* Error Passive                   */
//sbit RXOK   = CAN0STA ^ 4;          /* Received Message Successfully   */
//sbit TXOK   = CAN0STA ^ 3;          /* Transmit a Message Successfully */
//sbit LEC2   = CAN0STA ^ 2;          /* Last error code bit 2           */
//sbit LEC1   = CAN0STA ^ 1;          /* Last error code bit 1           */
//sbit LEC0   = CAN0STA ^ 0;          /* Last error code bit            */

// CAN0CN is bit addressable is the C8051F580
/* CAN0CN 0xC0 availible only when SFRPAGE = CAN0_PAGE.  CAN Control Register Page 14 of Bosch CAN manual*/
//sbit CANINIT = CAN0CN ^ 0;         /* CAN Initialization bit */
//sbit CANIE   = CAN0CN ^ 1;         /* CAN Module Interrupt Enable Bit */
//sbit CANSIE  = CAN0CN ^ 2;         /* CAN Status change Interrupt Enable Bit */
//sbit CANEIE  = CAN0CN ^ 3;         /* CAN Module Error Interrupt Enable Bit */
//sbit CANIF   = CAN0CN ^ 4;         /* CAN Module Interrupt Flag */
//sbit CANDAR  = CAN0CN ^ 5;         /* CAN Disable Automatic Retransmission bit */
//sbit CANCCE  = CAN0CN ^ 6;         /* CAN Configuration Change Enable bit */
//sbit CANTEST = CAN0CN ^ 7;         /* CAN Test Mode Enable bit */

// CAN:
global bool	bCanOn = false;
global pdata BOOL bPendTxMsg[ N_OUT_MSGS] = {false}; // xmission pending flag array

	
void CANTX ()
{


	SFRPAGE = CAN0_PAGE;	// ADDED OLED VERSION
	if (bCanOn)
	{
	if (CANINIT) // Gets set by CAN controller in BusOff condition. 
		{
			CANINIT = 0; // restart CAN 
		}
//;;	if (!BOFF) // for CAN STATUS RECOVERY: intrpt should be re-enabled after BusOFF recovery
		if(!(CAN0STAT & BOFF_MASK))
			CANSIE = 1;

	}
	else//if (!bCanOn)
	{
		if (!CANINIT) // if CAN should be OFF but still in RUN mode after setup/config command(s) (except @ON!)
		{
			CANINIT = 1; // stop CAN and enter init mode (CAN0CN = 0x0F) 
		}
	}

//;;if (bCanOn && !BOFF) // if CAN and BUS are ON
	if(bCanOn && (!(CAN0STAT & BOFF_MASK)))
		sendCanMsg(); // send it out the CAN bus


}




// CAN Functions
/*========================================================================
Description:
	Sets the CAN Bit Rate based on a 25.45625Mhz clock.

Entry: BdRate has a value from 1 to 4 where:
	1 = 125Kb
	2 = 250Kb
	3 = 500Kb
	4 = 670Kb

Exit:
	Can controller setup for selected bit rate.

History:
	2-27-2008 D.Lawe New

==========================================================================*/
// CAN buad rates using 25.45625Mhz. (see calculations at bottom of file) 
#define BR125K 0x77CB 
#define BR250K 0x68C5
#define BR500K 0x59C2
//#define BR670K 0x5BC1
#define BR670K 0x3DC1
// CAN buad rates using 22.1184Mhz. (see calculations at bottom of file) 
//#define BR670K 0x3582

void setCanBitrate() // see calc below
{

	SFRPAGE = CAN0_PAGE; 
	//
	CANINIT= 1;	// CAN0CN= 0x01(init) or 0x0F(run)
	CANCCE = 1; // CAN0CN= 0x41(init) or 0x4F(run)
	
//;;	CAN0ADR = BITREG;

	if (BdRate==1)
//;;	CAN0DAT = BR125K;	//  BdRate = 1 > 125Kb
		CAN0BT = BR125K;
	else if (BdRate==2)
//;;	CAN0DAT = BR250K;	//  BdRate = 1 > 250Kb
		CAN0BT = BR250K;
	else if (BdRate==3)
//;;	CAN0DAT = BR500K;	//  BdRate = 1 > 500Kb
		CAN0BT = BR500K;
	else//if (BdRate==4)
//;;	CAN0DAT = BR670K;	//  BdRate = 1 > 670Kb
		CAN0BT = BR670K;

   	// here CAN0CN= 0x41(init) or 0x4F(run)
	CANCCE = 0; // clear cce (unconfig), CAN0CN=0x01(init) or 0x0F(run)

	
}



//
//
void startCan()
{
	int8 i,SFR_Save;
	SFRPAGE = CAN0_PAGE;	// ADDED OLED VERSION
	SFR_Save = SFRPAGE; 

//	INTRPT_OFF; 
	CAN_INTRPT_OFF;
	clearCanMsgObjects();
	for (i=0; i<N_FILTERS; i++)
		initCanFilter(i);
	setCanBitrate(); // using setCanBitrate()
	for	(i=0; i < N_OUT_MSGS; i++) 
		bPendTxMsg[i] = 0 ;				// clear all pending messages

	CANINIT= 0;	// CAN0CN= 0x0E
	bCanOn = true;
	CANIE  = 1; // Enable EIE, IE and SIE Intrpt
	CANSIE = 1; 
//	CANSIE = 0; 
//	CANEIE = 1; // CAN0CN= 0x0F

// TEST TEST TEST
//	CANTEST = 1;
//	CAN0TST = 0x10;	// lback
// END TEST

	CAN_INTRPT_ON;
	SFRPAGE = SFR_Save;
//	INTRPT_ON; // global interupt enable
}
//
//
void clearCanMsgObjects()
{
	int8 i,SFR_Save;
	SFRPAGE  = CAN0_PAGE;
	//

// Message control register
// clear all bits except the EOB,
// in InitCanFilter the message objects used for
// RX registers will get set to another value.


//;;	CAN0ADR  = IF1ARB2;    // Point to arbitration 2 register
//;;	CAN0DAT  = 0x0000; 		   // Set arbitration 2 to invalid msg
	CAN0IF1A2 = 0x0000;
//;;	CAN0ADR  = IF1CMDMSK;    // Point to Command Mask Register 1
//;;	CAN0DATL = 0xFB;         // Set direction to WRITE all IF registers to Msg Obj, incld mask and data
	CAN0IF1CML = 0xFB;
	for (i=1;i<33;i++)
	{
//;;	do 	CAN0ADR = IF1CMDRQST; // Point to Command Request reg
//;;		while (CAN0DATH & 0x80); // wait while busy	
		while (CAN0IF1CRH & 0x80){};
		
//;;	CAN0ADR = IF1CMDRQST; // Write blank (reset) IF registers to each msg obj
//;;	CAN0DATL = i;
		CAN0IF1CRL = i;

	}
	SFRPAGE = SFR_Save;
}
//
void initCanFilter( int8 iMsg)
{
	int8 SFR_Save;

	if (iMsg>=N_FILTERS)
		return;

// clear the MsgVal bit for this message object


	SFR_Save = SFRPAGE;
	SFRPAGE  = CAN0_PAGE;

//;;	CAN0ADR  = IF1CMDMSK;  // Point to Command Mask 1
//;;	CAN0DAT  = 0x00F0;     // Set to WRITE, and alter all Msg Obj, incl mask,  except DATA
	CAN0IF1CM = 0x00F0;

//;;	CAN0ADR  = IF1ARB2;    // Point to arbitration 2 register
//;;	CAN0DAT  = 0x0000; 		   // Set arbitration 2 to invalid msg
	CAN0IF1A2 = 0x0000;
	
//;;	CAN0ADR  = IF1CMDRQST; // Point to Command Request reg.
//;;	CAN0DATL = iMsg+1;     // initiates write to Msg Obj (nMsg = iMsg+1)
	CAN0IF1CRL = iMsg+1;

	// 3-6 CAN clock cycles to move IF register contents to the Msg Obj in CAN RAM
//;;	do 	CAN0ADR = IF1CMDRQST; // Point to Command Request reg
//;;		while (CAN0DATH & 0x80); // wait while busy
	while (CAN0IF1CRH & 0x80) {}

// now set the mask registers

//;;	CAN0ADR  = IF1MSK1;    			   // Point to mask register
//;;	CAN0DAT  = 0x0000;		 // Set mask1 ID low word (auto-increment)(not used for 11 bit ID's)
	CAN0IF1M1 = 0x0000;

//;;	CAN0DAT  = CanFilters[iMsg] | 0xC000; // IF1 Mask2 (filter Mask or'ed with MXtd and MDir
	CAN0IF1M2 = CanFilters[iMsg] | 0xC000;
//	CAN0DAT  = 0xC000; // IF1 Mask2 (filter Mask or'ed with MXtd and MDir

// now set the arbitration registers

	//CAN0ADR= IF1ARB1;    			   // Point to arbitration register (auto-increment)
//;;	CAN0DAT  = 0x0000;		 // Set arbitration1 ID low word (auto-increment) (not used for 11 bit ID's)
	CAN0IF1A1 = 0x0000;
	//CAN0ADR= IF1ARB2;    			   // Point to arbitration register (auto-increment)
//;;	CAN0DAT  = CanIds[iMsg] | 0x8000;	// If1 Arb. 2 = ID ored with the MsgVal bit set
	CAN0IF1A2 = CanIds[iMsg] | 0x8000;

// set the message control register

//;;	CAN0DAT  = 0x1485;     			   // Msg Cntrl: set RxIE, UseMask, EoB (auto-increment)
	CAN0IF1MC = 0x1485;
	
// now transfer this to the message objects

//;;	CAN0ADR  = IF1CMDRQST; // Point to Command Request reg.
//;;	CAN0DATL = iMsg+1;       // initiates write to Msg Obj (nMsg = iMsg)
	CAN0IF1CRL = iMsg+1;
	// 3-6 CAN clock cycles to move IF register contents to the Msg Obj in CAN RAM
	SFRPAGE = SFR_Save;
}
//
//
void sendCanMsg()
{
	int8 iMsgNum,test1,test2,SFR_Save;

//	if(TEST_PIN)
//	{
//		TEST_PIN = 0;
//	}
//	else
//	{
//		TEST_PIN = 1;
//	}
//TEST---

	SFR_Save = SFRPAGE; 

	SFRPAGE = CAN0_PAGE;
	//
//;;	CAN0ADR = IF1CMDMSK;	// Point to Command Mask 1
//;;	CAN0DATL = 0xBF;		// Set to WRITE, & alter all Msg Obj except MASK (or B7)
	CAN0IF1CML = 0xBF;

//	CAN0ADR = IF1ARB1;		// Point to arbitration register

// write to the If arbitration register IF1 reg 2 (for 11 bit ID)
//;;	CAN0ADR = IF1ARB2;		// Point to arbitration register bits 28-16 for 11 bit ID
//;;	CAN0DATH = CanTxQ[0] >> 3 | 0xA0;	// Must shift the ID right 3, If1 Reg 2 = (MSGVal, Xtd, Dir, ID26 - 16), set MsgVal, and Dir
//;;	CAN0DATL = CanTxQ[0] << 5;	// the rest of ID bits in ID18-ID16
	CAN0IF1A2H = CanTxQ[0] >> 3 | 0xA0;
	CAN0IF1A2L = CanTxQ[0] << 5;
	
// write to the If1 message control register.
// message ctl reg: NewDat,MsgLst,IntPnd,UMask,TxIe,RxIE,RmtEn,TxRqst,EOB,res,res,res,DLC 3-0

//;;	CAN0DATH = 0x89;			// set the New Dat, TX Int. request and TX request bit (TxRqst)
//;;	CAN0DATL = 0x80 | CanTxQ[1]; 	// set the EOB then Or in the number of bytes 
	CAN0IF1MCH = 0x89;
	CAN0IF1MCL = 0x80 | CanTxQ[1];
// CanTxQ Format
//=================================================
// CanTxQ [0]= ID (command number in Raymond speak)
// CanTxQ [1]= DLC number of bytes in packet
// CanTxQ [2]= data 0
// CanTxQ [3]= data 1
// CanTxQ [4]= data 2
// CanTxQ [5]= data 3
// CanTxQ [6]= data 4
// CanTxQ [7]= data 5
// CanTxQ [8]= data 6
// CanTxQ [9]= data 7

//;;	CAN0DATH = CanTxQ[3];	// PUT data bytes
	CAN0IF1DA1H = CanTxQ[3];
//;;	CAN0DATL = CanTxQ[2];	// PUT data bytes Auto Increment
	CAN0IF1DA1L = CanTxQ[2];
//;;	CAN0DATH = CanTxQ[5];	// PUT data bytes
	CAN0IF1DA2H = CanTxQ[5];
//;;	CAN0DATL = CanTxQ[4];	// PUT data bytes Auto Increment
	CAN0IF1DA2L = CanTxQ[4];
//;;	CAN0DATH = CanTxQ[7];	// PUT data bytes
	CAN0IF1DB1H = CanTxQ[7];
//;;	CAN0DATL = CanTxQ[6];	// PUT data bytes Auto Increment
	CAN0IF1DB1L = CanTxQ[6];
//;;	CAN0DATH = CanTxQ[9];	// PUT data bytes
	CAN0IF1DB2H = CanTxQ[9];
//;;	CAN0DATL = CanTxQ[8];	// PUT data bytes Auto Increment
	CAN0IF1DB2L = CanTxQ[8];


	nLastSentTxMsg = 0; // reset to find out if set by ISR while scaning:
	//
	for	(iMsgNum=0; iMsgNum < N_OUT_MSGS; iMsgNum++) // scan for free (non-pending) slot
		if (!bPendTxMsg[ iMsgNum])
			break;

	if (iMsgNum == N_OUT_MSGS) // none free (all slots are pending)
	{
		if (!nLastSentTxMsg) // none sent out (and set by ISR) since the above scan
			return;
		iMsgNum = nLastSentTxMsg - _1ST_OUT_MSG; // nLastSentTxMsg just sent out and freed by ISR (iMsgNum=index)
	}
	
	bPendTxMsg[iMsgNum] = true; // take over slot iMsg
	test1 = iMsgNum;
	test2 = _1ST_OUT_MSG;
	iMsgNum += _1ST_OUT_MSG; // conver slot index to msg number

//;;	CAN0ADR = IF1CMDRQST; // Point to Command Request reg.
//;;	TXOK = 0x00;				// clear the TX OK for testing.
	CAN0STAT &= ~TXOK_MASK;
//;;	CAN0DATL = iMsgNum;      // Initiates write to Msg Obj.
	CAN0IF1CRL = iMsgNum;
	SFRPAGE = SFR_Save; 

	// 3-6 CAN clock cycles to move IF reg contents to the Msg Obj in CAN RAM.
}
//
//

//void CAN_ISR() interrupt 19 using 3
/*========================================================================\
	C A N _ I S R
Description:
	CAN controller interrupt service

Entry: Vectored from 0x83
Exit:
	Message sent

History:
	2-27-2008 D.Lawe New
	12-16-2010 R.Miller

==========================================================================*/

//void CAN_ISR() interrupt 19 using 3
void CAN_ISR() using 3  //
{
	byte byLEC, errH;
    byte status = CAN0STAT;               // Read status, which clears the Status
//    byte Interrupt_ID = CAN0IID;          // Read which message object caused
    byte Interrupt_ID = CAN0IIDL;          // Read which message object caused
//TEST+++
	TEST_PIN = 1;
//	if(TEST_PIN)
//	{
//		TEST_PIN = 0;
//	}
//	else
//	{
//		TEST_PIN = 1;
//	}
//TEST---

	while (Interrupt_ID &= 0x3F) // while rx/tx intrpt bit set by any msg obj
	{	
	  	if (Interrupt_ID<=N_FILTERS) // if interrupt caused by filtered reception (1<=nMsg<=N_FILTERS)
		{
      		SaveCanMsg( Interrupt_ID); // resets intrpt pend bit while reading & passing msg obj to uart1
//			CAN0STAT ^= RxOk;                  // clear the TX OK for testing.lly
			CAN0STAT &= ~RxOk;                  // clear the RXOK
		}
		else
	  	if (Interrupt_ID>=_1ST_OUT_MSG && Interrupt_ID<=LAST_OUT_MSG) 	// if xmission complete (1stOutMsg<=nMsg<=last=32)
	    {												//		reset intrpt pend bit:
			CAN0IF2CML = 0x1C;      // Command Mask 2 Config for TX ctrl: CLEAR IntPnd + NewDat bits in msg ctrl (while reading it)
			CAN0IF2CRL = Interrupt_ID;      // Command Request Reg. Reset Tx msg control in Obj "Interrupt_ID"
			bPendTxMsg[(nLastSentTxMsg = Interrupt_ID)-_1ST_OUT_MSG] = false; // reset logical pending flag and set last sent msg#
			CAN0STAT &= ~TxOk;                  // clear the TX OK 
	    }

//		p3_0on(); DO CAN LED HERE
//		Interrupt_ID = CAN0IID;	// read next pend message number from CAN INTREG, if any
//TEST+++
		Interrupt_ID = CAN0IIDL; // read next pend message number from CAN INTREG, if any
	}

// **	errH = CAN0IIDH; ** this was in the old Raymond code and possibly should be here
	// status intrpt &/or error handler
	errH = CAN0IIDH;
	
	#define LEC_MASK  7	// Last Error Code (LEC) bit-mask in CAN status reg (CAN0STA)

	byLEC = (byte)CAN0STAT & LEC_MASK; // Last Error Code (LEC)

	if ((byLEC != 0) && (byLEC != LEC_MASK))	// if LEC changed to any non-0 error
	{								//		handle error status update
		
//		p3_0off();

//		CanErrorCnt++;
		if (status & 0x20) 		// (EPASS)if recv error became passive (maxd out)
			bCanError = true; // set recv error passive indication (reset only next poer cycle)

		if (status & 0x80)		// if BOff
		{
			//p3_0off();

			if (byLEC==5) // if in BOFF recovery
				CANSIE = 0;		  // 	let it continue uninterrupted
			else
//				CANSIE = 1; 	  // may never get here, since no status intrpt till set again by main()
				CANSIE = 0; 	  // may never get here, since no status intrpt till set again by main()
//		}
//		else
//		{
			//p3_0on();
//			CANSIE = 0; 	  // may never get here, since no status intrpt till set again by main()
		}

		(byte)CAN0STAT = LEC_MASK;	// set LEC to No-Change state (also reset RXOK+TXOK)
	}
		TEST_PIN = 0;

}





 /* Calculation of the CAN bit timing for setCanBitrate():
;********************************************************************************
System clock        f_sys = 25.45625Mhz.
System clock period t_sys = 1/f_sys = 39.28308 ns.
;********************************************************************************
for 125Kb using a 25.45625Mhz crystal
Oscillator Selection and Prescaler
   CAN time quantum    tq 	= BRP/Fsys,  (prescaller / CPU Freq.)
   BPR = 12  (Prescaler)
   tq = 12/25.45625Mhz
   tq = 471.3979nS

  Desired bit rate is 125KBit/s, desired bit time is 8000 ns.
  Actual bit time = 17 tq = 8013.765 ns	
  Error = (8013.765ns-8000ns)/8000ns  = 0.17%

Bit Timing Register Settings
  CAN bus length = 10 m, with 5 ns/m signal delay time.
  Propagation delay time: 2*(transceiver loop delay + bus line delay) = 400 ns
  (maximum loop delay between CAN nodes = 400 ns)

  Prop_Seg = 1 tq = 471.3979 ns ( >= 400 ns).
  Sync_Seg = 1 tq

  Phase_Seg1 + Phase_Seg2+ = BitTime - (Sync_Seg + Prop_Seg)
   = Phase_seg1 + Phase_Seg2 = (17-1-1) tq = 15 tq
  Phase_seg1 <= Phase_Seg2  implies:  Phase_seg1 = 7 tq and Phase_Seg2 = 8 tq
  SJW = min( Phase_Seg1, 4) tq = 4 tq

  TSEG1 = Prop_Seg + Phase_Seg1 - 1 = 1+7-1 = 7
  TSEG2 = Phase_Seg2 - 1            = 7
  SJW_p = SJW - 1                   = 3
  
  Bit Timing Register =  TSEG2*0x1000 | TSEG1*0x0100 |  SJW_p*0x0040 | BRP-1 
  0x7000 | 0x0700 | 0x00C0 | 0x000C
	setting bit rate:

	 CAN0ADR = BITREG;
	 CAN0DAT = 0x77CB

;**********************************************************************************

for 250Kb using a 25.45625Mhz crystal
Oscillator Selection and Prescaler
   CAN time quantum    tq 	= BRP/Fsys,  (prescaller / CPU Freq.)
   BPR = 6  (Prescaler)
   tq = 6/25.45625Mhz
   tq = 235.6985nS

  Desired bit rate is 250KBit/s, desired bit time is 4000 ns.
  Actual bit time = 17 tq = 4006.875 ns	
  Error = (4000ns-4006.875ns)/4000ns  = 0.17%

Bit Timing Register Settings
  CAN bus length = 10 m, with 5 ns/m signal delay time.
  Propagation delay time: 2*(transceiver loop delay + bus line delay) = 400 ns
  (maximum loop delay between CAN nodes = 400 ns)
   tq = 235.6985nS

  Prop_Seg = 2 tq = 471.3979 ns ( >= 400 ns).
  Sync_Seg = 1 tq
  
  Phase_Seg1 + Phase_Seg2+ = BitTime - (Sync_Seg + Prop_Seg)
  	= Phase_seg1 + Phase_Seg2 = (17-1-2) tq = 14 tq
  Phase_seg1 <= Phase_Seg2  implies:  Phase_seg1 = 7 tq and Phase_Seg2 = 7 tq
  SJW = min( Phase_Seg1, 4) tq = 4 tq

  TSEG1 = Prop_Seg + Phase_Seg1 - 1 = 2+7-1 = 8
  TSEG2 = Phase_Seg2 - 1            = 6
  SJW_p = SJW - 1                   = 3
  
  Bit Timing Register =  TSEG2*0x1000 | TSEG1*0x0100 |  SJW_p*0x0040 | BRP-1
  0x6000 | 0x0800 | 0x00C0 | 0x0006
	setting bit rate:

	 CAN0ADR = BITREG;
	 CAN0DAT = 0x68C5

;**********************************************************************************
;**********************************************************************************

for 500Kb using a 25.45625Mhz crystal
Oscillator Selection and Prescaler
   CAN time quantum    tq 	= BRP/Fsys,  (prescaller / CPU Freq.)
   BPR = 3  (Prescaler)
   tq = 3/25.45625Mhz
   tq = 117.8493nS

  Desired bit rate is 500KBit/s, desired bit time is 2000 ns.
  Actual bit time = 17 tq = 2003.437 ns	
  Error = (2000ns-2003.8437ns)/2000ns  = 0.17%

Bit Timing Register Settings
  CAN bus length = 10 m, with 5 ns/m signal delay time.
  Propagation delay time: 2*(transceiver loop delay + bus line delay) = 400 ns
  (maximum loop delay between CAN nodes = 400 ns)
   tq = 117.8493nS

  Prop_Seg = 4 tq = 471.3979 ns ( >= 400 ns).
  Sync_Seg = 1 tq
  
  Phase_Seg1 + Phase_Seg2+ = BitTime - (Sync_Seg + Prop_Seg)
  	= Phase_seg1 + Phase_Seg2 = (17-1-4) tq = 12 tq
  Phase_seg1 <= Phase_Seg2  implies:  Phase_seg1 = 6 tq and Phase_Seg2 = 6 tq
  SJW = min( Phase_Seg1, 4) tq = 4 tq

  TSEG1 = Prop_Seg + Phase_Seg1 - 1 = 4+6-1 = 9
  TSEG2 = Phase_Seg2 - 1            = 5
  SJW_p = SJW - 1                   = 3
  
  Bit Timing Register =  TSEG2*0x1000 | TSEG1*0x0100 |  SJW_p*0x0040 | BRP-1 
  0x5000 | 0x0900 | 0x00C0 | 0x0002
	setting bit rate:

	 CAN0ADR = BITREG;
	 CAN0DAT = 0x59C2

;**********************************************************************************

;**********************************************************************************
for 670Kb - using a 25.45625Mhz crystal
Oscillator Selection and Prescaler
  Desired bit rate is 670KBit/s, desired bit time is 1492.537 ns.
  BPR = 2 (Prescaler)
  tq = 2/25.45625Mhz
  tq = 78.56617nS

  Actual bit time = 19 tq = 14927.57nS ~ 1492.537 ns.
  Actual bit rate is 669.9013KBit/s 
  Error = (1492.537ns-1492.757ns)/1492.537ns  = 0.05%

Bit Timing Register Settings
  CAN bus length = 10 m, with 5 ns/m signal delay time.
  Propagation delay time: 2*(transceiver loop delay + bus line delay) = 400 ns
  (maximum loop delay between CAN nodes = 400 ns)
 
  Prop_Seg = 6 tq = 471.3979 ns ( >= 400 ns).
  Sync_Seg = 1 tq
  
  Phase_Seg1 + Phase_Seg2 = BitTime - (Sync_Seg + Prop_Seg)
  	= Phase_seg1 + Phase_Seg2 = (19-1-6) tq = 12 tq
  Phase_seg1 <= Phase_Seg2  implies:  Phase_seg1 = 6 tq and Phase_Seg2 = 6 tq
  SJW = min( Phase_Seg1, 4) tq = 4 tq

  TSEG1 = Prop_Seg + Phase_Seg1 - 1 = 6+6-1 = 11
  TSEG2 = Phase_Seg2 - 1            = 5
  SJW_p = SJW - 1                   = 3
  
  Bit Timing Register =  TSEG2*0x1000 | TSEG1*0x0100 |  SJW_p*0x0040 | BRP-1
  0x5000 | 0x0B00 | 0x00C0 | 0x0001
	setting bit rate:

	 CAN0ADR = BITREG;
	 CAN0DAT = 0x5BC1


CHANGES REQUESTED BY DON PUTNEY TO MATCH RAYMOND CAN SAMPLE TIME
Increase phase segment 1 from 6tq to 8tq
Decrease phase segement 2 from 6tq to 4tq
"sample to a point which more closely approximates our 81%."

  TSEG1 = Prop_Seg + Phase_Seg1 - 1 = 6+8-1 = 13
  TSEG2 = Phase_Seg2 - 1            = 3
  SJW_p = SJW - 1                   = 3
  
  Bit Timing Register =  TSEG2*0x1000 | TSEG1*0x0100 |  SJW_p*0x0040 | BRP-1
  0x3000 | 0x0D00 | 0x00C0 | 0x0001
	setting bit rate:

	 CAN0ADR = BITREG;
	 CAN0DAT = 0x3DC1

;**********************************************************************************

;**********************************************************************************
for 1000Kb - using a 25.45625Mhz crystal
Oscillator Selection and Prescaler
  Desired bit rate is 1000KBit/s, desired bit time is 1 us.
  BPR = 1 (Prescaler)
  tq = 1/25.45625Mhz
  tq = 39.2831nS

  Actual bit time = 26 tq = 1021.37nS ~ 1000.00 ns.
  Actual bit rate is 979.077KBit/s 
  Error = (979.077ns-1000.00ns)/979.077ns  = 2.13%

Bit Timing Register Settings
  CAN bus length = 10 m, with 5 ns/m signal delay time.
  Propagation delay time: 2*(transceiver loop delay + bus line delay) = 400 ns
  (maximum loop delay between CAN nodes = 400 ns)
 
  Prop_Seg = 11 tq = 432.118 ns ( >= 400 ns).
  Sync_Seg = 1 tq
  
  Phase_Seg1 + Phase_Seg2 = BitTime - (Sync_Seg + Prop_Seg)
  	= Phase_seg1 + Phase_Seg2 = (26-1-11) tq = 14 tq
  Phase_seg1 <= Phase_Seg2  implies:  Phase_seg1 = 7 tq and Phase_Seg2 = 7 tq
  SJW = min( Phase_Seg1, 4) tq = 4 tq

  TSEG1 = Prop_Seg + Phase_Seg1 - 1 = 7+7-1 = 13
  TSEG2 = Phase_Seg2 - 1            = 6
  SJW_p = SJW - 1                   = 3
  
  Bit Timing Register =  TSEG2*0x1000 | TSEG1*0x0100 |  SJW_p*0x0040 | BRP-1
  0x5000 | 0x0B00 | 0x00C0 | 0x0001
	setting bit rate:

	 CAN0ADR = BITREG;
	 CAN0DAT = 0x5BC1


CHANGES REQUESTED BY DON PUTNEY TO MATCH RAYMOND CAN SAMPLE TIME
Increase phase segment 1 from 6tq to 8tq
Decrease phase segement 2 from 6tq to 4tq
"sample to a point which more closely approximates our 81%."

  TSEG1 = Prop_Seg + Phase_Seg1 - 1 = 6+8-1 = 13
  TSEG2 = Phase_Seg2 - 1            = 3
  SJW_p = SJW - 1                   = 3
  
  Bit Timing Register =  TSEG2*0x1000 | TSEG1*0x0100 |  SJW_p*0x0040 | BRP-1
  0x3000 | 0x0D00 | 0x00C0 | 0x0001
	setting bit rate:

	 CAN0ADR = BITREG;
	 CAN0DAT = 0x3DC1



;**********************************************************************************
;**********************************************************************************
for 670Kb XTAL = 22.1184Mhz
Oscillator Selection and Prescaler
  Desired bit rate is 670KBit/s, desired bit time is 1492.537 ns.
  BPR = 3 (Prescaler)
  tq = 3/22.1184Mhz
  tq = 135.6337nS

  Actual bit time = 11 tq = 1491.970nS ~ 1492.537 ns.
  Actual bit rate is 670.24242KBit/s 
  Error = (1492.537ns-1491.970nS)/1492.537ns  = 0.0023%

Bit Timing Register Settings
  CAN bus length = 10 m, with 5 ns/m signal delay time.
  Propagation delay time: 2*(transceiver loop delay + bus line delay) = 400 ns
  (maximum loop delay between CAN nodes = 400 ns)
 
  Prop_Seg = 3 tq = 406.901 ns ( >= 400 ns).
  Sync_Seg = 1 tq
  
  Phase_Seg1 + Phase_Seg2 = BitTime - (Sync_Seg + Prop_Seg)
  	= Phase_seg1 + Phase_Seg2 = (11-1-3) tq = 7 tq
  Phase_seg1 <= Phase_Seg2  implies:  Phase_seg1 = 3 tq and Phase_Seg2 = 4 tq
  SJW = min( Phase_Seg1, 4) tq = 3 tq

  TSEG1 = Prop_Seg + Phase_Seg1 - 1 = 3+3-1 = 5
  TSEG2 = Phase_Seg2 - 1            = 3
  SJW_p = SJW - 1                   = 2
  
  Bit Timing Register =  TSEG2*0x1000 | TSEG1*0x0100 |  SJW_p*0x0040 | BRP-1
  0x3000 | 0x0500 | 0x0080 | 0x0002
	setting bit rate:

	 CAN0ADR = BITREG;
	 CAN0DAT = 0x3582

;**********************************************************************************

*/



