#pragma once
#include <iostream>
#include <conio.h>
#include <string>
#include "sqlite3.h"
#include "md5.h"
#include "ConsoleMenu.h"
#include "Func.h"
#include "Tests.h"

namespace Admin
{
    bool createTable();

    class Users
    {
        static bool chooseUserId(int& userId);

        static void show();

        static void add();

        static void edit();

        static void del();

    public:
        static void Menu();
    };

    class Stats
    {
        static int chooseId(const char* sql, int parentId, const char* title);

        static void showReport(const char* sql, int id, const char* title);

        static void byCategory();

        static void byTest();

        static void byUser();

    public:
        static void Menu();
    };

    class Admin
    {
        std::string login;
        std::string passwordHash;

        static void changeCredentials()
        {
            sqlite3_stmt* stmt = nullptr;
            if (sqlite3_prepare_v2(db,
                "SELECT login FROM admin WHERE id = 1;",
                -1, &stmt, nullptr) != SQLITE_OK)
            {
                std::cerr << "Database error: " << sqlite3_errmsg(db) << '\n';
                system("pause");
                return;
            }

            if (sqlite3_step(stmt) != SQLITE_ROW)
            {
                std::cerr << "Could not load the admin account: "
                    << sqlite3_errmsg(db) << '\n';
                sqlite3_finalize(stmt);
                system("pause");
                return;
            }

            const auto* savedLoginText = sqlite3_column_text(stmt, 0);
            const std::string currentLogin = savedLoginText
                ? reinterpret_cast<const char*>(savedLoginText) : "";
            sqlite3_finalize(stmt);

            int x = (Settings::widthConsole - 40) / 2;
            int y = (Settings::heightConsole - 10) / 2;
            ConsoleMenu::inputFrame(50, 12, x, y, "              EDIT ADMIN");
            Func::SetCursor(x, y + 2);
            std::cout << "Note: Enter keeps current value.";

            std::vector<std::string> items = { "Current login : ", "New login            : ", "New password         : ", "Confirm new password : " };
            for (size_t i = 0; i < items.size(); i++)
            {
                Func::SetCursor(x, y + i + 3);
                std::cout << items[i];
            }
            Func::SetCursor(x + 16, y + 3);
            std::cout << currentLogin;

            std::string newLogin, newPassword, confirmation, password;
            Func::SetCursor(x + 23, y + 4);
            if (Func::readConsoleInput(newLogin) == Func::InputResult::cancelled)
                return;
            if (newLogin.empty())
                newLogin = currentLogin;
            do
            {
                std::vector<std::string*> pass = { &newPassword, &confirmation };
                for (size_t i = 0; i < pass.size(); i++)
                {
                    Func::SetCursor(x + 23, y + 5 + i);
                    std::cout << "                   ";
                }
                for (size_t i = 0; i < pass.size(); i++)
                {
                    Func::SetCursor(x + 23, y + 5 + i);
                    if (Func::readConsoleInput(*pass[i], true) == Func::InputResult::cancelled)
                    {
                        system("cls");
                        return;
                    }
                }

                if (newPassword != confirmation)
                {
                    Func::SetCursor(x, y + 8);
                    std::cout << "Passwords do not match. Try again.";
                    Func::SetCursor(x, y + 9);
                }
                else
                {
                    Func::SetCursor(x, y + 8);
                    std::cout << "                                  ";
                    Func::SetCursor(x, y + 9);
                    break;
                }
            } while (true);
            if (!newPassword.empty()) password = md5(newPassword);

            if (newLogin != currentLogin)
            {
                if (sqlite3_prepare_v2(db,
                    "SELECT 1 FROM users WHERE login = ? LIMIT 1;",
                    -1, &stmt, nullptr) != SQLITE_OK)
                {
                    std::cerr << "Database error: " << sqlite3_errmsg(db) << '\n';
                    Func::SetCursor(x, y + 9);
                    system("pause");
                    return;
                }
                sqlite3_bind_text(stmt, 1, newLogin.c_str(), -1, SQLITE_TRANSIENT);
                const int userResult = sqlite3_step(stmt);
                sqlite3_finalize(stmt);

                if (userResult == SQLITE_ROW)
                {
                    std::cout << "That login is already used by a guest.\n";
                    Func::SetCursor(x, y + 9);
                    system("pause");
                    return;
                }
                if (userResult != SQLITE_DONE)
                {
                    std::cerr << "Database error: " << sqlite3_errmsg(db) << '\n';
                    system("pause");
                    return;
                }
            }

            const char* sql = password.empty()
                ? "UPDATE admin SET login = ? WHERE id = 1;"
                : "UPDATE admin SET login = ?, password_hash = ? WHERE id = 1;";
            if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
            {
                std::cerr << "Database error: " << sqlite3_errmsg(db) << '\n';
                system("pause");
                return;
            }

            sqlite3_bind_text(stmt, 1, newLogin.c_str(), -1, SQLITE_TRANSIENT);
            if (!password.empty())
                sqlite3_bind_text(stmt, 2, password.c_str(), -1, SQLITE_TRANSIENT);

            const int result = sqlite3_step(stmt);
            const int changedRows = sqlite3_changes(db);
            const std::string error = sqlite3_errmsg(db);
            sqlite3_finalize(stmt);

            if (result == SQLITE_DONE && changedRows == 1)
                std::cout << "Admin credentials updated.";
            else if (result == SQLITE_DONE)
                std::cout << "Admin account was not changed.";
            else
                std::cerr << "Could not update admin credentials: " << error << '\n';

            Func::SetCursor(x, y + 9);
            system("pause");
        }

    public:
        Admin() = default;
        Admin(const std::string& login, const std::string& password) : login(login), passwordHash(md5(password)) {}

        bool save() const
        {
            sqlite3_stmt* stmt = nullptr;

            if (sqlite3_prepare_v2(db,
                "INSERT INTO admin (id, login, password_hash) VALUES (1, ?, ?);",
                -1, &stmt, nullptr) != SQLITE_OK)
            {
                std::cerr << "Database error: " << sqlite3_errmsg(db) << '\n';
                return false;
            }

            sqlite3_bind_text(stmt, 1, login.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 2, passwordHash.c_str(), -1, SQLITE_TRANSIENT);

            const bool success = sqlite3_step(stmt) == SQLITE_DONE;
            if (!success)
                std::cerr << "Database error: " << sqlite3_errmsg(db) << '\n';

            sqlite3_finalize(stmt);
            return success;
        }

        static void Menu()
        {
            std::vector<std::string> options = { "Tests", "Users", "Statistics", "Change Login/Password", "Log Out" };
            while (true)
            {
                int choice = ConsoleMenu::Menu::verticalMenu("   ADMIN MENU   ", options, ConsoleMenu::HorizontalAlignment::Center);
                switch (choice)
                {
                case 0:
                    Tests::Menu();
                    break;
                case 1:
                    Users::Menu();
                    break;
                case 2:
                    Stats::Menu();
                    break;
                case 3:
                    changeCredentials();
                    break;
                case 4:
                default:
                    return;
                }
            }
        }
    };
}
