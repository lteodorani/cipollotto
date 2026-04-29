// Private Header
// Should only be user by ui_sdlbackend.c

#include "common.h"

static const u32 white = 0xFFFFFFFF;
static const u32 black = 0x000000FF;
static const u32 green = 0x00FF00FF;
static const u32 lgray = 0x1C1C1CFF;
static const u32 colo1 = 0x996600ff;
static const u32 colo2 = 0xFFCC00ff;
static const u32 colo3 = 0xFF6600ff;


static const u32 On  = colo1;
static const u32 Off = lgray;

// compile-time initialized table (16 rows × 4 columns)
static const u32 draw_table[16][4] = {
    { Off, Off, Off, Off },
    { Off, Off, Off, On  },
    { Off, Off, On,  Off },
    { Off, Off, On,  On  },
    { Off, On,  Off, Off },
    { Off, On,  Off, On  },
    { Off, On,  On,  Off },
    { Off, On,  On,  On  },
    { On,  Off, Off, Off },
    { On,  Off, Off, On  },
    { On,  Off, On,  Off },
    { On,  Off, On,  On  },
    { On,  On,  Off, Off },
    { On,  On,  Off, On  },
    { On,  On,  On,  Off },
    { On,  On,  On,  On  },
};


static const u32 draw_table_large[256][8] = {
    { Off, Off, Off, Off, Off, Off, Off, Off },  // 0x00  ........
    { Off, Off, Off, Off, Off, Off, Off,  On },  // 0x01  .......█
    { Off, Off, Off, Off, Off, Off,  On, Off },  // 0x02  ......█.
    { Off, Off, Off, Off, Off, Off,  On,  On },  // 0x03  ......██
    { Off, Off, Off, Off, Off,  On, Off, Off },  // 0x04  .....█..
    { Off, Off, Off, Off, Off,  On, Off,  On },  // 0x05  .....█.█
    { Off, Off, Off, Off, Off,  On,  On, Off },  // 0x06  .....██.
    { Off, Off, Off, Off, Off,  On,  On,  On },  // 0x07  .....███
    { Off, Off, Off, Off,  On, Off, Off, Off },  // 0x08  ....█...
    { Off, Off, Off, Off,  On, Off, Off,  On },  // 0x09  ....█..█
    { Off, Off, Off, Off,  On, Off,  On, Off },  // 0x0A  ....█.█.
    { Off, Off, Off, Off,  On, Off,  On,  On },  // 0x0B  ....█.██
    { Off, Off, Off, Off,  On,  On, Off, Off },  // 0x0C  ....██..
    { Off, Off, Off, Off,  On,  On, Off,  On },  // 0x0D  ....██.█
    { Off, Off, Off, Off,  On,  On,  On, Off },  // 0x0E  ....███.
    { Off, Off, Off, Off,  On,  On,  On,  On },  // 0x0F  ....████
    { Off, Off, Off,  On, Off, Off, Off, Off },  // 0x10  ...█....
    { Off, Off, Off,  On, Off, Off, Off,  On },  // 0x11  ...█...█
    { Off, Off, Off,  On, Off, Off,  On, Off },  // 0x12  ...█..█.
    { Off, Off, Off,  On, Off, Off,  On,  On },  // 0x13  ...█..██
    { Off, Off, Off,  On, Off,  On, Off, Off },  // 0x14  ...█.█..
    { Off, Off, Off,  On, Off,  On, Off,  On },  // 0x15  ...█.█.█
    { Off, Off, Off,  On, Off,  On,  On, Off },  // 0x16  ...█.██.
    { Off, Off, Off,  On, Off,  On,  On,  On },  // 0x17  ...█.███
    { Off, Off, Off,  On,  On, Off, Off, Off },  // 0x18  ...██...
    { Off, Off, Off,  On,  On, Off, Off,  On },  // 0x19  ...██..█
    { Off, Off, Off,  On,  On, Off,  On, Off },  // 0x1A  ...██.█.
    { Off, Off, Off,  On,  On, Off,  On,  On },  // 0x1B  ...██.██
    { Off, Off, Off,  On,  On,  On, Off, Off },  // 0x1C  ...███..
    { Off, Off, Off,  On,  On,  On, Off,  On },  // 0x1D  ...███.█
    { Off, Off, Off,  On,  On,  On,  On, Off },  // 0x1E  ...████.
    { Off, Off, Off,  On,  On,  On,  On,  On },  // 0x1F  ...█████
    { Off, Off,  On, Off, Off, Off, Off, Off },  // 0x20  ..█.....
    { Off, Off,  On, Off, Off, Off, Off,  On },  // 0x21  ..█....█
    { Off, Off,  On, Off, Off, Off,  On, Off },  // 0x22  ..█...█.
    { Off, Off,  On, Off, Off, Off,  On,  On },  // 0x23  ..█...██
    { Off, Off,  On, Off, Off,  On, Off, Off },  // 0x24  ..█..█..
    { Off, Off,  On, Off, Off,  On, Off,  On },  // 0x25  ..█..█.█
    { Off, Off,  On, Off, Off,  On,  On, Off },  // 0x26  ..█..██.
    { Off, Off,  On, Off, Off,  On,  On,  On },  // 0x27  ..█..███
    { Off, Off,  On, Off,  On, Off, Off, Off },  // 0x28  ..█.█...
    { Off, Off,  On, Off,  On, Off, Off,  On },  // 0x29  ..█.█..█
    { Off, Off,  On, Off,  On, Off,  On, Off },  // 0x2A  ..█.█.█.
    { Off, Off,  On, Off,  On, Off,  On,  On },  // 0x2B  ..█.█.██
    { Off, Off,  On, Off,  On,  On, Off, Off },  // 0x2C  ..█.██..
    { Off, Off,  On, Off,  On,  On, Off,  On },  // 0x2D  ..█.██.█
    { Off, Off,  On, Off,  On,  On,  On, Off },  // 0x2E  ..█.███.
    { Off, Off,  On, Off,  On,  On,  On,  On },  // 0x2F  ..█.████
    { Off, Off,  On,  On, Off, Off, Off, Off },  // 0x30  ..██....
    { Off, Off,  On,  On, Off, Off, Off,  On },  // 0x31  ..██...█
    { Off, Off,  On,  On, Off, Off,  On, Off },  // 0x32  ..██..█.
    { Off, Off,  On,  On, Off, Off,  On,  On },  // 0x33  ..██..██
    { Off, Off,  On,  On, Off,  On, Off, Off },  // 0x34  ..██.█..
    { Off, Off,  On,  On, Off,  On, Off,  On },  // 0x35  ..██.█.█
    { Off, Off,  On,  On, Off,  On,  On, Off },  // 0x36  ..██.██.
    { Off, Off,  On,  On, Off,  On,  On,  On },  // 0x37  ..██.███
    { Off, Off,  On,  On,  On, Off, Off, Off },  // 0x38  ..███...
    { Off, Off,  On,  On,  On, Off, Off,  On },  // 0x39  ..███..█
    { Off, Off,  On,  On,  On, Off,  On, Off },  // 0x3A  ..███.█.
    { Off, Off,  On,  On,  On, Off,  On,  On },  // 0x3B  ..███.██
    { Off, Off,  On,  On,  On,  On, Off, Off },  // 0x3C  ..████..
    { Off, Off,  On,  On,  On,  On, Off,  On },  // 0x3D  ..████.█
    { Off, Off,  On,  On,  On,  On,  On, Off },  // 0x3E  ..█████.
    { Off, Off,  On,  On,  On,  On,  On,  On },  // 0x3F  ..██████
    { Off,  On, Off, Off, Off, Off, Off, Off },  // 0x40  .█......
    { Off,  On, Off, Off, Off, Off, Off,  On },  // 0x41  .█.....█
    { Off,  On, Off, Off, Off, Off,  On, Off },  // 0x42  .█....█.
    { Off,  On, Off, Off, Off, Off,  On,  On },  // 0x43  .█....██
    { Off,  On, Off, Off, Off,  On, Off, Off },  // 0x44  .█...█..
    { Off,  On, Off, Off, Off,  On, Off,  On },  // 0x45  .█...█.█
    { Off,  On, Off, Off, Off,  On,  On, Off },  // 0x46  .█...██.
    { Off,  On, Off, Off, Off,  On,  On,  On },  // 0x47  .█...███
    { Off,  On, Off, Off,  On, Off, Off, Off },  // 0x48  .█..█...
    { Off,  On, Off, Off,  On, Off, Off,  On },  // 0x49  .█..█..█
    { Off,  On, Off, Off,  On, Off,  On, Off },  // 0x4A  .█..█.█.
    { Off,  On, Off, Off,  On, Off,  On,  On },  // 0x4B  .█..█.██
    { Off,  On, Off, Off,  On,  On, Off, Off },  // 0x4C  .█..██..
    { Off,  On, Off, Off,  On,  On, Off,  On },  // 0x4D  .█..██.█
    { Off,  On, Off, Off,  On,  On,  On, Off },  // 0x4E  .█..███.
    { Off,  On, Off, Off,  On,  On,  On,  On },  // 0x4F  .█..████
    { Off,  On, Off,  On, Off, Off, Off, Off },  // 0x50  .█.█....
    { Off,  On, Off,  On, Off, Off, Off,  On },  // 0x51  .█.█...█
    { Off,  On, Off,  On, Off, Off,  On, Off },  // 0x52  .█.█..█.
    { Off,  On, Off,  On, Off, Off,  On,  On },  // 0x53  .█.█..██
    { Off,  On, Off,  On, Off,  On, Off, Off },  // 0x54  .█.█.█..
    { Off,  On, Off,  On, Off,  On, Off,  On },  // 0x55  .█.█.█.█
    { Off,  On, Off,  On, Off,  On,  On, Off },  // 0x56  .█.█.██.
    { Off,  On, Off,  On, Off,  On,  On,  On },  // 0x57  .█.█.███
    { Off,  On, Off,  On,  On, Off, Off, Off },  // 0x58  .█.██...
    { Off,  On, Off,  On,  On, Off, Off,  On },  // 0x59  .█.██..█
    { Off,  On, Off,  On,  On, Off,  On, Off },  // 0x5A  .█.██.█.
    { Off,  On, Off,  On,  On, Off,  On,  On },  // 0x5B  .█.██.██
    { Off,  On, Off,  On,  On,  On, Off, Off },  // 0x5C  .█.███..
    { Off,  On, Off,  On,  On,  On, Off,  On },  // 0x5D  .█.███.█
    { Off,  On, Off,  On,  On,  On,  On, Off },  // 0x5E  .█.████.
    { Off,  On, Off,  On,  On,  On,  On,  On },  // 0x5F  .█.█████
    { Off,  On,  On, Off, Off, Off, Off, Off },  // 0x60  .██.....
    { Off,  On,  On, Off, Off, Off, Off,  On },  // 0x61  .██....█
    { Off,  On,  On, Off, Off, Off,  On, Off },  // 0x62  .██...█.
    { Off,  On,  On, Off, Off, Off,  On,  On },  // 0x63  .██...██
    { Off,  On,  On, Off, Off,  On, Off, Off },  // 0x64  .██..█..
    { Off,  On,  On, Off, Off,  On, Off,  On },  // 0x65  .██..█.█
    { Off,  On,  On, Off, Off,  On,  On, Off },  // 0x66  .██..██.
    { Off,  On,  On, Off, Off,  On,  On,  On },  // 0x67  .██..███
    { Off,  On,  On, Off,  On, Off, Off, Off },  // 0x68  .██.█...
    { Off,  On,  On, Off,  On, Off, Off,  On },  // 0x69  .██.█..█
    { Off,  On,  On, Off,  On, Off,  On, Off },  // 0x6A  .██.█.█.
    { Off,  On,  On, Off,  On, Off,  On,  On },  // 0x6B  .██.█.██
    { Off,  On,  On, Off,  On,  On, Off, Off },  // 0x6C  .██.██..
    { Off,  On,  On, Off,  On,  On, Off,  On },  // 0x6D  .██.██.█
    { Off,  On,  On, Off,  On,  On,  On, Off },  // 0x6E  .██.███.
    { Off,  On,  On, Off,  On,  On,  On,  On },  // 0x6F  .██.████
    { Off,  On,  On,  On, Off, Off, Off, Off },  // 0x70  .███....
    { Off,  On,  On,  On, Off, Off, Off,  On },  // 0x71  .███...█
    { Off,  On,  On,  On, Off, Off,  On, Off },  // 0x72  .███..█.
    { Off,  On,  On,  On, Off, Off,  On,  On },  // 0x73  .███..██
    { Off,  On,  On,  On, Off,  On, Off, Off },  // 0x74  .███.█..
    { Off,  On,  On,  On, Off,  On, Off,  On },  // 0x75  .███.█.█
    { Off,  On,  On,  On, Off,  On,  On, Off },  // 0x76  .███.██.
    { Off,  On,  On,  On, Off,  On,  On,  On },  // 0x77  .███.███
    { Off,  On,  On,  On,  On, Off, Off, Off },  // 0x78  .████...
    { Off,  On,  On,  On,  On, Off, Off,  On },  // 0x79  .████..█
    { Off,  On,  On,  On,  On, Off,  On, Off },  // 0x7A  .████.█.
    { Off,  On,  On,  On,  On, Off,  On,  On },  // 0x7B  .████.██
    { Off,  On,  On,  On,  On,  On, Off, Off },  // 0x7C  .█████..
    { Off,  On,  On,  On,  On,  On, Off,  On },  // 0x7D  .█████.█
    { Off,  On,  On,  On,  On,  On,  On, Off },  // 0x7E  .██████.
    { Off,  On,  On,  On,  On,  On,  On,  On },  // 0x7F  .███████
    {  On, Off, Off, Off, Off, Off, Off, Off },  // 0x80  █.......
    {  On, Off, Off, Off, Off, Off, Off,  On },  // 0x81  █......█
    {  On, Off, Off, Off, Off, Off,  On, Off },  // 0x82  █.....█.
    {  On, Off, Off, Off, Off, Off,  On,  On },  // 0x83  █.....██
    {  On, Off, Off, Off, Off,  On, Off, Off },  // 0x84  █....█..
    {  On, Off, Off, Off, Off,  On, Off,  On },  // 0x85  █....█.█
    {  On, Off, Off, Off, Off,  On,  On, Off },  // 0x86  █....██.
    {  On, Off, Off, Off, Off,  On,  On,  On },  // 0x87  █....███
    {  On, Off, Off, Off,  On, Off, Off, Off },  // 0x88  █...█...
    {  On, Off, Off, Off,  On, Off, Off,  On },  // 0x89  █...█..█
    {  On, Off, Off, Off,  On, Off,  On, Off },  // 0x8A  █...█.█.
    {  On, Off, Off, Off,  On, Off,  On,  On },  // 0x8B  █...█.██
    {  On, Off, Off, Off,  On,  On, Off, Off },  // 0x8C  █...██..
    {  On, Off, Off, Off,  On,  On, Off,  On },  // 0x8D  █...██.█
    {  On, Off, Off, Off,  On,  On,  On, Off },  // 0x8E  █...███.
    {  On, Off, Off, Off,  On,  On,  On,  On },  // 0x8F  █...████
    {  On, Off, Off,  On, Off, Off, Off, Off },  // 0x90  █..█....
    {  On, Off, Off,  On, Off, Off, Off,  On },  // 0x91  █..█...█
    {  On, Off, Off,  On, Off, Off,  On, Off },  // 0x92  █..█..█.
    {  On, Off, Off,  On, Off, Off,  On,  On },  // 0x93  █..█..██
    {  On, Off, Off,  On, Off,  On, Off, Off },  // 0x94  █..█.█..
    {  On, Off, Off,  On, Off,  On, Off,  On },  // 0x95  █..█.█.█
    {  On, Off, Off,  On, Off,  On,  On, Off },  // 0x96  █..█.██.
    {  On, Off, Off,  On, Off,  On,  On,  On },  // 0x97  █..█.███
    {  On, Off, Off,  On,  On, Off, Off, Off },  // 0x98  █..██...
    {  On, Off, Off,  On,  On, Off, Off,  On },  // 0x99  █..██..█
    {  On, Off, Off,  On,  On, Off,  On, Off },  // 0x9A  █..██.█.
    {  On, Off, Off,  On,  On, Off,  On,  On },  // 0x9B  █..██.██
    {  On, Off, Off,  On,  On,  On, Off, Off },  // 0x9C  █..███..
    {  On, Off, Off,  On,  On,  On, Off,  On },  // 0x9D  █..███.█
    {  On, Off, Off,  On,  On,  On,  On, Off },  // 0x9E  █..████.
    {  On, Off, Off,  On,  On,  On,  On,  On },  // 0x9F  █..█████
    {  On, Off,  On, Off, Off, Off, Off, Off },  // 0xA0  █.█.....
    {  On, Off,  On, Off, Off, Off, Off,  On },  // 0xA1  █.█....█
    {  On, Off,  On, Off, Off, Off,  On, Off },  // 0xA2  █.█...█.
    {  On, Off,  On, Off, Off, Off,  On,  On },  // 0xA3  █.█...██
    {  On, Off,  On, Off, Off,  On, Off, Off },  // 0xA4  █.█..█..
    {  On, Off,  On, Off, Off,  On, Off,  On },  // 0xA5  █.█..█.█
    {  On, Off,  On, Off, Off,  On,  On, Off },  // 0xA6  █.█..██.
    {  On, Off,  On, Off, Off,  On,  On,  On },  // 0xA7  █.█..███
    {  On, Off,  On, Off,  On, Off, Off, Off },  // 0xA8  █.█.█...
    {  On, Off,  On, Off,  On, Off, Off,  On },  // 0xA9  █.█.█..█
    {  On, Off,  On, Off,  On, Off,  On, Off },  // 0xAA  █.█.█.█.
    {  On, Off,  On, Off,  On, Off,  On,  On },  // 0xAB  █.█.█.██
    {  On, Off,  On, Off,  On,  On, Off, Off },  // 0xAC  █.█.██..
    {  On, Off,  On, Off,  On,  On, Off,  On },  // 0xAD  █.█.██.█
    {  On, Off,  On, Off,  On,  On,  On, Off },  // 0xAE  █.█.███.
    {  On, Off,  On, Off,  On,  On,  On,  On },  // 0xAF  █.█.████
    {  On, Off,  On,  On, Off, Off, Off, Off },  // 0xB0  █.██....
    {  On, Off,  On,  On, Off, Off, Off,  On },  // 0xB1  █.██...█
    {  On, Off,  On,  On, Off, Off,  On, Off },  // 0xB2  █.██..█.
    {  On, Off,  On,  On, Off, Off,  On,  On },  // 0xB3  █.██..██
    {  On, Off,  On,  On, Off,  On, Off, Off },  // 0xB4  █.██.█..
    {  On, Off,  On,  On, Off,  On, Off,  On },  // 0xB5  █.██.█.█
    {  On, Off,  On,  On, Off,  On,  On, Off },  // 0xB6  █.██.██.
    {  On, Off,  On,  On, Off,  On,  On,  On },  // 0xB7  █.██.███
    {  On, Off,  On,  On,  On, Off, Off, Off },  // 0xB8  █.███...
    {  On, Off,  On,  On,  On, Off, Off,  On },  // 0xB9  █.███..█
    {  On, Off,  On,  On,  On, Off,  On, Off },  // 0xBA  █.███.█.
    {  On, Off,  On,  On,  On, Off,  On,  On },  // 0xBB  █.███.██
    {  On, Off,  On,  On,  On,  On, Off, Off },  // 0xBC  █.████..
    {  On, Off,  On,  On,  On,  On, Off,  On },  // 0xBD  █.████.█
    {  On, Off,  On,  On,  On,  On,  On, Off },  // 0xBE  █.█████.
    {  On, Off,  On,  On,  On,  On,  On,  On },  // 0xBF  █.██████
    {  On,  On, Off, Off, Off, Off, Off, Off },  // 0xC0  ██......
    {  On,  On, Off, Off, Off, Off, Off,  On },  // 0xC1  ██.....█
    {  On,  On, Off, Off, Off, Off,  On, Off },  // 0xC2  ██....█.
    {  On,  On, Off, Off, Off, Off,  On,  On },  // 0xC3  ██....██
    {  On,  On, Off, Off, Off,  On, Off, Off },  // 0xC4  ██...█..
    {  On,  On, Off, Off, Off,  On, Off,  On },  // 0xC5  ██...█.█
    {  On,  On, Off, Off, Off,  On,  On, Off },  // 0xC6  ██...██.
    {  On,  On, Off, Off, Off,  On,  On,  On },  // 0xC7  ██...███
    {  On,  On, Off, Off,  On, Off, Off, Off },  // 0xC8  ██..█...
    {  On,  On, Off, Off,  On, Off, Off,  On },  // 0xC9  ██..█..█
    {  On,  On, Off, Off,  On, Off,  On, Off },  // 0xCA  ██..█.█.
    {  On,  On, Off, Off,  On, Off,  On,  On },  // 0xCB  ██..█.██
    {  On,  On, Off, Off,  On,  On, Off, Off },  // 0xCC  ██..██..
    {  On,  On, Off, Off,  On,  On, Off,  On },  // 0xCD  ██..██.█
    {  On,  On, Off, Off,  On,  On,  On, Off },  // 0xCE  ██..███.
    {  On,  On, Off, Off,  On,  On,  On,  On },  // 0xCF  ██..████
    {  On,  On, Off,  On, Off, Off, Off, Off },  // 0xD0  ██.█....
    {  On,  On, Off,  On, Off, Off, Off,  On },  // 0xD1  ██.█...█
    {  On,  On, Off,  On, Off, Off,  On, Off },  // 0xD2  ██.█..█.
    {  On,  On, Off,  On, Off, Off,  On,  On },  // 0xD3  ██.█..██
    {  On,  On, Off,  On, Off,  On, Off, Off },  // 0xD4  ██.█.█..
    {  On,  On, Off,  On, Off,  On, Off,  On },  // 0xD5  ██.█.█.█
    {  On,  On, Off,  On, Off,  On,  On, Off },  // 0xD6  ██.█.██.
    {  On,  On, Off,  On, Off,  On,  On,  On },  // 0xD7  ██.█.███
    {  On,  On, Off,  On,  On, Off, Off, Off },  // 0xD8  ██.██...
    {  On,  On, Off,  On,  On, Off, Off,  On },  // 0xD9  ██.██..█
    {  On,  On, Off,  On,  On, Off,  On, Off },  // 0xDA  ██.██.█.
    {  On,  On, Off,  On,  On, Off,  On,  On },  // 0xDB  ██.██.██
    {  On,  On, Off,  On,  On,  On, Off, Off },  // 0xDC  ██.███..
    {  On,  On, Off,  On,  On,  On, Off,  On },  // 0xDD  ██.███.█
    {  On,  On, Off,  On,  On,  On,  On, Off },  // 0xDE  ██.████.
    {  On,  On, Off,  On,  On,  On,  On,  On },  // 0xDF  ██.█████
    {  On,  On,  On, Off, Off, Off, Off, Off },  // 0xE0  ███.....
    {  On,  On,  On, Off, Off, Off, Off,  On },  // 0xE1  ███....█
    {  On,  On,  On, Off, Off, Off,  On, Off },  // 0xE2  ███...█.
    {  On,  On,  On, Off, Off, Off,  On,  On },  // 0xE3  ███...██
    {  On,  On,  On, Off, Off,  On, Off, Off },  // 0xE4  ███..█..
    {  On,  On,  On, Off, Off,  On, Off,  On },  // 0xE5  ███..█.█
    {  On,  On,  On, Off, Off,  On,  On, Off },  // 0xE6  ███..██.
    {  On,  On,  On, Off, Off,  On,  On,  On },  // 0xE7  ███..███
    {  On,  On,  On, Off,  On, Off, Off, Off },  // 0xE8  ███.█...
    {  On,  On,  On, Off,  On, Off, Off,  On },  // 0xE9  ███.█..█
    {  On,  On,  On, Off,  On, Off,  On, Off },  // 0xEA  ███.█.█.
    {  On,  On,  On, Off,  On, Off,  On,  On },  // 0xEB  ███.█.██
    {  On,  On,  On, Off,  On,  On, Off, Off },  // 0xEC  ███.██..
    {  On,  On,  On, Off,  On,  On, Off,  On },  // 0xED  ███.██.█
    {  On,  On,  On, Off,  On,  On,  On, Off },  // 0xEE  ███.███.
    {  On,  On,  On, Off,  On,  On,  On,  On },  // 0xEF  ███.████
    {  On,  On,  On,  On, Off, Off, Off, Off },  // 0xF0  ████....
    {  On,  On,  On,  On, Off, Off, Off,  On },  // 0xF1  ████...█
    {  On,  On,  On,  On, Off, Off,  On, Off },  // 0xF2  ████..█.
    {  On,  On,  On,  On, Off, Off,  On,  On },  // 0xF3  ████..██
    {  On,  On,  On,  On, Off,  On, Off, Off },  // 0xF4  ████.█..
    {  On,  On,  On,  On, Off,  On, Off,  On },  // 0xF5  ████.█.█
    {  On,  On,  On,  On, Off,  On,  On, Off },  // 0xF6  ████.██.
    {  On,  On,  On,  On, Off,  On,  On,  On },  // 0xF7  ████.███
    {  On,  On,  On,  On,  On, Off, Off, Off },  // 0xF8  █████...
    {  On,  On,  On,  On,  On, Off, Off,  On },  // 0xF9  █████..█
    {  On,  On,  On,  On,  On, Off,  On, Off },  // 0xFA  █████.█.
    {  On,  On,  On,  On,  On, Off,  On,  On },  // 0xFB  █████.██
    {  On,  On,  On,  On,  On,  On, Off, Off },  // 0xFC  ██████..
    {  On,  On,  On,  On,  On,  On, Off,  On },  // 0xFD  ██████.█
    {  On,  On,  On,  On,  On,  On,  On, Off },  // 0xFE  ███████.
    {  On,  On,  On,  On,  On,  On,  On,  On },  // 0xFF  ████████
};
