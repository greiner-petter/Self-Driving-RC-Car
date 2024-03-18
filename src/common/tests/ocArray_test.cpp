#include "../ocAssert.h"
#include "../ocArray.h"

#include <algorithm>
#include <iostream>
#include <numeric>
//#include <ranges>

int main()
{
  {
    std::cout << "Test ocArray default constructor\n";
    ocArray<uint8_t> array;
    oc_assert(array.get_length() == 0, array.get_length());
  }

  {
    std::cout << "Test ocArray size constructor\n";
    ocArray<uint8_t> array(1);
    oc_assert(array.get_length() == 1, array.get_length());
  }

  {
    std::cout << "Test ocArray move constructor\n";
    auto l = [](){
        // All this is just to stop the compiler from optimizing away the move.
        auto arr = ocArray<int> {1, 2};
        if (2 == arr[0]) return ocArray<int>{};
        return arr;
    };
    ocArray<int> arr = l();
  }

  {
    std::cout << "Test ocArray make_ and get_space\n";
    ocArray<uint8_t> array;
    array.make_space(0, 10).fill('A');
    oc_assert(array.get_length() == 10, array.get_length());
    for (uint32_t i = 0; i < 10; ++i) oc_assert(array[i] == 'A', i, array[i]);
    array[6] = 'X';
    oc_assert(*array.get_space(6, 1) == 'X', *array.get_space(6, 1));
  }

  {
    std::cout << "Test ocArray view copying\n";
    ocArray<int64_t> a(10);
    ocArray<int64_t> b(10);

    a[0] = 1;
    a[1] = 2;

    a.get_space(0, 2).copy_to(b.get_space(5, 2));
    oc_assert(1 == b[5], b[5]);
    oc_assert(2 == b[6], b[6]);
  }

  {
    std::cout << "Test capped ocArray expansion\n";
    ocArray<int64_t> array(0);
    oc_assert(array.get_length() == 0, array.get_length());

    array.make_space(0, 1)[0] = 1;
    oc_assert(array.get_length() == 1, array.get_length());
    oc_assert(array[0]         == 1, array[0]);

    array.append_space(1)[0] = 2;
    oc_assert(array.get_length() == 2, array.get_length());
    oc_assert(array[0]         == 1, array[0]);
    oc_assert(array[1]         == 2, array[1]);

    array.prepend_space(1)[0] = 3;
    oc_assert(array.get_length() == 3, array.get_length());
    oc_assert(array[0]         == 3, array[0]);
    oc_assert(array[1]         == 1, array[1]);
    oc_assert(array[2]         == 2, array[2]);

    array.make_space(2, 1)[0] = 4;
    oc_assert(array.get_length() == 4, array.get_length());
    oc_assert(array[0]         == 3, array[0]);
    oc_assert(array[1]         == 1, array[1]);
    oc_assert(array[2]         == 4, array[2]);
    oc_assert(array[3]         == 2, array[3]);
  }

  {
    std::cout << "Test single element ocArray expansion\n";
    ocArray<int64_t> array(0);
    oc_assert(array.get_length() == 0);

    array.insert(0, 1);
    oc_assert(array.get_length() == 1, array.get_length());
    oc_assert(array[0]         == 1, array[0]);

    array.append(2);
    oc_assert(array.get_length() == 2, array.get_length());
    oc_assert(array[0]         == 1, array[0]);
    oc_assert(array[1]         == 2, array[1]);

    array.prepend(3);
    oc_assert(array.get_length() == 3, array.get_length());
    oc_assert(array[0]         == 3, array[0]);
    oc_assert(array[1]         == 1, array[1]);
    oc_assert(array[2]         == 2, array[2]);

    array.insert(2, 4);
    oc_assert(array.get_length() == 4, array.get_length());
    oc_assert(array[0]         == 3, array[0]);
    oc_assert(array[1]         == 1, array[1]);
    oc_assert(array[2]         == 4, array[2]);
    oc_assert(array[3]         == 2, array[3]);
  }

  {
    std::cout << "Test ocArray iterating\n";

    ocArray<int> arr = {1, 2, 3};
    int test[4] = {4, 5, 6, 7};

    int sum = std::accumulate(arr.begin(), arr.end(), 0);
    oc_assert(sum == 6, sum);

    size_t i = 0;
    for (auto &val : arr)
    {
        test[i] = val;
        val = 0;
        i++;
    }
    oc_assert(test[0] == 1, test[0]);
    oc_assert(test[1] == 2, test[1]);
    oc_assert(test[2] == 3, test[2]);
    oc_assert(test[3] == 7, test[3]);
    oc_assert(arr[0] == 0, arr[0]);
    oc_assert(arr[1] == 0, arr[1]);
    oc_assert(arr[2] == 0, arr[2]);
  }

  {
    std::cout << "Test const ocArray iterating\n";

    const ocArray<int> arr = {1, 2, 3};
    int test[4] = {4, 5, 6, 7};

    int sum = std::accumulate(arr.begin(), arr.end(), 0);
    oc_assert(sum == 6, sum);

    size_t i = 0;
    for (auto &val : arr)
    {
        test[i] = val;
        i++;
    }
    oc_assert(test[0] == 1, test[0]);
    oc_assert(test[1] == 2, test[1]);
    oc_assert(test[2] == 3, test[2]);
    oc_assert(test[3] == 7, test[3]);
  }

  {
    std::cout << "Test ocArrayView iterating\n";

    ocArray<int> arr = {9, 9, 9, 1, 2, 3, 8, 8, 8};
    auto view = arr.get_space(3, 3);
    int test[4] = {4, 5, 6, 7};

    int sum = std::accumulate(view.begin(), view.end(), 0);
    oc_assert(sum == 6, sum);

    size_t i = 0;
    for (auto &val : view)
    {
        test[i] = val;
        val = 0;
        i++;
    }
    oc_assert(arr[0] == 9, arr[0]);
    oc_assert(arr[1] == 9, arr[1]);
    oc_assert(arr[2] == 9, arr[2]);
    oc_assert(arr[3] == 0, arr[3]);
    oc_assert(arr[4] == 0, arr[4]);
    oc_assert(arr[5] == 0, arr[5]);
    oc_assert(arr[6] == 8, arr[6]);
    oc_assert(arr[7] == 8, arr[7]);
    oc_assert(arr[8] == 8, arr[8]);
    oc_assert(test[0] == 1, test[0]);
    oc_assert(test[1] == 2, test[1]);
    oc_assert(test[2] == 3, test[2]);
    oc_assert(test[3] == 7, test[3]);
    oc_assert(view[0] == 0, view[0]);
    oc_assert(view[1] == 0, view[1]);
    oc_assert(view[2] == 0, view[2]);
  }

  {
    std::cout << "Test const ocArrayView iterating\n";

    const ocArray<int> arr = {9, 9, 9, 1, 2, 3, 8, 8, 8};
    const auto view = arr.get_space(3, 3);
    int test[4] = {4, 5, 6, 7};

    int sum = std::accumulate(view.begin(), view.end(), 0);
    oc_assert(sum == 6, sum);

    size_t i = 0;
    for (auto &val : view)
    {
        test[i] = val;
        i++;
    }
    oc_assert(test[0] == 1, test[0]);
    oc_assert(test[1] == 2, test[1]);
    oc_assert(test[2] == 3, test[2]);
    oc_assert(test[3] == 7, test[3]);
  }

  {
    std::cout << "Test ocArrayView sorting\n";

    ocArray<int> arr = {9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    auto view = arr.get_space(1, 8);

    std::sort(view.begin(), view.end());

    oc_assert(arr[0] == 9, arr[0]);
    oc_assert(arr[1] == 1, arr[1]);
    oc_assert(arr[2] == 2, arr[2]);
    oc_assert(arr[3] == 3, arr[3]);
    oc_assert(arr[4] == 4, arr[4]);
    oc_assert(arr[5] == 5, arr[5]);
    oc_assert(arr[6] == 6, arr[6]);
    oc_assert(arr[7] == 7, arr[7]);
    oc_assert(arr[8] == 8, arr[8]);
    oc_assert(arr[9] == 0, arr[9]);
  }

  {
    std::cout << "Test ocArray remove_space\n";
    ocArray<int> arr{1, 2, 3, 4, 5};
    arr.remove_space(1, 2);
    oc_assert(arr.get_length() == 3, arr.get_length());
    oc_assert(arr[0]         == 1, arr[0]);
    oc_assert(arr[1]         == 4, arr[1]);
    oc_assert(arr[2]         == 5, arr[2]);

    arr.remove_space(2, 1);
    oc_assert(arr.get_length() == 2, arr.get_length());
    oc_assert(arr[0]         == 1, arr[0]);
    oc_assert(arr[1]         == 4, arr[1]);
  }

  {
    std::cout << "Test ocArray remove\n";
    ocArray<int> arr{1, 2, 3, 4};
    arr.remove_at(1);
    oc_assert(arr.get_length() == 3, arr.get_length());
    oc_assert(arr[0]         == 1, arr[0]);
    oc_assert(arr[1]         == 3, arr[1]);
    oc_assert(arr[2]         == 4, arr[2]);

    arr.remove_at(2);
    oc_assert(arr.get_length() == 2, arr.get_length());
    oc_assert(arr[0]         == 1, arr[0]);
    oc_assert(arr[1]         == 3, arr[1]);
  }

#if 0 // not supported in clang yet
  {
    std::cout << "Test ocArray range support\n";
    ocArray<int> arr{1, 2, 3, 4, 5, 6};

    int reverse[] = {6, 5, 4, 3, 2, 1};
    int *reverse_ptr = reverse;
    for (int i : std::views::reverse(arr))
    {
        oc_assert(i == *reverse_ptr, i, *reverse_ptr);
        reverse_ptr++;
    }

    int sum = 0;
    auto even = [](int i){ return 0 == i % 2; };
    for (int i : std::views::filter(arr, even))
    {
        sum += i;
    }

    oc_assert(sum == 12, sum);
  }
#endif

  return 0;
}
