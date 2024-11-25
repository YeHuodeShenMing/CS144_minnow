import matplotlib.pyplot as plt
import numpy as np
import re
from datetime import datetime as dt
from datetime import timedelta
from scipy.stats import pearsonr
import matplotlib.dates as mdates


with open("data_Rwanda.txt", "r") as file:

    # data = [file.readline() for _ in range(100)]
    data = file.readlines()


timestamp_data = []
ttl_data = set()
icmp_seq_data = []
rtt_data = []

for line in data:
    if matches := re.search(
        r"\[(\d+\.\d+)\] 64 bytes from 41.186.255.86: icmp_seq=(\d+) ttl=(\d+) time=(\d+) ms",
        line,
    ):
        timestamp = float(matches.group(1))
        icmp_seq = int(matches.group(2))
        ttl = int(matches.group(3))
        rtt_time = int(matches.group(4))
        # 将提取的信息存储在列表中
        timestamp_data.append(timestamp)
        ttl_data.add(ttl)
        icmp_seq_data.append(icmp_seq)
        rtt_data.append(rtt_time)


# 处理时间戳
timestamp_data = [dt.fromtimestamp(i).strftime('%H:%M:%S') for i in timestamp_data]

# print(f"DEBUG: {timestamp_data[:10]}")
# print(f"DEBUG: {rtt_data}")

# 1. 计算 delivery rate
if icmp_seq_data:
    min_seq = min(icmp_seq_data)
    max_seq = max(icmp_seq_data)

    total_seq = max_seq - min_seq + 1

    delivery_rate = len(icmp_seq_data) / total_seq

print(f"#1. The Delivery Rate is {delivery_rate:.2%}")

# 2. 计算最长successful ping / 3. 计算最长loss
longest_success = 0
curr_success = 1  # 计算成功连续的长度，从第一个数据点开始
longest_loss = 0
curr_loss = 0
previous_seq = icmp_seq_data[0]

for i in range(1, len(icmp_seq_data)):
    current_seq = icmp_seq_data[i]

    if current_seq == previous_seq + 1:
        longest_success += 1

    else:
        longest_success = max(longest_success, curr_success)
        curr_success = 1  # 重置

        # 对loss操作
        loss_length = current_seq - previous_seq + 1
        if loss_length > 0:
            curr_loss = loss_length
            longest_loss = max(loss_length, longest_loss)

    previous_seq = current_seq

longest_success = max(longest_success, curr_success)

print(f"#2. Longest Consecutive string of Successful Pings is {longest_success}")
print(f"#3. Longest Burst of Losses is {longest_loss}")

# 4. 丢包事件是否独立或相关
icmp_set = set(icmp_seq_data)

success_to_success = 0
success_to_failure = 0
failure_to_success = 0
failure_to_failure = 0

for seq in range(min_seq, max_seq):
    curr_success = seq in icmp_set
    next_success = (seq + 1) in icmp_set

    if current_seq and next_success:
        success_to_success += 1
    elif current_seq and not next_success:
        success_to_failure += 1
    elif not current_seq and next_success:
        failure_to_success += 1
    elif not current_seq and not next_success:
        failure_to_failure += 1

if (success_to_success + success_to_failure) > 0:
    prob_success_given_success = success_to_success / (
        success_to_success + success_to_failure
    )
else:
    prob_success_given_success = 0.0

if (failure_to_success + failure_to_failure) > 0:
    prob_failure_given_success = failure_to_success / (
        failure_to_success + failure_to_failure
    )
else:
    prob_failure_given_success = 0.0

print(f"#4.1. The Probability of Success_given_Success is {prob_success_given_success:.2%}")
print(f"#4.2. The Probability of Failure_given_success is {prob_failure_given_success:.2%}")

# 5. minimum RTT / 6. maximum RTT 
print(f"#5. The MINimum RTT is {min(rtt_data)}")
print(f"#6. The MAXimum RTT is {max(rtt_data)}")

# 7. A graph of the RTT as a function of time
rtt_n = np.array(timestamp_data)
rtt_n_plus_1 = np.array(rtt_data)

plt.figure(figsize=(12, 8))
plt.plot(rtt_n, rtt_n_plus_1, linestyle='-', label='RTT (ms)')


# start_time = dt.strptime('17:00:00', '%H:%M:%S')
# end_time = dt.strptime('00:41:00', '%H:%M:%S')

# time_points = []
# current_time = start_time
# while current_time <= end_time:
#     time_points.append(current_time)
#     current_time += timedelta(minutes=30)

# time_labels = [time.strftime('%H:%M:%S') for time in time_points]

# plt.xticks(time_labels, rotation=45)

plt.gca().xaxis.set_major_locator(mdates.AutoDateLocator())
plt.gca().xaxis.set_major_formatter(mdates.DateFormatter('%H:%M'))

plt.title('RTT as a function of Time')
plt.xlabel('Time of Day (HH:MM:SS)')
plt.ylabel('RTT (ms)')

# 自动布局
plt.tight_layout()
plt.savefig("./Rwanda_RTT.png")
# plt.show()


# 8. A histogram Distribution Function of the distribution of RTT sobserve
fig, ax = plt.subplots(figsize = (7, 12))
unique_rtt, rtt_counts = np.unique([x for x in rtt_data], return_counts=True)

ax.bar(unique_rtt, (rtt_counts / len(rtt_data)) * 100, label='The proportion of RTT (%)')
plt.title('The proportion of different RTT in total time')
plt.xlabel('RTT (ms)')
plt.ylabel('The proportion of RTT (%)')
plt.legend()
plt.xlim(390, 400)
plt.savefig("./Rwanda_distribution.png")
# plt.show()

# 9. A graph of the correlation between “RTT of ping #N” and “RTT of ping #N+1”
rtt_n = rtt_data[:-1]
rtt_n_plus_1 = rtt_data[1:]

plt.figure(figsize=(10, 6))
plt.scatter(rtt_n, rtt_n_plus_1)
plt.title('Correlation between RTT of ping #N and RTT of ping #N+1')
plt.xlabel('RTT of ping #N (ms)')
plt.ylabel('RTT of ping #N+1 (ms)')

correlation, _ = pearsonr(rtt_n, rtt_n_plus_1)
print(f"#9. Correlation coefficient between RTT(N) and RTT(N+1): {correlation:.2f}")
plt.savefig("./Rwanda_correlation.png")
# plt.show()