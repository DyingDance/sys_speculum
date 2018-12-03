/* vim:set ts=4 sw=4: */
#ifndef __LCD_H__
#define __LCD_H__

#include <stdint.h>

/* vendor and product id */
#define LCD_USB_VENDOR    0x0403
#define LCD_USB_DEVICE    0xC630
/* target is a bit map for CMD/DATA */

#define BUFFER_MAX_CMD     4        /* current protocol supports up to 4 bytes */
#define LCD_CTRL_0         (1<<3)
#define LCD_CTRL_1         (1<<4)
#define LCD_BOTH           (LCD_CTRL_0 | LCD_CTRL_1)

#define LCD_ECHO           (0<<5)
#define LCD_CMD            (1<<5)
#define LCD_DATA           (2<<5)
#define LCD_SET            (3<<5)
#define LCD_GET            (4<<5)

/* target is value to set */
#define LCD_SET_CONTRAST   (LCD_SET | (0<<3))
#define LCD_SET_BRIGHTNESS (LCD_SET | (1<<3))
#define LCD_SET_RESERVED0  (LCD_SET | (2<<3))
#define LCD_SET_RESERVED1  (LCD_SET | (3<<3))

/* target is value to get */
#define LCD_GET_FWVER      (LCD_GET | (0<<3))
#define LCD_GET_KEYS       (LCD_GET | (1<<3))
#define LCD_GET_CTRL       (LCD_GET | (2<<3))
#define LCD_GET_RESERVED1  (LCD_GET | (3<<3))

enum peep_state {
    chaos = 0 ,
    inoculation ,
    born ,
    waiting ,

    cached ,
    connected
} ;

typedef struct __show_buffer__ {
    char *show_buf ;
    int buffer_type ;
    int buffer_fill ;
} show_buffer ;

typedef struct __lcd_parament__ {
    int width ;
    int height ;
    int cellwidth ;
    int cellheight ;
    int contrast ;
    int backlight ;
    int brightness ;
    int controllers ;
    int ctrlr ;
    int num_of_chars ;
    int rows ;
    int columns ;
    int columnst ;
    int xpos ;
    int ypos ;
    uint8_t bright ;
    uint8_t cursor_control ;
} lcd_parament ;

typedef struct __lcd_device__ {
    libusb_device_handle *lcd_usb_handle ;
	char *device_name;          /* Device name */
	char *description;          /* Device description */
	unsigned int vendor_id;     /* vendor id for detection */
	unsigned int device_id;     /* device id for detection */
	int bklight_max;            /* maximum value for 'backlight on' */
	int bklight_min;            /* maximum value for 'backlight off' */
	int contrast_max;           /* contrast maximum value */
	int contrast_min;           /* minimum contrast value */
	int width;                  /* width of lcd screen */
	int height;                 /* height of lcd screen */
    lcd_parament *lcd_param ;
    show_buffer *buf ;
} lcd_device;

typedef struct __speculum_instance__ {
    enum peep_state speculum_life_cycle ;
    libusb_context *lib_ctx;
    libusb_device *dev , **devs ;
    struct libusb_device_descriptor desc;
	libusb_hotplug_callback_handle hp[2] ;
    lcd_device *lcd_dev ;
} peep_instance ;

int lcd_init( peep_instance * ) ;
int lcd_send( libusb_device_handle * , uint16_t , uint16_t , uint16_t ) ;
void lcd_write( lcd_device * , const unsigned char * ) ;

int lcd_get(  libusb_device_handle * , uint8_t ) ;				    // get a value from the lcd2usb interface
void lcd_set(  libusb_device_handle * , uint8_t , int ) ;             // set a value in the LCD interface
void lcd_flush( lcd_device * ) ;					    // change or due to explicit request

void lcd_clear_scr( lcd_device * ) ;
void lcd_write( lcd_device * , const unsigned char *data ) ;

void lcd_display_on( lcd_device * ) ;
void lcd_display_off( lcd_device * ) ;
void lcd_backlight_set( lcd_device * , uint8_t ) ;
void lcd_contrast_set( lcd_device * , int ) ;

void lcd_cursor_blink( lcd_device * ) ;
void lcd_cursor_underline( lcd_device * ) ;
void lcd_cursor_off( lcd_device * ) ;
void lcd_setpos( lcd_device * , uint8_t , uint8_t ) ;
void lcd_cursor_home( lcd_device * ) ;

void lcd_custom_char( lcd_device * , uint8_t address , uint8_t *matrix ) ;
#endif /* __LCD_H__ */
