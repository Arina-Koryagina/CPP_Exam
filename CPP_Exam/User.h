#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <format>
#include "sqlite3.h"
#include "ConsoleMenu.h"
#include "Settings.h"
#include "Func.h"
#include "Tests.h"

namespace User
{
    class User
    {
        sqlite3* db = nullptr;
        int userId  = -1;

        int choosePausedAttempt();

        void takeTest();

        void continueTest();

        void runAttempt(int attemptId);

        void showAttemptResult(int attemptId);

    public:
        User(sqlite3* connection, int id) : db(connection), userId(id) {}

        void showProfile();

        void showResults();

        void Menu();
    };
}
