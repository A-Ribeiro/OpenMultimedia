/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package network;

import static java.lang.System.out;
import java.net.Inet4Address;
import java.net.Inet6Address;
import java.net.InetAddress;
import java.net.InterfaceAddress;
import java.net.NetworkInterface;
import java.net.SocketException;
import java.net.UnknownHostException;
import java.util.Arrays;
import java.util.Collections;
import java.util.Enumeration;
import java.util.LinkedList;
import java.util.Random;

/**
 *
 * @author alessandro
 */
public class NetworkHelper {

    public enum InterfaceType{
        IPv4,
        IPv6
    }

    public static InetAddress GetBroadcastAddress(InetAddress address, InetAddress mask) throws UnknownHostException {
        byte[] ipAddress = address.getAddress();
        byte[] ipMaskV4 = mask.getAddress();

        return InetAddress.getByAddress(new byte[]{
            (byte) (ipAddress[0] | (~ipMaskV4[0])),
            (byte) (ipAddress[1] | (~ipMaskV4[1])),
            (byte) (ipAddress[2] | (~ipMaskV4[2])),
            (byte) (ipAddress[3] | (~ipMaskV4[3])),});
    }

    public static LinkedList<String> lan_wlan_ips(InterfaceType iType) {
        LinkedList<String> result = new LinkedList<String>();

        for (InterfaceAddress iFace: lan_wlan_interfaces(iType)) {
            boolean is_a_network = false;

            if (iType == InterfaceType.IPv4)
                // 32 bits are the network mask: 255.255.255.255
                // 24 bits are the network mask: 255.255.255.0
                is_a_network = iFace.getNetworkPrefixLength() < 32;
            else if (iType == InterfaceType.IPv6)
                // 128 bits are the network mask: ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff
                // 64 bits are the network mask: ffff:ffff:ffff:ffff:0:0:0:0
                is_a_network = iFace.getNetworkPrefixLength() < 128;

            if (is_a_network)
                result.push(iFace.getAddress().getHostAddress());
        }

        return result;
    }

    public static LinkedList<InterfaceAddress> lan_wlan_interfaces(InterfaceType iType) {
        LinkedList<InterfaceAddress> result = new LinkedList<InterfaceAddress>();

        for(NetworkInterface ni : GetNetworkInterfacesList()){
            try {
                if (ni.isLoopback())
                    continue;
            } catch (SocketException e) {
                e.printStackTrace();
            }

            if (ni.isVirtual())
                continue;

            try {
                if (ni.isPointToPoint())
                    continue;
            } catch (SocketException e) {
                e.printStackTrace();
            }

            if (!ni.getName().toLowerCase().startsWith("wlan") && !ni.getName().toLowerCase().startsWith("eth"))
                continue;

            for(InterfaceAddress ia : ni.getInterfaceAddresses()) {
                if (iType == InterfaceType.IPv4) {
                    if (ia.getAddress() instanceof Inet4Address)// && ia.getNetworkPrefixLength() < 32)
                        result.push(ia);
                }
                else if (iType == InterfaceType.IPv6) {
                    if (ia.getAddress() instanceof Inet6Address)// && ia.getNetworkPrefixLength() < 128)
                        result.push(ia);
                }
            }
        }

        return result;
    }

    public static LinkedList<NetworkInterface> GetNetworkInterfacesList() {
        LinkedList<NetworkInterface> result = new LinkedList<NetworkInterface>();
        try {
            Enumeration<NetworkInterface> iFaces = NetworkInterface.getNetworkInterfaces();
            while (iFaces.hasMoreElements()) {
                NetworkInterface ni = iFaces.nextElement();
                Enumeration<InetAddress> iAddr = ni.getInetAddresses();
                while (iAddr.hasMoreElements()) {
                    InetAddress addr = iAddr.nextElement();
                    if (addr instanceof Inet4Address) {
                        result.add(ni);
                        break;
                    }
                }
            }
        } catch (SocketException ex) {
            ex.printStackTrace();
        }
        return result;
    }

