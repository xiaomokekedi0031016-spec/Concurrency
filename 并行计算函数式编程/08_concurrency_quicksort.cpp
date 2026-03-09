#if 0
#include <iostream>
#include <algorithm>
#include <vector>

template<typename T>
void quickSortMid(T arr[], int start, int end) {
	if (start >= end) return;
	T key = arr[(start + end) / 2];
	int left = start, right = end;
	while (left < right) {
		while (arr[right] > key && left < right) right--;
		while (arr[left] < key && left < right) left++;
		if (left <= right) {
			std::swap(arr[left], arr[right]);
			left++;
			right--;
		}
	}
	quickSortMid(arr, start, left - 1);
	quickSortMid(arr, left, end);
}

template<typename T>
void quickSortFunc(T arr[], int len) {
	quickSortMid(arr, 0, len - 1);
}

void test_quick_sort() {
	//int num_arr[] = { 5,3,7,6,4,1,0,2,9,10,8 };
	int num_arr[] = { 2,1,3 };
	int length = sizeof(num_arr) / sizeof(int);
	quickSortFunc(num_arr, length);
	std::cout << "sorted result is ";
	for (int i = 0; i < length; i++) {
		std::cout << " " << num_arr[i];
	}
	std::cout << std::endl;
}

int main() {
	test_quick_sort();
	return 0;
}

#endif