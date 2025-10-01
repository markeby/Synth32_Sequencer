//#######################################################################
// Module:     TouchGT911.cpp
// Creator:    markeby
// Date:       9/29/2025
//#######################################################################
#include <Arduino.h>

#include "TouchGT911.h"

//#######################################################################
//Touchscreen pad max
#define GT911_MAX_WIDTH     320
#define GT911_MAX_HEIGHT    480

// GT911 registers
#define GT_CTRL_REG         0X8040
#define GT_CFGS_REG         0X8047
#define GT_CHECK_REG        0X80FF
#define GT_PID_REG          0X8140

#define GT_GSTID_REG        0X814E
#define GT911_READ_XY_REG   0x814E

//I2C commands
#define GT_CMD_WR           0XBA
#define GT_CMD_RD           0XBB

// I2C ports
int IIC_SCL  =   32;
int IIC_SDA  =   33;
int IIC_RST  =   25;

//#######################################################################
// Macros
#define IIC_SCL_0  digitalWrite(IIC_SCL,LOW)
#define IIC_SCL_1  digitalWrite(IIC_SCL,HIGH)

#define IIC_SDA_0  digitalWrite(IIC_SDA,LOW)
#define IIC_SDA_1  digitalWrite(IIC_SDA,HIGH)

#define IIC_RST_0  digitalWrite(IIC_RST,LOW)
#define IIC_RST_1  digitalWrite(IIC_RST,HIGH)

#define READ_SDA   digitalRead(IIC_SDA)

//#######################################################################
inline void SDA_IN (void)
    {
    pinMode (IIC_SDA, INPUT);
    }

//#######################################################################
inline void SDA_OUT (void)
    {
    pinMode (IIC_SDA, OUTPUT);
    }

//#######################################################################
//#######################################################################
void GT11_C::delay_us (unsigned int xus)  //1us
    {
    for ( ;  xus > 1;  xus-- );
    }

//#######################################################################
void GT11_C::IIC_Init (void)
    {
    pinMode (IIC_SDA, OUTPUT);
    pinMode (IIC_SCL, OUTPUT);
    pinMode (IIC_RST, OUTPUT);
    IIC_SCL_1;
    IIC_SDA_1;
    }

//#######################################################################
void GT11_C::IIC_Start (void)
    {
    SDA_OUT ();
    IIC_SDA_1;
    IIC_SCL_1;
    delay_us (4);
    IIC_SDA_0; //START:when CLK is high,DATA change form high to low
    delay_us (4);
    IIC_SCL_0;
    }

//#######################################################################
void GT11_C::IIC_Stop (void)
    {
    SDA_OUT ();
    IIC_SCL_0;
    IIC_SDA_0; //STOP:when CLK is high DATA change form low to high
    delay_us (4);
    IIC_SCL_1;
    IIC_SDA_1;
    delay_us (4);
    }

//#######################################################################
uint8_t GT11_C::IIC_Wait_Ack (void)
    {
    uint8_t ucErrTime = 0;
    SDA_IN ();
    IIC_SDA_1; delay_us (1);
    IIC_SCL_1; delay_us (1);
    while ( READ_SDA )
        {
        ucErrTime++;
        if ( ucErrTime > 250 )
            {
            IIC_Stop ();
            return 1;
            }
        }
    IIC_SCL_0;
    return 0;
    }

//#######################################################################
void GT11_C::IIC_Ack (void)
    {
    IIC_SCL_0;
    SDA_OUT ();
    IIC_SDA_0;
    delay_us (2);
    IIC_SCL_1;
    delay_us (2);
    IIC_SCL_0;
    }

//#######################################################################
void GT11_C::IIC_NAck (void)
    {
    IIC_SCL_0;
    SDA_OUT ();
    IIC_SDA_1;
    delay_us (2);
    IIC_SCL_1;
    delay_us (2);
    IIC_SCL_0;
    }

//#######################################################################
void GT11_C::IIC_Send_Byte (uint8_t txd)
    {
    SDA_OUT ();
    IIC_SCL_0;
    for ( int z = 0;  z < 8;  z++)
        {
        if ( (txd & 0x80) >> 7 )
            IIC_SDA_1;
        else
            IIC_SDA_0;
        txd <<= 1;
        delay_us (2);
        IIC_SCL_1;
        delay_us (2);
        IIC_SCL_0;
        delay_us (2);
        }
    }

//#######################################################################
uint8_t GT11_C::IIC_Read_Byte (unsigned char ack)
    {
    uint8_t receive = 0;

    SDA_IN ();
    for ( int z = 0;  z < 8;  z++ )
        {
        IIC_SCL_0;
        delay_us (2);
        IIC_SCL_1;
        receive <<= 1;
        if ( READ_SDA )
            receive++;
        delay_us (1);
        }
    if ( !ack )
        IIC_NAck ();
    else
        IIC_Ack ();
    return (receive);
    }

//#######################################################################
uint8_t GT11_C::RegWR (uint16_t reg, uint8_t* buf, uint8_t len)
    {
    uint8_t ret = 0;

    IIC_Start ();
    IIC_Send_Byte (GT_CMD_WR);
    IIC_Wait_Ack ();
    IIC_Send_Byte (reg >> 8);
    IIC_Wait_Ack ();
    IIC_Send_Byte (reg & 0XFF);
    IIC_Wait_Ack ();
    for ( int z = 0; z < len; z++ )
        {
        IIC_Send_Byte (buf[z]);
        ret = IIC_Wait_Ack ();
        if ( ret )
            break;
        }
    IIC_Stop ();
    return (ret);
    }

