#include "Admin.h"

namespace Admin
{
    bool createTable()
    {
        const char* sql =
            "CREATE TABLE IF NOT EXISTS admin ("
            "id INTEGER PRIMARY KEY CHECK (id = 1),"
            "login TEXT NOT NULL,"
            "password_hash TEXT NOT NULL"
            ");";

        char* error = nullptr;
        const int result = sqlite3_exec(db, sql, nullptr, nullptr, &error);

        if (result != SQLITE_OK)
        {
            std::cerr << "Database error: " << error << '\n';
            sqlite3_free(error);
            return false;
        }

        return true;
    }

    bool Users::chooseUserId(int& userId)
    {
        std::vector<int> userIds;
        std::vector<std::string> options;
    
        sqlite3_stmt* stmt = nullptr;
        const char* sql =
            "SELECT id, login, full_name "
            "FROM users ORDER BY full_name;";
    
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        {
            std::cerr << "Database error: " << sqlite3_errmsg(db) << '\n';
            return false;
        }
    
        int result;
        while ((result = sqlite3_step(stmt)) == SQLITE_ROW)
        {
            userIds.push_back(sqlite3_column_int(stmt, 0));
    
            const auto* loginText = sqlite3_column_text(stmt, 1);
            const auto* nameText = sqlite3_column_text(stmt, 2);
    
            const std::string login =
                loginText ? reinterpret_cast<const char*>(loginText) : "";
            const std::string fullName =
                nameText ? reinterpret_cast<const char*>(nameText) : "";
    
            options.push_back(fullName + "  (" + login + ")");
        }
    
        sqlite3_finalize(stmt);
    
        if (result != SQLITE_DONE)
        {
            std::cerr << "Database error: " << sqlite3_errmsg(db) << '\n';
            return false;
        }
    
        const bool noUsers = userIds.empty();
    
        if (noUsers)
            options.push_back("(No users registered)");
    
        options.push_back("Back");
    
        const int choice = ConsoleMenu::Menu::verticalMenu(
            "   USERS   ",
            options,
            ConsoleMenu::HorizontalAlignment::Center);
    
        // Esc or Back cancels the selection.
        if (choice < 0 || choice == static_cast<int>(options.size()) - 1)
            return false;
    
        if (noUsers)
            return false;
    
        userId = userIds[choice];
        return true;
    }
    
    void Users::show()
    {
        while (true)
        {
            std::vector<int> userIds;
            std::vector<std::string> options;
    
            sqlite3_stmt* stmt = nullptr;
            const char* sql =
                "SELECT id, login, full_name "
                "FROM users ORDER BY full_name;";
    
            if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
            {
                std::cerr << "Database error: " << sqlite3_errmsg(db) << '\n';
                return;
            }
    
            int result;
            while ((result = sqlite3_step(stmt)) == SQLITE_ROW)
            {
                const int id = sqlite3_column_int(stmt, 0);
                const auto* loginText = sqlite3_column_text(stmt, 1);
                const auto* nameText = sqlite3_column_text(stmt, 2);
    
                const std::string login =
                    loginText ? reinterpret_cast<const char*>(loginText) : "";
                const std::string fullName =
                    nameText ? reinterpret_cast<const char*>(nameText) : "";
    
                userIds.push_back(id);
                options.push_back(fullName + "  (" + login + ")");
            }
    
            sqlite3_finalize(stmt);
    
            if (result != SQLITE_DONE)
            {
                std::cerr << "Database error: " << sqlite3_errmsg(db) << '\n';
                return;
            }
    
            const bool noUsers = userIds.empty();
            if (noUsers)
                options.push_back("(No users registered)");
    
            options.push_back("Back");
    
            const int choice = ConsoleMenu::Menu::verticalMenu(
                "   USERS   ",
                options,
                ConsoleMenu::HorizontalAlignment::Center);
    
            // Esc or Back returns to the admin menu.
            if (choice < 0 || choice == static_cast<int>(options.size()) - 1)
                return;
    
            if (noUsers)
                continue;
    
            const int selectedUserId = userIds[choice];
    
            if (sqlite3_prepare_v2(db,
                "SELECT full_name, login, home_address, phone "
                "FROM users WHERE id = ?;",
                -1, &stmt, nullptr) != SQLITE_OK)
            {
                std::cerr << "Database error: " << sqlite3_errmsg(db) << '\n';
                return;
            }
    
            sqlite3_bind_int(stmt, 1, selectedUserId);
    
            if (sqlite3_step(stmt) == SQLITE_ROW)
            {
                auto columnText = [stmt](int column)
                    {
                        const auto* value = sqlite3_column_text(stmt, column);
                        return value
                            ? std::string(reinterpret_cast<const char*>(value))
                            : std::string{};
                    };
    
                const std::string fullName = columnText(0);
                const std::string login = columnText(1);
                const std::string address = columnText(2);
                const std::string phone = columnText(3);
                std::vector<std::string> data = { fullName, login, address, phone };
                std::vector<std::string> text = { "Name    : ", "Login   : ", "Address : ", "Phone   : " };
    
                int x = (Settings::widthConsole - 30) / 2;
                int y = (Settings::heightConsole - data.size() - 2) / 2;
                ConsoleMenu::inputFrame(40, 10, x, y, "         USER DETAILS");
    
                for (size_t i = 0; i < data.size(); i++)
                {
                    Func::SetCursor(x, y + i + 2);
                    std::cout << text[i] << data[i];
                }
                Func::SetCursor(x, y + data.size() + 3);
                std::cout << "Press any key to return...";
                _getch();
            }
            else
            {
                std::cerr << "Could not load that user: "
                    << sqlite3_errmsg(db) << '\n';
                system("pause");
            }
    
            sqlite3_finalize(stmt);
        }
    }
    
