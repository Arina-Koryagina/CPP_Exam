#include "TestSystemApp.h"

int main()
{
	SetConsoleCP(65001);
	SetConsoleOutputCP(65001);
	SetConsoleTitleA("Testing System");

	test::TestSystemApp app;
	app.menu();

	return 0;
}