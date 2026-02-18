#if 0
#include <thread>
#include <iostream>
#include <memory>
#include <chrono>
using namespace std;

//test2 >> err
struct func {
    int& _i;
    func(int& i) : _i(i) {}
    void operator()() {
        for (int i = 0; i < 3; i++) {
            _i = i;
            std::cout << "_i is " << _i << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
};

void oops() {
    int some_local_state = 0;
    func myfunc(some_local_state);
    std::thread functhread(myfunc);
    //隐患，访问局部变量，局部变量可能会随着}结束而回收或随着主线程退出而回收(线程分离)
    functhread.detach();
}

//test2 >> 伪闭包 >> 引用计数3
// 如果此时主线程结束，智能指针会自动回收资源，子线程访问智能指针时会发现资源已经被回收了，所以是安全的;
// 此时的引用计数就是1
struct func1 {
    std::shared_ptr<int> _i;  // 用智能指针持有
    func1(std::shared_ptr<int> i) : _i(i) {}
    void operator()() {
        for (int j = 0; j < 3; j++) {
            *_i = j;
            std::cout << "_i is " << *_i << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
};

void example_shared_ptr() {
	//auto some_loacl_state = std::shared_ptr<int>(new int(0)); // 用shared_ptr管理
    auto some_local_state = std::make_shared<int>(0); // 用shared_ptr管理
    func1 myfunc(some_local_state);
    std::thread t(myfunc);
    t.detach(); // 可以安全detach
}

//test2 >> 使用值传递替代引用传递
struct func2 {
    int _i; // 保存副本

    func2(int i) : _i(i) {}

    void operator()() {
        for (int j = 0; j < 3; j++) {
            _i = j;
            std::cout << "_i is " << _i << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
};

void example_copy() {
    int some_local_state = 0;
    func2 myfunc(some_local_state); // 拷贝构造
    std::thread t(myfunc);
    t.detach(); // 可以detach，线程只操作自己的副本
}

//test2 >> 使用join替代detach
struct func3 {
    int& _i; // 仍然是引用

    func3(int& i) : _i(i) {}

    void operator()() {
        for (int j = 0; j < 3; j++) {
            _i = j;
            std::cout << "_i is " << _i << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
};

void example_join() {
    int some_local_state = 0;
    func3 myfunc(some_local_state);
    std::thread t(myfunc);
    t.join(); // 等线程结束再返回
}

//test7
void threadFunc(const int& x) { ... }

void mainThread() {
    int localVal = 10;
    std::thread t(threadFunc, localVal); // 假设这里传的是引用
    t.detach();
} // localVal 在这里被销毁！
//如果 t 仅仅持有了 localVal 的引用，当 mainThread 结束时 localVal 被销毁。此时子线程如果尝试访问这个引用，就会导致未定义行为(Undefined Behavior)。

void test1() {
	//test2 >> err
	oops();
}

void test2() {
    //test2 >> 伪闭包
	example_shared_ptr();
}

void test3() {
    //test2 >> 使用值传递替代引用传递
    example_copy();
}

void test4() {
    //test2 >> 使用join替代detach
    example_join();
}

int main() {
    //test1();
    //test2();
    //test3();
    test4();

	return 0;
}


#endif