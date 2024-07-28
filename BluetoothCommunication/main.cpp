#include <initguid.h>
#include <MouseEmulator.h>
#include <stdio.h>
#include <string.h>
#include <WinSock2.h>
#include <ws2bth.h>



// {B62C4E8D-62CC-404b-BBBF-BF3E3BBB1374}
DEFINE_GUID(g_guidServiceClass, 0xb62c4e8d, 0x62cc, 0x404b, 0xbb, 0xbf, 0xbf, 0x3e, 0x3b, 0xbb, 0x13, 0x74);


#define CXN_BDADDR_STR_LEN              17
#define CXN_MAX_INQUIRY_RETRY           3
#define CXN_DELAY_NEXT_INQUIRY          15 // seconds
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
        //
        // Resolve bluetooth device address
        //
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

        //
        // Create socket for client
        //
        sock = socket(AF_BTH, SOCK_STREAM, BTHPROTO_RFCOMM);
        if (INVALID_SOCKET == sock) {
            printf("-FATAL- | socket() call failed. WSAGetLastError = [%d]\n", WSAGetLastError());
            ulRetCode = CXN_ERROR;
        }
    }

    if (CXN_SUCCESS == ulRetCode) {
        //
        // Bind to socket and connect to device
        //
        if (SOCKET_ERROR == connect(sock, (const sockaddr*)&deviceBthAddr, sizeof(SOCKADDR_BTH))) {
            printf("-FATAL- | connect() call failed. WSAGetLastError=[%d]\n", WSAGetLastError());
            ulRetCode = CXN_ERROR;
        }
    }

    //
    // Create a buffer for listening to incoming data
    //
    char* dataBuffer = (char*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, CXN_DATA_PACKET_SIZE);
    if (NULL == dataBuffer) {
        printf("-FATAL- | HeapAlloc failed | out of memory | gle = [%d] \n", GetLastError());
        ulRetCode = CXN_ERROR;
    }

    if (CXN_SUCCESS == ulRetCode) {
        //
        // Listen for incoming data from device (for one data packet)
        //
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
                printf("=CRITICAL= | recv() call failed. WSAGetLastError=[%d]\n", WSAGetLastError());
                continueReading = FALSE;
                ulRetCode = CXN_ERROR;
                break;

            default:
                if (bytesReceived > (CXN_DATA_PACKET_SIZE - totalBytesReceived)) {
                    printf("=CRITICAL= | Received too many bytes\n");
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

    //
    // Close socket connection
    //
    if (SOCKET_ERROR == closesocket(sock)) {
        printf("=CRITICAL= | closesocket() call failed w/socket = [0x%I64X]. WSAGetLastError=[%d]\n", sock, WSAGetLastError());
        ulRetCode = CXN_ERROR;
    }

    sock = INVALID_SOCKET;

    return ulRetCode;
}

ULONG NameToBthAddr(const char deviceName[], PSOCKADDR_BTH remoteBthAddr) {
    UINT                    res = CXN_SUCCESS;
    PWSAQUERYSET            pWSAQuerySet = NULL;
    ULONG                   flags = 0, sizeWSAQuerySet = sizeof(WSAQUERYSET);
    bool                    deviceFound = FALSE, continueLookup = FALSE;
    HANDLE                  lookupHandle = NULL;

    pWSAQuerySet = (PWSAQUERYSET)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeWSAQuerySet);
    if (NULL == pWSAQuerySet) {
        printf("!ERROR! | Unable to allocate memory for WSAQUERYSET\n");
        res = STATUS_NO_MEMORY;
    }

    //
    // Search for the device with the correct name
    //
    if (CXN_SUCCESS == res) {
        for (UINT retryCount = 0;
            !deviceFound && (retryCount < CXN_MAX_INQUIRY_RETRY);
            retryCount++) {
            //
            // WSALookupService is used for both service search and device inquiry
            // LUP_CONTAINERS is the flag which signals that we're doing a device inquiry.
            //
            flags = LUP_CONTAINERS;

            //
            // Friendly device name (if available) will be returned in lpszServiceInstanceName
            //
            flags |= LUP_RETURN_NAME;

            //
            // BTH_ADDR will be returned in lpcsaBuffer member of WSAQUERYSET
            //
            flags |= LUP_RETURN_ADDR;

            if (retryCount == 0) {
                printf("*INFO* | Inquiring device from cache...\n");
            }
            else {
                //
                // Flush the device cache for all inquiries, except for the first inquiry
                //
                // By setting LUP_FLUSHCACHE flag, we're asking the lookup service to do
                // a fresh lookup instead of pulling the information from device cache.
                //
                flags |= LUP_FLUSHCACHE;

                //
                // Pause for some time before all the inquiries after the first inquiry
                //
                // Remote Name requests will arrive after device inquiry has
                // completed.  Without a window to receive IN_RANGE notifications,
                // we don't have a direct mechanism to determine when remote
                // name requests have completed.
                //
                printf("*INFO* | Unable to find device.  Waiting for %d seconds before re-inquiry...\n", CXN_DELAY_NEXT_INQUIRY);
                Sleep(CXN_DELAY_NEXT_INQUIRY * 1000);

                printf("*INFO* | Inquiring device ...\n");
            }

            if (NULL == pWSAQuerySet) {
                break;
            }
            ZeroMemory(pWSAQuerySet, sizeWSAQuerySet);
            pWSAQuerySet->dwNameSpace = NS_BTH;
            pWSAQuerySet->dwSize = sizeWSAQuerySet;

            res = WSALookupServiceBegin(pWSAQuerySet, flags, &lookupHandle);

            //
            // Even if we have an error, we want to continue until we
            // reach the CXN_MAX_INQUIRY_RETRY
            //
            if ((NO_ERROR == res) && (NULL != lookupHandle)) {
                continueLookup = TRUE;
            }
            else if (retryCount > 0) {
                printf("=CRITICAL= | WSALookupServiceBegin() failed with error code %d, WSAGetLastError = %d\n", res, WSAGetLastError());
                break;
            }

            while (continueLookup) {
                //
                // Get information about next bluetooth device
                //
                if (NO_ERROR == WSALookupServiceNext(lookupHandle, flags, &sizeWSAQuerySet, pWSAQuerySet)) {
                    //
                    // Compare the name to see if this is the device we are looking for.
                    //
                    if ((pWSAQuerySet->lpszServiceInstanceName != NULL) &&
                        (CXN_SUCCESS == strcmp((char *)pWSAQuerySet->lpszServiceInstanceName, deviceName))) {
                        //
                        // Found a remote bluetooth device with matching name.
                        // Get the address of the device and exit the lookup.
                        //
                        CopyMemory(remoteBthAddr,
                            (PSOCKADDR_BTH)pWSAQuerySet->lpcsaBuffer->RemoteAddr.lpSockaddr,
                            sizeof(*remoteBthAddr));

                        deviceFound = TRUE;
                        continueLookup = FALSE;
                    }
                }
                else {
                    res = WSAGetLastError();

                    if (WSA_E_NO_MORE == res) { // No more data
                        //
                        // No more devices found.  Exit the lookup.
                        //
                        continueLookup = FALSE;
                    }
                    else if (WSAEFAULT == res) {
                        //
                        // The buffer for QUERYSET was insufficient.
                        // In such case 3rd parameter "ulPQSSize" of function "WSALookupServiceNext()" receives
                        // the required size.  So we can use this parameter to reallocate memory for QUERYSET.
                        //
                        HeapFree(GetProcessHeap(), 0, pWSAQuerySet);
                        pWSAQuerySet = (PWSAQUERYSET)HeapAlloc(GetProcessHeap(),
                            HEAP_ZERO_MEMORY,
                            sizeWSAQuerySet);
                        if (NULL == pWSAQuerySet) {
                            printf("!ERROR! | Unable to allocate memory for WSAQERYSET\n");
                            continueLookup = FALSE;
                        }
                    }
                    else {
                        printf("=CRITICAL= | WSALookupServiceNext() failed with error code %d\n", res);
                        continueLookup = FALSE;
                    }
                }
            }

            //
            // End the lookup service
            //
            WSALookupServiceEnd(lookupHandle);
        }
    }

    if (NULL != pWSAQuerySet) {
        HeapFree(GetProcessHeap(), 0, pWSAQuerySet);
        pWSAQuerySet = NULL;
    }


    res = deviceFound ? CXN_SUCCESS : CXN_ERROR;


    return res;
}