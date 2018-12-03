

//#include <sys/ioctl.h>
#include <sys/time.h>
//#include <sys/socket.h>

//#include <stdlib.h>
//#include <stdio.h>
//#include <string.h>
//#include <stdint.h>
//#include <stdarg.h>
//#include <sys/types.h>
//#include <getopt.h>
#include <libusb-1.0/libusb.h>
#include "lcd.h"

#include <errno.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

int lcd_send( libusb_device_handle *handle , uint16_t request, uint16_t value, uint16_t index)
{
        //if( libusb_control_transfer( handle , ( LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_ENDPOINT_OUT ) , request , value , index , NULL , 0 , 0 ) < 0 ) {
        if( libusb_control_transfer( handle , LIBUSB_REQUEST_TYPE_VENDOR , request , value , index , NULL , 0 , 500 ) < 0) {
                printf("Send data no OK! \n");
                return -1;
        }
        //printf("Send data OK! \n");	//for test
        return 0;

}

/* command format:
// 7 6 5 4 3 2 1 0
// C C C T T T L L

// TTT = target bit map
// R = reserved for future use, set to 0
// LL = number of bytes in transfer - 1
*/

void lcd_flush( lcd_device *dev )
{
        uint16_t request, value, index;
        // anything to flush? ignore request if not
        if ( dev->buf->buffer_type == -1 ) return ;

        /* build request byte */
        request = dev->buf->buffer_type | ( dev->buf->buffer_fill - 1 ) ;
        /* fill value and index with buffer contents. endianess should IMHO not */
        /* be a problem, since libusb_control_transfer() will handle this. */
        value =  dev->buf->show_buf[1] ;
        value = ( value << 8 ) + dev->buf->show_buf[0];
        index =  dev->buf->show_buf[3] ;
        index = ( index << 8 ) + dev->buf->show_buf[2];

        /* send current buffer contents */
        lcd_send( dev->lcd_usb_handle , request , value , index ) ;

        /* buffer is now free again */
        dev->buf->buffer_type = -1 ;
        dev->buf->buffer_fill = 0 ;
}

/* enqueue a command into the buffer */
static void lcd_enqueue( lcd_device *dev , int command_type , int value )
{
        if (( dev->buf->buffer_type >= 0 ) && ( dev->buf->buffer_type != command_type ))
              lcd_flush( dev ) ;

        /* add new item to buffer */
        dev->buf->buffer_type = command_type;
        dev->buf->show_buf[dev->buf->buffer_fill++] = value;

        /* flush buffer if it's full */
        if (dev->buf->buffer_fill == BUFFER_MAX_CMD)
              lcd_flush( dev ) ;
}

/* see HD44780 datasheet for a command description */
void lcd_command( lcd_device *dev , const uint8_t ctrl , const uint8_t cmd )
{
        lcd_enqueue( dev , LCD_CMD | ctrl , cmd ) ;
}

void lcd_set( libusb_device_handle *handle , uint8_t cmd , int value )
{
        if( libusb_control_transfer( handle , LIBUSB_REQUEST_TYPE_VENDOR , cmd ,
                                     value , 0 , NULL , 0 , 1000 ) < 0 ) {
                return;
        }
}

int lcd_get( libusb_device_handle *handle , uint8_t cmd )
{
        uint8_t   buff[2];
        int       nBytes;

        /* send control request and accept return value */
        nBytes = libusb_control_transfer( handle , LIBUSB_REQUEST_TYPE_VENDOR |
                                                   LIBUSB_RECIPIENT_DEVICE |
                                                   LIBUSB_ENDPOINT_IN , cmd , 0 , 0 ,
                                          ( char * )buff , sizeof( buff ) , 1000 ) ;

        if( nBytes < 0 ) {
                return -1;
        }
        return buff[0] + 256*buff[1];
}

void lcd_backlight_set( lcd_device *dev , uint8_t value )
{
        dev->lcd_param->brightness = value ;
        lcd_set(dev->lcd_usb_handle , LCD_SET_BRIGHTNESS , value ) ;
}

