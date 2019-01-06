#include <iostream>

int main() {
	try {
		std::cout << "hw_eghth" << std::endl;
	} catch(const std::exception &e) {
		std::cerr << e.what() << std::endl;
	}
	return 0;
}
