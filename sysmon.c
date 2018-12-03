/* vim:set ts=4 sw=4: */
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <arpa/inet.h>

#include "sysmon.h"
float cpu_usage()
{
    FILE *fp ;
    char buf[128] ;
    char cpu[5] ;
    long int user , nice , sys , idle , iowait , irq , softirq ;

    long int all1 , all2 , idle1 , idle2 ;
    float usage ;


    fp = fopen( "/proc/stat" , "r" ) ;
    if( fp == NULL ) {
        perror( "fopen:" ) ;
        return -1 ;
    }

    fgets( buf , sizeof( buf ) , fp ) ;
    fclose(fp) ;
    sscanf( buf , "%s%ld%ld%ld%ld%ld%ld%ld" , cpu , &user , &nice , &sys , &idle , &iowait , &irq , &softirq ) ;
    all1 = user + nice + sys + idle + iowait + irq + softirq ;
    idle1 = idle ;

    /*Get second time*/
    sleep( CK_TIME ) ;
    memset( buf , 0 , sizeof( buf )) ;
    cpu[0] = '\0' ;
    user = nice = sys = idle = iowait = irq = softirq = 0 ;
    fp = fopen( "/proc/stat" , "r" ) ;
    if( fp == NULL ) {
        perror( "fopen:" ) ;
        exit ( -1 ) ;
    }
    fgets( buf , sizeof( buf ) , fp ) ;
    fclose(fp);
    sscanf( buf , "%s%ld%ld%ld%ld%ld%ld%ld" , cpu , &user , &nice , &sys , &idle , &iowait , &irq , &softirq ) ;
    all2 = user + nice + sys + idle + iowait + irq + softirq ;
    idle2 = idle ;
    usage = (float)(all2-all1-(idle2-idle1)) / (all2-all1)*100 ;
    return ( usage ) ;
}

float cpu_temp()
{
    FILE *fp ;
    char buf[128] ;
    char tsens[16] ; 
    int temp1 , temp2 ;

    sprintf( buf , THERMAL_TEMP , 0 ) ;
    fp = fopen ( buf , "r") ;
    if ( fp == NULL ) 
    {
        perror( "fopen:" ) ;
        return ( -1 ) ;
    }
    fgets( tsens , sizeof( tsens ) , fp ) ;
    fclose( fp ) ;
    sscanf ( tsens , "%d" , &temp1 );

    sprintf( buf , THERMAL_TEMP , 1 ) ;
    fp = fopen ( buf , "r") ;
    if ( fp == NULL ) 
    {
        perror("fopen:");
        return ( -1 ) ;
    }
    fgets( tsens , sizeof( tsens ) , fp ) ;
    fclose( fp ) ;
    sscanf ( tsens , "%d" , &temp2 );

#if 0
    sprintf( buf , THERMAL_TEMP , 9 ) ;
    fp = fopen ( buf , "r") ;
    if ( fp == NULL ) 
    {
        perror( "fopen:" ) ;
        return( -1 ) ;
    }
    fgets( tsens , sizeof( tsens ) , fp ) ;
    fclose( fp ) ;
    sscanf ( tsens , "%d" , &temp3 );

    sprintf( buf , THERMAL_TEMP , 11 ) ;
    fp = fopen ( buf , "r") ;
    if ( fp == NULL ) 
    {
        perror( "fopen:" ) ;
        return(-1) ;
    }

    fgets( tsens , sizeof( tsens ) , fp ) ;
    fclose( fp ) ;
    sscanf ( tsens , "%d" , &temp4 );
#endif
    return ( (float)( temp1 + temp2 ) / 2000.0 ) ;
}

int ipv4_address( char *address_buffer )
{
    struct ifaddrs *if_addr , *ifa ;
    void *tmp_addr_ptr = NULL ;

    *address_buffer = '\0' ;

    if ( getifaddrs( &if_addr ) == -1 ) {
        return -1 ;
    }

    /* while ( if_addr_struct != NULL ) { */
    for ( ifa = if_addr ; ifa != NULL ; ifa = ifa->ifa_next ) {
        if ( !( strcmp ( ifa->ifa_name , "eth0" ) ) ) {
            if ( ifa->ifa_addr->sa_family == AF_INET ) {   // check it is a valid IP4 Address
                tmp_addr_ptr = &( ( struct sockaddr_in * )ifa->ifa_addr )->sin_addr ;
                inet_ntop ( AF_INET , tmp_addr_ptr , address_buffer , INET_ADDRSTRLEN ) ;
                freeifaddrs( if_addr ) ;
                return 0 ;
            }
        }
    }
    freeifaddrs( if_addr ) ;
    return -1 ;
}

