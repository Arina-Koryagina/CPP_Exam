#pragma once
#include <iostream>
#include <conio.h>
#include <string>
#include "sqlite3.h"
#include "md5.h"
#include "ConsoleMenu.h"
#include "Func.h"

namespace Admin
{
    inline sqlite3* db = nullptr;

    inline bool createTable()
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

    class Tests
    {
        static int chooseCategory()
        {
            std::vector<int> categoryIds;
            std::vector<std::string> options;

            sqlite3_stmt* stmt = nullptr;
            const char* sql =
                "SELECT id, name FROM categories ORDER BY name;";

            if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
            {
                std::cerr << "Database error: " << sqlite3_errmsg(db) << '\n';
                return -1;
            }

            int result;
            while ((result = sqlite3_step(stmt)) == SQLITE_ROW)
            {
                categoryIds.push_back(sqlite3_column_int(stmt, 0));

                const auto* nameText = sqlite3_column_text(stmt, 1);
                options.push_back(
                    nameText ? reinterpret_cast<const char*>(nameText) : "");
            }

            sqlite3_finalize(stmt);

            if (result != SQLITE_DONE)
            {
                std::cerr << "Database error: " << sqlite3_errmsg(db) << '\n';
                return -1;
            }

            const bool noCategories = categoryIds.empty();

            if (noCategories)
                options.push_back("(No categories yet)");

            options.push_back("Back");

            const int choice = ConsoleMenu::Menu::verticalMenu(
                "   CATEGORIES   ",
                options,
                ConsoleMenu::HorizontalAlignment::Center);

            if (choice < 0 || choice == static_cast<int>(options.size()) - 1)
                return -1;

            if (noCategories)
                return -1;

            return categoryIds[choice];
        }

        static void showTest(int testId)
        {
            sqlite3_stmt* questionStmt = nullptr;

            if (sqlite3_prepare_v2(db,
                "SELECT id, position, prompt "
                "FROM questions WHERE test_id = ? ORDER BY position;",
                -1, &questionStmt, nullptr) != SQLITE_OK)
            {
                std::cerr << "Database error: " << sqlite3_errmsg(db) << '\n';
                system("pause");
                return;
            }

            sqlite3_bind_int(questionStmt, 1, testId);

            system("cls");
            bool foundQuestions = false;
            int questionResult;

            while ((questionResult = sqlite3_step(questionStmt)) == SQLITE_ROW)
            {
                foundQuestions = true;

                const int questionId = sqlite3_column_int(questionStmt, 0);
                const int position = sqlite3_column_int(questionStmt, 1);
                const auto* promptText = sqlite3_column_text(questionStmt, 2);

                const std::string prompt =
                    promptText ? reinterpret_cast<const char*>(promptText) : "";

                std::cout << "\n" << position << ". " << prompt << '\n';

                sqlite3_stmt* answerStmt = nullptr;
                if (sqlite3_prepare_v2(db,
                    "SELECT text, is_correct FROM answers "
                    "WHERE question_id = ? ORDER BY position;",
                    -1, &answerStmt, nullptr) != SQLITE_OK)
                {
                    std::cerr << "Database error: " << sqlite3_errmsg(db) << '\n';
                    sqlite3_finalize(questionStmt);
                    system("pause");
                    return;
                }

                sqlite3_bind_int(answerStmt, 1, questionId);

                int answerResult;
                while ((answerResult = sqlite3_step(answerStmt)) == SQLITE_ROW)
                {
                    const auto* answerText = sqlite3_column_text(answerStmt, 0);
                    const int isCorrect = sqlite3_column_int(answerStmt, 1);

                    std::cout << "   - "
                        << (answerText
                            ? reinterpret_cast<const char*>(answerText)
                            : "");

                    if (isCorrect)
                        std::cout << "  [correct]";

                    std::cout << '\n';
                }

                sqlite3_finalize(answerStmt);

                if (answerResult != SQLITE_DONE)
                {
                    std::cerr << "Database error: " << sqlite3_errmsg(db) << '\n';
                    sqlite3_finalize(questionStmt);
                    system("pause");
                    return;
                }
            }

            sqlite3_finalize(questionStmt);

            if (questionResult != SQLITE_DONE)
                std::cerr << "Database error: " << sqlite3_errmsg(db) << '\n';
            else if (!foundQuestions)
                std::cout << "This test has no questions yet.\n";

            system("pause");
        }

