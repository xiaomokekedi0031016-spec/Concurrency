#if 0 
#include <iostream>
#include <future>
#include <chrono>
#include <thread>

//test1
// 定义一个异步任务
std::string fetchDataFromDB(std::string query) {
    // 模拟一个异步任务，比如从数据库中获取数据
    std::this_thread::sleep_for(std::chrono::seconds(5));
    return "Data: " + query;
}

void use_async() {
    // 使用 std::async 异步调用 fetchDataFromDB
	// 这里实际上是创建了一个新的线程来执行 fetchDataFromDB 函数
    std::future<std::string> resultFromDB = std::async(std::launch::async, fetchDataFromDB, "Data");

    // 在主线程中做其他事情
    std::cout << "Doing something else..." << std::endl;

    // 从 future 对象中获取数据
    std::string dbData = resultFromDB.get(); 
    std::cout << dbData << std::endl;
}

//test2
int my_task() {
    std::this_thread::sleep_for(std::chrono::seconds(5));
    std::cout << "my task run 5 s" << std::endl;
    return 42;
}

void use_package() {
    // 创建一个包装了任务的 std::packaged_task 对象  
	//<int()> 表示任务的类型，即一个不接受参数但返回 int 的函数
    std::packaged_task<int()> task(my_task);

    // 获取与任务关联的 std::future 对象  
    std::future<int> result = task.get_future();

    // 在另一个线程上执行任务  
    std::thread t(std::move(task));
    t.detach(); // 将线程与主线程分离，以便主线程可以等待任务完成  

    // 等待任务完成并获取结果  
    int value = result.get();
    std::cout << "The result is: " << value << std::endl;

}

//test3
//promise优势是线程获取值的时机更灵活，不需要等待线程执行完毕，可以在任意时刻获取值，而future必须等线程执行完毕才能获取值
void set_value(std::promise<int> prom) {
    // 设置 promise 的值
    prom.set_value(10);
    std::this_thread::sleep_for(std::chrono::seconds(5));
}

void use_promise() {
    // 创建一个 promise 对象
    std::promise<int> prom;
    // 获取与 promise 相关联的 future 对象
    std::future<int> fut = prom.get_future();
    // 在新线程中设置 promise 的值
    std::thread t(set_value, std::move(prom));
    // 在主线程中获取 future 的值(子线程的执行结果了)
    std::cout << "Waiting for the thread to set the value...\n";
    std::cout << "Value set by the thread: " << fut.get() << '\n';
    t.join();
}

//test4 promise 还可以用来传递异常，子线程抛出的异常可以在主线程中捕获到
void set_exception(std::promise<void> prom) {
    try {
        // 抛出一个异常
        throw std::runtime_error("An error occurred!");
    }
    catch (...) {
        // 设置 promise 的异常
        prom.set_exception(std::current_exception());
    }
}

void use_exception_promise() {
    // 创建一个 promise 对象
    std::promise<void> prom;
    // 获取与 promise 相关联的 future 对象
    std::future<void> fut = prom.get_future();
    // 在新线程中设置 promise 的异常
    std::thread t(set_exception, std::move(prom));
	// 在主线程中获取future的异常 >> 子线程抛出的异常需要主线程try-catch捕获，否则会导致程序异常退出
    try {
        std::cout << "Waiting for the thread to set the exception...\n";
        fut.get();
    }
    catch (const std::exception& e) {
        std::cout << "Exception set by the thread: " << e.what() << '\n';
    }
    t.join();
}

//test5 >> 安全因为promise对象在子线程中被移动了 >> promise要是活得
void use_promise_destruct() {
    std::thread t;
    std::future<int> fut;
    {
        // 创建一个 promise 对象
        std::promise<int> prom;
        // 获取与 promise 相关联的 future 对象
        fut = prom.get_future();
        // 在新线程中设置 promise 的值
        t = std::thread(set_value, std::move(prom));
    }
    // 在主线程中获取 future 的值
    std::cout << "Waiting for the thread to set the value...\n";
    std::cout << "Value set by the thread: " << fut.get() << '\n';
    t.join();
}

//test6
//当我们需要多个线程等待同一个执行结果时，需要使用std::shared_future
void myFunction(std::promise<int>&& promise) {
    // 模拟一些工作
    std::this_thread::sleep_for(std::chrono::seconds(1));
    promise.set_value(42); // 设置 promise 的值
}

void threadFunction(std::shared_future<int> future) {
    try {
        int result = future.get();
        std::cout << "Result: " << result << " " << std::endl;
    }
    catch (const std::future_error& e) {
        std::cout << "Future error: " << e.what() << std::endl;
    }
}

void use_shared_future() {
    std::promise<int> promise;
	//隐式转换为 shared_future 对象，多个线程可以共享同一个 future 对象
    std::shared_future<int> future = promise.get_future();

    std::thread myThread1(myFunction, std::move(promise)); // 将 promise 移动到线程中

    // 使用 share() 方法获取新的 shared_future 对象  
    //shared_future是可以复制的
    std::thread myThread2(threadFunction, future);
    std::thread myThread3(threadFunction, future);
    //err
    //std::thread myThread2(threadFunction, std::move(future));
    //std::thread myThread3(threadFunction, std::move(future));

    myThread1.join();
    myThread2.join();
    myThread3.join();
}

//test7 
//在std::future中获取异常
void may_throw()
{
    // 这里我们抛出一个异常。在实际的程序中，这可能在任何地方发生。
    throw std::runtime_error("Oops, something went wrong!");
}

void use_future_exception() {
    // 创建一个异步任务may_throw
    std::future<void> result(std::async(std::launch::async, may_throw));
    try
    {
        // 获取结果（如果在获取结果时发生了异常，那么会重新抛出这个异常）
        result.get();
    }
    catch (const std::exception& e)
    {
        // 捕获并打印异常
        std::cerr << "Caught exception: " << e.what() << std::endl;
    }

}

void test1() {
	use_async();
}

void test2() {
    use_package();
}

void test3() {
    use_promise();
}   

void test4() {
	use_exception_promise();
}

void test5() {
    use_promise_destruct();
}

void test6() {
    use_shared_future();
}

void test7() {
    use_future_exception();
}

int main() {
    //test1();
    //test2();
    //test3();
    //test4();
    //test5();
    //test6();
    //test7();

    return 0;
}

#endif