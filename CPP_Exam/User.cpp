#include "User.h"

namespace User
{
    int User::choosePausedAttempt()
    {
        std::vector<int> attemptIds;
        std::vector<std::string> options;
        sqlite3_stmt* stmt = nullptr;

        const char* sql =
            "SELECT a.id, c.name, t.title, a.started_at "
            "FROM attempts AS a "
            "JOIN tests AS t ON t.id = a.test_id "
            "JOIN categories AS c ON c.id = t.category_id "
            "WHERE a.user_id = ? AND a.status = 'in_progress' "
            "ORDER BY a.started_at;";

        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        {
            std::cerr << "Database error: " << sqlite3_errmsg(db) << '\n';
            system("pause");
            return -1;
        }

        sqlite3_bind_int(stmt, 1, userId);

        int result;
        while ((result = sqlite3_step(stmt)) == SQLITE_ROW)
        {
            attemptIds.push_back(sqlite3_column_int(stmt, 0));

            const auto* categoryText = sqlite3_column_text(stmt, 1);
            const auto* testText = sqlite3_column_text(stmt, 2);
            const auto* startedText = sqlite3_column_text(stmt, 3);

            const std::string category = categoryText
                ? reinterpret_cast<const char*>(categoryText) : "";
            const std::string test = testText
                ? reinterpret_cast<const char*>(testText) : "";
            const std::string started = startedText
                ? reinterpret_cast<const char*>(startedText) : "";

            options.push_back(category + " - " + test + "  (" + started + ")");
        }

        sqlite3_finalize(stmt);

        if (result != SQLITE_DONE)
        {
            std::cerr << "Database error: " << sqlite3_errmsg(db) << '\n';
            system("pause");
            return -1;
        }

        const bool noAttempts = attemptIds.empty();

        if (noAttempts)
            options.push_back("(No unfinished tests)");

        options.push_back("Back");

        const int choice = ConsoleMenu::Menu::verticalMenu(
            "   CONTINUE  TEST   ",
            options,
            ConsoleMenu::HorizontalAlignment::Center);

        if (choice < 0 || choice == static_cast<int>(options.size()) - 1)
            return -1;

        if (noAttempts)
            return -1;

        return attemptIds[choice];
    }

    void User::takeTest()
    {
        while (true)
        {
            const int categoryId = Test::chooseCategory(db);
            if (categoryId < 0)
                return; // Back to User menu.

            while (true)
            {
                const int testId = Test::chooseTest(db, categoryId);
                if (testId < 0)
                    break; // Back to category list.

                sqlite3_stmt* stmt = nullptr;

                // Don't start an attempt for a test with no questions.
                if (sqlite3_prepare_v2(db,
                    "SELECT COUNT(*) FROM questions WHERE test_id = ?;",
                    -1, &stmt, nullptr) != SQLITE_OK)
                {
                    std::cerr << "Database error: "
                        << sqlite3_errmsg(db) << '\n';
                    system("pause");
                    return;
                }

                sqlite3_bind_int(stmt, 1, testId);

                int questionCount = 0;
                if (sqlite3_step(stmt) == SQLITE_ROW)
                    questionCount = sqlite3_column_int(stmt, 0);

                sqlite3_finalize(stmt);

                if (questionCount == 0)
                {
                    ConsoleMenu::output("This test has no questions yet");
                    system("pause");
                    continue; // Back to the test list.
                }

                // Create an attempt. runAttempt() will save answers as the
                // user goes and leave status as "in_progress" if they pause.
                stmt = nullptr;
                if (sqlite3_prepare_v2(db,
                    "INSERT INTO attempts (user_id, test_id) VALUES (?, ?);",
                    -1, &stmt, nullptr) != SQLITE_OK)
                {
                    std::cerr << "Database error: "
                        << sqlite3_errmsg(db) << '\n';
                    system("pause");
                    return;
                }

                sqlite3_bind_int(stmt, 1, userId);
                sqlite3_bind_int(stmt, 2, testId);

                const int result = sqlite3_step(stmt);
                sqlite3_finalize(stmt);

                if (result != SQLITE_DONE)
                {
                    std::cerr << "Could not start the test: "
                        << sqlite3_errmsg(db) << '\n';
                    system("pause");
                    return;
                }

                const int attemptId =
                    static_cast<int>(sqlite3_last_insert_rowid(db));

                runAttempt(attemptId);
                return; // After finishing or pausing, go to User menu.
            }
        }
    }

