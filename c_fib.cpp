#include <iostream>

extern "C" {

int fibonacci(int n) {
    if (n < 3) return 1;
    return fibonacci(n-1) + fibonacci(n-2);
}
}

//int main(int argc, char **argv) {
//    int n = argc > 1 ? atol(argv[1]) : 24;
//    int res = fibonacci(n);
//    std::cout << "fibonacci result: " << res << std::endl;
//    return 0;
//}
