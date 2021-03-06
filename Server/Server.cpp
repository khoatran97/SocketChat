// Server.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <vector>

#pragma warning(disable:4996)

#pragma comment (lib, "Ws2_32.lib")
#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"

void listenClient(void* pClient);

int iChat;
struct Account
{
	char Username[25];
	char Password[25];
	SOCKET* Client = NULL;

	Account();
	Account(char username[], char password[])
	{
		strcpy(this->Username, username);
		strcpy(this->Password, password);
	}
};

// Nhom chat
class GroupChat
{
private:
	SOCKET* members[10];
	int iMember = 0; // So thanh vien thuc co
	int initMember = 0; // So vi tri da dung de luu thanh vien
public:
	GroupChat() {};
	// Cac thao tac xoa sua khong lam thay doi vi tri cua cac client trong nhom chat
	SOCKET* getSocket(int i)
	{
		return members[i];
	}
	int getPos(SOCKET* socket)
	{
		for (int i = 0; i < initMember; i++)
			if (members[i] == socket)
				return i;
	}
	void addMember(SOCKET* s)
	{
		iMember++;
		members[initMember++] = s;
	}
	// Gui tin nhan den tat cac ca thanh vien trong nhom
	void sendMessage(char mgs[])
	{
		for (int i = 0; i < initMember; i++)
			if (members[i] != NULL)
				send(*(members[i]), mgs, strlen(mgs), 0);
	}
	void deleteMember(int pos)
	{
		members[pos] = NULL;
		iMember--;
	}
	int getIMember()
	{
		return iMember;
	}
	~GroupChat()
	{
		memset(members, NULL, iMember);
	}
};

// Luu thong tin cua thread lang nghe tin nhan tu 1 client
typedef struct Group_
{
	int Member; // Vi tri cua client trong nhom
	GroupChat* Group; // Nhom chat
}Group;

std::vector<Account> listAccount;

bool isExistedAccount(char username[])
{
	for (int i = 0; i < listAccount.size(); i++)
		if (strcmp(username, listAccount[i].Username) == 0)
			return true;
	return false;
}

void setOffline(SOCKET* client)
{
	for (int i = 0; i < listAccount.size(); i++)
		if (listAccount[i].Client == client)
			listAccount[i].Client = NULL;
}

SOCKET* isOnline(char username[])
{
	for (int i = 0; i < listAccount.size(); i++)
	{
		if (strcmp(listAccount[i].Username, username) == 0)
		{
			return listAccount[i].Client;
		}
	}
	return NULL;
}

int checkPassword(char username[], char password[])
{
	for (int i = 0; i < listAccount.size(); i++)
		if (strcmp(username, listAccount[i].Username) == 0 && strcmp(password, listAccount[i].Password) == 0)
			return i;
	return -1;
}

// Khoi tao server
SOCKET initServer()
{
	WSADATA wsaData;
	int iResult;

	SOCKET ListenSocket = INVALID_SOCKET;

	struct addrinfo *result = NULL;
	struct addrinfo hints;

	int iSendResult;
	char recvbuf[DEFAULT_BUFLEN];
	int recvbuflen = DEFAULT_BUFLEN;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed with error: %d\n", iResult);
		return NULL;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve the server address and port
	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return NULL;
	}

	// Create a SOCKET for connecting to server
	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET) {
		printf("socket failed with error: %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return NULL;
	}

	// Setup the TCP listening socket
	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		return NULL;
	}

	freeaddrinfo(result);

	return ListenSocket;
}

// Lang nghe cac tin nhan duoc gui tu client khi chat
void chatMgs(void* param)
{
	Group* client = (Group*)param;
	char mgs[200];
	char name[230];
	SOCKET thisClient = *(client->Group->getSocket(client->Member));
	do
	{
		if (client->Group->getIMember() <= 1)
			break;
		int i = recv(thisClient, mgs, 200, 0);
		mgs[i] = '\0';
		// Tin hieu ngung chat
		if (strcmp(mgs, "#") == 0)
		{
			// Ngung viec nhan tin nhan va xoa ra khoi nhom chat
			_beginthread(listenClient, 0, (void*)(client->Group->getSocket(client->Member)));
			client->Group->deleteMember(client->Member);
			break;
		}
		// Bo qua tin hieu chap nha chat (neu co gui)
		if (strcmp(mgs, "_chat") == 0)
			continue;
		// Tin hieu them thanh vien // Xu ly gan giong voi tao ra nhom chat moi
		if (strcmp(mgs, "add") == 0)
		{
			char strMgs[300];
			char username[10][25];
			int i = recv(thisClient, strMgs, 300, 0);
			strMgs[i] = '\0';

			int c = 0;
			char*p = strtok(strMgs, ", ");
			while (p != NULL)
			{
				strcpy(username[c++], p);
				p = strtok(NULL, ", ");
			}
			int tmp = 0;
			for (int k = 0; k < c; k++)
			{
				SOCKET* toUser = isOnline(username[k]);
				if (toUser != NULL && *toUser != thisClient)
				{
					client->Group->addMember(toUser);

					Group* group = new Group;
					group->Group = client->Group;
					group->Member = client->Group->getPos(toUser);
					_beginthread(chatMgs, 0, (void*)group);

					send(*toUser, "chat", 4, 0);

					tmp++;
				}
			}
			if (tmp == 0)
				send(thisClient, "0", 1, 0);
			else
				send(thisClient, "1", 1, 0);
			continue;
		}
		// Tim ten dang nhap cua client gui tin nhan
		for (int k = 0; k < listAccount.size(); k++)
		{
			if (listAccount[k].Client == client->Group->getSocket(client->Member))
			{
				strcpy(name, listAccount[k].Username);
				break;
			}
		}
		// Tao tin nhan duoi dang: "username: mgs"
		strcat(name, ": ");
		strcat(name, mgs);
		// Gui den tat ca cac thanh vien trong nhom chat
		client->Group->sendMessage(name);
	} while (true);
	_endthread();
}

