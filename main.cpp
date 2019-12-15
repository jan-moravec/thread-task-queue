#include <iostream>
#include <string>
#include <cassert>

#include "taskqueue.h"

int main()
{
    TaskQueue<std::string> queue;

    queue.push("Hello");
    queue.push("Hi");
    queue.push({"How", "are", "you"});

    std::this_thread::sleep_for(std::chrono::seconds(1));

    return 0;
}
