#pragma once

#include"Time.h"
#include"String.h"

class Date
{
	int day;
	int month;
	int year;
	Time time;

public:
	Date();
	Date(int d, int m, int y, Time t);
	Date(String s);

	String formDate() const;

	friend std::ostream& operator<<(std::ostream& out, const Date& obj);
};