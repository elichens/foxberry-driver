/**********************************************************************//**
 * @file xprintf.c
 * @brief  Universal string handler for user console interface
 *
 *  Copyright (C) 2011, ChaN, all right reserved.
 *
 *  This software is a free software and there is NO WARRANTY.
 *  No restriction on use. You can use, modify and redistribute it for
 *  personal, non-profit or commercial products UNDER YOUR RESPONSIBILITY.
 *  Redistributions of source code must retain the above copyright notice.
 *
 *************************************************************************/

#include <stdint.h>

#include "xprintf.h"


#if _USE_XFUNC_OUT
#include <stdarg.h>
void         ( *xfunc_out )( unsigned char ); /* Pointer to the output stream */
static char *outptr;

/*----------------------------------------------*/
/* Put a character                              */
/*----------------------------------------------*/

void xputc ( char c )
{
    if ( _CR_CRLF && ( c == '\n' ) )
    {
        xputc( '\r' );                                  /* CR -> CRLF */
    }

    if ( outptr )
    {
        *outptr++ = (unsigned char)c;
        return;
    }

    if ( xfunc_out ) { xfunc_out( (unsigned char)c ); }
}


/*----------------------------------------------*/
/* Put a null-terminated string                 */
/*----------------------------------------------*/

void xputs (                                    /* Put a string to the default device */
    const char*str                              /* Pointer to the string */
    )
{
    while ( *str )
    {
        xputc( *str++ );
    }
}


/**********************************************************************//**
 * @brief Put a string to the specified device
 * @param func Pointer to the output function
 * @param str Pointer to the string
 *************************************************************************/
void xfputs ( void ( *func )( unsigned char ), const char*str )
{
    void ( *pf )( unsigned char );


    pf        = xfunc_out;      /* Save current output device */
    xfunc_out = func;           /* Switch output to specified device */

    while ( *str )              /* Put the string */
    {
        xputc( *str++ );
    }

    xfunc_out = pf;             /* Restore output device */
}


/**********************************************************************//**
 * @brief Formatted string output
 *  xprintf("%d", 1234);			"1234"
 *  xprintf("%6d,%3d%%", -200, 5);	"  -200,  5%"
 *  xprintf("%-6u", 100);			"100   "
 *  xprintf("%ld", 12345678L);		"12345678"
 *  xprintf("%04x", 0xA3);			"00a3"
 *  xprintf("%08LX", 0x123ABC);		"00123ABC"
 *  xprintf("%016b", 0x550F);		"0101010100001111"
 *  xprintf("%s", "String");		"String"
 *  xprintf("%-4s", "abc");			"abc "
 *  xprintf("%4s", "abc");			" abc"
 *  xprintf("%c", 'a');				"a"
 *  xprintf("%f", 10.0);            <xprintf lacks floating point support>
 * @param fmt Pointer to the format string
 * @param arp Pointer to arguments
 *************************************************************************/
static void xvprintf ( const char*fmt, va_list arp )
{
    unsigned int  r, i, j, w, f;
    unsigned long v;
    char          s[ 16 ], c, d, *p;

    for (;;)
    {
        c = *fmt++;                                             /* Get a char */

        if ( !c )
        {
            break;                                      /* End of format? */
        }

        if ( c != '%' )                                 /* Pass through it if not a % sequense */
        {
            xputc( c ); continue;
        }

        f = 0;
        c = *fmt++;                                             /* Get first char of the sequense */

        if ( c == '0' )                                 /* Flag: '0' padded */
        {
            f = 1; c = *fmt++;
        }
        else
        {
            if ( c == '-' )                             /* Flag: left justified */
            {
                f = 2; c = *fmt++;
            }
        }

        for ( w = 0; c >= '0' && c <= '9'; c = *fmt++ )         /* Minimum width */
        {
            w = w * 10 + c - '0';
        }

        if ( ( c == 'l' ) || ( c == 'L' ) )     /* Prefix: Size is long int */
        {
            f |= 4; c = *fmt++;
        }

        if ( !c )
        {
            break;                                      /* End of format? */
        }

        d = c;

        if ( d >= 'a' ) { d -= 0x20; }

        switch ( d )                                    /* Type is... */
        {
            case 'S':                                           /* String */
                p = va_arg( arp, char* );

                for ( j = 0; p[ j ]; j++ )
                {
                    ;
                }

                while ( !( f & 2 ) && j++ < w )
                {
                    xputc( ' ' );
                }

                xputs( p );

                while ( j++ < w )
                {
                    xputc( ' ' );
                }

                continue;
            case 'C':                                           /* Character */
                xputc( (char)va_arg( arp, int ) ); continue;
            case 'B':                                           /* Binary */
                r = 2; break;
            case 'O':                                           /* Octal */
                r = 8; break;
            case 'D':                                           /* Signed decimal */
            case 'U':                                           /* Unsigned decimal */
                r = 10; break;
            case 'X':                                           /* Hexdecimal */
                r = 16; break;
            default:                                            /* Unknown type (passthrough) */
                xputc( c ); continue;
        }

        /* Get an argument and put it in numeral */
        v = ( f & 4 ) ? va_arg( arp, long ) : ( ( d == 'D' ) ? (long)va_arg( arp, int ) : (long)va_arg( arp, unsigned int ) );

        if ( ( d == 'D' ) && ( v & 0x80000000 ) )
        {
            v  = 0 - v;
            f |= 8;
        }

        i = 0;

        do
        {
            d = (char)( v % r ); v /= r;

            if ( d > 9 ) { d += ( c == 'x' ) ? 0x27 : 0x07; }

            s[ i++ ] = d + '0';
        }
        while ( v && i < sizeof( s ) );

        if ( f & 8 ) { s[ i++ ] = '-'; }

        j = i; d = ( f & 1 ) ? '0' : ' ';

        while ( !( f & 2 ) && j++ < w )
        {
            xputc( d );
        }

        do
        {
            xputc( s[ --i ] );
        }
        while ( i );

        while ( j++ < w )
        {
            xputc( ' ' );
        }
    }
}


