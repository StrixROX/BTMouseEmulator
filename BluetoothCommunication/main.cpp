#include <initguid.h>
#include <MouseEmulator.h>
#include <stdio.h>
#include <WinSock2.h>
#include <ws2bth.h>



// {B62C4E8D-62CC-404b-BBBF-BF3E3BBB1374}
DEFINE_GUID(g_guidServiceClass, 0xb62c4e8d, 0x62cc, 0x404b, 0xbb, 0xbf, 0xbf, 0x3e, 0x3b, 0xbb, 0x13, 0x74);


#define CXN_BDADDR_STR_LEN              17
#define CXN_MAX_INQUIRY_RETRIES         3
#define CXN_SUCCESS                     0
#define CXN_ERROR                       1
#define CXN_DATA_PACKET_SIZE            6



ULONG NameToBthAddr(const char deviceName[], PSOCKADDR_BTH remoteBthAddr);



int main() {
    ULONG               ulRetCode = CXN_SUCCESS;
    WSADATA             wsaData = { 0 };
    const char          deviceName[] = "SHARD 21";
    SOCKADDR_BTH        deviceBthAddr = { 0 };
    SOCKET              sock = INVALID_SOCKET;
    //int                 iCxnCount = 0;
    
    //
    // Ask for Winsock v2.2
    //
    ulRetCode = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (CXN_SUCCESS != ulRetCode) {
        printf("-FATAL- | Unable to initialize Winsock version 2.2\n");
        return ulRetCode;
    }

    if (CXN_SUCCESS == ulRetCode) {
        // Resolve bluetooth device address
        ulRetCode = NameToBthAddr(deviceName, &deviceBthAddr);
        if (CXN_SUCCESS != ulRetCode) {
            printf("-FATAL- | Unable to get address of the remote radio having name %s\n", deviceName);
        }
    }

    if (CXN_SUCCESS == ulRetCode) {
        //
        // Setting address family to AF_BTH indicates winsock2 to use Bluetooth sockets
        // Port should be set to 0 if ServiceClassId is spesified.
        //
        deviceBthAddr.addressFamily = AF_BTH;
        deviceBthAddr.serviceClassId = g_guidServiceClass;
        deviceBthAddr.port = 0;

        // Create socket for client
        sock = socket(AF_BTH, SOCK_STREAM, BTHPROTO_RFCOMM);
        if (INVALID_SOCKET == sock) {
            printf("-FATAL- | socket() call failed. WSAGetLastError = [%d]\n", WSAGetLastError());
            ulRetCode = CXN_ERROR;
        }
    }

    if (CXN_SUCCESS == ulRetCode) {
        // Bind to socket and connect to device
        if (SOCKET_ERROR == connect(sock, (const sockaddr*)&deviceBthAddr, sizeof(SOCKADDR_BTH))) {
            printf("-FATAL- | connect() call failed. WSAGetLastError=[%d]\n", WSAGetLastError());
            ulRetCode = CXN_ERROR;
        }
    }

    // Create a buffer for listening to incoming data
    char* dataBuffer = (char*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, CXN_DATA_PACKET_SIZE);
    if (NULL == dataBuffer) {
        printf("-FATAL- | HeapAlloc failed | out of memory | gle = [%d] \n", GetLastError());
        ulRetCode = CXN_ERROR;
    }

    if (CXN_SUCCESS == ulRetCode) {
        // Listen for incoming data from device (for one data packet)
        char* dataBufferIndex = dataBuffer;
        UINT totalBytesReceived = 0;

        bool continueReading = TRUE;

        while (continueReading && (totalBytesReceived < CXN_DATA_PACKET_SIZE)) {
            UINT bytesReceived = recv(sock, dataBufferIndex, CXN_DATA_PACKET_SIZE - totalBytesReceived, 0);

            switch (bytesReceived) {
            case 0:
                continueReading = FALSE;
                break;

            case SOCKET_ERROR:
                printf("-CRITICAL- | recv() call failed. WSAGetLastError=[%d]\n", WSAGetLastError());
                continueReading = FALSE;
                ulRetCode = CXN_ERROR;
                break;

            default:
                if (bytesReceived > (CXN_DATA_PACKET_SIZE - totalBytesReceived)) {
                    printf("-CRITICAL- | Received too many bytes\n");
                    continueReading = FALSE;
                    ulRetCode = CXN_ERROR;
                    break;
                }

                dataBufferIndex += bytesReceived;
                totalBytesReceived += bytesReceived;
                break;
            }
        }
    }

    // Close socket connection
    if (SOCKET_ERROR == closesocket(sock)) {
        wprintf(L"-CRITICAL- | closesocket() call failed w/socket = [0x%I64X]. WSAGetLastError=[%d]\n", sock, WSAGetLastError());
        ulRetCode = CXN_ERROR;
    }

    sock = INVALID_SOCKET;

    return ulRetCode;
}

ULONG NameToBthAddr(const char deviceName[], PSOCKADDR_BTH remoteBthAddr) {
    return 0;
}