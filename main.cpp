#include "program.hpp"
int main(int argc, char **argv)
{
	try
	{
		Program program(argc, argv);
		program.Execute();
		return 0;
	}
	catch (const std::exception &e)
	{
		std::cerr << e.what() << '\n';
		return -1;
	}
}