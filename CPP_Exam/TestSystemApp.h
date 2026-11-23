#pragma once
#include <vector>
#include "ConsoleMenu.h"
#include "Settings.h"
#include "Admin.h"
#include "User.h"
#include "Func.h"

namespace test
{
    struct LoginResult
    {
        enum class Role { failed, admin, user } role;
        int userId = -1;
    };

    LoginResult login(sqlite3* db);

    bool signUp(sqlite3* db);

    class TestSystemApp
    {
    public:
	    int menu()
	    {
		    Func::SetColor(Settings::textColorConsole, Settings::backColorConsole);
		    system("cls");
		    sqlite3* database = nullptr;
		    if (sqlite3_open("testing.db", &database) != SQLITE_OK)
			    return 1;
            Admin::db = database;
            User::db = database;

		    std::vector<std::string> options = { "Log In", "Sign Up", "Settings", "Exit" };
		    while (true)
		    {
			    int choice = ConsoleMenu::Menu::verticalMenu("   MAIN  MENU   ", options, ConsoleMenu::HorizontalAlignment::Center);
			    switch (choice)
			    {
			    case 0:
                {
                    const LoginResult result = login(database);

                    if (result.role == LoginResult::Role::admin)
                    {
                        Admin::Admin::Menu();
                    }
                    else if (result.role == LoginResult::Role::user)
                    {
                        User::User::setId(result.userId);
                        User::User::Menu();
                    }
                    break;
                }
                case 1:
                    signUp(database);
                    break;
			    case 2:
				    ConsoleMenu::Menu::consoleSettings();
				    break;
			    case 3:
			    default:
                    sqlite3_close(database);
				    exit(0);
				    break;
			    }
		    }

		    return 0;
	    }
    };
}