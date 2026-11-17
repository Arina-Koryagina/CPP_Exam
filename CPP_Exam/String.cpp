#include"String.h"

namespace exam
{
	String::String() : String(80) {}
	String::String(int l)
	{
		clear(l);
	}
	String::String(const char* line)
	{
		input(line);
	}
	String::String(const String& obj)
	{
		size = obj.size;
		capacity = obj.capacity;
		str = new char[capacity + 1];
		strcpy(str, obj.str);
	}


	String& String::operator=(const String& obj)
	{
		if (this == &obj)
		{
			return *this;
		}

		delete[] str;

		size = obj.size;
		capacity = obj.capacity;
		str = new char[capacity + 1];
		strcpy(str, obj.str);

		return *this;
	}

	String String::operator+(const String& obj) const
	{
		char* temp = new char[size + obj.size + 1];
		strcpy(temp, str);
		strcpy(temp + size, obj.str);

		String st(temp);
		delete[] temp;
		return st;
	}

	String String::operator*(int times) const
	{
		if (times > 0)
		{
			char* temp = new char[size * times + 1];

			for (int i = 0; i < times; i++)
			{
				strcpy(temp + size * i, str);
			}

			String st(temp);
			delete[] temp;
			return st;
		}
		return "";
	}

	String& String::operator+=(const String& obj)
	{
		*this = *this + obj;
		return *this;
	}

	String& String::operator*=(int times)
	{
		*this = *this * times;
		return *this;
	}


	const char String::operator[](int index) const
	{
		assert(index >= 0 && index < size);
		return str[index];
	}
	char& String::operator[](int index)
	{
		assert(index >= 0 && index < size);
		return str[index];
	}

	void String::operator()(const char* line)
	{
		input(line);
	}
	void String::operator()(const String& obj)
	{
		*this = obj;
	}


	bool String::operator==(const String& obj) const
	{
		return strcmp(str, obj.str) == 0;
	}
	bool String::operator!=(const String& obj) const
	{
		return !(*this == obj);
	}
	bool String::operator<(const String& obj) const
	{
		if (strcmp(str, obj.str) < 0)
		{
			return true;
		}
		return false;
	}
	bool String::operator>(const String& obj) const
	{
		return !(*this < obj || *this == obj);
	}
	bool String::operator<=(const String& obj) const
	{
		return (*this < obj || *this == obj);
	}
	bool String::operator>=(const String& obj) const
	{
		return (*this > obj || *this == obj);
	}


	String String::operator+(int value) const
	{
		char digits[12];
		int count = 0;

		do
		{
			digits[count++] = '0' + value % 10;
			value /= 10;
		} while (value > 0);

		char text[12];
		for (int i = 0; i < count; ++i)
		{
			text[i] = digits[count - 1 - i];
		}
		text[count] = '\0';

		return *this + String(text);
	}

	String& String::operator+=(int value)
	{
		*this = *this + value;
		return *this;
	}


	std::ostream& operator<<(std::ostream& out, const String& s)
	{
		out << s.str;
		return out;
	}

	std::istream& operator>>(std::istream& in, String& s)
	{
		char word[80];
		in >> word;
		if (in)
		{
			s.input(word);
		}
		return in;
	}


	String::operator char* () const
	{
		return str;
	}
	String::operator int() const
	{
		return atoi(str);
	}


	String::~String()
	{
		delete[] str;
	}


	void String::clear(int l)
	{
		delete[] str;

		capacity = l;
		size = 0;
		str = new char[capacity + 1];
		str[0] = '\0';
	}
	void String::clear()
	{
		clear(80);
	}


	void String::input()
	{
		char buffer[256];
		std::cin.getline(buffer, 256);
		input(buffer);
	}
	void String::input(const char* line)
	{
		if (line == nullptr)
		{
			line = "";
		}

		int newSize = lenStr(line);
		char* newStr = new char[newSize + 1];
		strcpy(newStr, line);

		delete[] str;
		str = newStr;
		size = newSize;
		capacity = newSize;
	}


	int String::getLen() const
	{
		return size;
	}

	const char* String::getStr() const
	{
		return str;
	}


	void String::print() const
	{
		std::cout << str << std::endl;
	}


	void String::reverse()
	{
		for (int i = 0; i < size / 2; i++)
		{
			char c = str[size - 1 - i];
			str[size - 1 - i] = str[i];
			str[i] = c;
		}
	}
}