// get_deveui.c
//
// Example program to get LoraWAN device EUI from MAC Address
// Use the Makefile in this directory:
// cd example/raspi/get_deveui
// make
// sudo ./get_deveui
//
// written  by Charles-Henri Hallard (hallard.me)

#include <stdio.h>
#include <ifaddrs.h>
#include <netpacket/packet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <string.h>
#include <sys/stat.h> 
#include <fcntl.h>
#include <unistd.h>

int main (int argc, const char * argv[])
{
  struct ifaddrs *ifaddr=NULL;
  struct ifaddrs *ifa = NULL;
  int family = 0;
  int i = 0;
  int all=0;

  if (argc==2 && !strncasecmp(argv[1],"all", 3) ) {
    all=1;
  } else {
    fprintf(stdout, "Use \"%s all\" to see all interfaces and details\n", __BASEFILE__ );
  }
  
  // get linked list of the network interfaces
  if (getifaddrs(&ifaddr) == -1) {
    perror("getifaddrs");
    
  } else {
    // Loop thru interfaces list
    for ( ifa=ifaddr; ifa!=NULL; ifa=ifa->ifa_next) {
      // Ethernet
      if ( (ifa->ifa_addr) && (ifa->ifa_addr->sa_family==AF_PACKET) ) {
        // Not loopback interface
        if (! (ifa->ifa_flags & IFF_LOOPBACK)) {
          char fname[128];
          int fd;
          int up=0;
          struct sockaddr_ll *s = (struct sockaddr_ll*)ifa->ifa_addr;
         
          // Get interface status 
          // Interface can be up with no cable connected and to be sure
          // It's up, active and connected we need to get operstate
          // 
          // if up + cable    if up + NO cable    if down + cable
          // =============    ==========          ==================
          // carrier:1        carrier:0           carrier:Invalid
          // dormant:0        dormant:0           dormant:Invalid
          // operstate:up     operstate:down      operstate :own
          sprintf(fname, "/sys/class/net/%s/operstate", ifa->ifa_name);
          
          if ( (fd = open( fname, O_RDONLY)) > 0 ){
            char buf[2];
            if ( read(fd, buf, 2) > 0 ) {
              if ( buf[0]=='u' && buf[1]=='p' ) {
                up=1;
              }
              close(fd);
            } else {
              perror("read()");
            }
          } else {
            perror(fname);
          }
          
          //active interface or all wanted
          if (all || up) {
            char deveui[8];
            char *p = deveui;
            // deveui is LSB to we reverse it so TTN display 
            // will remain the same as MAC address
            // MAC is 6 bytes, devEUI 8, set first 2 ones 
            // with an arbitrary value
            *p++ = 0x00;
            *p++ = 0x04;
            // Then next 6 bytes are mac address still reversed
            for ( i=0; i<6 ; i++) {
              *p++ = s->sll_addr[5-i];
            }
            
            // Device Info
            fprintf(stdout, "// %s %s %s ", ifa->ifa_name, ifa->ifa_flags&IFF_UP?"Up":"Down", up?"Linked":"No Link");
            // Raw format for ttnctl
            p = deveui+8;
            fprintf(stdout, "TTN Dashboard DEVEUI format ");
            for ( i=0 ; i<8 ; i++) {
              fprintf( stdout, "%02X", *--p);
            }
                        
            p = deveui;
            // LSB format for application code
            fprintf(stdout, "\nstatic const u1_t PROGMEM DEVEUI[8]={");
            for ( i=0 ; i<8 ; i++) {
              fprintf( stdout, " 0x%02x%c", *p++, i<7?',':' ');
            }
            fprintf( stdout, "}; // %s\n", ifa->ifa_name);
                
          } 
        }
      }
    }
    
    // Free our Linked list
    freeifaddrs(ifaddr);
  }
  return 0;
}

