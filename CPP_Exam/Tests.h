#pragma once
#include <iostream>
#include <vector>
#include <string>
#include "sqlite3.h"
#include "Func.h"
#include "ConsoleMenu.h"
#include "FileHandler.h"

namespace Test
{
    int chooseCategory(sqlite3* db);

    int chooseTest(sqlite3* db, int categoryId);
}

namespace Admin
{
    class Tests
    {
        sqlite3* db = nullptr;

        void showTest(int testId);

        void show();

        bool readField(const char* prompt, std::string& value);

        int chooseQuestion(int testId);

        int nextPosition(const char* sql, int parentId);

        bool updateText(
            const char* sql,
            const std::string& value,
            int id,
            const char* successMessage);

        int chooseAnswer(int questionId);

        void addCategory();

        void addTest();

        void addQuestion();

        void addAnswer();
        void addAnswer(int categoryId, int testId, int questionId);

        void addMenu();

        void editCategory();

        void editTest();

        void editQuestion();

        void editAnswer();

        void editMenu();

        void deleteMenu();

    public:
        explicit Tests(sqlite3* database) : db(database) {}

        void Menu();
    };
}
