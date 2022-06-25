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

import util.ObjectBuffer;
import util.ObjectPool;
import util.ObjectQueue;

public class StreamTCPServer {

    public interface Callback{
        public void OnNewClient(Socket clientSocket);
    }

    static final String TAG = "StreamTCPServer";

    ServerSocket serverSocket = null;
    Thread acceptThread = null;
    Thread sendThread = null;

    LinkedList<Socket> clients = null;
    //boolean newConnection = false;
    public Callback callback = null;

    Object _sync = new Object();

    public ObjectPool<ObjectBuffer> objectPool = new ObjectPool<>(ObjectBuffer.class);
    public ObjectQueue<ObjectBuffer> objectQueue = new ObjectQueue<>();

    /*
    public boolean hasNewConnection() {
        synchronized(_sync) {
            boolean result = newConnection;
            newConnection = false;
            return result;
        }
    }
    */

    private StreamTCPServer( int listen_port, Callback _callback ) {
        callback = _callback;
        try {
            serverSocket = new ServerSocket(listen_port);
            serverSocket.setReuseAddress(true);

            acceptThread = new Thread(new AcceptAdapter(this), "StreamTCPServer (Accept Thread)" );
            sendThread = new Thread(new SendAdapter(this), "StreamTCPServer (Send Thread)" );

            clients = new LinkedList<Socket>();

            acceptThread.start();
            sendThread.start();

        } catch (IOException e) {
            close();
            e.printStackTrace();
        }
    }

    public static StreamTCPServer create(int listen_port, Callback _callback) {
        StreamTCPServer result = new StreamTCPServer(listen_port, _callback);
        if (result.serverSocket == null)
            return null;
        return result;
    }

    public void send(byte []data, int offset, int size) {
        ObjectBuffer buffer = objectPool.create();

        buffer.setSize(size);
        buffer.resetWrite();
        buffer.arrayReadFrom( data, offset, size );

        objectQueue.enqueue(buffer);
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

        if (sendThread != null){
            sendThread.interrupt();
            //wait
            while (sendThread.isAlive()) {
                try {
                    Thread.sleep(100);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }
            sendThread = null;
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
                        streamTCPServer.callback.OnNewClient(clientSocket);
                        //streamTCPServer.newConnection = true;
                    }
                } catch (IOException e) {
                    e.printStackTrace();
                    break;
                }
            }
        }
    }



    static class SendAdapter implements Runnable{
        StreamTCPServer streamTCPServer;
        public SendAdapter(StreamTCPServer streamTCPServer) {
            this.streamTCPServer = streamTCPServer;
        }
        @Override
        public void run() {

            Log.w(TAG, "SEND THREAD RUNNING...");

            List<Socket> toRemove = new LinkedList<>();
            List<Socket> toWrite = null;

            while (!Thread.currentThread().isInterrupted()){
                ObjectBuffer buffer = streamTCPServer.objectQueue.dequeue();
                if (buffer == null)//signal
                    break;

                synchronized(streamTCPServer._sync) {
                    toWrite = new LinkedList<>(streamTCPServer.clients);
                }

                for(Socket c:toWrite){
                    try {
                        c.getOutputStream().write(buffer.getArray(), 0, buffer.getSize());
                    } catch (IOException e) {
                        //e.printStackTrace();
                        toRemove.add(c);
                    }
                }

                synchronized(streamTCPServer._sync) {
                    for (Socket c : toRemove) {
                        Log.w(TAG, "removing Client: " + c.getInetAddress().getHostAddress());
                        streamTCPServer.clients.remove(c);
                        try {
                            c.close();
                        } catch (IOException e) {
                            //e.printStackTrace();
                        }
                    }
                }

                toRemove.clear();

                streamTCPServer.objectPool.release(buffer);
            }
        }
    }

}
