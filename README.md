Tool testing CPU single-thread performance by calculating taylor series expansion.

```bash	
make run
```

## TODO: 
- measure idle jitter
- measure rdtsc jitter
- use subcommand


## results:

USD/Hour is based on ap-northeast-1

SingleThread is based on `1000/ median_of_numbers_got_from_test `

MultiThread is based on `num_of_cpu * 1000 / sum_of_numbers * num_of_cpu`

Score and ratio is for higher is better. USD/Hour is for lower is better.

|                | **CPU** | **RAM** | **USD/Hour** | **Multithread** | **Singlethread** | **MultithreadRatio** | **SingleThreadRatio** |
| -------------- | ------- | ------- | ------------ | --------------- | ---------------- | -------------------- | --------------------- |
| M7a.medium     | 1       | 4GiB    | 0.074865     | 5.32            | 5.32             | 71.05                | 71.05                 |
| M7a.large      | 2       | 8GiB    | 0.14973      | 10.75           | 5.32             | 71.81                | 35.52                 |
| M6a.large      | 2       | 8GiB    | 0.1116       | 6.34            | 5.00             | 56.81                | 44.80                 |
| M7i.large      | 2       | 8GiB    | 0.1302       | 6.10            | 4.46             | 46.83                | 34.29                 |
| M7i-flex.large | 2       | 8GiB    | 0.12369      | 6.19            | 4.48             | 50.06                | 36.25                 |
| M6i.large      | 2       | 8GiB    | 0.124        | 4.62            | 4.18             | 37.25                | 33.74                 |
| m7g.medium     | 1       | 4GiB    | 0.0527       | 6.21            | 6.21             | 117.86               | 117.86                |
| m7g.large      | 2       | 8GiB    | 0.1054       | 12.82           | 6.21             | 121.64               | 58.93                 |
|                |         |         |              |                 |                  |                      |                       |
| t3.large       | 2       | 8GiB    | 0.1088       | 3.75            | 3.21             | 34.46                | 29.46                 |
| t3.medium      | 2       | 4GiB    | 0.0544       | 3.22            | 2.68             | 59.15                | 49.28                 |
| T4g.large      | 2       | 8GiB    | 0.0864       | 7.80            | 3.91             | 90.25                | 45.21                 |
| T4g.medium     | 2       | 4GiB    | 0.0432       | 7.53            | 3.92             | 174.37               | 90.78                 |
|                |         |         |              |                 |                  |                      |                       |
| MacBook.m1pro  | 1       |         |              |                 | 7.58             |                      |                       |
| 7950x3d        | 1       |         |              |                 | 7.25             |                      |                       |

