
// 1-wire device rd/wr driver

//----------------------------------------------------------------------------
int SMB_WrByte_1W(unsigned char cmd, unsigned char dat, unsigned char SlaveAddr)
{	 
	 unsigned int  countdown = 400;
	
   while ( SMB_BUSY )                   // Wait for SMBus to be free.
   {
     if ( --countdown == 0 )
     {
        SMB_BUSY = 0;
        return 0;  
     }	
   }	 
   SMB_BUSY = 1;                       // Claim SMBus (set to busy)

   // Set SMBus ISR parameters
   TargetAddr = SlaveAddr;        // Set target slave address
   SMB_RW = WRITE;            // Mark next transfer as a write
   SendAddr_N = 1;      // Send destination Address after Slave Address
   SMB_RANDOMREAD = 0;        // Do not send a repeat START  
   SMB_ACKPOLL = 0;           // 1: Enable Acknowledge Polling (The ISR will automatically 
                              // restart the transfer if the slave does not acknoledge
   // Specify the payload
   BYTEADDR_Lo = cmd;         // overload the variable to use for cmd paramter

   SMB_SINGLEBYTE_OUT = dat;  // Store <dat> (local variable) in a global variable
                              // so the ISR can read it after this function exits

   // The outgoing data pointer points to the <dat> variable
   pSMB_DATA_OUT = &SMB_SINGLEBYTE_OUT;
   SMB_DATA_LEN = 1;          // Specify to ISR that the next transfer
                              // will contain one data byte
   // Initiate SMBus Transfer
   STA = 1;
   
   return 1;
}

//-----------------------------------------------------------------------------
int SMB_RdByte_1W(unsigned char *dest_addr, unsigned char SlaveAddr)
{
	unsigned int  countdown = 4000;

   while ( (SMB0CF & 0x20) >> 5) // Wait for SMBus to be free.
	{
		if (--countdown == 0)
		{
         STO = 0;
         ACK = 0;
         SMB_DATA_LEN = 0;
         SMB_BUSY = 0;                    // Free SMBus
         return 0;	// time out
		}
	}
   SMB_BUSY = 1;           // Claim SMBus (set to busy)
    
    pSMB_DATA_IN = (unsigned char*)dest_addr;	// Set the the incoming data pointer
    TargetAddr = SlaveAddr;
    SendAddr_N = 0;    // don't need to send destination after slave address
	SMB_DATA_LEN = 1;    // Specify to ISR that next transfer will contain <len> data bytes
	SMB_RW = READ;   // A random read starts as write then changes to read after repeated start sent.
    SMB_RANDOMREAD = 0;
	// The ISR handles this switch-over if <SMB_RANDOMREAD> bit is set.
	SMB_ACKPOLL = 0;           // 1: Enable Acknowledge Polling
    STA = 1;						// Initiate SMBus Transfer

   countdown = 4000;
   while ( (SMB0CF & 0x20) >> 5) // Wait for SMBus to be free.
	{
		if (--countdown == 0)
		{
         STO = 0;
         ACK = 0;
         SMB_DATA_LEN = 0;
         SMB_BUSY = 0;                    // Free SMBus
		}
	}
   
    return 1;
}

///////////////////////////// Exported routines API ///////////////////////////
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
unsigned char DSS_Init(void)
{
    unsigned char RdData = 0xFF;
    unsigned char RdData64 = 0xFF, RdData65 = 0xFF;
    
    // per data sheet set the master to over-drive mode to match the slave
    SMB_WrByte_1W(DSS_REG_CFG, (DSS_OVERDRIVE + DSS_STRONGPULLUP), ONEWIREMASTER_ADDR); // write to config register
    //SMB_WrByte_1W(DSS_REG_CFG, (DSS_OVERDRIVE), ONEWIREMASTER_ADDR); // write to config register
    Wait_ms(3);
    SMB_WrByte_1W(DSS_REG_CMD, CMD_1WRS, ONEWIREMASTER_ADDR); // write 1W-RST commmand to cmd reg.
    Wait_ms(3);
    SMB_RdByte_1W(&RdData, ONEWIREMASTER_ADDR);    // just read to a dumy byte, lsb should be cleared
    Wait_ms(1);
    
    // read Protection Status Register 64h in the Master device
    SMB_WrByte(DSS_REG_PSTATUS64, ONEWIREMASTER_ADDR, 0); // read RP1, WP1, RP0, WP0
    Wait_ms(1);
    SMB_RdByte_1W(&RdData64, ONEWIREMASTER_ADDR); // read data byte
    RdData64 &= 0xF0;
    Wait_ms(1); 
    SMB_WrByte(DSS_REG_PSTATUS65, ONEWIREMASTER_ADDR, 0); // read RP3, WP3, RP2, WP2
    RdData65 &= 0xF0;
    Wait_ms(1);
    SMB_RdByte_1W(&RdData65, ONEWIREMASTER_ADDR); // read data byte
    Wait_ms(1);    
    
    return RdData;    // should be 0x0A: device presence is true, bus is not busy    
}

//-----------------------------------------------------------------------------
int DSS_Reset(unsigned char* RdData)
{   
    SMB_WrByte_1W(DSS_REG_CMD, CMD_1WRS, ONEWIREMASTER_ADDR); // write 1W-RST commmand to cmd reg.
    delay_us4(50);
    SMB_RdByte_1W(RdData, ONEWIREMASTER_ADDR);    // just read to a dummy byte, lsb should be cleared
    delay_us4(20);
    return 0;   // no error 
}

