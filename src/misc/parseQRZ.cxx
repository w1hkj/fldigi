#include <ctime>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <string>
#include <iostream>
#include <fstream>

using namespace std;
using std::cout;
using std::cin;

int main(int argc, char *argv[])
{
	size_t pos = 0, pos2 = 0, pos3 = 0, pos4 = 0, pos5 = 0;
	
	string commandline = "";
	string name = "";
	string qth = "";
	string answer = "";
	
	char c = cin.get();
    while (!cin.eof()) {        
		commandline += c;
		c = cin.get();
	}

	pos = commandline.find("calldata");
	if (pos == string::npos)
		goto noresponse;

	pos += 31;
//	cout << pos << endl;

	pos2 = commandline.find("<br", pos);
	if (pos2 == string::npos)
		goto noresponse;
		
	name = commandline.substr(pos, pos2 - pos);

	while (name[0] <= ' ' && !name.empty()) name.erase(0,1);
    
	pos3 = name.find(32);
	if (pos3 != string::npos)
		name = name.substr(0, pos3);

	for (size_t i = 1; i < name.length(); i++) name[i] = tolower(name[i]);

	answer = "$NAME";
	answer.append(name);
	
	pos5 = commandline.find("</span", pos2);

	pos5 = commandline.rfind( "<br", pos5);

	pos4 = commandline.rfind( 9, pos5 - 2);
	
	qth = commandline.substr(pos4, pos5 - pos4);
	
	while (qth[0] <= ' ' && !qth.empty()) qth.erase(0,1);
	
	answer.append("$QTH");
	answer.append(qth);
	cout << answer.c_str();
	
	return 0;

noresponse:
	cout << "$NAME?$QTH?";
	return 0;
}

