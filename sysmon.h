/* vim:set ts=4 sw=4: */
#ifndef __SYSMON_H__
#define  __SYSMON_H__

#define CK_TIME 1
#define THERMAL_TEMP "/sys/devices/virtual/thermal/thermal_zone%d/temp"
float cpu_usage( void ) ;
float cpu_temp( void ) ;
int ipv4_address( char * ) ;
int sys_boot_time ( char * ) ;
#endif
