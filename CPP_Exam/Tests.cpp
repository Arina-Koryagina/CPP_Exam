#include "Tests.h"

namespace Test
{
    int chooseCategory(sqlite3* db)
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

    int chooseTest(sqlite3* db, int categoryId)
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
}

namespace Admin
{
    void Tests::showTest(int testId)
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
        std::vector<std::string> lines;

        while ((questionResult = sqlite3_step(questionStmt)) == SQLITE_ROW)
        {
            foundQuestions = true;

            const int questionId = sqlite3_column_int(questionStmt, 0);
            const int position = sqlite3_column_int(questionStmt, 1);
            const auto* promptText = sqlite3_column_text(questionStmt, 2);

            const std::string prompt =
                promptText ? reinterpret_cast<const char*>(promptText) : "";

            lines.push_back(std::to_string(position) + ". " + prompt);

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
                const std::string answer = answerText
                    ? reinterpret_cast<const char*>(answerText)
                    : "";

                lines.push_back("   - " + answer +
                    (isCorrect ? "  [correct]" : ""));
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
        {
            std::vector<std::string> options = { "(No active questions in this test)", "Back" };
            ConsoleMenu::Menu::verticalMenu(
                "   QUESTIONS   ",
                options,
                ConsoleMenu::HorizontalAlignment::Center);
            return;
        }
        const int frameWidth = Settings::widthConsole - 10;
        const int contentWidth = frameWidth - 10;
        const int frameHeight = static_cast<int>(lines.size()) + 6;

        int x = (Settings::widthConsole - contentWidth) / 2;
        int y = (Settings::heightConsole - frameHeight) / 2;

        ConsoleMenu::infoFrame(frameWidth, frameHeight, x, &y, "TEST PREVIEW");

        for (size_t i = 0; i < lines.size(); ++i)
        {
            Func::SetCursor(x, y + static_cast<int>(i));
            std::cout << lines[i];
        }

        Func::SetCursor(x, y + static_cast<int>(lines.size()) + 1);
        system("pause");
    }

    void Tests::show()
    {
        while (true)
        {
            const int categoryId = Test::chooseCategory(db);
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

    bool Tests::readField(const char* prompt, std::string& value)
    {
        system("cls");
        int x = (Settings::widthConsole - 40) / 2;
        int y = (Settings::heightConsole - 1) / 2;
        ConsoleMenu::frame(50, 5, x - 5, y - 2);
        Func::SetCursor(x, y);
        std::cout << prompt;

        Func::SetCursor(x + std::strlen(prompt), y);
        if (Func::readConsoleInput(value) == Func::InputResult::cancelled)
        {
            Func::SetCursor(x, y + 2);
            system("cls");
            return false;
        }

        if (value.empty())
        {
            ConsoleMenu::output("This field cannot be empty");
            system("pause");
            return false;
        }

        return true;
    }

    int Tests::chooseQuestion(int testId)
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

    int Tests::nextPosition(const char* sql, int parentId)
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

    bool Tests::updateText(
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
            ConsoleMenu::output(successMessage);
            system("pause");
            return true;
        }

        if (result == SQLITE_DONE)
            ConsoleMenu::output("Record not found");
        else if (result == SQLITE_CONSTRAINT)
            ConsoleMenu::output("That name is already in use");
        else
            std::cerr << "Database error: " << error << '\n';

        system("pause");
        return false;
    }

    int Tests::chooseAnswer(int questionId)
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

    void Tests::addCategory()
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
        {
            ConsoleMenu::output("Category added");
            system("pause");
            const int categoryId = static_cast<int>(sqlite3_last_insert_rowid(db));
            addTest(categoryId);
        }
        else if (result == SQLITE_CONSTRAINT)
            std::cout << "That category already exists";
        else
            std::cerr << "Database error: " << error << '\n';

        system("pause");
    }

