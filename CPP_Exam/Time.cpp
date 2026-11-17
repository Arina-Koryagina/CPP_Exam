#include"Time.h"

namespace exam
{
	Time::Time() : Time(0) {}
	Time::Time(int s) : Time(0, s) {}
	Time::Time(int m, int s) : Time(0, m, s) {}
	Time::Time(int h, int m, int s)
	{
		if (s > 0 && int(s) / 60 > 0)
		{
			m += s / 60;
			s -= m * 60;
			if (m > 0 && m / 60 > 0)
			{
				h += m / 60;
				m -= h * 60;
			}
		}
		hour = h;
		minute = m;
		second = s;
	}

	Time::Time(String s) //s = "HH:MM:SS"
	{
		hour = int(s[0] - '0') * 10 + int(s[1] - '0');
		minute = int(s[3] - '0') * 10 + int(s[4] - '0');
		second = (strlen(s) > 5) ? int(s[6] - '0') * 10 + int(s[7] - '0') : 0;
	}

	String Time::formTime() const
	{
		String time(8);  time = "";
		if (hour < 10)   time += "0";
		time += hour;    time += ":";
		if (minute < 10) time += "0";
		time += minute;  time += ":";
		if (second < 10) time += "0";
		time += second;
		return time;
	}

	bool Time::operator==(const Time& obj) const
	{
		if (hour == obj.hour)
		{
			if (minute == obj.minute)
			{
				if (second == obj.second)
				{
					return true;
				}
			}
		}
		return false;
	}

	std::ostream& operator<<(std::ostream& out, const Time& obj)
	{
		out << obj.formTime();
		return out;
	}
}