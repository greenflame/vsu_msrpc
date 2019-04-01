#include <iostream>
#include <string>

#include "../Example1/Example1.h"
#include "../Example1/ReturnCodes.h"

using namespace std;

bool logged_in = false;
string login;
string password;

void help()
{
	cout << "help [h] - show this help" << endl;
	cout << "register - register a new account" << endl;
	cout << "login - login using existing account" << endl;
	cout << "logout - logout from the current account" << endl;
	cout << "list - list all users" << endl;
}

void client_sign_on()
{
	string login, password;
	cout << "Login: ";
	cin >> login;
	cout << "Password: ";
	cin >> password;

	user_dto u;
	strcpy(u.login, login.c_str());
	strcpy(u.password, password.c_str());

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
	string l, p;
	cout << "Login: ";
	cin >> l;
	cout << "Password: ";
	cin >> p;

	user_dto u;
	strcpy(u.login, l.c_str());
	strcpy(u.password, p.c_str());

	int result = sign_in(u);

	switch (result)
	{
	case RC_OK:
		login = l;
		password = p;
		logged_in = true;
		cout << "Ok" << endl;
		break;
	case RC_USER_DOES_NOT_EXIST:
		cout << "Invalid credentials" << endl;
		break;
	default:
		break;
	}
}

void sign_out()
{
	login = "";
	password = "";
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

	while (read = get_users(buffer, offset, 2), read != 0)
	{
		for (int i = 0; i < read; i++)
		{
			cout << buffer[i].login << endl;
		}

		offset += read;
	}
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

		if (command == "list")
		{
			if (!logged_in)
			{
				warn_login();
				continue;
			}

			list_users();
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
