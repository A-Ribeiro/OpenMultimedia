#pragma once

#include <InteractiveToolkit/InteractiveToolkit.h>

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
//#include <net/if_dl.h>
#include <ifaddrs.h>
#include <net/if.h>


class NetworkInterface {
public:
    std::string name;
    std::string broadcast;
    struct sockaddr_in broadcast_addr;

    static std::vector<NetworkInterface> ListIPv4WithBroadcast() {
        std::vector<NetworkInterface> result;

        struct ifaddrs *ifap, *ifaptr;
        unsigned char *ptr;

        if (getifaddrs(&ifap) == 0) {
            for(ifaptr = ifap; ifaptr != NULL; ifaptr = (ifaptr)->ifa_next) {
                if (ifaptr->ifa_addr->sa_family == AF_INET) { //AF_INET6

                    if (ifaptr->ifa_flags & IFF_BROADCAST){

                        //struct sockaddr_in bCast_addr = *(const struct sockaddr_in*)ifaptr->ifa_broadaddr;

                        char broadcast[NI_MAXHOST];
                        int rc = getnameinfo(ifaptr->ifa_broadaddr,
                            sizeof(struct sockaddr_in),
                            broadcast, NI_MAXHOST,
                            NULL, 0, NI_NUMERICHOST);

                        if (rc == 0){
                            printf("iFace name: %s\n", ifaptr->ifa_name);
                            printf("     bcast: %s\n", broadcast);
                            NetworkInterface ni;
                            ni.name = ifaptr->ifa_name;
                            ni.broadcast = broadcast;
                            ni.broadcast_addr = *(struct sockaddr_in*)ifaptr->ifa_broadaddr;
                            result.push_back(ni);
                        }

                    }
                }
            }
            freeifaddrs(ifap);
        }

        return result;
    }
};




class NetworkDiscovery : public EventCore::HandleCallback {
public:
    std::string name;
    Platform::SocketUDP socket_write;
    Platform::Thread thread;
    std::vector<NetworkInterface> bcast_iface;

    void sendThread() {
        
        struct sockaddr_in broadcast_addr;// = SocketUtils::mountAddress("127.0.0.1", NetworkConstants::PUBLIC_PORT_MULTICAST_START);

        while (!Platform::Thread::isCurrentThreadInterrupted()){

            for(size_t i=0;i<bcast_iface.size();i++){

                broadcast_addr = bcast_iface[i].broadcast_addr;
                broadcast_addr.sin_port = htons(Platform::NetworkConstants::PUBLIC_PORT_MULTICAST_START);

                //printf("     sending pkt to: %s\n", inet_ntoa(broadcast_addr.sin_addr));

                socket_write.write_buffer(broadcast_addr, (const uint8_t*)name.c_str(), name.length() + 1 );
            }

            Platform::Sleep::millis(1000);
        }
    }

    NetworkDiscovery(const char* _name):thread(EventCore::CallbackWrapper(&NetworkDiscovery::sendThread, this)) {
        name = std::string("OpenMultimediaVideoSource:") + std::string(_name);
        

        socket_write.createFD(false, true);
        socket_write.bind();// ephemeral port and address...

        bcast_iface = NetworkInterface::ListIPv4WithBroadcast();

    }

    ~NetworkDiscovery(){
        stop();
    }

    void start() {
        thread.start();
    }

    void stop() {
        thread.interrupt();
        thread.wait();
    }


};