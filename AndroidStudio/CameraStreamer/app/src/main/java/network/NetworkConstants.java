/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package network;

/**
 *
 * @author alessandro
 */
public class NetworkConstants {
    public static final int UDP_MAX_PACKET_SIZE = 0xffff;
    public static final int UDP_HEADER_SIZE = 8;
    public static final int IP_V4_HEADER_SIZE = 20;
    public static final int UDP_IPV4_TOTAL_HEADER_SIZE = IP_V4_HEADER_SIZE + UDP_HEADER_SIZE;

    //Known MTUs (Maximum transfer units)

    // Ethernet: 1500 - UDP_IPV4_TOTAL_HEADER_SIZE = 1472
    // test: ping 10.0.0.105 -f -l 1472
    public static final int ETHERNET_MTU = 1500;

    // Internet: 1492 - UDP_IPV4_TOTAL_HEADER_SIZE = 1464
    // test: ping www.tp-link.com -f -l 1464
    // test: ping google.com -f -l 1464
    public static final int INTERNET_MTU = 1492;

    // Minimum packet size: 576 - UDP_IPV4_TOTAL_HEADER_SIZE = 548
    // StackOverflow - Safest to transmit
    public static final int MINIMUM_MTU = 576;

    //generates fragmentation
    public static final int UDP_DATA_MAX_DATAGRAM_SIZE = UDP_MAX_PACKET_SIZE - UDP_IPV4_TOTAL_HEADER_SIZE;

    public static final int UDP_DATA_MTU_ETHERNET = ETHERNET_MTU - UDP_IPV4_TOTAL_HEADER_SIZE;
    public static final int UDP_DATA_MTU_INTERNET = INTERNET_MTU - UDP_IPV4_TOTAL_HEADER_SIZE;
    public static final int UDP_DATA_MTU_MINIMUM = MINIMUM_MTU - UDP_IPV4_TOTAL_HEADER_SIZE;

    public static final int UDP_DATA_MTU_LOCALHOST = UDP_DATA_MAX_DATAGRAM_SIZE;


    // SELECTED MTU TO WORK
    public static final int UDP_DATA_MAXIMUM_MTU = UDP_DATA_MTU_INTERNET;
    public static final int UDP_DATA_MINIMUM_MTU = UDP_DATA_MTU_MINIMUM;
    
    
    // try to avoid overlap with the OS ephemeral port 
    public static final int PUBLIC_PORT_START = 5001;
    public static final int PUBLIC_PORT_END = 18884;
    public static final int PUBLIC_PORT_MULTICAST_START = PUBLIC_PORT_END+1;
    public static final int PUBLIC_PORT_MULTICAST_END = 32767;
    
}
