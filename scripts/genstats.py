import json
import os
import subprocess
from multiprocessing import Pool
from pathlib import Path
from typing import Dict, List, Tuple

import matplotlib.pyplot as plt

NUM_SEEDS = 10
MAX_DIFFICULTY = 22
STATS_DIR = Path('stats')


def calculate(seed: bytes) -> List[Tuple[int, int]]:
    result: List[Tuple[int, int]] = []
    seed_str = seed.hex()

    for i in range(1, MAX_DIFFICULTY + 1):
        print(f"[*] Seed: {seed_str} [{i}/{MAX_DIFFICULTY}]")
        command = ["./jurischain", str(i), seed_str]

        res = subprocess.check_output(command, timeout=300).splitlines()
        diff = int(str(res[0]).split(':')[1].strip("'"))
        tries = int(str(res[3]).split(':')[1].strip("'"))

        assert diff == i
        result.append((diff, tries))

    return result


def generate_data() -> Dict[int, float]:
    seeds = [os.urandom(32) for _ in range(NUM_SEEDS)]

    print("[*] Generating data...")
    with Pool(NUM_SEEDS) as pool:
        result = pool.map(calculate, seeds)

    results: Dict[int, float] = {x: 0.0 for x in range(1, MAX_DIFFICULTY + 1)}

    for seed_result in result:
        for diff, tries in seed_result:
            results[diff] += tries

    for diff in results:
        results[diff] = results[diff] / NUM_SEEDS

    return results


def generate_graph() -> None:
    data = generate_data()

    STATS_DIR.mkdir(parents=True, exist_ok=True)

    with open(STATS_DIR / 'dataset.txt', 'w') as dataset:
        json.dump(data, dataset)

    means = list(data.values())
    plt.semilogy(range(len(means)), means)
    plt.title('Average tries per challenge difficulty')
    plt.xlabel('Difficulty')
    plt.ylabel('Tries')
    plt.savefig(STATS_DIR / 'statistics.png')


if __name__ == '__main__':
    generate_graph()
