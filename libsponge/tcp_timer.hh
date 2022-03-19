using namespace std;

class TCPTimer {
  private:
    //记录该计时器启动的时间.
    size_t _start_time{0};

    //计时器是否开启
    bool _on;

    //计时上限
    size_t _rto{0};

  public:
    TCPTimer() : _on(false) {}

    //开启计时器
    void start_timer(const size_t start_time, const size_t rto) {
            _start_time = start_time;
            _rto = rto;
            _on = true;
    }

    void stop_timer() { _on = false; }

    //查询计时器状态
    bool check(const size_t current_time) {
        if (!_on) {
            return false;
        }
        size_t passed = current_time - _start_time;
        if (passed >= _rto) {
            _on = false;
            return true;
        }
        return false;
    }

    bool running() { return _on; }
};