#include <iostream>
#include <string>
#include <fstream>

#include "timercpp.h"

#include "../Example1/Example1.h"
#include "../Example1/ReturnCodes.h"

using namespace std;

user_dto current_user;

bool logged_in = false;

void help()
{
	cout << "help [h] - show this help" << endl;
	cout << "register - register a new account" << endl;
	cout << "login - login using existing account" << endl;
	cout << "logout - logout from the current account" << endl;
	cout << "users - list all users" << endl;
	cout << "write - write an message" << endl;
	cout << "read - list last 10 messages" << endl;
}

void client_sign_on()
{
	user_dto u;

	cout << "Login: ";
	scanf("%s", u.login);
	cout << "Password: ";
	scanf("%s", u.password);

	int result = sign_up(u);

	switch (result)
	{
	case RC_OK:
		cout << "Ok" << endl;
		break;
	case RC_USER_ALREADY_EXISTS:
		cout << "The login is already taken" << endl;
		break;
	default: 
		break;
	}
}

void client_sign_in()
{
	user_dto u;

	cout << "Login: ";
	scanf("%s", u.login);
	cout << "Password: ";
	scanf("%s", u.password);

	int result = sign_in(u);

	switch (result)
	{
	case RC_OK:
		current_user = u;
		logged_in = true;
		cout << "Ok" << endl;
		break;
	case RC_INVALID_CREDENTIALS:
		cout << "Invalid credentials" << endl;
		break;
	default:
		break;
	}
}

void sign_out()
{
	strcpy(current_user.login, "");
	strcpy(current_user.password, "");

	logged_in = false;

	cout << "Ok" << endl;
}

void warn_login()
{
	cout << "Please login first" << endl;
}

void list_users()
{
	user_dto buffer[2];
	int offset = 0;
	int read;

	while (read = get_users(current_user, buffer, offset, 2), read != 0)
	{
		for (int i = 0; i < read; i++)
		{
			cout << buffer[i].login;

			if (buffer[i].online)
			{
				cout << " [online]" << endl;
			}
			else
			{
				cout << " [offline]" << endl;
			}
		}

		offset += read;
	}
}

void write_message()
{
	message_dto message;
	message.from = current_user;

	cout << "Recipient: ";
	
	string recipient;
	scanf("%s", message.to.login);

	cout << "Message: ";
	fgets(message.body, 60, stdin);
	fgets(message.body, 60, stdin);
	message.body[strlen(message.body) - 1] = 0;

	int result = write_message(message);

	switch (result)
	{
	case RC_OK:
		cout << "Ok" << endl;
		break;
	case RC_INVALID_CREDENTIALS:
		cout << "Login or password is not valid" << endl;
		break;
	case RC_INVALID_RECIPIENT:
		cout << "Invalid recipient username" << endl;
		break;
	}
}

void list_messages()
{
	message_dto buffer[2];
	const int buffer_size = 2;
	const int max_read = 10;

	int offset = 0;
	int read;

	while (read = read_messages(current_user, buffer, offset, buffer_size), read != 0 && offset < max_read)
	{
		for (int i = 0; i < read; i++)
		{
			cout << "[" << buffer[i].from.login << " -> " << buffer[i].to.login << "]" << buffer[i].body;

			if (!buffer[i].read)
			{
				cout << " [new]" << endl;
			}
			else
			{
				cout << " [read]" << endl;
			}
		}

		offset += read;
	}
}

void upload()
{
	cout << "File name: ";
	string filename;
	cin >> filename;

	ifstream in(filename, ios::in | ios::binary);

	if (!in.good())
	{
		cout << "File is not good" << endl;
		return;
	}

	char buffer[256];
	long long offset = 0;

	while (in.read(buffer, 256), in.gcount() != 0)
	{
		file_write(filename.c_str(), buffer, offset, in.gcount());
		offset += in.gcount();
	}

	in.close();

	cout << "Ok" << endl;
}

void download()
{
	cout << "File name: ";
	string filename;
	cin >> filename;

	ofstream out(filename, ios::out | ios::binary | ios::app);

	char buffer[256];
	long long offset = 0;
	int read;

	auto sdf = out.good();

	while (read = file_read(filename.c_str(), buffer, offset, 256), read != 0)
	{
		out.write(buffer, read);
		offset += read;
	}

	out.close();

	cout << "Ok" << endl;
}

void loop()
{
	while (true)
	{
		string command;
		cin >> command;

		if (command == "help" || command == "h")
		{
			help();
			continue;
		}

		if (command == "register")
		{
			client_sign_on();
			continue;
		}

		if (command == "login")
		{
			client_sign_in();
			continue;
		}

		if (command == "logout")
		{
			sign_out();
			continue;
		}

		if (command == "users")
		{
			if (!logged_in)
			{
				warn_login();
				continue;
			}

			list_users();
			continue;
		}

		if (command == "write")
		{
			if (!logged_in)
			{
				warn_login();
				continue;
			}

			write_message();
			continue;
		}

		if (command == "read")
		{
			if (!logged_in)
			{
				warn_login();
				continue;
			}

			list_messages();
			continue;
		}

		if (command == "upload")
		{
			//if (!logged_in)
			//{
			//	warn_login();
			//	continue;
			//}

			upload();
			continue;
		}

		if (command == "download")
		{
			//if (!logged_in)
			//{
			//	warn_login();
			//	continue;
			//}

			download();
			continue;
		}

		cout << "Unknown command. Please use help." << endl;
	}
}

int main()
{
	RPC_STATUS status;
	unsigned char* szStringBinding = NULL;

	// Creates a string binding handle.
	// This function is nothing more than a printf.
	// Connection is not done here.
	status = RpcStringBindingCompose(
		NULL, // UUID to bind to.
		reinterpret_cast<unsigned char*>("ncacn_ip_tcp"), // Use TCP/IP
		// protocol.
		reinterpret_cast<unsigned char*>("localhost"), // TCP/IP network
		// address to use.
		reinterpret_cast<unsigned char*>("4747"), // TCP/IP port to use.
		NULL, // Protocol dependent network options to use.
		&szStringBinding); // String binding output.

	if (status)
		exit(status);

	// Validates the format of the string binding handle and converts
	// it to a binding handle.
	// Connection is not done here either.
	status = RpcBindingFromStringBinding(
		szStringBinding, // The string binding to validate.
		&hExample1Binding); // Put the result in the implicit binding
	// handle defined in the IDL file.

	if (status)
		exit(status);

	RpcTryExcept
		{
			loop();
		}
	RpcExcept(1)
		{
			std::cerr << "Runtime reported exception " << RpcExceptionCode() << std::endl;
		}
	RpcEndExcept

	// Free the memory allocated by a string.
	status = RpcStringFree(
		&szStringBinding); // String to be freed.

	if (status)
		exit(status);

	// Releases binding handle resources and disconnects from the server.
	status = RpcBindingFree(
		&hExample1Binding); // Frees the implicit binding handle defined in
	// the IDL file.

	if (status)
		exit(status);
}

// Memory allocation function for RPC.
// The runtime uses these two functions for allocating/deallocating
// enough memory to pass the string to the server.
void* __RPC_USER midl_user_allocate(size_t size)
{
	return malloc(size);
}

// Memory deallocation function for RPC.
void __RPC_USER midl_user_free(void* p)
{
	free(p);
}
