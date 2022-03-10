package network;

import android.util.Log;

import java.io.BufferedWriter;
import java.io.IOException;
import java.io.OutputStreamWriter;
import java.net.DatagramPacket;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.ArrayList;
import java.util.LinkedList;
import java.util.List;

public class StreamTCPServer {

    static final String TAG = "StreamTCPServer";

    ServerSocket serverSocket = null;
    Thread acceptThread = null;

    LinkedList<Socket> clients = null;
    boolean newConnection = false;

    Object _sync = new Object();

    public boolean hasNewConnection() {
        synchronized(_sync) {
            boolean result = newConnection;
            newConnection = false;
            return result;
        }
    }

    private StreamTCPServer( int listen_port ) {
        try {
            serverSocket = new ServerSocket(listen_port);
            serverSocket.setReuseAddress(true);

            acceptThread = new Thread(new AcceptAdapter(this), "StreamTCPServer (Accept Thread)" );

            clients = new LinkedList<Socket>();

            acceptThread.start();

        } catch (IOException e) {
            close();
            e.printStackTrace();
        }
    }

    public static StreamTCPServer create(int listen_port) {
        StreamTCPServer result = new StreamTCPServer(listen_port);
        if (result.serverSocket == null)
            return null;
        return result;
    }

    public void send(byte []data, int offset, int size) {
        List<Socket> toRemove = new LinkedList<>();

        List<Socket> toWrite = null;

        synchronized(_sync) {
            toWrite = new LinkedList<>(clients);
        }

        for(Socket c:toWrite){
            try {
                c.getOutputStream().write(data, offset, size);
            } catch (IOException e) {
                //e.printStackTrace();
                toRemove.add(c);
            }
        }

        synchronized(_sync) {
            for (Socket c : toRemove) {
                Log.w(TAG, "removing Client: " + c.getInetAddress().getHostAddress());
                clients.remove(c);
                try {
                    c.close();
                } catch (IOException e) {
                    //e.printStackTrace();
                }
            }
        }
    }

    public void close() {

        try {
            if (serverSocket != null)
                serverSocket.close();
        } catch (IOException e) {
            e.printStackTrace();
        }

        if (acceptThread != null){
            acceptThread.interrupt();
            //wait
            while (acceptThread.isAlive()) {
                try {
                    Thread.sleep(100);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }
            acceptThread = null;
        }

        serverSocket = null;

        //disconnect all clients
        synchronized(_sync) {
            for (Socket c : clients) {
                try {
                    c.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
            clients.clear();
        }

    }


    static class AcceptAdapter implements Runnable{
        StreamTCPServer streamTCPServer;
        public AcceptAdapter(StreamTCPServer streamTCPServer) {
            this.streamTCPServer = streamTCPServer;
        }
        @Override
        public void run() {

            Log.w(TAG, "ACCEPT THREAD RUNNING...");
            while (!Thread.currentThread().isInterrupted()){
                try {
                    Socket clientSocket = streamTCPServer.serverSocket.accept();
                    clientSocket.setTcpNoDelay(true);
                    clientSocket.setKeepAlive(false);// force disconnection...

                    Log.w(TAG, "adding Client: " + clientSocket.getInetAddress().getHostAddress());
                    synchronized(streamTCPServer._sync) {
                        streamTCPServer.clients.add(clientSocket);
                        streamTCPServer.newConnection = true;
                    }
                } catch (IOException e) {
                    e.printStackTrace();
                    break;
                }
            }
        }
    }

}