        static void show()
        {
            while (true)
            {
                const int categoryId = chooseCategory();
                if (categoryId < 0)
                    return;

                while (true)
                {
                    std::vector<int> testIds;
                    std::vector<std::string> options;

                    sqlite3_stmt* stmt = nullptr;
                    if (sqlite3_prepare_v2(db,
                        "SELECT id, title FROM tests "
                        "WHERE category_id = ? AND is_active = 1 ORDER BY title;",
                        -1, &stmt, nullptr) != SQLITE_OK)
                    {
                        std::cerr << "Database error: " << sqlite3_errmsg(db) << '\n';
                        system("pause");
                        return;
                    }

                    sqlite3_bind_int(stmt, 1, categoryId);

                    int result;
                    while ((result = sqlite3_step(stmt)) == SQLITE_ROW)
                    {
                        testIds.push_back(sqlite3_column_int(stmt, 0));

                        const auto* titleText = sqlite3_column_text(stmt, 1);
                        options.push_back(
                            titleText ? reinterpret_cast<const char*>(titleText) : "");
                    }

                    sqlite3_finalize(stmt);

                    if (result != SQLITE_DONE)
                    {
                        std::cerr << "Database error: " << sqlite3_errmsg(db) << '\n';
                        system("pause");
                        return;
                    }

                    const bool noTests = testIds.empty();

                    if (noTests)
                        options.push_back("(No active tests in this category)");

                    options.push_back("Back");

                    const int choice = ConsoleMenu::Menu::verticalMenu(
                        "   TESTS   ",
                        options,
                        ConsoleMenu::HorizontalAlignment::Center);

                    if (choice < 0 || choice == static_cast<int>(options.size()) - 1)
                        break;

                    if (noTests)
                        break;

                    const int testId = testIds[choice];
                    showTest(testId);
                }
            }
        }

        static bool readField(const char* prompt, std::string& value)
        {
            std::cout << prompt;

            if (Func::readConsoleInput(value) == Func::InputResult::cancelled)
            {
                system("cls");
                return false;
            }

            if (value.empty())
            {
                std::cout << "This field cannot be empty.\n";
                system("pause");
                return false;
            }

            return true;
        }

        static int chooseTest(int categoryId)
        {
            std::vector<int> ids;
            std::vector<std::string> options;
            sqlite3_stmt* stmt = nullptr;

            if (sqlite3_prepare_v2(db,
                "SELECT id, title FROM tests WHERE category_id = ? ORDER BY title;",
                -1, &stmt, nullptr) != SQLITE_OK)
            {
                std::cerr << "Database error: " << sqlite3_errmsg(db) << '\n';
                return -1;
            }

            sqlite3_bind_int(stmt, 1, categoryId);

            int result;
            while ((result = sqlite3_step(stmt)) == SQLITE_ROW)
            {
                ids.push_back(sqlite3_column_int(stmt, 0));

                const auto* text = sqlite3_column_text(stmt, 1);
                options.push_back(text ? reinterpret_cast<const char*>(text) : "");
            }

            sqlite3_finalize(stmt);

            if (result != SQLITE_DONE)
            {
                std::cerr << "Database error: " << sqlite3_errmsg(db) << '\n';
                return -1;
            }

            const bool noTests = ids.empty();
            if (noTests)
                options.push_back("(No tests in this category)");

            options.push_back("Back");

            const int choice = ConsoleMenu::Menu::verticalMenu(
                "   TESTS   ",
                options,
                ConsoleMenu::HorizontalAlignment::Center);

            if (choice < 0 || choice == static_cast<int>(options.size()) - 1)
                return -1;

            if (noTests)
                return -1;

            return ids[choice];
        }

