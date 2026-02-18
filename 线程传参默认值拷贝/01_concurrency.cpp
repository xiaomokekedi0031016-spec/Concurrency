#if 1
#define _CRT_SECURE_NO_WARNINGS
#include <thread>
#include <iostream>
#include <string>

//test1
class background_task {
public:
    void operator()(std::string str) {
        std::cout << "str is " << str << std::endl;
    }
};

//test2
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

//test3
void use_join() {
    int some_local_state = 0;
    func myfunc(some_local_state);
    std::thread functhread(myfunc);
    functhread.join();
}

//test4
//当我们启动一个线程后，如果主线程产生崩溃，会导致子线程也会异常退出，就是调用terminate，如果子线程在进行一些重要的操作比如将充值信息入库等，丢失这些信息是很危险的。所以常用的做法是捕获异常，并且在异常情况下保证子线程稳定运行结束后，主线程抛出异常结束运行。如下面的逻辑
void catch_exception() {
    int some_local_state = 0;
    func myfunc(some_local_state);
    std::thread functhread{ myfunc };
    try {
        //本线程做一些事情,可能引发崩溃
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    catch (std::exception& e) {
        functhread.join();
        throw;
    }

    functhread.join();
}

//test5
class thread_guard {
private:
    std::thread& t;

public:
	explicit thread_guard(std::thread& _t) : t(_t) {}
    //RAII
    ~thread_guard() {
        //join是否被调用过
        if (t.joinable()) {
            t.join();
        }
	}
};

void auto_guard() {
    int some_local_state = 0;
    func myfunc(some_local_state);
    std::thread functhread{ myfunc };
    thread_guard tg(functhread);
	std::cout << "auto guard finish" << std::endl;
}

//test6
void print_str(int i, const std::string& str) {
    std::cout << "i is " << i << " str is " << str << std::endl;
}   


//传 buffer -> 传的是“地址” 
//传 std::string(buffer) -> 传的是“内容”所以一个会悬空，一个不会。
//给线程传递参数时，参数会被转化为右值进行存储，所以如果传递的是指针，线程内部访问时可能已经被回收了，所以是危险的；如果传递的是值，线程内部访问时是安全的。
void danger_oops(int som_param) {
    char buffer[1024];
    sprintf(buffer, "%i", som_param);
    //在线程内部将char const* 转化为std::string
    //指针常量  char const*  指针本身不能变
    //常量指针  const char * 指向的内容不能变
    std::thread t(print_str, 3, buffer);
    t.detach();
    std::cout << "danger oops finished " << std::endl;
}

void safe_oops(int some_param) {
    char buffer[1024];
    sprintf(buffer, "%i", some_param);
    std::thread t(print_str, 3, std::string(buffer));
    t.detach();
    std::cout << "safe oops finished " << std::endl;
}

//test7 ***
// 回调函数的参数是引用类型时，传递参数时需要使用std::ref进行显示转换，否则会被转化为右值进行存储，线程内部访问时可能已经被回收了，所以是危险的。
//传递给thread回调函数参数的时候都会转化为右值进行存储
void change_param(int& param) {
    param++;
}

#if 0
void ref_oops(int some_param) {
    std::cout << "before change , param is " << some_param << std::endl;
    //需使用引用显示转换
    std::thread t2(change_param, some_param);//some_param左值 >> err
    t2.join();
    std::cout << "after change , param is " << some_param << std::endl;
}
#endif

void ref_oops(int some_param) {
    std::cout << "before change , param is " << some_param << std::endl;
    //需使用引用显示转换
    std::thread t2(change_param, std::ref(some_param));
    t2.join();
    std::cout << "after change , param is " << some_param << std::endl;
}

//test8
class X
{
public:
    void do_lengthy_work() {
        std::cout << "do_lengthy_work " << std::endl;
    }
};

void bind_class_oops() {
    X my_x;
    //std::thread绑定类的成员函数需要加&
    std::thread t(&X::do_lengthy_work, &my_x);
    //err 
    //std::thread t(X::do_lengthy_work, &my_x);
    t.join();
}

void thead_work1(std::string str) {
    std::cout << "str is " << str << std::endl;
}

//test9
void deal_unique(std::unique_ptr<int> p) {
    std::cout << "unique ptr data is " << *p << std::endl;
    (*p)++;

    std::cout << "after unique ptr data is " << *p << std::endl;
}

void move_oops() {
    auto p = std::make_unique<int>(100);
    std::thread  t(deal_unique, std::move(p));
    t.join();
    //不能再使用p了，p已经被move废弃
   // std::cout << "after unique ptr data is " << *p << std::endl;
}

void test1() {
    std::thread t2{ background_task(),"hello" };
    t2.join();
}

void test2() {
    oops();
	std::this_thread::sleep_for(std::chrono::seconds(1));//给后台线程
}

void test3() {
    use_join();
}

void test4() {
    catch_exception();
}

void test5() {
    auto_guard();
}

void test6() {
	//danger_oops(42);
    //std::this_thread::sleep_for(std::chrono::seconds(1));
	safe_oops(42);
	std::this_thread::sleep_for(std::chrono::seconds(1));
}

void test7() {
    ref_oops(10);
}   

void test8() {
    bind_class_oops();
    std::string hellostr = "hello world!";
    //两种方式都正确
    std::thread t1(thead_work1, hellostr);
    std::thread t2(&thead_work1, hellostr);
    t1.join();
    t2.join();
}

void test9() {
	move_oops();
}

int main01() {
#if 0
    //err
    std::thread t2(background_task());
    t2.join();
#endif
	//test1();
    //test2();
	//test3();
    //test4();
    //test5();
    //test6();
    //test7();
    //test8();
    //test9();

    return 0;
}

#endif