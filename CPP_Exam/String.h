#pragma once
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <cassert>

class String
{
	char* str      = nullptr;
	int   size     = 0;
	int   capacity = 0;

	int lenStr(const char* str) const
	{
		int i = 0;
		if (str != nullptr)
		{
			while (str[i] != '\0')
			{
				i++;
			}
		}
		return i;
	}

public:
	String();
	String(int l);
	String(const char* line);
	String(const String& obj);

	String&    operator=(const String& obj);

	String     operator+(const String& obj)  const;
	String     operator*(int times)          const;
	String&    operator+=(const String& obj);
	String&    operator*=(int times);

	const char operator[](int index)         const;
	char&      operator[](int index);
	void       operator()(const char* line);
	void       operator()(const String& obj);

	bool       operator==(const String& obj) const;
	bool       operator!=(const String& obj) const;
	bool       operator<(const String& obj)  const;
	bool       operator>(const String& obj)  const;
	bool       operator<=(const String& obj) const;
	bool       operator>=(const String& obj) const;

	String     operator+(int value)          const;
	String&    operator+=(int value);

	friend std::ostream& operator<<(std::ostream& out, const String& s);
	friend std::istream& operator>>(std::istream& in, String& s);

	operator char* () const;
	operator int   () const;

	~String();

	void        clear();
	void        clear(int l);

	void        input();
	void        input(const char* line);

	const char* getStr() const;
	int         getLen() const;

	void        print()  const;

	void        reverse();
};