void lcd_write( lcd_device *dev , const unsigned char *data )
{
        int c , x ;

        while(( dev->lcd_param->xpos++ < dev->lcd_param->columns ) && ( c = (int)*data++ )) {
                if(  dev->lcd_param->rows == 1 ) {
                        if( dev->lcd_param->xpos > dev->lcd_param->columnst ) {
                                x = dev->lcd_param->xpos-dev->lcd_param->columnst ;
                                lcd_setpos( dev , x-1 , 1 ) ;     //y=1
                        }
                }
                // translation pattern for custom characters
                switch (c) {
                        case 176: c = 0 ; break ;
                        case 158: c = 1 ; break ;
                        case 131: c = 2 ; break ;
                        case 132: c = 3 ; break ;
                        case 133: c = 4 ; break ;
                        case 134: c = 5 ; break ;
                        case 135: c = 6 ; break ;
                        case 136: c = 7 ; break ;
                        default: break ;
                }

                lcd_enqueue( dev , LCD_DATA | dev->lcd_param->ctrlr , c ) ;
        }

        lcd_flush( dev ) ;
}

void lcd_setpos( lcd_device *dev , uint8_t x , uint8_t y )
{
        uint8_t pos = 0 ;
        dev->lcd_param->xpos = x ;
        dev->lcd_param->ypos = y ;

        /* displays with more two rows and 20 columns have a logical width */
        /* of 40 chars and use more than one controller */
        if (( dev->lcd_param->num_of_chars > 80 ) && ( dev->lcd_param->ypos > 1 )) {
                /* use second controller */
                dev->lcd_param->ypos -= 2 ;
                dev->lcd_param->ctrlr = LCD_CTRL_1 ;
        } else { dev->lcd_param->ctrlr = LCD_CTRL_0 ; }    /* use first controller */ 

        /* 16x4 Displays use a slightly different layout */
        switch ( dev->lcd_param->ypos )
        {
                case 0:	pos = 0x80 ; break ;              // row1
                case 1:	pos = 0xc0 ; break ;              // row2
                case 2:	pos = 0x80 + dev->lcd_param->columns ; break ;    // row3
                case 3:	pos = 0xc0 + dev->lcd_param->columns ; break ;    // row4
        }
        dev->buf->buffer_type = LCD_CMD | dev->lcd_param->ctrlr ;
        dev->buf->show_buf[0] = pos + dev->lcd_param->xpos ;
        dev->buf->buffer_fill = 1 ;
        lcd_flush( dev ) ;
}

void lcd_cursor_home( lcd_device *dev )
{
        lcd_setpos( dev , 0 , 0 ) ;
}

void GUI_lcdwrites( lcd_device *dev , uint8_t n , uint8_t dat )
{
        uint8_t i ;
        for( i = 0 ; i < n ; i++ ) {
                lcd_enqueue( dev , LCD_DATA | dev->lcd_param->ctrlr , dat ) ;
        }
        lcd_flush( dev );
}

void lcd_clear_scr( lcd_device *dev )
{
        dev->lcd_param->ctrlr = LCD_CTRL_0 ;
        for( uint8_t ii = 0 ; ii < dev->lcd_param->rows ; ii++ ) {
                lcd_setpos( dev , 0 , ii ) ;
                GUI_lcdwrites( dev , dev->lcd_param->columns , 0x20 ) ;
        }
        lcd_setpos( dev , 0 , 0 ) ;	//write address
}

void lcd_custom_char( lcd_device *dev , uint8_t address , uint8_t *matrix )
{
        lcd_command( dev , LCD_BOTH , 0x40 | 8 * address ) ;

        for ( int i = 0 ; i < 8 ; i++ ) {
                lcd_enqueue( dev , LCD_DATA | LCD_BOTH , *matrix++ & 0x1f ) ;
        }
        lcd_flush( dev ) ;
}

void lcd_contrast_set( lcd_device *dev , int value )
{
        dev->lcd_param->contrast = value ;
        lcd_set( dev->lcd_usb_handle , LCD_SET_CONTRAST , value ) ;
}

