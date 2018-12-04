/* vim:set ts=4 sw=4: */
#ifndef __SYSMON_H__
#define  __SYSMON_H__

#define CK_TIME 1
#define THERMAL_TEMP "/sys/devices/virtual/thermal/thermal_zone%d/temp"
#define CORE_CLOCK "/sys/devices/system/cpu/cpufreq/policy%d/scaling_cur_freq"

int sys_boot_time ( char * ) ;
int core_clock ( int , char* ) ;
float cpu_usage( void ) ;
float cpu_temp( void ) ;
int ipv4_address( char * ) ;
#endif
