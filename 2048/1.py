import itertools

nums = [1, 2, 3, 4, 5, 6, 7]
a0_set = set()

for comb in itertools.combinations(nums, 5):
    prod = 1
    for n in comb:
        prod *= n
    a0_set.add(-prod)

S = sum(a0_set)
print(S % 1000)