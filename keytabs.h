/****************************************************************************
*                                                                           *
*                            Third Year Project                             *
*                                                                           *
*                            An IBM PC Emulator                             *
*                          For Unix and X Windows                           *
*                                                                           *
*                             By David Hedley                               *
*                                                                           *
*                                                                           *
* This program is Copyrighted.  Consult the file COPYRIGHT for more details *
*                                                                           *
****************************************************************************/

BYTE scan_unshifted[256] =
{
     0 ,0x1b, '1', '2', '3', '4', '5', '6',
    '7', '8', '9', '0', '-', '=',0x08,0x09,
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i',
    'o', 'p', '[', ']',0x0d,  0 , 'a', 's',
    'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',
    '\'','`',  0 , 
#ifdef KBUK     /* Keycode 0x2b */
     '#', 
#else
     '\\',
#endif
     'z', 'x', 'c', 'v',
     'b', 'n', 'm', ',', '.', '/',  0 , '*',
      0 , ' ',  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,
      0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 , '7',
     '8', '9', '-', '4', '5', '6', '+', '1',
     '2', '3', '0', '.',  0 ,  0 , '\\'
};


BYTE scan_shift[256] =
{
     0 ,0x1b, '!', 
#ifdef KBUK     /* Keycode 0x03 and 0x04 + shift */
     '"',0x9c,
#else
     '@','#',
#endif
     '$', '%', '^',
     '&', '*', '(', ')', '_', '+',0x08,  0 ,
     'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I',
     'O', 'P', '{', '}',0x0d,  0 , 'A', 'S',
     'D', 'F', 'G', 'H', 'J', 'K', 'L', ':',
#ifdef KBUK     /* Keycode 0x28 and 0x29 + shift */
     '@',0xaa,
#else
     '"','~',
#endif
     0 , 
#ifdef KBUK
     '~', 
#else
     '|',
#endif
    'Z', 'X', 'C', 'V',
    'B', 'N', 'M', '<', '>', '?', '/', '*',
     0 , ' ',  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,
     0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0  ,'7',
    '8', '9', '-', '4', '5', '6', '+', '1',
    '2', '3', '0', '.',  0 ,  0 , '|'
};


BYTE scan_alt[256] = 
{
    0x00,0x01,0x78,0x79,0x7a,0x7b,0x7c,0x7d,
    0x7e,0x7f,0x80,0x81,0x82,0x83,0x0e,0xa5,
    0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,
    0x18,0x19,0x1a,0x1b,0x1c,0x00,0x1e,0x1f,
    0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,
    0x28,0x29,0x00,0x2b,0x2c,0x2d,0x2e,0x2f,
    0x30,0x31,0x32,0x33,0x34,0x35,0xa4,0x37,
    0x00,0x39,0x00,0x68,0x69,0x6a,0x6b,0x6c,
    0x6d,0x6e,0x6f,0x70,0x71,0x00,0x00,0x97,
    0x98,0x99,0x4a,0x9b,0x4c,0x9d,0x4e,0x9f,
    0xa0,0xa1,0xa2,0xa3
};


BYTE scan_ctrl[256] =
{
     0 ,0x1b,  0 ,  0 ,  0 ,  0 ,  0 ,0x1e,
     0 ,  0 ,  0 ,  0 ,0x1f,  0 ,0x7f,  0 ,
    17 , 23 ,  5 , 18 , 20 , 25 , 21 ,  9 ,
    15 , 16 ,0x1b,0x1d,0x0a,  0 ,  1 , 19 ,
     4 ,  6 ,  7 ,  8 , 10 , 11 , 12 ,  0 ,
     0 ,0x1c,  0 ,  0 , 26 , 24 ,  3 , 22 ,
     2 , 14 , 13
};

BYTE scan_ctrl_high[] =
{
    0x95, 0x96,    0,    0,    0, 0x5e, 0x5f, 0x60, 0x61, 0x62,
    0x63, 0x64, 0x65, 0x66, 0x67,    0,    0, 0x77, 0x8d, 0x84,
    0x8e, 0x73, 0x8f, 0x74, 0x90, 0x75, 0x91, 0x76, 0x92, 0x93,
       0,    0,    0, 0x89, 0x8a
};