    void User::continueTest()
    {
        const int attemptId = choosePausedAttempt();

        if (attemptId < 0)
            return; // Back to the User menu.

        runAttempt(attemptId);
    }

    void User::runAttempt(int attemptId)
    {
        // Make sure this is an unfinished attempt belonging to the logged-in user.
        sqlite3_stmt* stmt = nullptr;

        if (sqlite3_prepare_v2(db,
            "SELECT test_id FROM attempts "
            "WHERE id = ? AND user_id = ? AND status = 'in_progress';",
            -1, &stmt, nullptr) != SQLITE_OK)
        {
            std::cerr << "Database error: " << sqlite3_errmsg(db) << '\n';
            system("pause");
            return;
        }

        sqlite3_bind_int(stmt, 1, attemptId);
        sqlite3_bind_int(stmt, 2, userId);

        if (sqlite3_step(stmt) != SQLITE_ROW)
        {
            sqlite3_finalize(stmt);
            ConsoleMenu::output("Could not find an unfinished attempt");
            system("pause");
            return;
        }

        const int testId = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);

        while (true)
        {
            // Find the first question that this attempt has not answered yet.
            stmt = nullptr;
            const char* questionSql =
                "SELECT q.id, q.prompt "
                "FROM questions AS q "
                "LEFT JOIN attempt_answers AS aa "
                "  ON aa.question_id = q.id AND aa.attempt_id = ? "
                "WHERE q.test_id = ? AND aa.attempt_id IS NULL "
                "ORDER BY q.position "
                "LIMIT 1;";

            if (sqlite3_prepare_v2(db, questionSql, -1, &stmt, nullptr) != SQLITE_OK)
            {
                std::cerr << "Database error: " << sqlite3_errmsg(db) << '\n';
                system("pause");
                return;
            }

            sqlite3_bind_int(stmt, 1, attemptId);
            sqlite3_bind_int(stmt, 2, testId);

            const int questionResult = sqlite3_step(stmt);

            if (questionResult == SQLITE_DONE)
            {
                sqlite3_finalize(stmt);
                break; // No unanswered questions remain; calculate the result.
            }

            if (questionResult != SQLITE_ROW)
            {
                sqlite3_finalize(stmt);
                std::cerr << "Database error: " << sqlite3_errmsg(db) << '\n';
                system("pause");
                return;
            }

            const int questionId = sqlite3_column_int(stmt, 0);
            const auto* promptText = sqlite3_column_text(stmt, 1);
            const std::string prompt = promptText
                ? reinterpret_cast<const char*>(promptText)
                : "";

            sqlite3_finalize(stmt);

            // Load the answers for this question.
            std::vector<int> answerIds;
            std::vector<std::string> options;

            stmt = nullptr;
            if (sqlite3_prepare_v2(db,
                "SELECT id, text FROM answers "
                "WHERE question_id = ? ORDER BY position;",
                -1, &stmt, nullptr) != SQLITE_OK)
            {
                std::cerr << "Database error: " << sqlite3_errmsg(db) << '\n';
                system("pause");
                return;
            }

            sqlite3_bind_int(stmt, 1, questionId);

            int answerResult;
            while ((answerResult = sqlite3_step(stmt)) == SQLITE_ROW)
            {
                answerIds.push_back(sqlite3_column_int(stmt, 0));

                const auto* answerText = sqlite3_column_text(stmt, 1);
                options.push_back(answerText
                    ? reinterpret_cast<const char*>(answerText)
                    : "");
            }

            sqlite3_finalize(stmt);

            if (answerResult != SQLITE_DONE)
            {
                std::cerr << "Database error: " << sqlite3_errmsg(db) << '\n';
                system("pause");
                return;
            }

            if (answerIds.empty())
            {
                ConsoleMenu::output("This question has no answers yet: attempt is saved");
                system("pause");
                return;
            }

            options.push_back("Pause and save");

            const int choice = ConsoleMenu::Menu::verticalMenu(
                prompt,
                options,
                ConsoleMenu::HorizontalAlignment::Center);

            // Esc or the explicit pause option returns to the User menu.
            if (choice < 0 || choice == static_cast<int>(answerIds.size()))
                return;

            // Save this answer. The next loop iteration will load the next
            // unanswered question.
            stmt = nullptr;
            if (sqlite3_prepare_v2(db,
                "INSERT INTO attempt_answers "
                "(attempt_id, question_id, answer_id) VALUES (?, ?, ?);",
                -1, &stmt, nullptr) != SQLITE_OK)
            {
                std::cerr << "Database error: " << sqlite3_errmsg(db) << '\n';
                system("pause");
                return;
            }

            sqlite3_bind_int(stmt, 1, attemptId);
            sqlite3_bind_int(stmt, 2, questionId);
            sqlite3_bind_int(stmt, 3, answerIds[choice]);

            const int saveResult = sqlite3_step(stmt);
            sqlite3_finalize(stmt);

            if (saveResult != SQLITE_DONE)
            {
                std::cerr << "Could not save your answer: "
                    << sqlite3_errmsg(db) << '\n';
                system("pause");
                return;
            }
        }

