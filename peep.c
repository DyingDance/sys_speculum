/* vim:set ts=4 sw=4: */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <libusb-1.0/libusb.h>

#include "sysmon.h"
#include "lcd.h"

#define NUMB_OF_CHARS_IN_A_LINE 16
#define NUMB_OF_LINES 2

char ip_addr[INET_ADDRSTRLEN] ;

static lcd_device JQMUC = {
     .device_name = "HD44780 LCD USB Module" ,
     .description = "used to spy on the system running state" ,
       .vendor_id = LCD_USB_VENDOR ,
       .device_id = LCD_USB_DEVICE ,
     .bklight_max = 255 ,
     .bklight_min = 0 ,
    .contrast_max = 40 ,
    .contrast_min = 0 ,
           .width = 16 ,
          .height = 2 ,
} ;

static lcd_parament J_param = {
              .rows = NUMB_OF_LINES ,
           .columns = NUMB_OF_CHARS_IN_A_LINE ,
          .columnst = NUMB_OF_CHARS_IN_A_LINE/2 ,
          .contrast = 50 ,
        .brightness = 100 ,
         .backlight = 100 ,
              .xpos = 0 ,
              .ypos = 0 ,
    .cursor_control = 0 ,
} ;

static int init_speculum ( peep_instance *instance )
{
    instance->speculum_life_cycle = chaos ;
    instance->lcd_dev = &JQMUC ;
    instance->lcd_dev->lcd_param = &J_param ;
    instance->lcd_dev->lcd_param->num_of_chars = instance->lcd_dev->lcd_param->rows *
                                                 instance->lcd_dev->lcd_param->columns ;
    instance->lcd_dev->buf = (show_buffer *)malloc( sizeof ( show_buffer ) ) ;
    instance->lcd_dev->buf->buffer_type = -1 ;
    instance->lcd_dev->buf->buffer_fill = 0 ;
    instance->lcd_dev->buf->show_buf = (char *)calloc( BUFFER_MAX_CMD , sizeof (char)) ;
    printf ( "instance initialized , waiting for usb device...\n" ) ;
    return 0 ;
}

static int LIBUSB_CALL hotplug_callback( libusb_context *ctx ,
                                         libusb_device *dev ,
                                         libusb_hotplug_event event ,
                                         void *user_data )
{
	struct libusb_device_descriptor desc ;
	int rc ;

	rc = libusb_get_device_descriptor( dev , &desc ) ;
	if ( LIBUSB_SUCCESS != rc ) {
		fprintf ( stderr , "Error getting device descriptor\n" ) ;
	}
	printf ("Device attached: %04x:%04x\n" , desc.idVendor , desc.idProduct ) ;
	if ( (( peep_instance *)user_data )->lcd_dev->lcd_usb_handle ) {
		libusb_close ( (( peep_instance *)user_data )->lcd_dev->lcd_usb_handle ) ;
		(( peep_instance *)user_data )->lcd_dev->lcd_usb_handle = NULL ;
	}
#if 1
    (( peep_instance *)user_data)->speculum_life_cycle = born ;
#endif
	return 0;
}

static int LIBUSB_CALL hotplug_callback_detach( libusb_context *ctx ,
                                                libusb_device *dev ,
                                                libusb_hotplug_event event ,
                                                void *user_data )
{
	printf ("Device detached\n");

	if ((( peep_instance *)user_data )->lcd_dev->lcd_usb_handle ) {
		libusb_close ( (( peep_instance *)user_data )->lcd_dev->lcd_usb_handle ) ;
		(( peep_instance *)user_data )->lcd_dev->lcd_usb_handle = NULL;
	}
    (( peep_instance *)user_data )->speculum_life_cycle = inoculation ;
	return 0;
}