    void Users::add()
    {
        int x = (Settings::widthConsole - 40) / 2;
        int y = (Settings::heightConsole - 10) / 2;
        ConsoleMenu::inputFrame(50, 13, x, y, "               ADD USER");
        std::vector<std::string> items = { "Full name        : ", "Home address     : ", "Phone            : ", "Login            : ", "Password         : ", "Confirm password : " };
        for (size_t i = 0; i < items.size(); i++)
        {
            Func::SetCursor(x, y + i + 2);
            std::cout << items[i];
        }
    
        std::string fullName, address, phone, login, password, confirmation;
        std::vector<std::string*> info = { &fullName, &address, &phone, &login };
        for (size_t i = 0; i < info.size(); i++)
        {
            Func::SetCursor(x + 19, y + 2 + i);
            if (Func::readConsoleInput(*info[i]) == Func::InputResult::cancelled)
            {
                system("cls");
                return;
            }
        }
        do
        {
            std::vector<std::string*> pass = { &password, &confirmation };
            for (size_t i = 0; i < pass.size(); i++)
            {
                Func::SetCursor(x + 19, y + 2 + i + info.size());
                std::cout << "                   ";
            }
            for (size_t i = 0; i < pass.size(); i++)
            {
                Func::SetCursor(x + 19, y + 2 + i + info.size());
                if (Func::readConsoleInput(*pass[i], true) == Func::InputResult::cancelled)
                {
                    system("cls");
                    return;
                }
            }
    
            if (password != confirmation)
            {
                Func::SetCursor(x, y + 5 + info.size());
                std::cout << "Passwords do not match. Try again.";
                Func::SetCursor(x, y + 6 + info.size());
            }
            else
            {
                Func::SetCursor(x, y + 5 + info.size());
                std::cout << "                                  ";
                Func::SetCursor(x, y + 6 + info.size());
                break;
            }
        } while (true);
    
        if (fullName.empty() || address.empty() || phone.empty() ||
            login.empty() || password.empty())
        {
            Func::SetCursor(x, y + 5 + info.size());
            std::cout << "Fields cannot be empty.";
            Func::SetCursor(x, y + 6 + info.size());
            system("pause");
            return;
        }
    
        if (password != confirmation)
        {
            Func::SetCursor(x, y + 5 + info.size());
            std::cout << "Passwords do not match.";
            Func::SetCursor(x, y + 6 + info.size());
            system("pause");
            return;
        }
    
        sqlite3_stmt* stmt = nullptr;
        const char* sql =
            "INSERT INTO users "
            "(login, password_hash, full_name, home_address, phone) "
            "VALUES (?, ?, ?, ?, ?);";
    
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        {
            std::cerr << "Database error: " << sqlite3_errmsg(db) << '\n';
            return;
        }
    
        const std::string passwordHash = md5(password);
    
        sqlite3_bind_text(stmt, 1, login.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, passwordHash.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, fullName.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 4, address.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 5, phone.c_str(), -1, SQLITE_TRANSIENT);
    
        const int result = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    
        if (result == SQLITE_DONE)
        {
            Func::SetCursor(x, y + 5 + info.size());
            std::cout << "User created.";
        }
        else if (result == SQLITE_CONSTRAINT)
        {
            Func::SetCursor(x, y + 5 + info.size());
            std::cout << "That login is already taken.";
        }
        else
            std::cerr << "Database error: " << sqlite3_errmsg(db) << '\n';
        Func::SetCursor(x, y + 6 + info.size());
        system("pause");
    }
    