        // All questions have been answered. Count correct answers.
        stmt = nullptr;
        if (sqlite3_prepare_v2(db,
            "SELECT COUNT(q.id), "
            "SUM(CASE WHEN a.is_correct = 1 THEN 1 ELSE 0 END) "
            "FROM questions AS q "
            "LEFT JOIN attempt_answers AS aa "
            "  ON aa.question_id = q.id AND aa.attempt_id = ? "
            "LEFT JOIN answers AS a "
            "  ON a.id = aa.answer_id AND a.question_id = q.id "
            "WHERE q.test_id = ?;",
            -1, &stmt, nullptr) != SQLITE_OK)
        {
            std::cerr << "Database error: " << sqlite3_errmsg(db) << '\n';
            system("pause");
            return;
        }

        sqlite3_bind_int(stmt, 1, attemptId);
        sqlite3_bind_int(stmt, 2, testId);

        if (sqlite3_step(stmt) != SQLITE_ROW)
        {
            sqlite3_finalize(stmt);
            std::cerr << "Could not calculate the test result.\n";
            system("pause");
            return;
        }

        const int questionCount = sqlite3_column_int(stmt, 0);
        const int correctCount = sqlite3_column_int(stmt, 1);
        sqlite3_finalize(stmt);

        if (questionCount == 0)
        {
            ConsoleMenu::output("This test has no questions");
            system("pause");
            return;
        }

        // Convert the result to a 1–12 grade, rounding to the nearest grade.
        int grade = (correctCount * 12 + questionCount / 2) / questionCount;
        if (grade < 1)
            grade = 1;

        // Mark the attempt completed and save its result.
        stmt = nullptr;
        if (sqlite3_prepare_v2(db,
            "UPDATE attempts SET "
            "status = 'completed', "
            "finished_at = CURRENT_TIMESTAMP, "
            "correct_count = ?, "
            "question_count = ?, "
            "grade = ? "
            "WHERE id = ? AND user_id = ?;",
            -1, &stmt, nullptr) != SQLITE_OK)
        {
            std::cerr << "Database error: " << sqlite3_errmsg(db) << '\n';
            system("pause");
            return;
        }

        sqlite3_bind_int(stmt, 1, correctCount);
        sqlite3_bind_int(stmt, 2, questionCount);
        sqlite3_bind_int(stmt, 3, grade);
        sqlite3_bind_int(stmt, 4, attemptId);
        sqlite3_bind_int(stmt, 5, userId);

        const int updateResult = sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        if (updateResult != SQLITE_DONE)
        {
            std::cerr << "Could not finish the attempt: "
                << sqlite3_errmsg(db) << '\n';
            system("pause");
            return;
        }

        const double percent =
            100.0 * correctCount / questionCount;
        std::string count = std::to_string(correctCount) + " / " + std::to_string(questionCount),
            perc = std::format("{:.1f}%", percent),
            graded = std::to_string(grade) + " / 12";

