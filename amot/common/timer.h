namespace amot {

// 定时器
class Timer {
public:
    Timer();
    void Add();
    void Adjust();

private:
    double m_startTime;
    double m_lastTime;
    double m_deltaTime;

    double m_totalTime;
};

} // namespace amot
