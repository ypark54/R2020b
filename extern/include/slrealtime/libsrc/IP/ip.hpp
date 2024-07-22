/* Copyright 2019 The MathWorks, Inc. */

#ifndef ip_hpp
#define ip_hpp

#include    <vector>
#include    <stdio.h>
#include    <string>
#include    <memory>
#include    <algorithm>

#if defined(MATLAB_MEX_FILE)
#define IPAPI __declspec(dllexport)
#else
#define IPAPI
#endif

namespace slrealtime {
    namespace ip {
       
        #define HostIPAddrForUseHostTargetConn "0.0.0.0"

        class Socket;

        IPAPI Socket *GetSocket(std::string address, uint16_t port);
        IPAPI void ThrowWarning(std::string);
        IPAPI std::string Trim(std::string);

        IPAPI std::string GetTargetIPAddr();

        class SocketList
        {
        private:
            static std::vector<Socket *> sockets;
            SocketList() {}
            ~SocketList() {}
        public:
            static IPAPI Socket *findSocket(std::string localAddress, uint16_t port);
            static IPAPI void addSocket(Socket * socket);
            static IPAPI void removeSocket(Socket * socket);
            static IPAPI int getNumSockets();
        };

        class Socket
        {
        protected:
            std::string localAddress_;
            uint16_t port_ = 0;
            bool disabled_ = false;
           
        public:
            IPAPI Socket(std::string localAddress, uint16_t port);
            IPAPI virtual ~Socket() { SocketList::removeSocket(this); }
            IPAPI std::string localAddress() const { return localAddress_; }
            IPAPI uint16_t port() const { return port_; }
            IPAPI virtual std::string type() = 0;
            IPAPI virtual void disable() = 0;
        };

    }
}

#endif