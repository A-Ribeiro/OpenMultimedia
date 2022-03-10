package network.core;

import java.io.IOException;
import java.io.UnsupportedEncodingException;
import java.net.DatagramPacket;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.MulticastSocket;
import java.net.UnknownHostException;
import java.nio.ByteBuffer;

import network.NetworkHelper;

public class SocketUDPMulticastSend {

    MulticastSocket sending_socket = null;
    InetAddress sendEndPoint_addr = null;
    int sendEndPoint_port = 0;

    private SocketUDPMulticastSend(String group_addr, int port, int ttl) {
        try {
            sendEndPoint_addr = InetAddress.getByName(group_addr);
            sendEndPoint_port = port;

            sending_socket = new MulticastSocket();
            sending_socket.setTimeToLive(ttl);


        } catch (UnknownHostException e) {
            e.printStackTrace();
            sending_socket = null;
        } catch (IOException e) {
            e.printStackTrace();
            sending_socket = null;
        }
    }

    public static SocketUDPMulticastSend create(String group_addr, int port, int ttl) {
        SocketUDPMulticastSend result = new SocketUDPMulticastSend(group_addr, port, ttl);
        if (result.sending_socket == null)
            return null;
        return result;
    }

    public int getListenPort() {
        return ((InetSocketAddress)sending_socket.getLocalSocketAddress()).getPort();
    }

    public String getListenAddress() {
        InetAddress addr = ((InetSocketAddress)sending_socket.getLocalSocketAddress()).getAddress();
        return NetworkHelper.InetAddressToString(addr);
    }

    public Boolean sendString(String str, int max_size) {

        str = str.trim();
        if (str.length() > max_size)
            str = str.substring(0, max_size);

        ByteBuffer buffer = ByteBuffer.allocateDirect(max_size+1);

        try {
            buffer.put(str.getBytes("US-ASCII"));
            buffer.put((byte) 0);
        } catch (UnsupportedEncodingException e) {
            e.printStackTrace();
        }

        return send(buffer.array(), buffer.arrayOffset(), buffer.position());
    }

    public Boolean send(byte []data, int offset, int size) {
        try {
            DatagramPacket pack = new DatagramPacket(data, offset, size, sendEndPoint_addr, sendEndPoint_port);
            sending_socket.send(pack);
        } catch (IOException e) {
            e.printStackTrace();
            return false;
        }
        return true;
    }

    public void close() {
        if (sending_socket != null)
            sending_socket.close();
        sending_socket = null;
    }

}
