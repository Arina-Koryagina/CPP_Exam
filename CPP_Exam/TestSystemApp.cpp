#include "TestSystemApp.h"
#include "Func.h"

namespace test
{
    bool createTestTables(sqlite3* db)
    {
        const char* sql = R"SQL(
        PRAGMA foreign_keys = ON;

        CREATE TABLE IF NOT EXISTS categories (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL UNIQUE
        );

        CREATE TABLE IF NOT EXISTS tests (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            category_id INTEGER NOT NULL REFERENCES categories(id),
            title TEXT NOT NULL,
            is_active INTEGER NOT NULL DEFAULT 1
        );

        CREATE TABLE IF NOT EXISTS questions (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            test_id INTEGER NOT NULL REFERENCES tests(id),
            prompt TEXT NOT NULL,
            position INTEGER NOT NULL,
            UNIQUE (test_id, position)
        );

        CREATE TABLE IF NOT EXISTS answers (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            question_id INTEGER NOT NULL REFERENCES questions(id),
            text TEXT NOT NULL,
            is_correct INTEGER NOT NULL CHECK (is_correct IN (0, 1)),
            position INTEGER NOT NULL,
            UNIQUE (question_id, position),
            UNIQUE (id, question_id)
        );

        CREATE TABLE IF NOT EXISTS attempts (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id INTEGER NOT NULL REFERENCES users(id),
            test_id INTEGER NOT NULL REFERENCES tests(id),
            status TEXT NOT NULL DEFAULT 'in_progress'
                CHECK (status IN ('in_progress', 'completed')),
            started_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
            finished_at TEXT,
            correct_count INTEGER,
            question_count INTEGER,
            grade INTEGER CHECK (grade BETWEEN 1 AND 12)
        );

        CREATE TABLE IF NOT EXISTS attempt_answers (
            attempt_id INTEGER NOT NULL REFERENCES attempts(id),
            question_id INTEGER NOT NULL REFERENCES questions(id),
            answer_id INTEGER,
            PRIMARY KEY (attempt_id, question_id),
            FOREIGN KEY (answer_id, question_id)
                REFERENCES answers(id, question_id)
        );
    )SQL";

        char* error = nullptr;

        if (sqlite3_exec(db, sql, nullptr, nullptr, &error) != SQLITE_OK)
        {
            std::cerr << "Database error: " << error << '\n';
            sqlite3_free(error);
            return false;
        }

