
#pragma comment(lib, "glfw3dll.lib")
#pragma comment(lib, "tbox.lib")

extern int __main_01(int argc, char *argv[]);
extern int __main_02(int argc, char *argv[]);
//
//#include <cassert>
//#include <iostream>
//#include <cstdlib>
//
//class Deque {
//public:
//	Deque(int size) {
//		_datas = new int[size + 1];
//		_cycle = size + 1;
//
//		_head = 0;
//		_tail = 0;
//	}
//	~Deque() {
//		delete[] _datas;
//	}
//
//	int head () {
//		assert(_head != _tail, "Deque is Empty!");
//		return _datas[_head];
//	}
//	int tail() {
//		assert(_head != _tail, "Deque is Empty!");
//		return _datas[_tail];
//	}
//
//	void pushHead(int elem) {
//		assert((_tail + 1) % _cycle != _head, "Deque Overflow");
//
//		_head = ((_head - 1) + _cycle) % _cycle;
//		_datas[_head] = elem;
//	}
//	void pushTail(int elem) {
//		assert((_tail + 1) % _cycle != _head, "Deque Overflow");
//
//		_datas[_tail] = elem;
//		_tail = (_tail + 1) % _cycle;
//	}
//
//	int popHead() {
//		assert(_head != _tail, "Deque is Empty!");
//
//		auto head = this->head();
//		_head = (_head + 1) % _cycle;
//
//		return head;
//	}
//	int popTail() {
//		assert(_head != _tail, "Deque is Empty!");
//
//		auto tail = this->tail();
//		_tail = ((_tail - 1) + _cycle) % _cycle;
//
//		return tail;
//	}
//
//	int length() {
//		return (_tail + _cycle - _head) % _cycle;
//	}
//	int size() {
//		return _cycle - 1;
//	}
//
//	void log() {
//		for (int i = _head; i != _tail; i = (i + 1) %_cycle)
//		{
//			printf("%d  ", _datas[i]);
//		}
//	}
//
//private:
//	int _head;
//	int _tail;
//
//	int _cycle;
//	int *_datas;
//};

int main(int argc, char *argv[])
{
	return __main_02(argc, argv);
/*
	Deque que(5);

	std::cout << que.length() << std::endl;

	que.pushTail(2);
	que.pushTail(3);
	que.pushHead(1);
	std::cout << que.length() << std::endl;
	que.pushTail(4);
	que.pushHead(0);

	que.log();

	printf("\n");
	system("pause");
*/
	return 0;
}