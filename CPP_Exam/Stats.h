#pragma once
#include"Admin.h"

namespace Admin
{
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
            std::vector<std::string> text = { "Times completed    : ", "Average grade      : ", "Correct percentage : " };
            int x = (Settings::widthConsole - 30) / 2;
            int y = (Settings::heightConsole - text.size() - 3) / 2;
            ConsoleMenu::inputFrame(40, 9, x, y, title);
            for (size_t i = 0; i < text.size(); i++)
            {
                Func::SetCursor(x, y + i + 2);
                std::cout << text[i];
            }

            const int result = sqlite3_step(stmt);
            if (result == SQLITE_ROW)
            {
                const int columns = sqlite3_column_count(stmt);

                for (int i = 0; i < columns; ++i)
                {
                    const auto* name = sqlite3_column_name(stmt, i);
                    const auto* value = sqlite3_column_text(stmt, i);

                    Func::SetCursor(x + 21, y + i + 2);
                    std::cout << (value ? reinterpret_cast<const char*>(value) : "0");
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
            Func::SetCursor(x, y + 3 + text.size());
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

        static void byUser()
        {
            while (true)
            {

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
                    byUser();
                    break;
                case 3:
                default:
                    return;
                }
            }
        }
    };
}
