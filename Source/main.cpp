#include <winsock2.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ws2tcpip.h>
#include <stdio.h>

#define BUFSIZE 2048
SOCKET connection = INVALID_SOCKET;
char receivebuffer[BUFSIZE];
static char Base64Table[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
                             'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
                             'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
                             'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
                             'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
                             'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
                             'w', 'x', 'y', 'z', '0', '1', '2', '3',
                             '4', '5', '6', '7', '8', '9', '+', '/'};
static int ModTable[] = {0, 2, 1};

char* EncodeBase64(const char* input,int inputlength) {

    int outlength = 4 * ((inputlength + 2) / 3);

    char* output = (char*)malloc(outlength+1);

    for(int i = 0,j = 0;i < inputlength;) {

        unsigned int octet_a = i < inputlength ? input[i++] : 0;
        unsigned int octet_b = i < inputlength ? input[i++] : 0;
        unsigned int octet_c = i < inputlength ? input[i++] : 0;

        unsigned int triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

        output[j++] = Base64Table[(triple >> 3 * 6) & 0x3F];
        output[j++] = Base64Table[(triple >> 2 * 6) & 0x3F];
        output[j++] = Base64Table[(triple >> 1 * 6) & 0x3F];
        output[j++] = Base64Table[(triple >> 0 * 6) & 0x3F];
    }

    for(int i = 0; i < ModTable[inputlength % 3]; i++) {
    	output[outlength - 1 - i] = '=';
    }
    output[outlength] = 0;
    return output;
}

bool WinSockSend(const char* buffer,int length) {
	printf("C: %s\n",buffer);
	int iresult = send(connection,buffer,length, 0);
	Sleep(1000); // why is this needed...
	if(iresult == SOCKET_ERROR) {
	    printf("send failed: %d\n", WSAGetLastError());
	    return false;
	} else {
		int receivecount = 0;
		do {
			memset(receivebuffer,0,BUFSIZE);
			receivecount = recv(connection,receivebuffer,BUFSIZE,0);
			if(receivecount > 0) {
				printf("S: %s\n",receivebuffer);
				if(receivecount < BUFSIZE) { // assume it's done
					break;
				}
			} else if(receivecount == SOCKET_ERROR) {
			    printf("recv failed: %d\n", WSAGetLastError());
			}
		} while(receivecount > 0);
		return true;
	}
}
bool WinSockSend(const char* buffer) {
	return WinSockSend(buffer,strlen(buffer));
}
bool WinSockSend64(const char* command,const char* data,int datalength) {
	// encode data before sending
	char* encodeddata = EncodeBase64(data,datalength);
	
	// Then combine string
	char combined[1024];
	snprintf(combined,1024,"%s %s",command,encodeddata);
	bool result = WinSockSend(combined,strlen(combined));
	
	// Free encoded string
	delete[] encodeddata;
	
	return result;
}
char WinSockSendInput() {
	char input[512] = "";
	scanf("%c",input);
	char combined[1024];
	snprintf(combined,1024,"%s\r\n",input);
	return WinSockSend(combined);
}
bool WinSockInit(const char* server,const char* port) {
	
	// Initialize WinSock2
	WSADATA wsadata;
	memset(&wsadata,0,sizeof(wsadata));
	int iresult = WSAStartup(MAKEWORD(2,2),&wsadata);
	if(iresult != 0) {
		printf("WSAStartup failed: %d\n",iresult);
		return false;
	}
	
	addrinfo *result = NULL;
	addrinfo hints;
	
	memset(&hints,0,sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	// Check if we can connect to google via port 25
	iresult = GetAddrInfo(server,port,&hints,&result);
	if(iresult != 0) {
	    printf("getaddrinfo failed: %d\n",iresult);
	    WSACleanup();
	    return false;
	}

	// Create a socket object to our url, try all addresses
	addrinfo *walker = result;
	while(walker) {
		connection = INVALID_SOCKET;
		connection = socket(walker->ai_family,walker->ai_socktype,walker->ai_protocol);
		if(connection == INVALID_SOCKET) {
		    printf("Error at socket(): %d\n",WSAGetLastError());
		    freeaddrinfo(result);
		    WSACleanup();
		    return false; // no hope, quit
		}
		
		// Yay. Try the next step
		iresult = connect(connection,walker->ai_addr,(int)walker->ai_addrlen);
		if(iresult == SOCKET_ERROR) {
			printf("Error at connect(): %d\n", WSAGetLastError());
		    closesocket(connection);
		}
		walker = walker->ai_next;
	}
	
	freeaddrinfo(result);
	if(connection == INVALID_SOCKET) {
	    WSACleanup();
	    return false;
	} else {
		return true;
	}
}

void WinSockDelete() {
	closesocket(connection);
	WSACleanup();
}

int main() {
	// Try to connect (slow...)
	if(!WinSockInit("smtp.gmail.com","587")) {
		return 1;
	}
	
	// Send stuff here using the WinSockSend* functions
//	WinSockSend()
	
	// Free everything
	WinSockDelete();
	return 0;
}
