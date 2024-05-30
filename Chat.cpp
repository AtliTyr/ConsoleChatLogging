#include "Chat.h"

//int iSendResult;
//SOCKET ClientSocket = INVALID_SOCKET;


constexpr auto SQL_RESULT_LEN = 240;
constexpr auto SQL_RETURN_CODE_LEN = 1024;

SQLHANDLE sqlConnHandle{ nullptr }; // дескриптор для соединения с базой данных
SQLHANDLE sqlEnvHandle{ nullptr }; // дескриптор окружения базы данных
SQLHANDLE sqlStmtHandle{ nullptr };  // дескриптор для выполнения запросов к базе данных
SQLWCHAR retconstring[SQL_RETURN_CODE_LEN]{}; // строка для кода возврата из функций API ODBC


Chat::Chat() = default;
Chat::~Chat() = default;

//#elif __linux__
//void Chat::SendUsersList(int con)
//{
//	char message[MESSAGE_LENGTH];
//	bzero(message, sizeof(message));
//
//	strcpy(message, "$");
//
//	ssize_t bytes = write(con, message, sizeof(message));
//
//	for (int i{ 0 }; i < base.getUsersCount(); i++)
//	{
//		strcpy(message, base.getUsersNameNLoginByIndex(i).c_str());
//		ssize_t bytes = write(con, message, sizeof(message));
//	}
//
//	strcpy(message, "$");
//	write(con, message, sizeof(message));
//
//}
//
//void Chat::SendGeneralMessages(int con)
//{
//	char message[MESSAGE_LENGTH];
//	bzero(message, sizeof(message));
//
//	Message tmp;
//
//	strcpy(message, "$");
//	ssize_t bytes = write(con, message, sizeof(message));
//
//	for (int i{ 0 }; i < m_base.getMessageCount(); i++)
//	{
//		tmp = m_base.getMessageByIndex(i);
//		if (tmp.getRecepientOfMessage() == "")
//		{
//			string t = "<" + tmp.getSenderOfMessage() + ">" + tmp.getMessage();
//			strcpy(message, t.c_str());
//			ssize_t bytes = write(con, message, sizeof(message));
//		}
//	}
//	strcpy(message, "$");
//	write(con, message, sizeof(message));
//
//}
//
//void Chat::SendPrivateMessages(int con, string login)
//{
//	char message[MESSAGE_LENGTH];
//	bzero(message, sizeof(message));
//
//	Message tmp;
//
//	strcpy(message, "$");
//	ssize_t bytes = write(con, message, sizeof(message));
//
//	for (int i{ 0 }; i < m_base.getMessageCount(); i++)
//	{
//		tmp = m_base.getMessageByIndex(i);
//		if ((tmp.getSenderOfMessage() == login && tmp.getRecepientOfMessage() != "") || (tmp.getRecepientOfMessage() == login))
//		{
//			string t = "<" + tmp.getSenderOfMessage() + "> <" + tmp.getRecepientOfMessage() + "> " + tmp.getMessage();
//			strcpy(message, t.c_str());
//			ssize_t bytes = write(con, message, sizeof(message));
//		}
//	}
//
//	strcpy(message, "$");
//	write(con, message, sizeof(message));
//}
//#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool Chat::SetDataBaseConnection()
{

	std::fstream conf("DatabaseConnectionSettings.txt", std::ios_base::in);

	std::string config;
	getline(conf, config);

	std::wstring c(config.begin(), config.end());

	conf.close();


	if (SQL_SUCCESS != SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &sqlEnvHandle))
		goto COMPLETED;

	if (SQL_SUCCESS != SQLSetEnvAttr(sqlEnvHandle, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0))
		goto COMPLETED;

	if (SQL_SUCCESS != SQLAllocHandle(SQL_HANDLE_DBC, sqlEnvHandle, &sqlConnHandle))
		goto COMPLETED;

	cout << "Attempting connection to SQL Server...\n";

	// Устанавливаем соединение с сервером  
	switch (SQLDriverConnect(sqlConnHandle,
		GetDesktopWindow(),
		/*(SQLWCHAR*)c.c_str(),*/
		(SQLWCHAR*)L"DRIVER={MySQL ODBC 8.3 ANSI Driver};SERVER=localhost;Trusted_Connection=yes;PORT=3306;UID=root;PWD=12345",
		SQL_NTS,
		retconstring,
		1024,
		NULL,
		SQL_DRIVER_COMPLETE)) {


	case SQL_SUCCESS:
	case SQL_SUCCESS_WITH_INFO:
		cout << "Successfully connected to SQL Server\n";
		break;

	case SQL_INVALID_HANDLE:
	case SQL_ERROR:
		cout << "Could not connect to SQL Server\n";
		goto COMPLETED;

	default:
		break;
	}

	// Если соединение не установлено, то выходим из программы
	if (SQL_SUCCESS != SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &sqlStmtHandle))
		goto COMPLETED;

	return true;

	// Закрываем соединение и выходим из программы
