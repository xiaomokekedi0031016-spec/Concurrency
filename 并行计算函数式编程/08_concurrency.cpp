#include <iostream>
#include <algorithm>
#include <list>
#include <atomic>
#include <condition_variable>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

//test1
//c++ 版本的快速排序算法
template<typename T>
void quick_sort_recursive(T arr[], int start, int end) {
    if (start >= end) return;
    T key = arr[start];
    int left = start, right = end;
    while (left < right) {
        while (arr[right] >= key && left < right) right--;
        while (arr[left] <= key && left < right) left++;
        std::swap(arr[left], arr[right]);
    }
    if (arr[left] < key) {
        std::swap(arr[left], arr[start]);
    }
    quick_sort_recursive(arr, start, left - 1);
    quick_sort_recursive(arr, left + 1, end);
}
template<typename T>
void quick_sort(T arr[], int len) {
    quick_sort_recursive(arr, 0, len - 1);
}

void test_quick_sort() {
    int num_arr[] = { 5,3,7,6,4,1,0,2,9,10,8 };
    int length = sizeof(num_arr) / sizeof(int);
    quick_sort(num_arr, length);
    std::cout << "sorted result is ";
    for (int i = 0; i < length; i++) {
        std::cout << " " << num_arr[i];
    }
    std::cout << std::endl;
}

//test2函数式编程风格的快速排序算法
template<typename T>
std::list<T> sequential_quick_sort(std::list<T> input)
{
    if (input.empty())
    {
        return input;
    }
    std::list<T> result;
    //  ① 将input中的第一个元素放入result中，并且将这第一个元素从input中删除
    result.splice(result.begin(), input, input.begin());
    //  ② 取result的第一个元素，将来用这个元素做切割，切割input中的列表。
    T const& pivot = *result.begin();
    //  ③std::partition 是一个标准库函数，用于将容器或数组中的元素按照指定的条件进行分区，
    // 使得满足条件的元素排在不满足条件的元素之前。
    // 所以经过计算divide_point指向的是input中第一个大于等于pivot的元素
    auto divide_point = std::partition(input.begin(), input.end(),
        [&](T const& t) {return t < pivot; });
    // ④ 我们将小于pivot的元素放入lower_part中
    std::list<T> lower_part;
    lower_part.splice(lower_part.end(), input, input.begin(),
        divide_point);
    // ⑤我们将lower_part传递给sequential_quick_sort 返回一个新的有序的从小到大的序列
    //lower_part 中都是小于divide_point的值
    auto new_lower(
        sequential_quick_sort(std::move(lower_part)));
    // ⑥我们剩余的input列表传递给sequential_quick_sort递归调用，input中都是大于divide_point的值。
    auto new_higher(
        sequential_quick_sort(std::move(input)));
    //⑦到此时new_higher和new_lower都是从小到大排序好的列表
    //将new_higher 拼接到result的尾部
    result.splice(result.end(), new_higher);
    //将new_lower 拼接到result的头部
    result.splice(result.begin(), new_lower);
    return result;
}

//test3并行计算的快速排序算法
//并行版本
template<typename T>
std::list<T> parallel_quick_sort(std::list<T> input)
{
    if (input.empty())
    {
        return input;
    }
    std::list<T> result;
    result.splice(result.begin(), input, input.begin());
    T const& pivot = *result.begin();
    auto divide_point = std::partition(input.begin(), input.end(),
        [&](T const& t) {return t < pivot; });
    std::list<T> lower_part;
    lower_part.splice(lower_part.end(), input, input.begin(),
        divide_point);
    // ①因为lower_part是副本，所以并行操作不会引发逻辑错误，这里可以启动future做排序
    std::future<std::list<T>> new_lower(
        std::async(&parallel_quick_sort<T>, std::move(lower_part)));
    // ②
    auto new_higher(
        parallel_quick_sort(std::move(input)));
    result.splice(result.end(), new_higher);
    result.splice(result.begin(), new_lower.get());
    return result;
}

void test_sequential_quick() {
    std::list<int> numlists = { 6,1,0,7,5,2,9,-1 };
    auto sort_result = sequential_quick_sort(numlists);
    std::cout << "sorted result is ";
    for (auto iter = sort_result.begin(); iter != sort_result.end(); iter++) {
        std::cout << " " << (*iter);
    }
    std::cout << std::endl;
}