int main( void )
{
    int status = 0 ;
    char first_line[NUMB_OF_CHARS_IN_A_LINE+1] ;
    char second_line[NUMB_OF_CHARS_IN_A_LINE+1] ;
#if 1
    peep_instance *usb_lcd ;
    usb_lcd = (peep_instance *)malloc( sizeof ( peep_instance ) ) ;
    if ( usb_lcd == NULL ) {
        printf ( "maby not enouth memory.\n" ) ;
        return -1 ;
    }

    init_speculum ( usb_lcd ) ;

    do {
        switch ( usb_lcd->speculum_life_cycle ) {
            case chaos :
                status = libusb_init ( &( usb_lcd->lib_ctx ) ) ;
                if ( status < 0 ) {
                    printf ( "failed to initialise libusb: %s\n", libusb_error_name( status ) ) ;
                    return EXIT_FAILURE;
                }
                if ( !libusb_has_capability ( LIBUSB_CAP_HAS_HOTPLUG ) ) {
                    printf ( "Hotplug capabilites are not supported on this platform\n" ) ;
                    libusb_exit ( usb_lcd->lib_ctx ) ;
                    return EXIT_FAILURE ;
                }
                status = libusb_hotplug_register_callback ( usb_lcd->lib_ctx /* NULL */,
                                                            LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED ,
                                                            0 , usb_lcd->lcd_dev->vendor_id /* LCD_USB_VENDOR */ ,
                                                            usb_lcd->lcd_dev->device_id /* LCD_USB_DEVICE */ ,
                                                            LIBUSB_HOTPLUG_MATCH_ANY ,
                                                            hotplug_callback ,
                                                            (void *)usb_lcd ,
                                                            &( usb_lcd->hp[0] ) ) ;
                if ( LIBUSB_SUCCESS != status ) {
                    printf ( "Error registering callback 0\n" ) ;
                    libusb_exit ( usb_lcd->lib_ctx ) ;
                    return EXIT_FAILURE ;
                }
                status = libusb_hotplug_register_callback ( usb_lcd->lib_ctx /* NULL */ ,
                                                            LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT ,
                                                            0 , usb_lcd->lcd_dev->vendor_id /* LCD_USB_VENDOR */ ,
                                                            usb_lcd->lcd_dev->device_id  /* LCD_USB_DEVICE */ ,
                                                            LIBUSB_HOTPLUG_MATCH_ANY ,
                                                            hotplug_callback_detach ,
                                                            (void *)usb_lcd ,
                                                            &( usb_lcd->hp[1] ) ) ;
                if ( LIBUSB_SUCCESS != status ) {
                    printf ( "Error registering callback 1\n" ) ;
                    libusb_hotplug_deregister_callback ( usb_lcd->lib_ctx , usb_lcd->hp[0] ) ;
                    libusb_exit ( usb_lcd->lib_ctx ) ;
                    return EXIT_FAILURE ;
                }
                if ( libusb_get_device_list ( usb_lcd->lib_ctx , &( usb_lcd->devs ) ) < 0 ) {
                    printf("libusb_get_device_list() failed: %s\n", libusb_error_name(status));
                    goto error ;
                }
                for ( int i = 0 ; ( usb_lcd->dev = usb_lcd->devs[i] ) != NULL ; i++ ) {
                    status = libusb_get_device_descriptor ( usb_lcd->dev , &( usb_lcd->desc ) ) ;
                    if ( status < 0 ) {
                        printf ("falied to get device descriptor\n" ) ;
                        goto error ;
                    }
                    status = -1 ;
                    if (( usb_lcd->desc.idVendor == usb_lcd->lcd_dev->vendor_id ) &&
                                ( usb_lcd->desc.idProduct == usb_lcd->lcd_dev->device_id ) ) {
                        printf ("found LCD2USB interface.\n" ) ;
                        usb_lcd->speculum_life_cycle = born ;
                        status = 0 ;
                        break ;
                    }
                }
                if ( status < 0 ) {

                    usb_lcd->speculum_life_cycle = inoculation ;
                }
#if 0
               printf ( "usb context cached , waiting for speculum plug in...\n" ) ;
                usb_lcd->speculum_life_cycle = inoculation ;
#endif
                break ;
error:
                libusb_hotplug_deregister_callback ( usb_lcd->lib_ctx , usb_lcd->hp[0] ) ;
                libusb_hotplug_deregister_callback ( usb_lcd->lib_ctx , usb_lcd->hp[1] ) ;
                libusb_exit ( usb_lcd->lib_ctx ) ;
                usb_lcd->speculum_life_cycle = chaos ;
                break ;

            case inoculation :
                    printf ( "waiting for speculum plug in...\n" ) ;
                    status = libusb_handle_events ( usb_lcd->lib_ctx ) ;
                    if ( status < 0 )
                      printf ( "libusb_handle_events() failed: %s\n", libusb_error_name( status )) ;
                /* sleep ( 2 ) ; */
                break ;

            case born :
                if ( lcd_init ( usb_lcd ) == 0 ) {
                    usb_lcd->speculum_life_cycle = connected ;
                } else {
                    if ( usb_lcd->lcd_dev->lcd_usb_handle ) {
                        libusb_close ( usb_lcd->lcd_dev->lcd_usb_handle ) ;
                        usb_lcd->lcd_dev->lcd_usb_handle = NULL ;
                    }
                }
                break ;

            case waiting:
                break ;

            case connected:
                printf ( "waiting for speculum pull out...\n" ) ;
                snprintf (first_line , NUMB_OF_CHARS_IN_A_LINE+1 , "cpu load: %3.2f%%" , cpu_usage() ) ;
                snprintf (second_line , NUMB_OF_CHARS_IN_A_LINE+1 , "cpu temp: %2.2f digC" , cpu_temp() ) ;
                lcd_clear_scr( usb_lcd->lcd_dev ) ;
                lcd_setpos( usb_lcd->lcd_dev , 0, 0 ) ;
                /* DISPLAYDLL_Write( first_line ) ; */
                lcd_write ( usb_lcd->lcd_dev , first_line ) ;
                lcd_setpos( usb_lcd->lcd_dev , 0, 1) ;
                /* DISPLAYDLL_Write( second_line ) ; */
                lcd_write ( usb_lcd->lcd_dev , second_line ) ;
                sleep( 2 ) ;
                sleep( 2 ) ;
                if ( ipv4_address ( ip_addr ) == 0 ) {
                    snprintf ( second_line , NUMB_OF_CHARS_IN_A_LINE+1 , ip_addr ) ;
                } else {
                    sprintf ( second_line , "127.0.0.1" ) ;
                }
                lcd_clear_scr( usb_lcd->lcd_dev ) ;
                lcd_setpos( usb_lcd->lcd_dev , 0, 0);
                /* DISPLAYDLL_Write("ip address"); */
                lcd_write ( usb_lcd->lcd_dev , "ip address" ) ;
                lcd_setpos( usb_lcd->lcd_dev , 0, 1 ) ;
                /* DISPLAYDLL_Write( second_line ); */
                lcd_write ( usb_lcd->lcd_dev , second_line ) ;
                /* sleep(2); */
                /* sleep(2); */
                status = libusb_handle_events ( usb_lcd->lib_ctx ) ;
                if ( status < 0 )
                  printf ( "libusb_handle_events() failed: %s\n", libusb_error_name( status )) ;
                break ;

            default :   /* some thing wrong here */
                if ( usb_lcd->lcd_dev->lcd_usb_handle ) libusb_close ( usb_lcd->lcd_dev->lcd_usb_handle ) ;
                /* libusb_exit ( usb_lcd->lib_ctx ) ; */
                libusb_exit ( NULL ) ;
                usb_lcd->speculum_life_cycle = born ;
                break ;
    }
    } while ( 1 ) ;

#else
    status = DISPLAYDLL_Init( NUMB_OF_CHARS_IN_A_LINE , NUMB_OF_LINES ) ;
    if( status == 0 )
    {
        printf("USB INTERFACE IS OPEN \n");
        lcd_display_on();
        lcd_ContrastSet(50);
        Set_Backlight(100);
        while ( 1 ) {
            snprintf (first_line , NUMB_OF_CHARS_IN_A_LINE+1 , "cpu load: %3.2f%%" , cpu_usage() ) ;
            snprintf (second_line , NUMB_OF_CHARS_IN_A_LINE+1 , "cpu temp: %2.2f digC" , cpu_temp() ) ;
            lcd_cleardisplay();
            lcd_setpos( 0, 0);
            DISPLAYDLL_Write( first_line ) ;
            lcd_setpos( 0, 1);
            DISPLAYDLL_Write( second_line ) ;
            sleep(2);
            sleep(2);
            if ( ipv4_address ( ip_addr ) == 0 ) {
                snprintf ( second_line , NUMB_OF_CHARS_IN_A_LINE+1 , ip_addr ) ;
            } else {
                sprintf ( second_line , "127.0.0.1" ) ;
            }
            lcd_cleardisplay();
            lcd_setpos( 0, 0);
            DISPLAYDLL_Write("ip address");
            lcd_setpos( 0, 1);
            DISPLAYDLL_Write( second_line );
            sleep(2);
            sleep(2);
        }
    }
    else
    {
        DISPLAYDLL_Done();
        printf("Could not open USB device,Error \n");

    }
#endif
}
