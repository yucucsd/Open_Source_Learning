#include <stdio.h>

class pp
{
public:
	inline void panic();
};
inline void pp::panic()
{
	//printf("This is not SegFault!\n");
	printf("File: %s, line: %d\n",__FILE__, __LINE__);
}
int main()
{
	pp* t = new pp();
	t->panic();
}
