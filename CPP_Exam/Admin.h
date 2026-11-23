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
        static int chooseId(const char* sql, int parentId, const char* title)
        {
            std::vector<int> ids;
            std::vector<std::string> options;
            sqlite3_stmt* stmt = nullptr;

            if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
            {
                std::cerr << "Database error: " << sqlite3_errmsg(db) << '\n';
                system("pause");
                return -1;
            }

            if (parentId >= 0)
                sqlite3_bind_int(stmt, 1, parentId);

            int result;
            while ((result = sqlite3_step(stmt)) == SQLITE_ROW)
            {
                ids.push_back(sqlite3_column_int(stmt, 0));

                const auto* text = sqlite3_column_text(stmt, 1);
                options.push_back(text
                    ? reinterpret_cast<const char*>(text)
                    : "");
            }

            sqlite3_finalize(stmt);

            if (result != SQLITE_DONE)
            {
                std::cerr << "Database error: " << sqlite3_errmsg(db) << '\n';
                system("pause");
                return -1;
            }

            const bool empty = ids.empty();
            if (empty)
                options.push_back("(Nothing to show)");

            options.push_back("Back");

            const int choice = ConsoleMenu::Menu::verticalMenu(
                title, options, ConsoleMenu::HorizontalAlignment::Center);

            if (choice < 0 || choice == static_cast<int>(options.size()) - 1 || empty)
                return -1;

            return ids[choice];
        }

        static void showReport(const char* sql, int id, const char* title)
        {
            sqlite3_stmt* stmt = nullptr;

            if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
            {
                std::cerr << "Database error: " << sqlite3_errmsg(db) << '\n';
                system("pause");
                return;
            }

            sqlite3_bind_int(stmt, 1, id);

            system("cls");
            std::cout << title << "\n\n";

            const int result = sqlite3_step(stmt);
            if (result == SQLITE_ROW)
            {
                const int columns = sqlite3_column_count(stmt);

                for (int i = 0; i < columns; ++i)
                {
                    const auto* name = sqlite3_column_name(stmt, i);
                    const auto* value = sqlite3_column_text(stmt, i);

                    std::cout << (name ? name : "") << ": "
                        << (value
                            ? reinterpret_cast<const char*>(value)
                            : "0")
                        << '\n';
                }
            }
            else if (result == SQLITE_DONE)
            {
                std::cout << "No completed attempts yet.\n";
            }
            else
            {
                std::cerr << "Database error: " << sqlite3_errmsg(db) << '\n';
            }

            sqlite3_finalize(stmt);
            system("pause");
        }

        static void byCategory()
        {
            const char* sql =
                "SELECT COUNT(*), "
                "ROUND(AVG(grade), 2), "
                "ROUND(AVG(100.0 * correct_count / NULLIF(question_count, 0)), 2) "
                "FROM attempts "
                "JOIN tests ON tests.id = attempts.test_id "
                "WHERE tests.category_id = ? AND attempts.status = 'completed';";

            while (true)
            {
                const int categoryId = chooseId(
                    "SELECT id, name FROM categories ORDER BY name;",
                    -1,
                    "   CATEGORY STATISTICS   ");

                if (categoryId < 0)
                    return;

                showReport(sql, categoryId, "CATEGORY STATISTICS");
            }
        }

        static void byTest()
        {
            while (true)
            {
                const int categoryId = chooseId(
                    "SELECT id, name FROM categories ORDER BY name;",
                    -1,
                    "   CHOOSE CATEGORY   ");

                if (categoryId < 0)
                    return;

                while (true)
                {
                    const int testId = chooseId(
                        "SELECT id, title FROM tests "
                        "WHERE category_id = ? ORDER BY title;",
                        categoryId,
                        "   CHOOSE TEST   ");

                    if (testId < 0)
                        break; // Go back to the category list.

                    showReport(
                        "SELECT COUNT(*), "
                        "ROUND(AVG(grade), 2), "
                        "ROUND(AVG(100.0 * correct_count / NULLIF(question_count, 0)), 2) "
                        "FROM attempts "
                        "WHERE test_id = ? AND status = 'completed';",
                        testId,
                        "TEST STATISTICS");
                }
            }
        }

    public:
        static void Menu()
        {
            const std::vector<std::string> options = {
                "By category",
                "By test",
                "By user",
                "Back"
            };

            while (true)
            {
                const int choice = ConsoleMenu::Menu::verticalMenu(
                    "   STATISTICS   ",
                    options,
                    ConsoleMenu::HorizontalAlignment::Center);

                switch (choice)
                {
                case 0:
                    byCategory();
                    break;
                case 1:
                    byTest();
                    break;
                case 2:
                    // Add the user report here.
                    break;
                default:
                    return;
                }
            }
        }
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
