#if 0

#include <mutex>
#include <thread>
#include <iostream>
#include <condition_variable>
#include <queue>
using namespace std;

//test1 >> 如果B处理完了此时的A还在睡眠，是对资源的浪费，也错过了最佳的处理时机
std::mutex mtx_num;
int num = 1;
void PoorImpleman() {
    std::thread t1([]() {
        for (;;) {
            {
                std::lock_guard<std::mutex> lock(mtx_num);
                if (num == 1) {
                    std::cout << "thread A print 1....." << std::endl;
                    num++;
                    continue;
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
        });
    std::thread t2([]() {
        for (;;) {
            {
                std::lock_guard<std::mutex> lock(mtx_num);
                if (num == 2) {
                    std::cout << "thread B print 2....." << std::endl;
                    num--;
                    continue;
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
        });
    t1.join();
    t2.join();
}

//test2 >> 通过条件变量来实现线程间的通信，避免了资源的浪费和错过最佳处理时机的问题
std::condition_variable cvA, cvB;
void ResonableImplemention() {
    std::thread t1([]() {
        for (;;) {
            std::unique_lock<std::mutex> lock(mtx_num);
   //         while(num != 1) {
   //             cvA.wait(lock);
			//}
            cvA.wait(lock, []() {
                return num == 1;
                });
            num++;
            std::cout << "thread A print 1....." << std::endl;
            cvB.notify_one();
        }
        });
    std::thread t2([]() {
        for (;;) {
            std::unique_lock<std::mutex> lock(mtx_num);
            cvB.wait(lock, []() {
                return num == 2;
                });
            num--;
            std::cout << "thread B print 2....." << std::endl;
            cvA.notify_one();
        }
        });
    t1.join();
    t2.join();
}

//test3 >> 生产者消费者模型，使用条件变量来实现线程间的通信，避免了资源的浪费和错过最佳处理时机的问题
template<typename T>
class threadsafe_queue {
private:
    mutable std::mutex mut;
    std::queue<T> data_queue;
    std::condition_variable data_cond;

public:
    threadsafe_queue(){}
    threadsafe_queue(const threadsafe_queue& other) {
		std::lock_guard<std::mutex> lk(other.mut);
		data_queue = other.data_queue;
    }
    void push(T new_value) {
        std::lock_guard<std::mutex> lk(mut);
		data_queue.push(new_value);
		data_cond.notify_one();
    }
    //原始写法
    T pop() {
        std::unique_lock<std::mutex> lk(mut);
        data_cond.wait(lk, [this] { return !data_queue.empty(); });
		//因为这里value是创建的因此需要拷贝构造
        T value = data_queue.front();  // 拷贝构造
        data_queue.pop();
        return value;                  // 可能再拷贝/移动
    }
    //&写法
    void wait_and_pop(T& value) {
		std::unique_lock<std::mutex> lk(mut);   
		data_cond.wait(lk, [this] { return !data_queue.empty(); });
		//这里的value是外部传入的，因此不需要拷贝构造，直接赋值即可
		value = data_queue.front();//这里没有拷贝构造，直接赋值
		data_queue.pop();
    }
	//智能指针写法
    std::shared_ptr<T> wait_and_pop() {
        std::unique_lock<std::mutex> lk(mut);
        data_cond.wait(lk, [this] { return !data_queue.empty(); });
        std::shared_ptr<T> res(std::make_shared<T>(data_queue.front()));
        data_queue.pop();
        return res;
    }
    bool try_pop(T& value) {
        std::lock_guard<std::mutex> lk(mut);
        if (data_queue.empty()) {
            return false;
        }
        value = data_queue.front();
        data_queue.pop();
        return true;
	}
    std::shared_ptr<T> try_pop() {
        std::lock_guard<std::mutex> lk(mut);
        if (data_queue.empty()) {
            return std::shared_ptr<T>();
        }
        std::shared_ptr<T> res(std::make_shared<T>(data_queue.front()));
        data_queue.pop();
        return res;
	}
    bool empty() const {
        std::lock_guard<std::mutex> lk(mut);
        return data_queue.empty();
	}
};

void test_safe_que() {
    threadsafe_queue<int> safe_que;
    std::mutex mtx_print;
    std::thread producer(
        [&]() {
            for (int i = 0; ; i++) {
                safe_que.push(i);
                {
                    std::lock_guard<std::mutex> printlk(mtx_print);
                    std::cout << "producer push data is " << i << std::endl;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
            }
        }
    );
    std::thread consumer1(
        [&]() {
            for (;;) {
                auto data = safe_que.wait_and_pop();
                {
                    std::lock_guard<std::mutex> printlk(mtx_print);
                    std::cout << "consumer1 wait and pop data is " << *data << std::endl;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
        }
    );
    std::thread consumer2(
        [&]() {
            for (;;) {
                auto data = safe_que.try_pop();
                if (data != nullptr) {
                    {
                        std::lock_guard<std::mutex> printlk(mtx_print);
                        std::cout << "consumer2 try_pop data is " << *data << std::endl;
                    }
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
        }
    );
    producer.join();
    consumer1.join();
    consumer2.join();
}

void test1() {
	PoorImpleman();
}

void test2() {
    ResonableImplemention();
}

void test3() {
    test_safe_que();
}

int main() {
    //test1();
    //test2();
    test3();

    return 0;
}
#endif