        system("cls");
        std::vector<std::string> text = { "Correct answers : ", "Percent correct : ", "Grade           : " };
        std::vector<std::string> data = { count, perc, graded };
        int x = (Settings::widthConsole - 30) / 2;
        int y = (Settings::heightConsole - text.size() - 4) / 2;
        ConsoleMenu::infoFrame(40, 6 + text.size(), x, &y, "TEST  RESULT");
        for (size_t i = 0; i < text.size(); i++)
        {
            Func::SetCursor(x, y + i);
            std::cout << text[i];
        }
        for (size_t i = 0; i < data.size(); i++)
        {
            Func::SetCursor(x + 18, y + i);
            std::cout << data[i];
        }

        Func::SetCursor(x, y + 1 + text.size());
        system("pause");
    }

    void User::showAttemptResult(int attemptId)
    {
        sqlite3_stmt* stmt = nullptr;

        const char* sql =
            "SELECT c.name, t.title, a.started_at, a.finished_at, "
            "a.correct_count, a.question_count, a.grade "
            "FROM attempts AS a "
            "JOIN tests AS t ON t.id = a.test_id "
            "JOIN categories AS c ON c.id = t.category_id "
            "WHERE a.id = ? AND a.user_id = ? "
            "AND a.status = 'completed';";

        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        {
            std::cerr << "Database error: " << sqlite3_errmsg(db) << '\n';
            system("pause");
            return;
        }

        sqlite3_bind_int(stmt, 1, attemptId);
        sqlite3_bind_int(stmt, 2, userId);


        const int result = sqlite3_step(stmt);
        if (result != SQLITE_ROW)
        {
            sqlite3_finalize(stmt);

            if (result == SQLITE_DONE)
            {
                ConsoleMenu::output("Could not find that completed result");
            }
            else
                std::cerr << "Database error: " << sqlite3_errmsg(db) << '\n';

            system("pause");
            return;
        }

        auto columnText = [stmt](int column)
            {
                const auto* value = sqlite3_column_text(stmt, column);
                return value
                    ? std::string(reinterpret_cast<const char*>(value))
                    : std::string{};
            };

        const std::string category = columnText(0);
        const std::string test = columnText(1);
        const auto* startedText = sqlite3_column_text(stmt, 2);
        const auto* finishedText = sqlite3_column_text(stmt, 3);

        const std::string started = startedText
            ? reinterpret_cast<const char*>(startedText)
            : "Not available";

        const std::string finished = finishedText
            ? reinterpret_cast<const char*>(finishedText)
            : "Not available";
        const int correctCount = sqlite3_column_int(stmt, 4);
        const int questionCount = sqlite3_column_int(stmt, 5);
        const int grade = sqlite3_column_int(stmt, 6);

        sqlite3_finalize(stmt);

        const double percent = questionCount > 0
            ? 100.0 * correctCount / questionCount
            : 0.0;
        std::string count = std::to_string(correctCount) + " / " + std::to_string(questionCount),
            perc = std::format("{:.1f}%", percent),
            graded = std::to_string(grade) + " / 12";

        system("cls");
        std::vector<std::string> text = { "Category        : ",
                                          "Test            : ",
                                          "Started         : ",
                                          "Finished        : ",
                                          "Correct answers : ",
                                          "Percent correct : ",
                                          "Grade           : " };
        std::vector<std::string> data = { category, test, started, finished, count, perc, graded };
        int x = (Settings::widthConsole - 30) / 2;
        int y = (Settings::heightConsole - text.size() - 3) / 2;
        ConsoleMenu::infoFrame(48, 6 + text.size(), x, &y, "TEST  RESULT");
        for (size_t i = 0; i < text.size(); i++)
        {
            Func::SetCursor(x, y + i);
            std::cout << text[i];
        }
        for (size_t i = 0; i < data.size(); i++)
        {
            Func::SetCursor(x + 18, y + i);
            std::cout << data[i];
        }

        Func::SetCursor(x, y + 1 + text.size());
        system("pause");
    }

    void User::showProfile()
    {
        sqlite3_stmt* stmt = nullptr;

        if (sqlite3_prepare_v2(db,
            "SELECT full_name, login, home_address, phone "
            "FROM users WHERE id = ?;",
            -1, &stmt, nullptr) != SQLITE_OK)
        {
            std::cerr << "Database error: " << sqlite3_errmsg(db) << '\n';
            return;
        }

        sqlite3_bind_int(stmt, 1, userId);

        int result = sqlite3_step(stmt);
        if (result == SQLITE_ROW)
        {
            auto columnText = [stmt](int column)
                {
                    const auto* value = sqlite3_column_text(stmt, column);
                    return value
                        ? std::string(reinterpret_cast<const char*>(value))
                        : std::string{};
                };

            std::vector<std::string> text = { "Name    : ", "Login   : ", "Address : ", "Phone   : " };
            std::vector<std::string> data = {
                                                columnText(0), // full_name
                                                columnText(1), // login
                                                columnText(2), // home_address
                                                columnText(3)  // phone
            };

            int x = (Settings::widthConsole - 30) / 2;
            int y = (Settings::heightConsole - text.size() - 2) / 2;
            ConsoleMenu::infoFrame(40, 10, x, &y, "USER DETAILS");

            for (size_t i = 0; i < text.size(); i++)
            {
                Func::SetCursor(x, y + i);
                std::cout << text[i] << data[i];
            }
            Func::SetCursor(x, y + text.size() + 1);
        }
        else if (result == SQLITE_DONE)
        {
            ConsoleMenu::output("Could not find user profile");
        }
        else
        {
            std::cerr << "Database error: " << sqlite3_errmsg(db) << '\n';
        }

        system("pause");
        sqlite3_finalize(stmt);
    }

    void User::showResults()
    {
        std::vector<int> attemptIds;
        std::vector<std::string> options;
        sqlite3_stmt* stmt = nullptr;

        const char* sql =
            "SELECT a.id, c.name, t.title, a.finished_at, a.grade "
            "FROM attempts AS a "
            "JOIN tests AS t ON t.id = a.test_id "
            "JOIN categories AS c ON c.id = t.category_id "
            "WHERE a.user_id = ? AND a.status = 'completed' "
            "ORDER BY a.finished_at DESC;";

        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        {
            std::cerr << "Database error: " << sqlite3_errmsg(db) << '\n';
            system("pause");
            return;
        }

        sqlite3_bind_int(stmt, 1, userId);

        int result;
        while ((result = sqlite3_step(stmt)) == SQLITE_ROW)
        {
            attemptIds.push_back(sqlite3_column_int(stmt, 0));

            const auto* categoryText = sqlite3_column_text(stmt, 1);
            const auto* testText = sqlite3_column_text(stmt, 2);
            const auto* finishedText = sqlite3_column_text(stmt, 3);

            const std::string category = categoryText
                ? reinterpret_cast<const char*>(categoryText) : "";
            const std::string test = testText
                ? reinterpret_cast<const char*>(testText) : "";
            const std::string finished = finishedText
                ? reinterpret_cast<const char*>(finishedText)
                : "Not available";
            const int grade = sqlite3_column_int(stmt, 4);

            options.push_back(
                category + " - " + test +
                " | " + finished +
                " | Grade: " + std::to_string(grade) + "/12");
        }

        sqlite3_finalize(stmt);

        if (result != SQLITE_DONE)
        {
            std::cerr << "Database error: " << sqlite3_errmsg(db) << '\n';
            system("pause");
            return;
        }

        if (attemptIds.empty())
        {
            ConsoleMenu::output("No completed attempts yet");
            system("pause");
            return;
        }

        options.push_back("Back");

        while (true)
        {
            const int choice = ConsoleMenu::Menu::verticalMenu(
                "   USER RESULTS   ",
                options,
                ConsoleMenu::HorizontalAlignment::Center);

            if (choice < 0 || choice == static_cast<int>(options.size()) - 1)
                return; // Back to the User menu.

            showAttemptResult(attemptIds[choice]);
            // When the details screen returns, show this results list again.
        }
    }

    void User::Menu()
    {
        const std::vector<std::string> options = {
            "Take a test",
            "Continue a test",
            "My info",
            "My results",
            "Log out"
        };

        while (true)
        {
            const int choice = ConsoleMenu::Menu::verticalMenu(
                "   USER  MENU   ",
                options,
                ConsoleMenu::HorizontalAlignment::Center);

            switch (choice)
            {
            case 0:
                takeTest();
                break;
            case 1:
                continueTest();
                break;
            case 2:
                showProfile();
                break;
            case 3:
                showResults();
                break;
            case 4:
            default:
                return;
            }
        }
    }
}