/**********************************************************************//**
 * @brief Put a formatted string to the default device
 * @param fmt Pointer to the format string
 * @param ... Optional arguments
 *************************************************************************/
void xprintf ( const char*fmt, ... )
{
    va_list arp;

    va_start( arp, fmt );
    xvprintf( fmt, arp );
    va_end( arp );
}


/**********************************************************************//**
 * @brief Put a formatted string to the memory
 * @param buff Pointer to the output buffer
 * @param fmt Pointer to the format string
 * @param ... Optional arguments
 *************************************************************************/
void xsprintf ( char *buff, const char*fmt, ... )
{
    va_list arp;

    outptr = buff;              /* Switch destination for memory */

    va_start( arp, fmt );
    xvprintf( fmt, arp );
    va_end( arp );

    *outptr = 0;                /* Terminate output string with a \0 */
    outptr  = 0;                /* Switch destination for device */
}


/**********************************************************************//**
 * @brief Put a formatted string to the memory
 * @param buff Pointer to the output buffer
 * @param fmt Pointer to the format string
 * @param arp Pointer to arguments
 *************************************************************************/
void xvsprintf ( char *buff, const char*fmt, va_list arp )
{
    outptr = buff;              /* Switch destination for memory */

    xvprintf( fmt, arp );

    *outptr = 0;                /* Terminate output string with a \0 */
    outptr  = 0;                /* Switch destination for device */
}


/**********************************************************************//**
 * @brief Put a formatted string to the specified device
 * @param func Pointer to the output function
 * @param fmt Pointer to the format string
 * @param ... Optional arguments
 *************************************************************************/
void xfprintf ( void ( *func )( unsigned char ), const char*fmt, ... )
{
    va_list arp;

    void    ( *pf )( unsigned char );

    pf        = xfunc_out;      /* Save current output device */
    xfunc_out = func;           /* Switch output to specified device */

    va_start( arp, fmt );
    xvprintf( fmt, arp );
    va_end( arp );

    xfunc_out = pf;             /* Restore output device */
}


/**********************************************************************//**
 * @brief Dump a line of binary dump
 * @param buff Pointer to the array to be dumped
 * @param addr Heading address value
 * @param len Number of items to be dumped
 * @param width Size of the items (DF_CHAR, DF_SHORT, DF_LONG)
 *************************************************************************/
void put_dump ( const void *buff, unsigned long addr, int len, int width )
{
    int                   i;
    const unsigned char  *bp;
    const unsigned short *sp;
    const unsigned long  *lp;

    xprintf( "%08lX ", addr ); /* address */

    switch ( width )
    {
        case DW_CHAR:
            bp = buff;

            for ( i = 0; i < len; i++ ) /* Hexdecimal dump */
            {
                xprintf( " %02X", bp[ i ] );
            }

            xputc( ' ' );

            for ( i = 0; i < len; i++ ) /* ASCII dump */
            {
                xputc( ( bp[ i ] >= ' ' && bp[ i ] <= '~' ) ? bp[ i ] : '.' );
            }

            break;
        case DW_SHORT:
            sp = buff;

            do /* Hexdecimal dump */
            {
                xprintf( " %04X", *sp++ );
            }
            while ( --len );

            break;
        case DW_LONG:
            lp = buff;

            do /* Hexdecimal dump */
            {
                xprintf( " %08LX", *lp++ );
            }
            while ( --len );

            break;
    }

    xputc( '\n' );
}


#endif /* _USE_XFUNC_OUT */



#if _USE_XFUNC_IN
unsigned char ( *xfunc_in )( void );        /* Pointer to the input stream */


/**********************************************************************//**
 * @brief Get a line from the input
 * @param buff Pointer to the buffer
 * @param len Buffer length
 * @return 0:End of stream, 1:A line arrived
 *************************************************************************/
