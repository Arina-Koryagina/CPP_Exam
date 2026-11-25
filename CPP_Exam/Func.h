#pragma once
#include <iostream>
#include <conio.h>
#include <Windows.h>
#include <string>
#include "sqlite3.h"

namespace Func
{
	enum Color
	{
		Black = 0, Blue = 1, Green = 2, Cyan = 3, Red = 4, Magenta = 5, Brown = 6, LightGray = 7, DarkGray = 8,
		LightBlue = 9, LightGreen = 10, LightCyan = 11, LightRed = 12, LightMagenta = 13, Yellow = 14, White = 15
	};

	inline void SetColor(int text, int background)
	{
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), (WORD)((background << 4) | text));
	}

	inline void SetCursor(int x, int y)
	{
		COORD coord;
		coord.X = x;
		coord.Y = y;
		SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
	}

    enum class InputResult { entered, cancelled };

    inline InputResult readConsoleInput(std::string& value, bool mask = false)
    {
        value.clear();

        while (true)
        {
            const int ch = _getch();

            if (ch == 27) // Esc
            {
                std::cout << '\n';
                return InputResult::cancelled;
            }

            // Consume the second byte of arrow and other extended keys.
            if (ch == 0 || ch == 224)
            {
                _getch();
                continue;
            }

            if (ch == '\r') // Enter
            {
                std::cout << '\n';
                return InputResult::entered;
            }

            if (ch == '\b') // Backspace
            {
                if (!value.empty())
                {
                    // Remove a whole UTF-8 character, not just its last byte.
                    std::size_t first = value.size() - 1;
                    while (first > 0 &&
                        (static_cast<unsigned char>(value[first]) & 0xC0) == 0x80)
                    {
                        --first;
                    }

                    value.erase(first);
                    std::cout << "\b \b";
                }
                continue;
            }

            if (ch >= 32 && ch != 127)
            {
                value.push_back(static_cast<char>(ch));
                std::cout << (mask ? '*' : static_cast<char>(ch));
            }
        }
    }

    //sqlite3_stmt* stmt
}