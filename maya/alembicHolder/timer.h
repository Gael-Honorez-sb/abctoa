#include <time.h>
#include <iostream>

class timer
{
  public:
    void start() {
       m_start = GetTickCount();
    }
    void stop() {
       m_stop = GetTickCount();
    }
    void print(){
      m_result = m_stop - m_start;
      std::cout << "time: " << m_result << " ms" << std::endl;
    }
    
  private:
    int m_start;
    int m_stop;
    int m_result;
    
};
    