//#######################################################################
void GT11_C::RegR (uint16_t reg, uint8_t* buf, uint8_t len)
    {
    IIC_Start ();
    IIC_Send_Byte (GT_CMD_WR);
    IIC_Wait_Ack ();
    IIC_Send_Byte (reg >> 8);
    IIC_Wait_Ack ();
    IIC_Send_Byte (reg & 0XFF);
    IIC_Wait_Ack( );
    IIC_Start ();
    IIC_Send_Byte (GT_CMD_RD);
    IIC_Wait_Ack ();
    for ( int z = 0;  z < len;  z++ )
        buf[z] = IIC_Read_Byte (z == (len - 1) ? 0 : 1);
    IIC_Stop ();
    }

//#######################################################################
void GT11_C::SendConfig (uint8_t mode)
    {
    uint8_t buf[2] = { 0, mode };

    RegWR (GT_CHECK_REG, buf, 2);
    }

//#######################################################################
bool GT11_C::Scan ()
    {
    uint8_t buf[41];
    uint8_t clearbuf = 0;

    bool touched = false;
    Current.Touch = 0;
    RegR (GT911_READ_XY_REG, buf, 1);

    if ( (buf[0] & 0x80) == 0x00 )  // no touch
        {
        touched = false;
        RegWR (GT911_READ_XY_REG, (uint8_t*)&clearbuf, 1);
        delay (10);
        }
    else
        {
        touched = true;
        Current.TouchpointFlag = buf[0];
        Current.TouchCount = buf[0] & 0x0f;

        if ( Current.TouchCount > 5 )   // detect messy touch
            {
            touched = false;
            RegWR (GT911_READ_XY_REG, (uint8_t*)&clearbuf, 1);
            return (touched);
            }

        // tracking touch data
        RegR  (GT911_READ_XY_REG + 1, &buf[1], Current.TouchCount * 8);
        RegWR (GT911_READ_XY_REG, (uint8_t *)&clearbuf, 1);

        Current.Touchkeytrackid[0] = buf[1];
        Current.X[0] = ((uint16_t)buf[3] << 8) + buf[2];
        Current.Y[0] = ((uint16_t)buf[5] << 8) + buf[4];
        Current.S[0] = ((uint16_t)buf[7] << 8) + buf[6];

        Current.Touchkeytrackid[1] = buf[9];
        Current.X[1] = ((uint16_t)buf[11] << 8) + buf[10];
        Current.Y[1] = ((uint16_t)buf[13] << 8) + buf[12];
        Current.S[1] = ((uint16_t)buf[15] << 8) + buf[14];

        Current.Touchkeytrackid[2] = buf[17];
        Current.X[2] = ((uint16_t)buf[19] << 8) + buf[18];
        Current.Y[2] = ((uint16_t)buf[21] << 8) + buf[20];
        Current.S[2] = ((uint16_t)buf[23] << 8) + buf[22];

        Current.Touchkeytrackid[3] = buf[25];
        Current.X[3] = ((uint16_t)buf[27] << 8) + buf[26];
        Current.Y[3] = ((uint16_t)buf[29] << 8) + buf[28];
        Current.S[3] = ((uint16_t)buf[31] << 8) + buf[30];

        Current.Touchkeytrackid[4] = buf[33];
        Current.X[4] = ((uint16_t)buf[35] << 8) + buf[34];
        Current.Y[4] = ((uint16_t)buf[37] << 8) + buf[36];
        Current.S[4] = ((uint16_t)buf[39] << 8) + buf[38];

        // limit clipping
        for ( int z = 0;  z < Previous.TouchCount;  z++ )
            {
            if ( Current.Y[z] < 0 )
                Current.Y[z]  = 0;
            if ( Current.Y[z] > 480 )
                Current.Y[z]  = 480;
            if ( Current.X[z] < 0 )
                Current.X[z]  = 0;
            if ( Current.X[z] > 320 )
                Current.X[z]  = 320;
            }
        // limit check
        for ( int z = 0;  z < Current.TouchCount;  z++ )
            {
            if ( Current.Y[z] < 0 )
                touched = false;
            if ( Current.Y[z] > 480 )
                touched = false;
            if ( Current.X[z] < 0 )
                touched = false;
            if ( Current.X[z] > 320 )
                touched = false;

            // Touch state confirmed
            if ( touched == true )
                {
                Previous.X[z]       = Current.X[z];
                Previous.Y[z]       = Current.Y[z];
                Previous.TouchCount = Current.TouchCount;
                }
            }
        // touch is invalid
        if ( Current.TouchCount == 0 )
            touched = false;
        }
    return (touched);
    }

//#######################################################################
void GT11_C::Begin ()
    {
    uint8_t buf[4];
    uint8_t CFG_TBL[184];

    pinMode (IIC_SDA, OUTPUT);
    pinMode (IIC_SCL, OUTPUT);
    pinMode (IIC_RST, OUTPUT);
    delay (50);
    digitalWrite (IIC_RST, LOW);
    delay (10);
    digitalWrite (IIC_RST, HIGH);
    delay (50);

    RegR (0X8140, (uint8_t *)&buf, 4);
    buf[0] = 0x02;

    RegWR( GT_CTRL_REG, buf, 1);
    RegR (GT_CFGS_REG, buf, 1);
    if ( buf[0] < 0X60 )
        SendConfig (1);

    RegR (GT_CFGS_REG, (uint8_t *)&CFG_TBL[0], 184);
    delay (10);
    buf[0] = 0x00;
    RegWR (GT_CTRL_REG, buf, 1);
    }

//#######################################################################
GT11_C  TouchPanel;

