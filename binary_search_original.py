def quick_sort(array, left, right):
    pivot = array[int((left + right) / 2)]
    i = left
    j = right
    while True:
        while pivot > array[i]:
            i += 1
        while pivot < array[j]:
            j -= 1
        if i >= j:
            break
        temp = array[i]
        array[i] = array[j]
        array[j] = temp
        i += 1
        j -= 1
    if left < i - 1:
        quick_sort(array, left, i - 1)
    if right > j + 1:
        quick_sort(array, j + 1, right)

def sort(array):
    quick_sort(array, 0, len(array) - 1)

def binary_search(array, target):
    left = 0
    right = len(array) - 1
    while left <= right - 1:
        middle = int((left + right) / 2)
        if array[middle] == target:
            return True
        if target < array[middle]:
            right = middle
        else:
            left = middle
    return False


# Read input data
print("Array: ", end="")
array = list(map(int, input().split()))

# Sort the array
sort(array)

while True:
    print("Number:", end="")
    target = int(input())

    # Search if the target number is contained or not
    found = binary_search(array, target)
    if found:
        print("Found")
    else:
        print("Not found")