    void Tests::addTest()
    {
        while (true)
        {
            const int categoryId = Test::chooseCategory(db);
            if (categoryId < 0)
                return;

            addTest(categoryId);
        }
    }
    void Tests::addTest(int categoryId)
    {
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
        {
            ConsoleMenu::output("Test added");
            system("pause");
            const int testId = static_cast<int>(sqlite3_last_insert_rowid(db));
            addQuestion(categoryId, testId);
        }
        else
            std::cerr << "Database error: " << error << '\n';

        system("pause");
    }

    void Tests::addQuestion()
    {
        while (true)
        {
            const int categoryId = Test::chooseCategory(db);
            if (categoryId < 0)
                return;

            while (true)
            {
                const int testId = Test::chooseTest(db, categoryId);
                if (testId < 0)
                    break; // back to Category list

                addQuestion(categoryId, testId);
            }
        }
    }
    void Tests::addQuestion(int categoryId, int testId)
    {
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
            ConsoleMenu::output("Question added");
            system("pause");

            const int questionId = static_cast<int>(sqlite3_last_insert_rowid(db));
            addAnswer(categoryId, testId, questionId);
            while (true)
            {
                const int questionId = chooseQuestion(testId);
                if (questionId < 0)
                    break; // back to Test list

                addAnswer(categoryId, testId, questionId);
            }
        }
        else
            std::cerr << "Database error: " << error << '\n';
    }

    void Tests::addAnswer()
    {
        while (true)
        {
            const int categoryId = Test::chooseCategory(db);
            if (categoryId < 0)
                return; // back to Main menu

            while (true)
            {
                const int testId = Test::chooseTest(db, categoryId);
                if (testId < 0)
                    break; // back to Category list

                while (true)
                {
                    const int questionId = chooseQuestion(testId);
                    if (questionId < 0)
                        break; // back to Test list

                    addAnswer(categoryId, testId, questionId);
                }
            }
        }
    }
    void Tests::addAnswer(int categoryId, int testId, int questionId)
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
            "   ANSWER  TYPE   ",
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
            ConsoleMenu::output("Answer added");
        else
            std::cerr << "Database error: " << error << '\n';

        system("pause");
    }

    void Tests::addMenu()
    {
        const std::vector<std::string> options = { "Category", "Test", "Question", "Answer", "Back" };

        while (true)
        {
            const int choice = ConsoleMenu::Menu::verticalMenu("   ADD  NEW   ", options, ConsoleMenu::HorizontalAlignment::Center);
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

    void Tests::editCategory()
    {
        while (true)
        {
            const int categoryId = Test::chooseCategory(db);
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

    void Tests::editTest()
    {
        while (true)
        {
            const int categoryId = Test::chooseCategory(db);
            if (categoryId < 0)
                return;

            while (true)
            {
                const int testId = Test::chooseTest(db, categoryId);
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

    void Tests::editQuestion()
    {
        while (true)
        {
            const int categoryId = Test::chooseCategory(db);
            if (categoryId < 0)
                return;

            while (true)
            {
                const int testId = Test::chooseTest(db, categoryId);
                if (testId < 0)
                    break;

                while (true)
                {
                    const int questionId = chooseQuestion(testId);
                    if (questionId < 0)
                        break;

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

    void Tests::editAnswer()
    {
        while (true)
        {
            const int categoryId = Test::chooseCategory(db);
            if (categoryId < 0)
                return;

            while (true)
            {
                const int testId = Test::chooseTest(db, categoryId);
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
                                "   ANSWER  TYPE   ",
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
                                ConsoleMenu::output("Answer updated");
                            else if (result == SQLITE_DONE)
                                ConsoleMenu::output("Answer not found");
                            else
                                std::cerr << "Database error: " << error << '\n';

                            system("pause");
                        }
                    }
                }
            }
        }
    }

    void Tests::editMenu()
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
                "   EDIT   ",
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

    void Tests::deleteMenu()
    {
        ConsoleMenu::output("Feature not implemented yet");
    }

    void Tests::Menu()
    {
        std::vector<std::string> options = { "Show", "Add New", "Edit", "Delete", "File Handler", "Back" };
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
                deleteMenu();
                break;
            case 4:
                filesMenu();
                break;
            case 5:
            default:
                return;
            }
        }
    }
}