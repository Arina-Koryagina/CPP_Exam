#pragma once
#include <iostream>
#include <conio.h>
#include <string>
#include "sqlite3.h"
#include "ConsoleMenu.h"

namespace User
{
    class User
    {
    public:
        static void Menu(sqlite3* db, int userId)
        {
            std::vector<std::string> options = { "Tests", "Your info", "Statistics", "Log out" };

            while (true)
            {
                int choice = ConsoleMenu::Menu::verticalMenu("   USER  MENU   ", options, ConsoleMenu::HorizontalAlignment::Center);
                switch (choice)
                {
                case 0:
                    // Show available tests for userId
                    break;
                case 1:
                    // Load this user's profile using userId
                    break;
                case 2:
                    // Load this user's results using userId
                    break;
                case 3:
                default:
                    return;
                }
            }
        }
    };
}
