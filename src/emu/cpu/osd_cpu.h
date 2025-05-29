/*******************************************************************************
*                                                                              *
*  Define size independent data types and operations.                          *
*                                                                              *
*   The following types must be supported by all platforms:                    *
*                                                                              *
*  UINT8  - Unsigned 8-bit Integer    INT8  - Signed 8-bit integer             *
*  UINT16 - Unsigned 16-bit Integer  INT16 - Signed 16-bit integer             *
*  UINT32 - Unsigned 32-bit Integer  INT32 - Signed 32-bit integer             *
*                                                                              *
*   The macro names for the artithmatic operations are composed as follows:    *
*                                                                              *
*   XXX_R_A_B, where XXX - 3 letter operation code (ADD, SUB, etc.)            *
*           R   - The type of the result                                       *
*           A   - The type of operand 1                                        *
*           B   - The type of operand 2 (if binary operation)                  *
*                                                                              *
*             Each type is one of: U8,8,U16,16,U32,32,U64,64                   *
*                                                                              *
*******************************************************************************/

#ifndef _OSD_CPU_H_
#define _OSD_CPU_H_

typedef unsigned char UINT8;
typedef unsigned short UINT16;
typedef unsigned int UINT32;
typedef signed char INT8;
typedef signed short INT16;
typedef signed int INT32;

/******************************************************************************
 * Union of UINT8, UINT16 and UINT32 in native endianess of the target
 * This is used to access bytes and words in a machine independent manner.
 * The upper bytes h2 and h3 normally contain zero (16 bit CPU cores)
 * thus PAIR.d can be used to pass arguments to the memory system
 * which expects 'int' really.
 ******************************************************************************/
typedef union
{
    struct { UINT8 h3,h2,h,l; } b;
    struct { UINT16 h,l; } w;
    UINT32 d;
} PAIR;

#endif  /* _OSD_CPU_H_ */