        static int chooseQuestion(int testId)
        {
            std::vector<int> ids;
            std::vector<std::string> options;
            sqlite3_stmt* stmt = nullptr;

            if (sqlite3_prepare_v2(db,
                "SELECT id, position, prompt "
                "FROM questions WHERE test_id = ? ORDER BY position;",
                -1, &stmt, nullptr) != SQLITE_OK)
            {
                std::cerr << "Database error: " << sqlite3_errmsg(db) << '\n';
                return -1;
            }

            sqlite3_bind_int(stmt, 1, testId);

            int result;
            while ((result = sqlite3_step(stmt)) == SQLITE_ROW)
            {
                ids.push_back(sqlite3_column_int(stmt, 0));

                const int position = sqlite3_column_int(stmt, 1);
                const auto* text = sqlite3_column_text(stmt, 2);
                const std::string prompt = text
                    ? reinterpret_cast<const char*>(text) : "";

                options.push_back(
                    std::to_string(position) + ". " + prompt);
            }

            sqlite3_finalize(stmt);

            if (result != SQLITE_DONE)
            {
                std::cerr << "Database error: " << sqlite3_errmsg(db) << '\n';
                return -1;
            }

            const bool noQuestions = ids.empty();
            if (noQuestions)
                options.push_back("(No questions in this test)");

            options.push_back("Back");

            const int choice = ConsoleMenu::Menu::verticalMenu(
                "   QUESTIONS   ",
                options,
                ConsoleMenu::HorizontalAlignment::Center);

            if (choice < 0 || choice == static_cast<int>(options.size()) - 1)
                return -1;

            if (noQuestions)
                return -1;

            return ids[choice];
        }

        static int nextPosition(const char* sql, int parentId)
        {
            sqlite3_stmt* stmt = nullptr;

            if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
            {
                std::cerr << "Database error: " << sqlite3_errmsg(db) << '\n';
                return -1;
            }

            sqlite3_bind_int(stmt, 1, parentId);

            int position = -1;
            if (sqlite3_step(stmt) == SQLITE_ROW)
                position = sqlite3_column_int(stmt, 0);

            sqlite3_finalize(stmt);
            return position;
        }

