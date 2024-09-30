/*
 * Priority task pool
 * Implemented by C++
 *
 * Author wanch
 * Date 2024/7/15
 * Email wzhhnet@gmail.com
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <unistd.h>
#include <iostream>
#include "priority_task_pool.h"

std::atomic<int> idx(0);
/// a function
static int AddFunc(int x, int y)
{
   std::cout << ++idx << ". Add";
   std::cout << " x=" << x << " y=" << y;
   std::cout << std::endl;
   usleep(100);
   return x+y;
}

/// a function object class
struct SubFunc
{
    short y = 2;
    long operator() (short x) {
	std::cout << ++idx << ". Sub";
	std::cout << " x=" << x << " y=" << y;
	std::cout << std::endl;
	usleep(100);
        return x-y;
    }
};

/// a class with function member
struct MulFunc
{
    int mul(int x, int y) {
    	std::cout << ++idx << ". Mul";
	std::cout << " x=" << x << " y=" << y;
	std::cout << std::endl;
	usleep(100);
	return x*y;
    }
};

int main(int argc, char **argv)
{
    std::cout << "PTPL TEST" << std::endl;
    std::cout << "====================" << std::endl;
    utils::TaskPool tp(1);

    MulFunc mf;
    /// a lambda object
    auto divfunc = [](int x, int y) -> float {
	std::cout << ++idx << ". Div";
	std::cout << " x=" << x << " y=" << y;
	std::cout << std::endl;
	usleep(100);
	return x/y;
    };
    /// put callable object into taskpool
    auto f1 = tp.enqueue(0, &AddFunc, 1, 2);
    auto f2 = tp.enqueue(0, SubFunc(), 3);
    auto f3 = tp.enqueue(1, &MulFunc::mul, &mf, 2, 3);
    auto f4 = tp.enqueue(1, divfunc, 8, 2);

    sleep(1);
    std::cout << "====================" << std::endl;
    std::cout << "+>" << f1.get() << std::endl;
    std::cout << "->" << f2.get() << std::endl;
    std::cout << "*>" << f3.get() << std::endl;
    std::cout << "/>" << f4.get() << std::endl;

    return 0;
}
