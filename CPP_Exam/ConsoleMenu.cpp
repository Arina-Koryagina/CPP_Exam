#include "ConsoleMenu.h"
#include "Func.h"

int ConsoleMenu::Menu::textColor = Settings::textColorConsole;
int ConsoleMenu::Menu::backColor = Settings::backColorConsole;
int ConsoleMenu::Menu::widthConsole = Settings::widthConsole;
int ConsoleMenu::Menu::heightConsole = Settings::heightConsole;
HANDLE ConsoleMenu::Menu::hStdOut = nullptr;
CONSOLE_CURSOR_INFO ConsoleMenu::Menu::structCursorInfo = CONSOLE_CURSOR_INFO();

void ConsoleMenu::frame(int width, int height, int xPos, int yPos)
{
	Func::SetCursor(xPos, yPos);
	SetConsoleOutputCP(866);
	std::cout << (char)201;
	for (int i = 0; i < width - 2; i++)
		std::cout << (char)205;
	std::cout << (char)187;

	for (int j = 0; j < height - 2; j++)
	{
		Func::SetCursor(xPos, yPos + 1 + j);
		std::cout << (char)186;
		for (int i = 0; i < width - 2; i++)
			std::cout << " ";
		std::cout << (char)186;
	}
	Func::SetCursor(xPos, yPos + height - 1);
	std::cout << (char)200;
	for (int i = 0; i < width - 2; i++)
		std::cout << (char)205;
	std::cout << (char)188;
	SetConsoleOutputCP(65001);
}

void ConsoleMenu::inputFrame(int width, int height, int x, int y, const std::string&& header)
{
	system("cls");
	ConsoleMenu::frame(width, height, x - 5, y - 2);
	Func::SetCursor(x, y);
	std::cout << header;
	Func::SetCursor(x, y + 1);
	for (int i = 0; i < width - 10; i++)
	{
		std::cout << '=';
	}
}

void ConsoleMenu::Menu::init()
{
	SetConsoleOutputCP(65001);
	hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
	GetConsoleScreenBufferInfo(hStdOut, &consoleInfo);

	GetConsoleCursorInfo(hStdOut, &structCursorInfo);
	structCursorInfo.bVisible = FALSE;
	SetConsoleCursorInfo(hStdOut, &structCursorInfo);

	textColor = Settings::textColorConsole;
	backColor = Settings::backColorConsole;
	//textColor = consoleInfo.wAttributes & 15;
	//backColor = (consoleInfo.wAttributes >>= 4) & 15;

	widthConsole = Settings::widthConsole;
	heightConsole = Settings::heightConsole;
	//widthConsole = consoleInfo.dwSize.X;
	//heightConsole = consoleInfo.dwSize.Y;
}
