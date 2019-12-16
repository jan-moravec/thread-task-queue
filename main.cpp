#include <iostream>
#include <string>
#include <set>
#include <cassert>

#include "taskqueue.h"

void test1();
void test2();

int main()
{
    std::cout << "Starting test 1" << std::endl;
    test1();

    std::cout << "Starting test 2" << std::endl;
    test2();

    std::cout << "Finished" << std::endl;
    return 0;
}

void test1()
{
    DataProcessQueue<std::string> queue([](const std::string &data){
        std::cout << data << std::endl;
    });

    queue.push("Hello");
    queue.push("Hi");
    std::this_thread::sleep_for(std::chrono::seconds(1));

    queue.push({"How", "are", "you"});
}

void test2()
{
    const unsigned max = 10'000'000;
    std::set<unsigned> data;
    for (unsigned i = 0; i < max; ++i) {
        data.insert(i);
    }

    {
        DataProcessQueue<unsigned> queue([&](const unsigned &number){
            data.erase(number);
        });

        std::thread t0([&]{
            for (unsigned i = 0; i < max / 2; ++i) {
                queue.push(i);
            }
        });

        std::thread t1([&]{
            for (unsigned i = max / 2; i < max; ++i) {
                queue.push(i);
            }
        });

        t0.join();
        t1.join();
    }

    assert(data.size() != 0);
}