COMPLETED:
	SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
	SQLDisconnect(sqlConnHandle);
	SQLFreeHandle(SQL_HANDLE_DBC, sqlConnHandle);
	SQLFreeHandle(SQL_HANDLE_ENV, sqlEnvHandle);
	return false;

}

void Chat::CloseDataBaseConnection()
{

	std::cout << getLastLogInput() << std::endl;

	SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
	SQLDisconnect(sqlConnHandle);
	SQLFreeHandle(SQL_HANDLE_DBC, sqlConnHandle);
	SQLFreeHandle(SQL_HANDLE_ENV, sqlEnvHandle);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int Chat::Identification(std::string Email)
{
	std::string q = "select id from users where email='" + Email + "';";
	std::wstring query(q.begin(), q.end());

	SQLCloseCursor(sqlStmtHandle);

	SQLExecDirect(sqlStmtHandle,(SQLWCHAR*)query.c_str(), SQL_NTS);

	SQLINTEGER id;

	SQLBindCol(sqlStmtHandle, 1, SQL_INTEGER, &id, sizeof(id), nullptr);

	if (SQLFetch(sqlStmtHandle) != SQL_NO_DATA)
		return id;
	return -1;
}

bool Chat::Registration(std::string second_name, std::string name, std::string email, std::string pass)
{
	if (Identification(email) == -1)
	{
		//std::string hash_to_text = "";
		//for (int i = 0; i < 5; i++)
		//	hash_to_text = hash_to_text + std::to_string(pass[i]) + " ";

		std::string q = "call adding_new_user('" + second_name + "', '" + name + "', '" + email + "', '" + pass + "');";
		std::wstring query(q.begin(), q.end());
		SQLExecDirect(sqlStmtHandle, (SQLWCHAR*)query.c_str(), SQL_NTS);

		return true;
	}
	cout << "Email - " << email << " is occupied!" << endl;
	return false;
}

bool Chat::Authentification(std::string email, std::string pass)
{

	std::string q = "select * from passwords where user_id = (select id from users where email = '" + email + "');";
	std::wstring query(q.begin(), q.end());

	SQLCloseCursor(sqlStmtHandle);

	SQLRETURN rc;
	rc = SQLExecDirect(sqlStmtHandle, (SQLWCHAR*)query.c_str(), SQL_NTS);

	SQLLEN sql_str_length;

	SQLINTEGER V_OD_err, V_OD_id;
	SQLCHAR V_OD_buffer[200];

	V_OD_err = SQLBindCol(sqlStmtHandle, 1, SQL_INTEGER, &V_OD_id, sizeof(V_OD_id), nullptr);
	V_OD_err = SQLBindCol(sqlStmtHandle, 2, SQL_CHAR, &V_OD_buffer, SQL_RESULT_LEN, &sql_str_length);

	SQLFetch(sqlStmtHandle);

	std::string comp(reinterpret_cast<char*>(V_OD_buffer));

	if (pass == comp)
		return true;
	return false;
}

bool Chat::Authorization(std::string email, std::string pass)
{
	if (Identification(email) != -1)
	{
		if (Authentification(email, pass) == true)
		{
			//std::string q = "update users set status = 'online' where email = '" + email + "';";
			std::string q = "call users_authorization('" + email + "');";
			std::wstring query(q.begin(), q.end());
			
			SQLCloseCursor(sqlStmtHandle);

			SQLExecDirect(sqlStmtHandle, (SQLWCHAR*)query.c_str(), SQL_NTS);


			return true;
		}
	}
	return false;
}

void Chat::QuitServer(std::string email)
{
	std::string q = " update users set status = 'offline' where email = '" + email + "'; ";
	std::wstring query(q.begin(), q.end());
	
	SQLCloseCursor(sqlStmtHandle);
	SQLExecDirect(sqlStmtHandle,(SQLWCHAR*)query.c_str(), SQL_NTS);

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool Chat::WriteMessage(std::string sender_email, std::string recepient_email, std::string message)
{
	int s_id = Identification(sender_email);
	int r_id;
	if (recepient_email == "ALL")
		r_id = 0;
	else
		r_id = Identification(recepient_email);
	

	if (r_id == -1)
		return false;

	std::string q = "call adding_to_messages(" + std::to_string(s_id) + ", " + std::to_string(r_id) + ", '" + message + "');";
	std::wstring query(q.begin(), q.end());

	//////////////////////////////// Logging message
	
	SYSTEMTIME time;
	GetLocalTime(&time);

	std::stringstream t;
	t << time.wYear << "-" << time.wMonth << "-" << time.wDay << " " << time.wHour << ":" << time.wMinute << ":" << time.wSecond;
	
	std::string s = "[" + t.str() + "] (" + std::to_string(s_id) + " -> " + std::to_string(r_id) + ") { " + message + " }";
	logs.inputToLogs(s);

	////////////////////////////////

	SQLCloseCursor(sqlStmtHandle);
	SQLExecDirect(sqlStmtHandle, (SQLWCHAR*)query.c_str(), SQL_NTS);

	return true;
}

bool Chat::DeleteFromChat(std::string email)
{
	int id = Identification(email);

	if (id > 1)
	{
		std::string q = "delete from users where id = " + std::to_string(id) + ";";
		std::wstring query(q.begin(), q.end());

		SQLCloseCursor(sqlStmtHandle);
		SQLExecDirect(sqlStmtHandle, (SQLWCHAR*)query.c_str(), SQL_NTS);
	
		return true;
	}
	return false;
}	

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Chat::ReadGeneralMessages(std::string _email, SOCKET& clientSocket)
{
	std::string q = "call check_general_messages(" + std::to_string(Identification(_email)) + ");";
	std::wstring query(q.begin(), q.end());

	SQLCloseCursor(sqlStmtHandle);
	SQLExecDirect(sqlStmtHandle, (SQLWCHAR*)query.c_str(), SQL_NTS);

	SQLSMALLINT status;
	SQLCHAR email[200], msg[200], date[200];

	SQLBindCol(sqlStmtHandle, 1, SQL_CHAR, &email, SQL_RESULT_LEN, nullptr);
	SQLBindCol(sqlStmtHandle, 2, SQL_CHAR, &msg, SQL_RESULT_LEN, nullptr);
	SQLBindCol(sqlStmtHandle, 3, SQL_CHAR, &date, SQL_RESULT_LEN, nullptr);
	

	int iSendResult;
	char message[MESSAGE_LENGTH];

	memset(message, 0, sizeof(message));

	strcpy(message, "$");

	iSendResult = send(clientSocket, message, sizeof(message), 0);

	//std::string tmp;
	while (SQLFetch(sqlStmtHandle) != SQL_NO_DATA)
	{
		stringstream tmp;
		tmp << "| " << setw(30) << reinterpret_cast<char*>(email) << " | " << setw(50) << reinterpret_cast<char*>(msg) << " | " << setw(10) << reinterpret_cast<char*>(date) << " |";

		strcpy(message, tmp.str().c_str());
		iSendResult = send(clientSocket, message, sizeof(message), 0);
	}

	strcpy(message, "$");
	iSendResult = send(clientSocket, message, sizeof(message), 0);

}

void Chat::ReadPrivateMessages(std::string email, SOCKET& clientSocket)
{
	std::string q = "call check_private_messages(" + std::to_string(Identification(email)) + ");";
	/*string ID = std::to_string(Identification(email));
	std::string q = "select u1.email, u2.email, m.text, m.sent_at, m.status from messages as m join users as u1 on u1.id = m.sender_id join users as u2 on u2.id = m.recepient_id where m.recepient_id != 0 and (u1.id = " + ID + " or u2.id = " + ID + ");";
	*/
	std::wstring query(q.begin(), q.end());

	SQLCloseCursor(sqlStmtHandle);
	SQLExecDirect(sqlStmtHandle, (SQLWCHAR*)query.c_str(), SQL_NTS);

	SQLCHAR sender_email[200], recepient_email[200], text[200], date[200];
	SQLSMALLINT status;

	SQLBindCol(sqlStmtHandle, 1, SQL_CHAR, &sender_email, SQL_RESULT_LEN, nullptr);
	SQLBindCol(sqlStmtHandle, 2, SQL_CHAR, &recepient_email, SQL_RESULT_LEN, nullptr);
	SQLBindCol(sqlStmtHandle, 3, SQL_CHAR, &text, SQL_RESULT_LEN, nullptr);
	SQLBindCol(sqlStmtHandle, 4, SQL_CHAR, &date, SQL_RESULT_LEN, nullptr);
	SQLBindCol(sqlStmtHandle, 5, SQL_SMALLINT, &status, sizeof(status), nullptr);

	int iSendResult;
	char message[MESSAGE_LENGTH];

	memset(message, 0, sizeof(message));
	strcpy(message, "$");

	iSendResult = send(clientSocket, message, sizeof(message), 0);

	while (SQLFetch(sqlStmtHandle) != SQL_NO_DATA)
	{
		stringstream tmp;
		tmp << "| " << setw(30) << reinterpret_cast<char*>(sender_email) << " | " << setw(30) << reinterpret_cast<char*>(recepient_email) << " | " << setw(30) << reinterpret_cast<char*>(text) << " | " << setw(10) << reinterpret_cast<char*>(date) << " | " << status << " |";
		
		strcpy(message, tmp.str().c_str());
		iSendResult = send(clientSocket, message, sizeof(message), 0);
	}

	strcpy(message, "$");
	iSendResult = send(clientSocket, message, sizeof(message), 0);
}

void Chat::ShowAllUsers(SOCKET& clientSocket)
{
	std::string q = "select * from check_users_list;";
	std::wstring query(q.begin(), q.end());

	SQLCloseCursor(sqlStmtHandle);
	SQLExecDirect(sqlStmtHandle, (SQLWCHAR*)query.c_str(), SQL_NTS);

	SQLCHAR second_name[200], name[200], email[200], status[10];
	SQLBindCol(sqlStmtHandle, 1, SQL_CHAR, &second_name, SQL_RESULT_LEN, nullptr);
	SQLBindCol(sqlStmtHandle, 2, SQL_CHAR, &name, SQL_RESULT_LEN, nullptr);
	SQLBindCol(sqlStmtHandle, 3, SQL_CHAR, &email, SQL_RESULT_LEN, nullptr);
	SQLBindCol(sqlStmtHandle, 4, SQL_CHAR, &status, SQL_RESULT_LEN, nullptr);

	int iSendResult;
	char message[MESSAGE_LENGTH];

	memset(message, 0, sizeof(message));
	strcpy(message, "$");

	iSendResult = send(clientSocket, message, sizeof(message), 0);

	while (SQLFetch(sqlStmtHandle) != SQL_NO_DATA)
	{
		stringstream tmp;
		tmp << "| " << setw(30) << reinterpret_cast<char*>(second_name) << " | " << setw(30) << reinterpret_cast<char*>(name) << " | " << setw(30) << reinterpret_cast<char*>(email) << " | " << setw(7) << reinterpret_cast<char*>(status) << " |";

		strcpy(message, tmp.str().c_str());
		iSendResult = send(clientSocket, message, sizeof(message), 0);
	}

	strcpy(message, "$");
	iSendResult = send(clientSocket, message, sizeof(message), 0);

}

void Chat::ShowUsersOnline(SOCKET& clientSocket)
{
	std::string q = "select * from check_users_online_list";
	std::wstring query(q.begin(), q.end());

	SQLCloseCursor(sqlStmtHandle);
	SQLExecDirect(sqlStmtHandle, (SQLWCHAR*)query.c_str(), SQL_NTS);

	SQLCHAR second_name[200], name[200], email[200], status[10];
	SQLBindCol(sqlStmtHandle, 1, SQL_CHAR, &second_name, SQL_RESULT_LEN, nullptr);
	SQLBindCol(sqlStmtHandle, 2, SQL_CHAR, &name, SQL_RESULT_LEN, nullptr);
	SQLBindCol(sqlStmtHandle, 3, SQL_CHAR, &email, SQL_RESULT_LEN, nullptr);
	SQLBindCol(sqlStmtHandle, 4, SQL_CHAR, &status, SQL_RESULT_LEN, nullptr);

	int iSendResult;
	char message[MESSAGE_LENGTH];

	memset(message, 0, sizeof(message));
	strcpy(message, "$");

	iSendResult = send(clientSocket, message, sizeof(message), 0);

	while (SQLFetch(sqlStmtHandle) != SQL_NO_DATA)
	{
		stringstream tmp;
		tmp << "| " << setw(30) << reinterpret_cast<char*>(second_name) << " | " << setw(30) << reinterpret_cast<char*>(name) << " | " << setw(30) << reinterpret_cast<char*>(email) << " | " << setw(7) << reinterpret_cast<char*>(status) << " |";

		strcpy(message, tmp.str().c_str());
		iSendResult = send(clientSocket, message, sizeof(message), 0);
	}

	strcpy(message, "$");
	iSendResult = send(clientSocket, message, sizeof(message), 0);

}

std::string Chat::getLastLogInput()
{
	
	std::string tmp = logs.getLastLogString();

	return tmp;
}

void Chat::createDatabase()
{
	cout << "Enter db name: " << endl;
	string t;
	cin >> t;
	string s = "create database " + t;
	wstring query(s.begin(), s.end());
	SQLExecDirect(sqlStmtHandle, (SQLWCHAR*)s.c_str(), SQL_NTS);
	cout << "Database created " << endl;

}