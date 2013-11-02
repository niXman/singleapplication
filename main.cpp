#include <singleapplication.hpp>

#include <iostream>

/***************************************************************************/

int main2(int, char**) {
	std::cout << "from main2()" << std::endl;

	return 0;
}

/***************************************************************************/

int main(int argc, char** argv) {
	int retcode = 0;
	try {
		single_application app(argc, argv, main2);
		retcode = app.exec();
	} catch (const std::exception& ex) {
		std::cout << "[exception] \"" << ex.what() << "\"" << std::endl;
		retcode = 1;
	}

	return retcode;
}

/***************************************************************************/
