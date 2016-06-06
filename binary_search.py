def quick_sort(array, left, right):
    pivot = array[(left + right) / 2]
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
print "Array: ",
array = map(int, raw_input().split())
print "Number:",
target = int(raw_input())

# Sort the array
sort(array)

# Search if the target number is contained or not
found = binary_search(array, target)

if found:
    print "Found\n"
else:
    print "Not found\n"

