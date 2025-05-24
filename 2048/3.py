ks = set()
max_exp = 0
while 3 ** max_exp <= 2019:
    max_exp += 1

for a in range(max_exp):
    for b in range(max_exp):
        for c in range(max_exp):
            k = 3**a + 3**b + 3**c
            if k <= 2019:
                ks.add(k)

print(len(ks))