    static byte[] uniqueID = null;

    public static byte[] getFirstMacAddressSimple() {
        if (uniqueID == null) {
            Random random = new Random( System.nanoTime() );

            byte[] addr = new byte[]{
                (byte) random.nextInt(256), (byte) random.nextInt(256), (byte) random.nextInt(256), 
                (byte) random.nextInt(256), (byte) random.nextInt(256), (byte) random.nextInt(256)};

            LinkedList<NetworkInterface> nif = NetworkHelper.GetNetworkInterfacesList();
            for (int i = 0; i < nif.size(); i++) {
                try {
                    byte[] iface = nif.get(i).getHardwareAddress();
                    if (iface != null) {
                        addr = iface;
                        break;
                    }
                } catch (SocketException ex) {
                    //Logger.getLogger(UDP_DataSend.class.getName()).log(Level.SEVERE, null, ex);
                    ex.printStackTrace();
                }
            }

            uniqueID = addr;
                    /*
            StringBuilder sb = new StringBuilder();
            for (int i = 0; i < addr.length; i++) {
                sb.append(String.format("%02X%s", addr[i], (i < addr.length - 1) ? ":" : ""));
            }

            //sb.append(":"+String.format("%016X",System.nanoTime()));
            uniqueID = sb.toString();

*/
                    
            System.out.println("FirstMacAddress: " + bytesToHexString( uniqueID ) );
        }
        
        return uniqueID;
    }

    public static void displayInterfaceInformation(NetworkInterface netint) {
        out.printf("Display name: %s\n", netint.getDisplayName());
        out.printf("Name: %s\n", netint.getName());
        Enumeration<InetAddress> inetAddresses = netint.getInetAddresses();
        for (InetAddress inetAddress : Collections.list(inetAddresses)) {
            out.printf("InetAddress: %s\n", inetAddress);
        }
        out.printf("\n");
    }

    public static String bytesToString(byte[] bytes) {
        StringBuilder sb = new StringBuilder();
        for (byte b : bytes) {
            if (sb.length() > 0)
                sb.append(".");
            sb.append((int) b & 0xff);
        }
        return sb.toString();
    }

    public static String bytesToHexString(byte[] bytes) {
        StringBuilder sb = new StringBuilder();
        for (byte b : bytes) {
            if (sb.length() > 0)
                sb.append(":");
            sb.append(String.format("%02X", (int) b & 0xff));
        }
        return sb.toString();
    }

    public static String InetAddressToString(InetAddress addr) {
        return bytesToString(addr.getAddress());
    }

    // range address: 224.0.1.0 to 239.255.255.255
    public static byte[] randomMulticastGroup() {
        Random random = new Random(System.nanoTime());
        byte[] bytes = new byte[]{(byte) 239,
            (byte) (random.nextInt(254) + 2),
            (byte) (random.nextInt(254) + 2),
            (byte) (random.nextInt(254) + 2)
        };
        return bytes;
    }

    public static int randomPublicPort() {
        Random random = new Random(System.nanoTime());
        return random.nextInt(NetworkConstants.PUBLIC_PORT_END - NetworkConstants.PUBLIC_PORT_START) + NetworkConstants.PUBLIC_PORT_START;
    }

    public static int randomMulticastPort() {
        Random random = new Random(System.nanoTime());
        return random.nextInt(NetworkConstants.PUBLIC_PORT_MULTICAST_END - NetworkConstants.PUBLIC_PORT_MULTICAST_START) + NetworkConstants.PUBLIC_PORT_MULTICAST_START;
    }

    public static boolean compareInetAddress(InetAddress a, InetAddress b) {
        return Arrays.equals(a.getAddress(), b.getAddress());
    }

    public static boolean compareInetAddress(InetAddress a, byte[] b) {
        return Arrays.equals(a.getAddress(), b);
    }

}
