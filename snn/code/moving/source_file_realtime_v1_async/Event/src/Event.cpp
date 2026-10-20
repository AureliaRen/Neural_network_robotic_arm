/*
* 该文件定义了Event类的具体实现方法。
*/

#include "../source_file_realtime_v1_async/Event/inc/Event.h"

// Event类的默认构造函数
Event::Event() :time(0){

}

// Event类的带参构造函数
Event::Event(int t, int index) :time(t), QueueIndex(index) {

}

// Event绫荤殑鏋愭瀯鍑芥暟
Event::~Event() {

}

// Event绫荤殑鎴愬憳鍑芥暟锛岀敤浜庤幏鍙栦簨浠跺彂鐢熺殑鏃堕棿
int Event::getTime() {
	return this->time;
}

// Event绫荤殑鎴愬憳鍑芥暟锛岀敤浜庤缃簨浠跺彂鐢熺殑鏃堕棿
void Event::setTime(int t) {
	time = t;
}

// Event绫荤殑鎴愬憳鍑芥暟锛岀敤浜庤幏鍙栦簨浠剁殑绱㈠紩
int Event::getIndex() {
    return this->QueueIndex;
}