void lcd_display_on( lcd_device *dev )
{
        dev->buf->buffer_type = LCD_CMD | LCD_BOTH ;
        dev->buf->show_buf[0] = 0x0c | dev->lcd_param->cursor_control ;
        dev->buf->buffer_fill = 1 ;
        lcd_flush( dev ) ;
}

void lcd_display_off( lcd_device *dev )
{
        dev->buf->buffer_type = LCD_CMD | LCD_BOTH ;
        dev->buf->show_buf[0] = 0x08 ;
        dev->buf->buffer_fill = 1 ;
        lcd_flush( dev ) ;
}

void lcd_cursor_blink( lcd_device *dev )
{

        if (( dev->lcd_param->num_of_chars > 80 ) && ( dev->lcd_param->ypos )) {
                dev->lcd_param->ctrlr = LCD_CTRL_1 ;
        } else {                                    /* use second controller */
                dev->lcd_param->ctrlr = LCD_CTRL_0 ;
        }                                           /* use first controller */
        dev->lcd_param->cursor_control = 0x01 ;			    /*Turn on block (blinking) cursor (0xFE 0x44) */
        dev->buf->buffer_type = LCD_CMD | dev->lcd_param->ctrlr ;
        dev->buf->show_buf[0] = 0x0c | dev->lcd_param->cursor_control ;
        dev->buf->buffer_fill = 1 ;
        lcd_flush( dev ) ;
}

void lcd_cursor_underline( lcd_device *dev )
{
        if (( dev->lcd_param->num_of_chars > 80 ) && ( dev->lcd_param->ypos > 1)) {
                dev->lcd_param->ctrlr = LCD_CTRL_1 ;
        } else {
                dev->lcd_param->ctrlr = LCD_CTRL_0 ;
        }
        dev->lcd_param->cursor_control = 0x02 ;
        dev->buf->buffer_type = LCD_CMD | dev->lcd_param->ctrlr ;
        dev->buf->show_buf[0] = 0x0c | dev->lcd_param->cursor_control ;
        dev->buf->buffer_fill = 1 ;
        lcd_flush( dev ) ;
}

void lcd_cursor_off( lcd_device *dev )
{
        if (( dev->lcd_param->num_of_chars > 80 ) && ( dev->lcd_param->ypos > 1 )) {
                dev->lcd_param->ctrlr = LCD_CTRL_1 ;
        } else {                                                        /* use second controller */
                dev->lcd_param->ctrlr = LCD_CTRL_0 ;
        }                                                               /* use first controller */
        dev->lcd_param->cursor_control = 0x00 ;                     	/* turn off cursor */
        dev->buf->buffer_type = LCD_CMD | dev->lcd_param->ctrlr ;
        dev->buf->show_buf[0] = 0x0c | dev->lcd_param->cursor_control ;
        dev->buf->buffer_fill = 1 ;
        lcd_flush( dev );
}

int lcd_init ( peep_instance *usb_lcd )
{

        usb_lcd->lcd_dev->lcd_usb_handle = libusb_open_device_with_vid_pid( usb_lcd->lib_ctx ,
                                                                            usb_lcd->lcd_dev->vendor_id ,
                                                                            usb_lcd->lcd_dev->device_id );
        if ( usb_lcd->lcd_dev->lcd_usb_handle == NULL ) {
                printf("libusb_open() failed\n");
                return -1 ;
        }

        lcd_clear_scr( usb_lcd->lcd_dev ) ;
        lcd_display_on( usb_lcd->lcd_dev ) ;
        lcd_contrast_set( usb_lcd->lcd_dev , usb_lcd->lcd_dev->lcd_param->contrast ) ;
        lcd_backlight_set( usb_lcd->lcd_dev , usb_lcd->lcd_dev->lcd_param->backlight ) ;
        lcd_setpos( usb_lcd->lcd_dev , 0 , 0 ) ;
        return 0 ;
}

