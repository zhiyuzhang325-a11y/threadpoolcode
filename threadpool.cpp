#include<bits/stdc++.h>
using namespace std;
class ThreadPool{
private:
    vector<thread> works;
    queue<function<void()>> tasks;
    mutex mtx;
    condition_variable cv;
    bool stop;
public:
    ThreadPool(size_t n):stop(false){
        for(int i=0;i<n;i++){
            works.emplace_back([this](){
                while(true){
                    unique_lock<mutex>lock(mtx);
                    cv.wait(lock, [this](){
                        return stop || tasks.size();
                    });
                    if(stop && tasks.empty()) return;
                    
                    auto task=move(tasks.front());
                    tasks.pop();
                    lock.unlock();
                    task();
                }
            });
        }
    }
    ~ThreadPool(){
        {
            unique_lock<mutex>lock(mtx);
            stop=true;
        }
        cv.notify_all();
        for(auto &t:works) t.join();
    }
    template<typename F,typename... Args>
    void enqueue(F &&f, Args&&... args){
        unique_lock<mutex>lock(mtx);
        tasks.emplace(bind(forward<F>(f), forward<Args>(args)...));
        lock.unlock();
        cv.notify_one();
    }
};

int main(){
    ThreadPool pool(4);
    for(int i=0;i<10;i++){
        pool.enqueue([i](){
            cout<<"task : "<<i<<" is running"<<endl;
            cout<<"task : "<<i<<" is finished"<<endl;
        });
    }
}
