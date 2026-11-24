#pragma once
#include <vector>
#include "sqlite3.h"
#include "ConsoleMenu.h"
#include "Settings.h"
#include "Admin.h"
#include "User.h"
#include "Func.h"

namespace test
{
    bool isDatabaseEmpty(sqlite3* db);

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
		    //if (sqlite3_open("TestingSystem.db", &database) != SQLITE_OK)
		    if (sqlite3_open("testing.db", &database) != SQLITE_OK)
			    return 1;
            Admin::db = database;
            User::db = database;

            if (isDatabaseEmpty(database))
            {
                std::vector<std::string> options = { "Create Admin", "Settings", "Exit" };
                bool adminCreated = true;
                while (adminCreated)
                {
                    int choice = ConsoleMenu::Menu::verticalMenu("   MAIN  MENU   ", options, ConsoleMenu::HorizontalAlignment::Center);
                    switch (choice)
                    {
                    case 0:
                    {
                        login(database);
                        adminCreated = false;
                        Admin::Admin::Menu();
                        break;
                    }
                    case 1:
                        ConsoleMenu::Menu::consoleSettings();
                        break;
                    case 2:
                    default:
                        sqlite3_close(database);
                        exit(0);
                        break;
                    }
                }
            }
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
