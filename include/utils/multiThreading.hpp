template<typename T>
class ThreadSafeQueue {
private:
    mutable std::mutex mtx;
    std::queue<std::shared_ptr<T>> dataQueue;
    std::condition_variable dataCond;
    std::atomic<bool> inputComplete;

public:
    ThreadSafeQueue() : inputComplete(false) {}

    // Prevent Queue copies
    ThreadSafeQueue(const ThreadSafeQueue& other) = delete;
    ThreadSafeQueue& operator=(const ThreadSafeQueue& other) = delete;

    void push(T newValue) {
        std::shared_ptr<T> data(std::make_shared<T>(std::move(newValue)));

        std::lock_guard<std::mutex> lock(mtx);
        dataQueue.push(data);

        dataCond.notify_one();
    }

    void pushFinal(T newValue) {
        std::shared_ptr<T> data(std::make_shared<T>(std::move(newValue)));

        std::lock_guard<std::mutex> lock(mtx);
        dataQueue.push(data);
        inputComplete = true;

        dataCond.notify_one();
    }

    std::shared_ptr<T> tryPop() {
        std::lock_guard<std::mutex> lock(mtx);
        if(dataQueue.empty()) {
            return std::shared_ptr<T>();
        }
        std::shared_ptr<T> res = dataQueue.front();
        dataQueue.pop();
        return res;
    }

    bool tryPop(T& value) {
        std::lock_guard<std::mutex> lock(mtx);
        if(dataQueue.empty()) {
            return false;
        }
        value = std::move(*dataQueue.front());
        dataQueue.pop();
        return true;
    }

    void waitAndPop(T& value) {
        std::unique_lock<std::mutex> lock(mtx);
        dataCond.wait(lock, [this]{return !dataQueue.empty();});
        if (!dataQueue.empty()) {
            value = std::move(*dataQueue.front());
            dataQueue.pop();
        }
    }

    std::shared_ptr<T> waitAndPop() {
        std::unique_lock<std::mutex> lock(mtx);
        dataCond.wait(lock, [this]{return !dataQueue.empty();});
        std::shared_ptr<T> res = nullptr;
        if (!dataQueue.empty()) {
            res = dataQueue.front();
            dataQueue.pop();
        }
        return res;
    }

    bool isInputComplete() const {
        return inputComplete.load();
    }

    bool empty() const {
        std::lock_guard<std::mutex> lock(mtx);
        return dataQueue.empty();
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(mtx);
        return dataQueue.size();
    }
};
