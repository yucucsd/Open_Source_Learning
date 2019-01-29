#include <string>
#include <iostream>
#include <unistd.h>
using namespace std;

int main()
{
	char cwd[1024];
	getcwd(cwd, sizeof(cwd));
	string abspath(cwd);
	cout << abspath << endl;
	abspath.pop_back();
	cout << abspath << endl;
}