int xgets ( char*buff, int len )
{
    int c, i;

    if ( !xfunc_in )
    {
        return 0; /* No input function specified */
    }

    i = 0;

    for (;;)
    {
        c = xfunc_in(); /* Get a char from the incoming stream */

        if ( !c )
        {
            return 0; /* End of stream? */
        }

        if ( c == '\r' )
        {
            break; /* End of line? */
        }

        if ( ( c == '\b' ) && i ) /* Back space? */
        {
            i--;

            if ( _LINE_ECHO ) { xputc( c ); }

            continue;
        }

        if ( ( c >= ' ' ) && ( i < len - 1 ) )  /* Visible chars */
        {
            buff[ i++ ] = c;

            if ( _LINE_ECHO ) { xputc( c ); }
        }
    }

    buff[ i ] = 0;      /* Terminate with a \0 */

    if ( _LINE_ECHO ) { xputc( '\n' ); }

    return 1;
}


/**********************************************************************//**
 * @brief Read formatted string from input device
 * @param func Pointer to the input stream function
 * @param buff Pointer to the buffer
 * @param len Buffer length
 * @return 0:End of stream, 1:A line arrived
 *************************************************************************/
int xfgets ( unsigned char ( *func )( void ), char *buff, int len )
{
    unsigned char ( *pf )( void );
    int           n;

    pf       = xfunc_in;           /* Save current input device */
    xfunc_in = func;               /* Switch input to specified device */
    n        = xgets( buff, len ); /* Get a line */
    xfunc_in = pf;                 /* Restore input device */

    return n;
}


/**********************************************************************//**
 * @brief Get a value of the string
 *	"123 -5   0x3ff 0b1111 0377  w "
 *   ^                             1st call returns 123 and next ptr
 *       ^                         2nd call returns -5 and next ptr
 *            ^                    3rd call returns 1023 and next ptr
 *                  ^              4th call returns 15 and next ptr
 *                         ^       5th call returns 255 and next ptr
 *                               ^ 6th call fails and returns 0
 * @param str Pointer to pointer to the string
 * @param res Pointer to the variable to store the value
 * @return 0:Failed, 1:Successful
 *************************************************************************/
int xatoi ( char **str, long  *res )
{
    unsigned long val;
    unsigned char c, r, s = 0;

    *res = 0;

    while ( ( c = **str ) == ' ' )
    {
        ( *str )++; /* Skip leading spaces */
    }

    if ( c == '-' )             /* negative? */
    {
        s = 1;
        c = *( ++( *str ) );
    }

    if ( c == '0' )
    {
        c = *( ++( *str ) );

        switch ( c )
        {
            case 'x':                   /* hexdecimal */
                r = 16; c = *( ++( *str ) );
                break;
            case 'b':                   /* binary */
                r = 2; c = *( ++( *str ) );
                break;
            default:

                if ( c <= ' ' )
                {
                    return 1; /* single zero */
                }

                if ( ( c < '0' ) || ( c > '9' ) )
                {
                    return 0; /* invalid char */
                }

                r = 8; /* octal */
        }
    }
    else
    {
        if ( ( c < '0' ) || ( c > '9' ) )
        {
            return 0; /* EOL or invalid char */
        }

        r = 10; /* decimal */
    }

    val = 0;

    while ( c > ' ' )
    {
        if ( c >= 'a' ) { c -= 0x20; }

        c -= '0';

        if ( c >= 17 )
        {
            c -= 7;

            if ( c <= 9 )
            {
                return 0; /* invalid char */
            }
        }

        if ( c >= r )
        {
            return 0; /* invalid char for current radix */
        }

        val = val * r + c;
        c   = *( ++( *str ) );
    }

    if ( s )
    {
        val = 0 - val; /* apply sign if needed */
    }

    *res = val;
    return 1;
}


/**********************************************************************//**
 * @brief convert one hexadecimal digit (0..9,a..f,A..F) to corresponding binary value
 * @param c hexadecimal digit
 * @return binary value
 *************************************************************************/
int xhtoi( char c )
{
    int res = 0;

    if ( ( c >= '0' ) && ( c <= '9' ) )
    {
        res = c - '0';
    }
    else if ( ( c >= 'a' ) && ( c <= 'f' ) )
    {
        res = c - 'a' + 10;
    }
    else if ( ( c >= 'A' ) && ( c <= 'F' ) )
    {
        res = c - 'A' + 10;
    }

    return res;
}


#endif /* _USE_XFUNC_IN */


/**********************************************************************//**
 * @brief Append s2 at the end of s1 (including terminating null char)
 * @param s1 initial string (null terminated)
 * @param s2 string to be added at the end of s1  (null terminated)
 *************************************************************************/
void xstrcat( char *s1, char *s2 )
{
    // go to end of s1
    while ( *s1 != 0 )
    {
        s1++;
    }

    // append s2 at the end of s1 (including terminating null char)
    *s1 = *s2;

    while ( *s2 != 0 )
    {
        s1++;
        s2++;
        *s1 = *s2;
    }
}


/**********************************************************************//**
 * @brief Return length of null-terminated string
 * @param string null-terminated string
 * @return number of char in string (excluding null-terminating char)
 *************************************************************************/
uint32_t xstrlen( char *string )
{
    uint32_t len = 0;

    // search end of string while counting chars
    while ( *string != 0 )
    {
        string++;
        len++;
    }

    return len;
}
