package network;

import java.net.InterfaceAddress;
import java.util.LinkedList;

import network.core.SocketUDPMulticastSend;

public class StreamDiscovering {

    static final String TAG = "StreamDiscovering";

    static class SendAdapter implements Runnable{
        StreamDiscovering streamDiscovering;
        public SendAdapter(StreamDiscovering streamDiscovering) {
            this.streamDiscovering = streamDiscovering;
        }
        @Override
        public void run() {
            while (!Thread.currentThread().isInterrupted()){
                for(SocketUDPMulticastSend socket : streamDiscovering.socketUDPMulticastSend)
                    socket.sendString("OpenMultimediaVideoSource:" + streamDiscovering.name,1023);
                try {
                    Thread.sleep(streamDiscovering.interval_ms);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                    break;
                }
            }
        }
    }

    SocketUDPMulticastSend []socketUDPMulticastSend = null;
    Thread sending_thread;
    int interval_ms;
    String name;

    public StreamDiscovering(String name, int interval_ms) {
        this.name = name;
        this.interval_ms = interval_ms;

        //socketUDPMulticastSend = SocketUDPMulticastSend.create( "239.0.0.1", NetworkConstants.PUBLIC_PORT_MULTICAST_START, 32 );
        LinkedList<InterfaceAddress> iFaces = NetworkHelper.lan_wlan_interfaces( NetworkHelper.InterfaceType.IPv4 );
        LinkedList<String> networkBroadcastAddresses = new LinkedList<>();

        if (iFaces.size() > 0) {
            for( InterfaceAddress iFace:iFaces ) {
                // 32 bits are the network mask: 255.255.255.255
                // 24 bits are the network mask: 255.255.255.0
                boolean is_a_network = iFace.getNetworkPrefixLength() < 32;
                if (is_a_network)
                    networkBroadcastAddresses.add(iFace.getBroadcast().getHostAddress());
            }
            socketUDPMulticastSend = new SocketUDPMulticastSend[networkBroadcastAddresses.size()];
            int count = 0;
            for(String bcast:networkBroadcastAddresses)
                socketUDPMulticastSend[count++] = SocketUDPMulticastSend.create(bcast, NetworkConstants.PUBLIC_PORT_MULTICAST_START, 32);
        }

        if (socketUDPMulticastSend != null){
            //start send thread
            sending_thread = new Thread(new SendAdapter(this), "StreamDiscovering (UDP/Multicast)" );
            sending_thread.start();
        }
    }

    public void close() {
        if (sending_thread != null) {
            sending_thread.interrupt();
            //wait
            while (sending_thread.isAlive()) {
                try {
                    Thread.sleep(100);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }

            sending_thread = null;
        }

        if (socketUDPMulticastSend != null) {
            for(SocketUDPMulticastSend socket : socketUDPMulticastSend)
                socket.close();
            socketUDPMulticastSend = null;
        }
    }

}
