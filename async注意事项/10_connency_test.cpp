#include <iostream>
#include <future>
//test1
class TestCopy {
public:
    TestCopy() {}
    TestCopy(const TestCopy& tp) {
        std::cout << "Test Copy Copy " << std::endl;
    }
    TestCopy(TestCopy&& cp) noexcept {
        std::cout << "Test Copy Move " << std::endl;
    }
};
TestCopy TestCp() {
    TestCopy tp;
    return tp;
}

//test2
void BlockAsync() {
    std::cout << "begin block async" << std::endl;
    {
        std::future<void> futures = std::async(std::launch::async, []() {
            std::this_thread::sleep_for(std::chrono::seconds(3));
            std::cout << "std::async called " << std::endl;
            });
	}//出作用域，futures被销毁，底层调用一系列的析构最终的操作是等待任务执行 >> 也就变成了同步了
    std::cout << "end block async" << std::endl;
}

//test3
void DeadLock() {//死锁 >> 互相等待对方释放锁 >> 原因是std::async的线程在等待主线程释放锁，而主线程在等待std::async的线程执行完任务
    std::mutex  mtx;
    std::cout << "DeadLock begin " << std::endl;
    std::lock_guard<std::mutex>  dklock(mtx);
    {
        std::future<void> futures = std::async(std::launch::async, [&mtx]() {
            std::cout << "std::async called " << std::endl;
            std::lock_guard<std::mutex>  dklock(mtx);
            std::cout << "async working...." << std::endl;
            });
    }
    std::cout << "DeadLock end " << std::endl;
}

//test4
//需求是func1中要异步执行asyncFunc函数。
//func2中先收集asyncFunc函数运行的结果，只有结果正确才执行
//func1启动异步任务后继续执行，执行完直接退出不用等到asyncFunc运行完
int asyncFunc() {
    std::this_thread::sleep_for(std::chrono::seconds(3));
    std::cout << "this is asyncFunc" << std::endl;
    return 0;
}
void func1(std::future<int>& future_ref) {
    std::cout << "this is func1" << std::endl;
    future_ref = std::async(std::launch::async, asyncFunc);
}
void func2(std::future<int>& future_ref) {
    std::cout << "this is func2" << std::endl;
    auto future_res = future_ref.get();
    if (future_res == 0) {
        std::cout << "get asyncFunc result success !" << std::endl;
    }
    else {
        std::cout << "get asyncFunc result failed !" << std::endl;
        return;
    }
}

void first_method() {
    std::future<int> future_tmp;
    func1(future_tmp);
    func2(future_tmp);
}

//test5 >> 纯异步
template<typename Func, typename... Args  >
auto  ParallenExe(Func&& func, Args && ... args) -> std::future<decltype(func(args...))> {
    typedef decltype(func(args...)) RetType;
    std::function<RetType()>  bind_func = std::bind(std::forward<Func>(func), std::forward<Args>(args)...);
    std::packaged_task<RetType()> task(bind_func);
    auto rt_future = task.get_future();
    std::thread t(std::move(task));
    t.detach();
    return rt_future;
}

void TestParallen1() {
    int i = 0;
    std::cout << "Begin TestParallen1 ..." << std::endl;
    {
        ParallenExe([](int i) {
            while (i < 3) {
                i++;
                std::cout << "ParllenExe thread func " << i << " times" << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            }, i);
    }
    std::cout << "End TestParallen1 ..." << std::endl;
}

void TestParallen2() {
    int i = 0;
    std::cout << "Begin TestParallen2 ..." << std::endl;
    auto rt_future = ParallenExe([](int i) {
        while (i < 3) {
            i++;
            std::cout << "ParllenExe thread func " << i << " times" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        }, i);
    std::cout << "End TestParallen2 ..." << std::endl;
    rt_future.wait();
}

void test1() {
    TestCp();
}

void test2() {
    BlockAsync();
}

void test3() {
    DeadLock();
}

void test4() {
    first_method();
}

void test5() {
    TestParallen1();
}   

void test6() {
    TestParallen2();
}

int main() {
    //test1();
    //test2();
    //test3();
    //test4();
 //   test5();
	//std::this_thread::sleep_for(std::chrono::seconds(3));

    test6();
    std::cout << "Main Exited\n";

    return 0;
}