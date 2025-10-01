//#######################################################################
// Module:     TouchGT911.h
// Creator:    markeby
// Date:       9/29/2025
//#######################################################################
#pragma once

class GT11_C
    {
private:
#define CT_MAX_TOUCH 5
    typedef struct
        {
        uint8_t     Touch;
        uint8_t     TouchpointFlag;
        uint8_t     TouchCount;
        uint8_t     Touchkeytrackid[CT_MAX_TOUCH];
        uint16_t    X[CT_MAX_TOUCH];
        uint16_t    Y[CT_MAX_TOUCH];
        uint16_t    S[CT_MAX_TOUCH];
        } GT911_S;
    GT911_S Current, Previous;

    void    delay_us        (unsigned int xus);
    void    IIC_Init        (void);
    void    IIC_Start       (void);
    void    IIC_Stop        (void);
    uint8_t IIC_Wait_Ack    (void);
    void    IIC_Ack         (void);
    void    IIC_NAck        (void);
    void    IIC_Send_Byte   (uint8_t txd);
    uint8_t IIC_Read_Byte   (unsigned char ack);
    uint8_t RegWR           (uint16_t reg, uint8_t* buf, uint8_t len);
    void    RegR            (uint16_t reg, uint8_t* buf, uint8_t len);
    void    SendConfig      (uint8_t mode);

public:
            GT11_C () {};

    void     Begin  (void);
    bool     Scan   (void);

    uint16_t GetX   (void)
        { return (Current.X[0]); }
    uint16_t GetY   (void)
        { return (Current.Y[0]); }

    };

//#######################################################################
extern GT11_C  TouchPanel;