//test4
//并行计算线程池
//线程池版本
//并行版本



class ThreadPool {
public:
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    static ThreadPool& instance() {
        static ThreadPool ins;
        return ins;
    }

    using Task = std::packaged_task<void()>;


    ~ThreadPool() {
        stop();
    }

    template <class F, class... Args>
    auto commit(F&& f, Args&&... args) -> std::future<decltype(f(args...))> {
        using RetType = decltype(f(args...));
        if (stop_.load())
            return std::future<RetType>{};
        //task指向一个包装了任务的std::packaged_task对象
        auto task = std::make_shared<std::packaged_task<RetType()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...));//bind里面去引用了decay_t

        //auto task = std::shared_ptr<std::packaged_task<RetType()>>(
        //    new std::packaged_task<RetType()>(std::bind(std::forward<F>(f), std::forward<Args>(args)...)));

        std::future<RetType> ret = task->get_future();
        {
            std::lock_guard<std::mutex> cv_mt(cv_mt_);
            tasks_.emplace([task] { (*task)(); });
        }
        cv_lock_.notify_one();
        return ret;
    }

    int idleThreadCount() {
        return thread_num_;
    }

private:
    ThreadPool(unsigned int num = 5)
        : stop_(false) {
            {
                if (num < 1)
                    thread_num_ = 1;
                else
                    thread_num_ = num;
            }
            start();
    }
    void start() {
        for (int i = 0; i < thread_num_; ++i) {
            pool_.emplace_back([this]() {
                while (!this->stop_.load()) {//.load()是原子操作，线程安全的(是以原子操作进行读取值)
                    Task task;
                    {
                        std::unique_lock<std::mutex> cv_mt(cv_mt_);
                        this->cv_lock_.wait(cv_mt, [this] {
                            return this->stop_.load() || !this->tasks_.empty();
                            });
                        if (this->tasks_.empty())
                            return;

                        task = std::move(this->tasks_.front());
                        this->tasks_.pop();
                    }
                    this->thread_num_--;
                    task();
                    this->thread_num_++;
                }
                });
        }
    }
    void stop() {
        stop_.store(true);
        cv_lock_.notify_all();
        for (auto& td : pool_) {
            if (td.joinable()) {
                std::cout << "join thread " << td.get_id() << std::endl;
                td.join();
            }
        }
    }

private:
    std::mutex               cv_mt_;
    std::condition_variable  cv_lock_;
    std::atomic_bool         stop_;
    std::atomic_int          thread_num_;
    std::queue<Task>         tasks_;
    std::vector<std::thread> pool_;
};


template<typename T>
std::list<T> thread_pool_quick_sort(std::list<T> input)
{
    if (input.empty())
    {
        return input;
    }
    std::list<T> result;
    result.splice(result.begin(), input, input.begin());
    T const& pivot = *result.begin();
    auto divide_point = std::partition(input.begin(), input.end(),
        [&](T const& t) {return t < pivot; });
    std::list<T> lower_part;
    lower_part.splice(lower_part.end(), input, input.begin(),
        divide_point);
    // ①因为lower_part是副本，所以并行操作不会引发逻辑错误，这里投递给线程池处理
    auto new_lower = ThreadPool::commit(&parallel_quick_sort<T>, std::move(lower_part));
    // ②
    auto new_higher(
        parallel_quick_sort(std::move(input)));
    result.splice(result.end(), new_higher);
    //这就是std::future的特性 ？ 通过get阻塞主线程进行线程同步？
    result.splice(result.begin(), new_lower.get());
    return result;
}

void test1() {
    test_quick_sort();
}

void test2() {
    std::list<int> num_list{ 5,3,7,6,4,1,0,2,9,10,8 };
    auto sorted_list = sequential_quick_sort(num_list);
    std::cout << "sorted result is ";
    for (auto& num : sorted_list) {
        std::cout << " " << num;
    }
    std::cout << std::endl;
}

void test3() {
    test_sequential_quick();
}

int main() {
    //test1();
	//test2();
    test3();

    return 0;
}   