        static bool updateText(
            const char* sql,
            const std::string& value,
            int id,
            const char* successMessage)
        {
            sqlite3_stmt* stmt = nullptr;

            if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
            {
                std::cerr << "Database error: " << sqlite3_errmsg(db) << '\n';
                system("pause");
                return false;
            }

            sqlite3_bind_text(stmt, 1, value.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_int(stmt, 2, id);

            const int result = sqlite3_step(stmt);
            const int changedRows = sqlite3_changes(db);
            const std::string error = sqlite3_errmsg(db);
            sqlite3_finalize(stmt);

            if (result == SQLITE_DONE && changedRows > 0)
            {
                std::cout << successMessage << '\n';
                system("pause");
                return true;
            }

            if (result == SQLITE_DONE)
                std::cout << "Record not found.\n";
            else if (result == SQLITE_CONSTRAINT)
                std::cout << "That name is already in use.\n";
            else
                std::cerr << "Database error: " << error << '\n';

            system("pause");
            return false;
        }

        static int chooseAnswer(int questionId)
        {
            std::vector<int> answerIds;
            std::vector<std::string> options;
            sqlite3_stmt* stmt = nullptr;

            if (sqlite3_prepare_v2(db,
                "SELECT id, position, text, is_correct "
                "FROM answers WHERE question_id = ? ORDER BY position;",
                -1, &stmt, nullptr) != SQLITE_OK)
            {
                std::cerr << "Database error: " << sqlite3_errmsg(db) << '\n';
                return -1;
            }

            sqlite3_bind_int(stmt, 1, questionId);

            int result;
            while ((result = sqlite3_step(stmt)) == SQLITE_ROW)
            {
                answerIds.push_back(sqlite3_column_int(stmt, 0));

                const int position = sqlite3_column_int(stmt, 1);
                const auto* text = sqlite3_column_text(stmt, 2);
                const int isCorrect = sqlite3_column_int(stmt, 3);

                std::string label =
                    std::to_string(position) + ". " +
                    (text ? reinterpret_cast<const char*>(text) : "");

                if (isCorrect)
                    label += "  [correct]";

                options.push_back(label);
            }

            sqlite3_finalize(stmt);

            if (result != SQLITE_DONE)
            {
                std::cerr << "Database error: " << sqlite3_errmsg(db) << '\n';
                return -1;
            }

            const bool noAnswers = answerIds.empty();
            if (noAnswers)
                options.push_back("(No answers for this question)");

            options.push_back("Back");

            const int choice = ConsoleMenu::Menu::verticalMenu(
                "   ANSWERS   ",
                options,
                ConsoleMenu::HorizontalAlignment::Center);

            if (choice < 0 || choice == static_cast<int>(options.size()) - 1)
                return -1;

            if (noAnswers)
                return -1;

            return answerIds[choice];
        }

        static void addCategory()
        {
            std::string name;

            if (!readField("Category name: ", name))
                return;

            sqlite3_stmt* stmt = nullptr;
            if (sqlite3_prepare_v2(db,
                "INSERT INTO categories (name) VALUES (?);",
                -1, &stmt, nullptr) != SQLITE_OK)
            {
                std::cerr << "Database error: " << sqlite3_errmsg(db) << '\n';
                return;
            }

            sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);

            const int result = sqlite3_step(stmt);
            const std::string error = sqlite3_errmsg(db);
            sqlite3_finalize(stmt);

            if (result == SQLITE_DONE)
                std::cout << "Category added.\n";
            else if (result == SQLITE_CONSTRAINT)
                std::cout << "That category already exists.\n";
            else
                std::cerr << "Database error: " << error << '\n';

            system("pause");
        }

        static void addTest()
        {
            while (true)
            {
                const int categoryId = chooseCategory();
                if (categoryId < 0)
                    return;

                std::string title;
                if (!readField("Test title: ", title))
                    return;

                sqlite3_stmt* stmt = nullptr;
                if (sqlite3_prepare_v2(db,
                    "INSERT INTO tests (category_id, title) VALUES (?, ?);",
                    -1, &stmt, nullptr) != SQLITE_OK)
                {
                    std::cerr << "Database error: " << sqlite3_errmsg(db) << '\n';
                    return;
                }

                sqlite3_bind_int(stmt, 1, categoryId);
                sqlite3_bind_text(stmt, 2, title.c_str(), -1, SQLITE_TRANSIENT);

                const int result = sqlite3_step(stmt);
                const std::string error = sqlite3_errmsg(db);
                sqlite3_finalize(stmt);

                if (result == SQLITE_DONE)
                    std::cout << "Test added.\n";
                else
                    std::cerr << "Database error: " << error << '\n';

                system("pause");
            }
        }

        static void addQuestion()
        {
            while (true)
            {
                const int categoryId = chooseCategory();
                if (categoryId < 0)
                    return;
                
                while (true)
                {
                    const int testId = chooseTest(categoryId);
                    if (testId < 0)
                        return;

                    std::string prompt;
                    if (!readField("Question text: ", prompt))
                        return;

                    const int position = nextPosition(
                        "SELECT COALESCE(MAX(position), 0) + 1 "
                        "FROM questions WHERE test_id = ?;",
                        testId);

                    if (position < 0)
                        return;

                    sqlite3_stmt* stmt = nullptr;
                    if (sqlite3_prepare_v2(db,
                        "INSERT INTO questions (test_id, prompt, position) "
                        "VALUES (?, ?, ?);",
                        -1, &stmt, nullptr) != SQLITE_OK)
                    {
                        std::cerr << "Database error: " << sqlite3_errmsg(db) << '\n';
                        return;
                    }

                    sqlite3_bind_int(stmt, 1, testId);
                    sqlite3_bind_text(stmt, 2, prompt.c_str(), -1, SQLITE_TRANSIENT);
                    sqlite3_bind_int(stmt, 3, position);

                    const int result = sqlite3_step(stmt);
                    const std::string error = sqlite3_errmsg(db);
                    sqlite3_finalize(stmt);

                    if (result == SQLITE_DONE)
                    {
                        std::cout << "Question added.\n";

                        const int questionId = static_cast<int>(sqlite3_last_insert_rowid(db));
                        addAnswer(categoryId, testId, questionId);
                        while (true)
                        {
                            const int questionId = chooseQuestion(testId);
                            if (questionId < 0)
                                break;

                            addAnswer(categoryId, testId, questionId);
                        }
                    }
                    else
                        std::cerr << "Database error: " << error << '\n';
                }
            }
        }