    void Users::edit()
    {
        int userId;
        if (!chooseUserId(userId))
            return;
    
        sqlite3_stmt* stmt = nullptr;
        const char* selectSql =
            "SELECT login, full_name, home_address, phone "
            "FROM users WHERE id = ?;";
    
        if (sqlite3_prepare_v2(db, selectSql, -1, &stmt, nullptr) != SQLITE_OK)
        {
            std::cerr << "Database error: " << sqlite3_errmsg(db) << '\n';
            return;
        }
    
        sqlite3_bind_int(stmt, 1, userId);
    
        if (sqlite3_step(stmt) != SQLITE_ROW)
        {
            sqlite3_finalize(stmt);
            std::cout << "User not found.";
            system("pause");
            return;
        }
    
        auto readColumn = [stmt](int column)
            {
                const auto* value = sqlite3_column_text(stmt, column);
                return value ? std::string(reinterpret_cast<const char*>(value))
                    : std::string{};
            };
    
        std::string login = readColumn(0);
        std::string fullName = readColumn(1);
        std::string address = readColumn(2);
        std::string phone = readColumn(3);
        sqlite3_finalize(stmt);
    
        std::string newLogin, newName, newAddress, newPhone;
    
        std::cout << "New login (Enter keeps " << login << "): ";
        std::getline(std::cin, newLogin);
    
        std::cout << "New full name (Enter keeps " << fullName << "): ";
        std::getline(std::cin, newName);
    
        std::cout << "New address (Enter keeps current): ";
        std::getline(std::cin, newAddress);
    
        std::cout << "New phone (Enter keeps " << phone << "): ";
        std::getline(std::cin, newPhone);
    
        if (!newLogin.empty()) login = newLogin;
        if (!newName.empty()) fullName = newName;
        if (!newAddress.empty()) address = newAddress;
        if (!newPhone.empty()) phone = newPhone;
    
        std::string newPassword, passwordHash;
        std::cout << "New password (Enter keeps current): ";
        //newPassword = readMaskedPassword();
    
        if (!newPassword.empty())
        {
            std::cout << "Confirm new password: ";
            //const std::string confirmation = readMaskedPassword();
    
            //if (newPassword != confirmation)
            {
                std::cout << "Passwords do not match.\n";
                system("pause");
                return;
            }
    
            passwordHash = md5(newPassword);
        }
    
        const char* updateSql =
            "UPDATE users SET login = ?, full_name = ?, home_address = ?, "
            "phone = ?, password_hash = COALESCE(?, password_hash) WHERE id = ?;";
    
        if (sqlite3_prepare_v2(db, updateSql, -1, &stmt, nullptr) != SQLITE_OK)
        {
            std::cerr << "Database error: " << sqlite3_errmsg(db) << '\n';
            return;
        }
    
        sqlite3_bind_text(stmt, 1, login.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, fullName.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, address.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 4, phone.c_str(), -1, SQLITE_TRANSIENT);
    
        if (passwordHash.empty())
            sqlite3_bind_null(stmt, 5);
        else
            sqlite3_bind_text(stmt, 5, passwordHash.c_str(), -1, SQLITE_TRANSIENT);
    
        sqlite3_bind_int(stmt, 6, userId);
    
        const int result = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    
        if (result == SQLITE_DONE)
            std::cout << "User updated.\n";
        else if (result == SQLITE_CONSTRAINT)
            std::cout << "That login is already taken.\n";
        else
            std::cerr << "Database error: " << sqlite3_errmsg(db) << '\n';
    
        system("pause");
    }
    