        return true;
    }

    LoginResult login(sqlite3* db)
    {
        if (!Admin::createTable())
            return { LoginResult::Role::failed, -1 };

        // The sign-up code should use this same table and hash both values.
        const char* usersSql =
            "CREATE TABLE IF NOT EXISTS users ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "login TEXT NOT NULL UNIQUE,"
            "password_hash TEXT NOT NULL,"
            "full_name TEXT NOT NULL,"
            "home_address TEXT NOT NULL,"
            "phone TEXT NOT NULL"
            ");";

        char* error = nullptr;
        if (sqlite3_exec(db, usersSql, nullptr, nullptr, &error) != SQLITE_OK)
        {
            std::cerr << "Database error: " << error << '\n';
            sqlite3_free(error);
            return { LoginResult::Role::failed, -1 };
        }
        if (!createTestTables(db))
            return { LoginResult::Role::failed, -1 };

        int x = (Settings::widthConsole - 30) / 2;
        int y = (Settings::heightConsole - 6) / 2;
        ConsoleMenu::inputFrame(40, 9, x, y, "        SYSTEM  LOG IN");
        Func::SetCursor(x, y + 2);
        std::cout << "Login    : ";
        Func::SetCursor(x, y + 3);
        std::cout << "Password : ";

        std::string loginName;
        Func::SetCursor(x + 11, y + 2);
        if (Func::readConsoleInput(loginName) == Func::InputResult::cancelled)
        {
            system("cls");
            return { LoginResult::Role::failed, -1 };
        }

        std::string password;
        Func::SetCursor(x + 11, y + 3);
        if (Func::readConsoleInput(password, true) == Func::InputResult::cancelled)
        {
            system("cls");
            return { LoginResult::Role::failed, -1 };
        }
        const std::string passwordHash = md5(password);

        // Check whether an admin exists, and distinguish "no row" from an error.
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db,
            "SELECT login, password_hash FROM admin WHERE id = 1;",
            -1, &stmt, nullptr) != SQLITE_OK)
        {
            std::cerr << "Database error: " << sqlite3_errmsg(db) << '\n';
            return { LoginResult::Role::failed, -1 };
        }

        const int result = sqlite3_step(stmt);

        if (result == SQLITE_DONE)
        {
            sqlite3_finalize(stmt);

            // First system login creates the admin account with these credentials.
            Admin::Admin firstAdmin(loginName, password);
            if (firstAdmin.save())
            {
                Func::SetCursor(x, y + 5);
                std::cout << "First login: admin created.";
                Func::SetCursor(x, y + 6); system("pause");
                return { LoginResult::Role::admin, -1 };
            }
            return { LoginResult::Role::failed, -1 };
        }

        if (result != SQLITE_ROW)
        {
            std::cerr << "Database error: " << sqlite3_errmsg(db) << '\n';
            sqlite3_finalize(stmt);
            return { LoginResult::Role::failed, -1 };
        }

        const auto* savedLoginText = sqlite3_column_text(stmt, 0);
        const auto* savedPasswordText = sqlite3_column_text(stmt, 1);
        const std::string savedAdminLogin =
            reinterpret_cast<const char*>(savedLoginText);
        const std::string savedAdminPassword =
            reinterpret_cast<const char*>(savedPasswordText);
        sqlite3_finalize(stmt);

        // If the login is the admin's, a wrong password must not fall through
        // and be checked as a user account.
        if (loginName == savedAdminLogin)
        {
            if (passwordHash == savedAdminPassword)
                return { LoginResult::Role::admin, -1 };

            Func::SetCursor(x, y + 5);
            std::cout << "Wrong login or password.";
            Func::SetCursor(x, y + 6); system("pause");
            return { LoginResult::Role::failed, -1 };
        }

        // Otherwise, try the guest/user accounts.
        if (sqlite3_prepare_v2(db,
            "SELECT id, password_hash FROM users WHERE login = ?;",
            -1, &stmt, nullptr) != SQLITE_OK)
        {
            std::cerr << "Database error: " << sqlite3_errmsg(db) << '\n';
            return { LoginResult::Role::failed, -1 };
        }

        sqlite3_bind_text(stmt, 1, loginName.c_str(), -1, SQLITE_TRANSIENT);

        const int userResult = sqlite3_step(stmt);

        if (userResult == SQLITE_ROW)
        {
            const int userId = sqlite3_column_int(stmt, 0);
            const auto* savedPasswordText = sqlite3_column_text(stmt, 1);

            const std::string savedPasswordHash =
                reinterpret_cast<const char*>(savedPasswordText);

            sqlite3_finalize(stmt);

            if (passwordHash == savedPasswordHash)
                return { LoginResult::Role::user, userId };
        }
        else
        {
            if (userResult != SQLITE_DONE)
                std::cerr << "Database error: " << sqlite3_errmsg(db) << '\n';

            sqlite3_finalize(stmt);
        }

        Func::SetCursor(x, y + 5);
        std::cout << "Wrong login or password.";
        Func::SetCursor(x, y + 6); system("pause");
        return { LoginResult::Role::failed, -1 };
    }

    bool signUp(sqlite3* db)
    {
        const char* createUsersSql =
            "CREATE TABLE IF NOT EXISTS users ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "login TEXT NOT NULL UNIQUE,"
            "password_hash TEXT NOT NULL,"
            "full_name TEXT NOT NULL,"
            "home_address TEXT NOT NULL,"
            "phone TEXT NOT NULL"
            ");";

        char* error = nullptr;
        if (sqlite3_exec(db, createUsersSql, nullptr, nullptr, &error) != SQLITE_OK)
        {
            std::cerr << "Database error: " << error << '\n';
            sqlite3_free(error);
            return false;
        }

        int x = (Settings::widthConsole - 40) / 2;
        int y = (Settings::heightConsole - 10) / 2;
        ConsoleMenu::inputFrame(50, 13, x, y, "               SIGN  UP");
        std::vector<std::string> items = { "Full name        : ", "Home address     : ", "Phone            : ", "Login            : ", "Password         : ", "Confirm password : " };
        for (size_t i = 0; i < items.size(); i++)
        {
            Func::SetCursor(x, y + i + 2);
            std::cout << items[i];
        }

        std::string fullName, address, phone, loginName, password, confirmation;
        std::vector<std::string*> info = { &fullName, &address, &phone, &loginName };
        for (size_t i = 0; i < info.size(); i++)
        {
            Func::SetCursor(x + 19, y + 2 + i);
            if (Func::readConsoleInput(*info[i]) == Func::InputResult::cancelled)
            {
                system("cls");
                return false;
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
                    return false;
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

        sqlite3_stmt* stmt = nullptr;
        const char* insertSql =
            "INSERT INTO users "
            "(login, password_hash, full_name, home_address, phone) "
            "VALUES (?, ?, ?, ?, ?);";

        if (sqlite3_prepare_v2(db, insertSql, -1, &stmt, nullptr) != SQLITE_OK)
        {
            std::cerr << "Database error: " << sqlite3_errmsg(db) << '\n';
            return false;
        }

        const std::string passwordHash = md5(password);

        sqlite3_bind_text(stmt, 1, loginName.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, passwordHash.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, fullName.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 4, address.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 5, phone.c_str(), -1, SQLITE_TRANSIENT);

        const int result = sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        if (result != SQLITE_DONE)
        {
            if (sqlite3_errcode(db) == SQLITE_CONSTRAINT)
            {
                Func::SetCursor(x, y + 9);
                std::cout << "That login is already taken.";
                Func::SetCursor(x, y + 10); system("pause");
            }
            else
                std::cerr << "Database error: " << sqlite3_errmsg(db) << '\n';

            return false;
        }

        Func::SetCursor(x, y + 9);
        std::cout << "Account created. You can now log in.";
        Func::SetCursor(x, y + 10);
        system("pause");
        return true;
    }
}