        static void addAnswer()
        {
            while (true)
            {
                const int categoryId = chooseCategory();
                if (categoryId < 0)
                    return;

                while (true)
                {
                    const int testId = chooseTest(categoryId);
                    if (testId < 0)
                        break;

                    while (true)
                    {
                        const int questionId = chooseQuestion(testId);
                        if (questionId < 0)
                            break;

                        addAnswer(categoryId, testId, questionId);
                    }
                }
            }
        }
        static void addAnswer(int categoryId, int testId, int questionId)
        {
            std::string answerText;
            if (!readField("Answer text: ", answerText))
                return;

            const std::vector<std::string> correctOptions = {
                "Correct answer",
                "Incorrect answer",
                "Back"
            };

            const int correctChoice = ConsoleMenu::Menu::verticalMenu(
                "   ANSWER TYPE   ",
                correctOptions,
                ConsoleMenu::HorizontalAlignment::Center);

            if (correctChoice < 0 || correctChoice == 2)
                return;

            const int position = nextPosition(
                "SELECT COALESCE(MAX(position), 0) + 1 "
                "FROM answers WHERE question_id = ?;",
                questionId);

            if (position < 0)
                return;

            sqlite3_stmt* stmt = nullptr;
            if (sqlite3_prepare_v2(db,
                "INSERT INTO answers "
                "(question_id, text, is_correct, position) "
                "VALUES (?, ?, ?, ?);",
                -1, &stmt, nullptr) != SQLITE_OK)
            {
                std::cerr << "Database error: " << sqlite3_errmsg(db) << '\n';
                return;
            }

            sqlite3_bind_int(stmt, 1, questionId);
            sqlite3_bind_text(stmt, 2, answerText.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_int(stmt, 3, correctChoice == 0 ? 1 : 0);
            sqlite3_bind_int(stmt, 4, position);

            const int result = sqlite3_step(stmt);
            const std::string error = sqlite3_errmsg(db);
            sqlite3_finalize(stmt);

            if (result == SQLITE_DONE)
                std::cout << "Answer added.\n";
            else
                std::cerr << "Database error: " << error << '\n';

            system("pause");
        }

        static void addMenu()
        {
            const std::vector<std::string> options = { "Category", "Test", "Question", "Answer", "Back" };
            
            while (true)
            {
                const int choice = ConsoleMenu::Menu::verticalMenu("    ADD  NEW    ", options, ConsoleMenu::HorizontalAlignment::Center);
                switch (choice)
                {
                case 0:
                    addCategory();
                    break;
                case 1:
                    addTest();
                    break;
                case 2:
                    addQuestion();
                    break;
                case 3:
                    addAnswer();
                    break;
                case 4:
                default:
                    return;
                }
            }
        }

        static void editCategory()
        {
            while (true)
            {
                const int categoryId = chooseCategory();
                if (categoryId < 0)
                    return;

                while (true)
                {
                    std::string newName;
                    if (!readField("New category name: ", newName))
                        break;

                    updateText(
                        "UPDATE categories SET name = ? WHERE id = ?;",
                        newName,
                        categoryId,
                        "Category updated.");
                }
            }
        }

        static void editTest()
        {
            while (true)
            {
                const int categoryId = chooseCategory();
                if (categoryId < 0)
                    return;

                while (true)
                {
                    const int testId = chooseTest(categoryId);
                    if (testId < 0)
                        break;

                    while (true)
                    {
                        std::string newTitle;
                        if (!readField("New test title: ", newTitle))
                            break;

                        updateText(
                            "UPDATE tests SET title = ? WHERE id = ?;",
                            newTitle,
                            testId,
                            "Test updated.");
                    }
                }
            }
        }

        static void editQuestion()
        {
            while (true)
            {
                const int categoryId = chooseCategory();
                if (categoryId < 0)
                    return;

                while (true)
                {
                    const int testId = chooseTest(categoryId);
                    if (testId < 0)
                        break;

                    while (true)
                    {
                        const int questionId = chooseQuestion(testId);
                        if (questionId < 0)
                            break;

                        while (true)
                        {
                            std::string newPrompt;
                            if (!readField("New question text: ", newPrompt))
                                break;

                            updateText(
                                "UPDATE questions SET prompt = ? WHERE id = ?;",
                                newPrompt,
                                questionId,
                                "Question updated.");
                        }
                    }
                }
            }
        }

        static void editAnswer()
        {
            while (true)
            {
                const int categoryId = chooseCategory();
                if (categoryId < 0)
                    return;

                while (true)
                {
                    const int testId = chooseTest(categoryId);
                    if (testId < 0)
                        break;

                    while (true)
                    {
                        const int questionId = chooseQuestion(testId);
                        if (questionId < 0)
                            break;
                        
                        while (true)
                        {
                            const int answerId = chooseAnswer(questionId);
                            if (answerId < 0)
                                break;

                            {
                                std::string newText;
                                if (!readField("New answer text: ", newText))
                                    return;

                                const std::vector<std::string> correctOptions = {
                                    "Correct answer",
                                    "Incorrect answer",
                                    "Back"
                                };

                                const int correctChoice = ConsoleMenu::Menu::verticalMenu(
                                    "   ANSWER TYPE   ",
                                    correctOptions,
                                    ConsoleMenu::HorizontalAlignment::Center);

                                if (correctChoice < 0 || correctChoice == 2)
                                    break;

                                sqlite3_stmt* stmt = nullptr;
                                const char* sql =
                                    "UPDATE answers SET text = ?, is_correct = ? WHERE id = ?;";

                                if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
                                {
                                    std::cerr << "Database error: " << sqlite3_errmsg(db) << '\n';
                                    system("pause");
                                    return;
                                }

                                sqlite3_bind_text(stmt, 1, newText.c_str(), -1, SQLITE_TRANSIENT);
                                sqlite3_bind_int(stmt, 2, correctChoice == 0 ? 1 : 0);
                                sqlite3_bind_int(stmt, 3, answerId);

                                const int result = sqlite3_step(stmt);
                                const int changedRows = sqlite3_changes(db);
                                const std::string error = sqlite3_errmsg(db);
                                sqlite3_finalize(stmt);

                                if (result == SQLITE_DONE && changedRows > 0)
                                    std::cout << "Answer updated.\n";
                                else if (result == SQLITE_DONE)
                                    std::cout << "Answer not found.\n";
                                else
                                    std::cerr << "Database error: " << error << '\n';

                                system("pause");
                            }
                        }
                    }
                }
            }
        }

        static void editMenu()
        {
            while (true)
            {
                const std::vector<std::string> options = {
                    "Category",
                    "Test",
                    "Question",
                    "Answer",
                    "Back"
                };

                const int choice = ConsoleMenu::Menu::verticalMenu(
                    "    EDIT    ",
                    options,
                    ConsoleMenu::HorizontalAlignment::Center);

                switch (choice)
                {
                case 0:
                    editCategory();
                    break;
                case 1:
                    editTest();
                    break;
                case 2:
                    editQuestion();
                    break;
                case 3:
                    editAnswer();
                    break;
                case 4:
                default:
                    return;
                }
            }
        }

        static void filesMenu()
        {

        }

    public:
        static void Menu()
        {
            std::vector<std::string> options = { "Show", "Add New", "Edit", "File Handler", "Back" };
            while (true)
            {
                int choice = ConsoleMenu::Menu::verticalMenu("   TESTS MENU   ", options, ConsoleMenu::HorizontalAlignment::Center);
                switch (choice)
                {
                case 0:
                    show();
                    break;
                case 1:
                    addMenu();
                    break;
                case 2:
                    editMenu();
                    break;
                case 3:
                    filesMenu();
                    break;
                case 4:
                default:
                    return;
                }
            }
        }
    };

