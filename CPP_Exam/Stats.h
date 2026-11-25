#pragma once
#include"Admin.h"
#include"User.h"

namespace Admin
{
    class Stats
    {
        sqlite3* db = nullptr;

        int chooseId(const char* sql, int parentId, const char* title);

        void showReport(const char* sql, int id, const char* title);

        void byCategory();

        void byTest();

        void byUser();

    public:
        explicit Stats(sqlite3* database) : db(database) {}

        void Menu();
    };
}
