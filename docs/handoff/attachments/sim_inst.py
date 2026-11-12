import math
import random
import sys

# Mirrors pulses.c onPulseTick/onPulsesHeartbeat and instantaneous.c updateInstantaneousRate
SENS = 108.345  # M4011 cpm per uSv/h (Cs137 factor 1.0)
MIN_TIMES = [0, 5, 60, 30, 10]


def conf(n):
    if n <= 1:
        return 2.6888794
    if n == 2:
        return 1.7858216
    return 1.959964 / math.sqrt(n) + 0.92095147 / (0.34074598 + n)


def calc_rate(first, last, count):
    ticks = last - first
    if count < 2:
        return 0.0, 0.0
    if count > ticks:
        count += 1
        ticks += 1
    return (count - 1) * 1000.0 / ticks, conf(count - 1)


def run(usvh, mode, seconds, seed):
    rnd = random.Random(seed)
    cps = usvh * SENS / 60.0
    p_tick = cps / 1000.0
    periods = []
    cur = [0, 0, 0]
    tick = 0
    over_warn = 0
    alert_secs = 0
    true_rate = cps
    sum_rate = 0.0
    n_rate = 0
    for s in range(seconds):
        for _ in range(1000):
            tick += 1
            n = 0
            if rnd.random() < p_tick:
                n = 1
            if cur[2] == 0:
                cur[0] = tick
            cur[1] = tick
            cur[2] += n
        period_tick = tick
        prev = list(cur)
        cur[2] = 0
        if prev[2]:
            periods.insert(0, prev)
            del periods[60:]
        min_time = MIN_TIMES[mode]
        min_pulse = 19 if mode <= 1 else 0
        af, al, ac = 0, 0, 0
        for p in periods:
            t = (period_tick - p[0]) // 1000 + 1
            if t > min_time and ac > min_pulse:
                break
            af = p[0]
            if not ac:
                al = p[1]
            ac += p[2]
        rate, c = calc_rate(af, al, ac)
        good = mode >= 2 or (0 < c < 0.75)
        if s < 120:
            continue
        sum_rate += rate
        n_rate += 1
        if good and rate * 60.0 / SENS >= 1.0:
            alert_secs += 1
    return true_rate, sum_rate / n_rate, alert_secs, seconds - 120


if __name__ == "__main__":
    usvh = float(sys.argv[1])
    secs = int(sys.argv[2])
    for mode, name in enumerate(["adaptive-fast", "adaptive-precision", "60s", "30s", "10s"]):
        tr, mean, alerts, total = run(usvh, mode, secs, 1234 + mode)
        print(f"{name:20s} true={tr*60:.1f}cpm mean_est={mean*60:.1f}cpm bias={100*(mean/tr-1):+.1f}% "
              f"secs>=1uSv/h(warning)={alerts}/{total} ({alerts/total*3600:.1f}/hour)")
