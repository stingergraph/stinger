#include <iostream>
#include <vector>
#include <string>

using namespace std;

string stringstrip(string str)
{
	string out = str;
	string whitespaces (" \t\f\v\n\r");
	size_t white = string::npos;
	white = out.find_first_not_of(whitespaces);
	if (white!=0) out.erase(0,white);
	white = string::npos;
	white = out.find_last_not_of(whitespaces);
	if (white!=string::npos) out.erase(white+1);
	return out;
}

vector<string> stringsplit(string str, char delim=' ')
{
    vector<string> out;
    string temp = stringstrip(str);
    size_t pos = string::npos;
    pos = temp.find_first_of(delim);
    while(pos != -1)
    {
        string elem = temp.substr(0, pos);
        out.push_back(elem);
        temp.erase(0, pos+1);
        pos = string::npos;
        pos = temp.find_first_of(delim);
    }
    if(temp.length() > 0) out.push_back(temp);
    return out;
}
