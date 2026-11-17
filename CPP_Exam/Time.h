#pragma once
#include <iostream>

#include"String.h"

class Time
{
	int hour;
	int minute;
	int second;

public:
	Time();
	Time(int s);
	Time(int m, int s);
	Time(int h, int m, int s);
	Time(String s);

	String formTime() const;

	bool operator==(const Time& obj) const;

	friend std::ostream& operator<<(std::ostream& out, const Time& obj);
};