    void Users::del()
    {
        while (true)
        {
            int userId;
            if (!chooseUserId(userId))
                return; // Go back to Admin Menu

            while (true)
            {
                sqlite3_stmt* stmt = nullptr;

                if (sqlite3_prepare_v2(db,
                    "SELECT COUNT(*) FROM attempts WHERE user_id = ?;",
                    -1, &stmt, nullptr) != SQLITE_OK)
                {
                    std::cerr << "Database error: " << sqlite3_errmsg(db) << '\n';
                    system("pause");
                    return;
                }

                sqlite3_bind_int(stmt, 1, userId);

                const int countResult = sqlite3_step(stmt);
                if (countResult != SQLITE_ROW)
                {
                    sqlite3_finalize(stmt);
                    std::cerr << "Could not check this user's test history: "
                        << sqlite3_errmsg(db) << '\n';
                    system("pause");
                    return;
                }

                const int attemptCount = sqlite3_column_int(stmt, 0);
                sqlite3_finalize(stmt);

                if (attemptCount > 0)
                {
                    std::cout << "This user has " << attemptCount
                        << " test attempt(s), so the account cannot be deleted "
                        "without removing their history.\n";
                    system("pause");
                    return;
                }

                // Load the account details so the confirmation names the right user.
                stmt = nullptr;
                if (sqlite3_prepare_v2(db,
                    "SELECT login, full_name FROM users WHERE id = ?;",
                    -1, &stmt, nullptr) != SQLITE_OK)
                {
                    std::cerr << "Database error: " << sqlite3_errmsg(db) << '\n';
                    system("pause");
                    return;
                }

                sqlite3_bind_int(stmt, 1, userId);

                const int userResult = sqlite3_step(stmt);
                if (userResult != SQLITE_ROW)
                {
                    sqlite3_finalize(stmt);

                    if (userResult == SQLITE_DONE)
                        std::cout << "User not found.\n";
                    else
                        std::cerr << "Database error: " << sqlite3_errmsg(db) << '\n';

                    system("pause");
                    return;
                }

                const auto* loginText = sqlite3_column_text(stmt, 0);
                const auto* nameText = sqlite3_column_text(stmt, 1);

                const std::string login = loginText
                    ? reinterpret_cast<const char*>(loginText) : "";
                const std::string fullName = nameText
                    ? reinterpret_cast<const char*>(nameText) : "";

                sqlite3_finalize(stmt);

                std::vector<std::string> options = { "Yes, delete", "No, go back" };
                int choice = ConsoleMenu::Menu::verticalMenu(" CONFIRM " + fullName + " DELETE ", options, ConsoleMenu::HorizontalAlignment::Center);
                if (choice != 0)
                {
                    std::cout << "Deletion cancelled.\n";
                    system("pause");
                    break; // Leave this user and return to the delete-user list.
                }

                // Delete only after the history check and confirmation.
                stmt = nullptr;
                if (sqlite3_prepare_v2(db,
                    "DELETE FROM users WHERE id = ?;",
                    -1, &stmt, nullptr) != SQLITE_OK)
                {
                    std::cerr << "Database error: " << sqlite3_errmsg(db) << '\n';
                    system("pause");
                    return;
                }

                sqlite3_bind_int(stmt, 1, userId);

                const int deleteResult = sqlite3_step(stmt);
                const int changedRows = sqlite3_changes(db);
                const std::string error = sqlite3_errmsg(db);

                sqlite3_finalize(stmt);

                if (deleteResult == SQLITE_DONE && changedRows == 1)
                    std::cout << "User deleted.\n";
                else if (deleteResult == SQLITE_DONE)
                    std::cout << "User not found.\n";
                else
                    std::cerr << "Could not delete user: " << error << '\n';

                system("pause");
                break;
            }
        }
    }

    void Users::Menu()
    {
        std::vector<std::string> options = { "Show All", "Add User", "Modify User", "Delete User", "Back" };
        while (true)
        {
            int choice = ConsoleMenu::Menu::verticalMenu("   USERS MENU   ", options, ConsoleMenu::HorizontalAlignment::Center);
            switch (choice)
            {
            case 0:
                show();
                break;
            case 1:
                add();
                break;
            case 2:
                edit();
                break;
            case 3:
                del();
                break;
            case 4:
            default:
                return;
            }
        }
    }
}