    class Users
    {
        static bool chooseUserId(int& userId)
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

        static void show()
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

        static void add()
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

        static void edit()
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

        static void del()
        {
            int userId;
            if (!chooseUserId(userId))
                return;

            sqlite3_stmt* stmt = nullptr;

            if (sqlite3_prepare_v2(db,
                "SELECT login FROM users WHERE id = ?;",
                -1, &stmt, nullptr) != SQLITE_OK)
            {
                std::cerr << "Database error: " << sqlite3_errmsg(db) << '\n';
                return;
            }

            sqlite3_bind_int(stmt, 1, userId);

            if (sqlite3_step(stmt) == SQLITE_ROW)
            {
                const auto* text = sqlite3_column_text(stmt, 0);
                const std::string login =
                    text ? reinterpret_cast<const char*>(text) : "";

                //int x = (Settings::widthConsole - 40) / 2;
                //int y = (Settings::heightConsole - 10) / 2;
                //system("cls");
                //ConsoleMenu::frame(50, 13, x - 5, y - 2);
                //Func::SetCursor(x, y);
                //std::cout << "               ADD USER";
                //Func::SetCursor(x, y + 1);
                //std::cout << "======================================";
                //std::vector<std::string> items = { "Full name        : ", "Home address     : ", "Phone            : ", "Login            : ", "Password         : ", "Confirm password : " };
                //for (size_t i = 0; i < items.size(); i++)
                //{
                //    Func::SetCursor(x, y + i + 2);
                //    std::cout << items[i];
                //}

                //std::string fullName, address, phone, login, password, confirmation;
                //std::vector<std::string*> info = { &fullName, &address, &phone, &login };
                //for (size_t i = 0; i < info.size(); i++)
                //{
                //    Func::SetCursor(x + 19, y + 2 + i);
                //    if (Func::readConsoleInput(*info[i]) == Func::InputResult::cancelled)
                //    {
                //        system("cls");
                //        return;
                //    }
                //}
            }

            sqlite3_finalize(stmt);

            std::string confirmation;
            std::cout << "Delete this user? Type Y to confirm: ";
            std::getline(std::cin >> std::ws, confirmation);

            if (confirmation != "Y" && confirmation != "y")
            {
                std::cout << "Deletion cancelled.\n";
                system("pause");
                return;
            }

            stmt = nullptr;
            if (sqlite3_prepare_v2(db,
                "DELETE FROM users WHERE id = ?;",
                -1, &stmt, nullptr) != SQLITE_OK)
            {
                std::cerr << "Database error: " << sqlite3_errmsg(db) << '\n';
                return;
            }

            sqlite3_bind_int(stmt, 1, userId);
            const int result = sqlite3_step(stmt);
            const int changedRows = sqlite3_changes(db);
            sqlite3_finalize(stmt);

            if (result == SQLITE_DONE && changedRows > 0)
                std::cout << "User deleted.\n";
            else if (result == SQLITE_DONE)
                std::cout << "User not found.\n";
            else
                std::cerr << "Database error: " << sqlite3_errmsg(db) << '\n';

            system("pause");
        }

    public:
        static void Menu()
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
            std::vector<std::string> options = { "Tests", "Users", "Statistics", "Log Out" };
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
                default:
                    return;
                }
            }
        }
    };
}