// Tao mot nhom chat
void createChatGroup(void* s)
{
	SOCKET** listSocket = (SOCKET**)s;
	GroupChat* chat = new GroupChat;
	int i = 0;
	while (listSocket[i] != NULL)
	{
		chat->addMember(listSocket[i]); // Them thanh vien vao nhom chat
		i++;
	}
	for (int k = 0; k < i; k++)
	{
		Group* group = new Group;
		group->Group = chat; // Nhom chat
		group->Member = k; // Vi tri của thanh vien duoc lang nghe trong nhom chat
		if (k != 0)
			send(*(chat->getSocket(k)), "chat", 4, 0); // Gui tin hieu yeu cau chat den cac client khac

		// Nhan tin nhan tu cac client trong nhom
		_beginthread(chatMgs, 0, (void*)group);
	}
	while (chat->getIMember() > 1)
	{
		Sleep(1000);
	}
	// Ket thuc nhom chat khi so thanh vien <= 1
	chat->sendMessage("#");
	chat = NULL;
	delete[] chat;
	_endthread();
}

// Lang nghe yeu cau tu client
void listenClient(void* pClient)
{
	char strReceive[10];
	SOCKET* client = (SOCKET*)pClient;
	bool isSignin = false;
	do
	{
		int iResult = -1;
		while (iResult == -1)
			iResult = recv(*client, strReceive, 10, 0);
		strReceive[iResult] = '\0';

		if (!isSignin)
		{
			// Yeu cau dang ky
			if (strcmp(strReceive, "signup") == 0)
			{
				char username[25], password[25];
				int i = recv(*client, username, 10, 0);
				username[i] = '\0';
				i = recv(*client, password, 10, 0);
				password[i] = '\0';

				if (!isExistedAccount(username))
				{
					Account newAccount(username, password);
					listAccount.push_back(newAccount);
					printf("%s dang ky thanh cong\n", username);
					send(*client, "1", 1, 0);
					//// Goi ham luu listAccount xuong file (chi luu username, password)
				}
				else
					send(*client, "0", 1, 0);
			}
			// Yeu cau dang nhap
			if (strcmp(strReceive, "signin") == 0)
			{
				char username[25], password[25];
				int i = recv(*client, username, 10, 0);
				username[i] = '\0';
				i = recv(*client, password, 10, 0);
				password[i] = '\0';

				int result = checkPassword(username, password);
				if (checkPassword(username, password) != -1)
				{
					printf("%s dang nhap thanh cong\n", username);
					listAccount[result].Client = client;
					send(*client, "1", 1, 0);
				}
				else
				{
					send(*client, "0", 1, 0);
				}
			}
			// Yeu cau bat dau nhom chat
			if (strcmp(strReceive, "chat") == 0)
			{
				char strMgs[300];
				char username[10][25];
				int i = recv(*client, strMgs, 300, 0);
				strMgs[i] = '\0';

				// Tach cac username va kiem tra Online 
				// Neu online thi them Socket cua client vao mang socket
				int c = 0;
				char*p = strtok(strMgs, ", ");
				while (p != NULL)
				{
					strcpy(username[c++], p);
					p = strtok(NULL, ", ");
				}
				int tmp = 1;
				SOCKET* sockets[10] = { client };
				for (int k = 0; k < c; k++)
				{
					SOCKET* toUser = isOnline(username[k]);
					if (toUser != NULL && toUser != client)
						sockets[tmp++] = toUser;
				}
				sockets[tmp] = NULL;
				if (tmp<=1)
				{
					send(*client, "0", 1, 0);
				}
				else
				{
					send(*client, "1", 1, 0);
					// Tao nhom chat
					_beginthread(createChatGroup, 0, sockets);
				}
			}

			// Ket thuc viec lang nghe yeu cau khi co tin hieu bao hieu bat dau chat tu phia client
			if (strcmp(strReceive, "_chat") == 0)
			{
				_endthread();
			}
		}
	} while (strcmp(strReceive, "*") != 0); // Dung viec lang nghe client khi co tin hieu thoat tu client
	setOffline(client);
	_endthread();
}

int main()
{
	SOCKET server = initServer();

	//// Goi ham nap listAccount tu file len (nap username, password)

	std::vector<SOCKET*> clients;
	
	int iResult;
	if (server != NULL)
	{
		iResult = listen(server, SOMAXCONN);
		if (iResult == SOCKET_ERROR) {
			printf("listen failed with error: %d\n", WSAGetLastError());
			closesocket(server);
			WSACleanup();
			return 1;
		}

		while (true)
		{

			// Accept a client socket

			SOCKET* client = new SOCKET;
			// Doi ket noi tu client
			*client = accept(server, NULL, NULL);
			if (*client == INVALID_SOCKET) {
				printf("accept failed with error: %d\n", WSAGetLastError());
				closesocket(server);
				WSACleanup();
				return 1;
			}

			printf("a client has connected\n");
			// Bat dau lang nghe client khi co ket noi
			_beginthread(listenClient, 0, (void*)client);
		}
	}
	closesocket(server);
	system("pause");
    return 0;
}

