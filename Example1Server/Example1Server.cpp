#include <iostream>
#include <string>
#include <vector>
#include <mutex>
#include <ctime>
#include <fstream>
#include <filesystem>

#include "../Example1/Example1.h"
#include "../Example1/ReturnCodes.h"

using namespace std;

class User
{
public:
	string login;
	string password;
	int last_activity;

	User(user_dto dto)
	{
		login = dto.login;
		password = dto.password;

		update_last_activity();
	}

	user_dto to_dto(boolean include_password = false) const
	{
		user_dto dto;

		strcpy(dto.login, login.c_str());

		if (include_password)
		{
			strcpy(dto.login, login.c_str());
		}
		else
		{
			strcpy(dto.password, "");
		}

		dto.online = time(nullptr) - last_activity < 1000 * 60;

		return dto;
	}

	void update_last_activity()
	{
		last_activity = time(nullptr);
	}
};

class Message
{
public:
	User* from;
	User* to;

	string body;
	int timestamp;
	bool read;

	Message(User* from, User* to, string body)
	{
		this->from = from;
		this->to = to;
		this->body = body;

		read = false;
		timestamp = time(nullptr);
	}

	message_dto to_dto() const
	{
		message_dto dto;

		dto.from = from->to_dto();
		dto.to = to->to_dto();

		strcpy(dto.body, body.c_str());
		dto.read = read;

		return dto;
	}
};

vector<User*> users;

vector<Message*> messages_;

User* get_user(string login)
{
	for (auto u : users)
	{
		if (u->login == login)
		{
			return u;
		}
	}

	return nullptr;
}

User* get_user(const user_dto dto)
{
	const auto user = get_user(dto.login);

	if (user->password != string(dto.password))
	{
		return nullptr;
	}

	return user;
}

int sign_up(user_dto dto)
{
	auto u = get_user(dto.login);

	if (u != nullptr)
	{
		cout << "User already exists: " + u->login << endl;
		return RC_USER_ALREADY_EXISTS;
	}

	users.push_back(new User(dto));

	cout << "User signed up: " + (new User(dto))->login << endl;
	return RC_OK;
}

int sign_in(user_dto dto)
{
	auto u = get_user(dto);

	if (u == nullptr)
	{
		cout << "User sign in error: " << dto.login << endl;
		return RC_INVALID_CREDENTIALS;
	}

	u->update_last_activity();

	cout << "User signed in: " + u->login << endl;
	return RC_OK;
}

int get_users(user_dto user, user_dto* buffer, int offset, int count)
{
	auto u = get_user(user);

	if (u == nullptr)
	{
		return 0;
	}

	u->update_last_activity();

	int read = 0;

	for (auto i = offset; i < offset + count && i < users.size(); i++)
	{
		buffer[read] = users[i]->to_dto();
		read++;
	}

	return read;
}

int write_message(message_dto dto)
{
	auto from = get_user(dto.from);

	if (from == nullptr)
	{
		return RC_INVALID_CREDENTIALS;
	}

	from->update_last_activity();

	auto to = get_user(dto.to.login);

	if (to == nullptr)
	{
		return RC_INVALID_RECIPIENT;
	}

	messages_.push_back(new Message(from, to, dto.body));

	cout << "New message form " << from->login << " to " << to->login << ": " << dto.body << endl;
	return RC_OK;
}

int read_messages(user_dto user, message_dto* buffer, int offset, int count)
{
	auto u = get_user(user);

	if (u == nullptr)
	{
		return 0;
	}

	int pos = messages_.size() - 1;
	int skipped = 0;

	while (skipped != offset && pos != -1)
	{
		if (messages_[pos]->from->login == u->login || messages_[pos]->to->login == u->login)
		{
			skipped++;
		}

		pos--;
	}

	if (skipped != offset)
	{
		return 0;
	}

	int result = 0;

	while (result != count && pos != -1)
	{
		if (messages_[pos]->from->login == u->login || messages_[pos]->to->login == u->login)
		{
			buffer[result] = messages_[pos]->to_dto();

			if (messages_[pos]->to->login == u->login)
			{
				messages_[pos]->read = true;
			}

			result++;
		}

		pos--;
	}

	return result;
}

const string storage_directory_name = "storage";

void file_write(const char filename[256], char* buffer, long long offset, int length)
{
	char name[128], extension[128];
	_splitpath(filename, nullptr, nullptr, name, extension);

	ofstream out(storage_directory_name + "\\" + name + extension, ios::out | ios::binary | ios::app);
	out.seekp(offset, ios_base::beg);
	out.write(buffer, length);
	out.close();
}

int file_read(const char filename[256], char* buffer, long long offset, int length)
{
	ifstream in(storage_directory_name + "\\" + filename, ios::binary | ios::in);

	if (!in.good())
	{
		return 0;
	}

	in.seekg(offset, ios_base::beg);
	in.read(buffer, length);
	int read = in.gcount();
	in.close();

	return read;
}

void init_storage_directory()
{
	if (experimental::filesystem::exists(storage_directory_name))
	{
		experimental::filesystem::remove_all(storage_directory_name);
	}

	experimental::filesystem::create_directory(storage_directory_name);
}

/* STARTUP */

// Naive security callback.
RPC_STATUS CALLBACK SecurityCallback(RPC_IF_HANDLE /*hInterface*/, void* /*pBindingHandle*/)
{
	return RPC_S_OK; // Always allow anyone.
}

int main()
{
	init_storage_directory();

	RPC_STATUS status;

	// Uses the protocol combined with the endpoint for receiving
	// remote procedure calls.
	status = RpcServerUseProtseqEp(
		reinterpret_cast<unsigned char*>("ncacn_ip_tcp"), // Use TCP/IP protocol.
		RPC_C_PROTSEQ_MAX_REQS_DEFAULT, // Backlog queue length for TCP/IP.
		reinterpret_cast<unsigned char*>("4747"), // TCP/IP port to use.
		NULL); // No security.

	if (status)
		exit(status);

	// Registers the Example1 interface.
	status = RpcServerRegisterIf2(
		Example1_v1_0_s_ifspec, // Interface to register.
		NULL, // Use the MIDL generated entry-point vector.
		NULL, // Use the MIDL generated entry-point vector.
		RPC_IF_ALLOW_CALLBACKS_WITH_NO_AUTH, // Forces use of security callback.
		1, //RPC_C_LISTEN_MAX_CALLS_DEFAULT, // Use default number of concurrent calls.
		(unsigned)-1, // Infinite max size of incoming data blocks.
		SecurityCallback); // Naive security callback.

	if (status)
		exit(status);

	// Start to listen for remote procedure calls for all registered interfaces.
	// This call will not return until RpcMgmtStopServerListening is called.
	status = RpcServerListen(
		1, // Recommended minimum number of threads.
		RPC_C_LISTEN_MAX_CALLS_DEFAULT, // Recommended maximum number of threads.
		FALSE); // Start listening now.

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
