# PageRank with POSIX Threads

**System Specs**:  
- **CPU**: AMD Ryzen 7 5700X3D  
- **Threads tested**: 1 to 4  
- **Implementation**: PageRank using POSIX Threads

---

## Results

### Dataset: `p2p-Gnutella24`

| Threads | Avg Time (s) | Std Dev | Speedup (%) | Individual Runs (s) |
|---------|--------------|---------|-------------|----------------------|
| 1       | 0.099904     | 0.000951| 0.00        | 0.101612, 0.098711, 0.099876, 0.099480, 0.099840 |
| 2       | 0.077117     | 0.000915| 22.81       | 0.078233, 0.077680, 0.076708, 0.075573, 0.077391 |
| 3       | 0.068784     | 0.000237| 31.15       | 0.068812, 0.069003, 0.068871, 0.068326, 0.068906 |
| 4       | 0.064947     | 0.001459| 34.99       | 0.063881, 0.064228, 0.067711, 0.065118, 0.063798 |

---

### Dataset: `ego-Facebook`

| Threads | Avg Time (s) | Std Dev | Speedup (%) | Individual Runs (s) |
|---------|--------------|---------|-------------|----------------------|
| 1       | 0.067732     | 0.000931| 0.00        | 0.066741, 0.068265, 0.069297, 0.067251, 0.067107 |
| 2       | 0.050635     | 0.001564| 25.24       | 0.053446, 0.049485, 0.051001, 0.050258, 0.048983 |
| 3       | 0.045064     | 0.001376| 33.47       | 0.044222, 0.043650, 0.047434, 0.045761, 0.044253 |
| 4       | 0.040916     | 0.000639| 39.59       | 0.039965, 0.041515, 0.040620, 0.040752, 0.041729 |

---

### Dataset: `email-Enron`

| Threads | Avg Time (s) | Std Dev | Speedup (%) | Individual Runs (s) |
|---------|--------------|---------|-------------|----------------------|
| 1       | 0.632457     | 0.010341| 0.00        | 0.628166, 0.641018, 0.623329, 0.648074, 0.621698 |
| 2       | 0.497420     | 0.007780| 21.35       | 0.500118, 0.492544, 0.492759, 0.511480, 0.490199 |
| 3       | 0.445711     | 0.005316| 29.53       | 0.438153, 0.440774, 0.451234, 0.447611, 0.450781 |
| 4       | 0.423735     | 0.016871| 33.00       | 0.416883, 0.418364, 0.406010, 0.421612, 0.455807 |

---

## Conclusions

The results clearly show that increasing the number of threads reduces execution time and improves performance. However, the speedup does not scale linearly. For example, the performance gain from 1 to 2 threads is more significant than the gain from 3 to 4 threads.

This is mainly due to how the computational workload is distributed. When going from 1 to 2 threads, each thread handles 50% of the work, which significantly lightens the load per thread. In contrast, increasing from 3 to 4 threads only reduces the workload from 33% per thread to 25%, which is a smaller relative improvement. This explains the diminishing returns as more threads are added.

Additionally, it’s important to note that the total execution times include both file I/O (reading the input and writing results) and the PageRank computation itself. When isolating just the computation time, the effects of multithreading are even more pronounced.

---

## PageRank Computation Time Only — `email-Enron`

| Threads | Avg Time (s) | Std Dev  | Speedup (%) | Individual Runs (s) |
|---------|--------------|----------|-------------|----------------------|
| 1       | 0.279180     | 0.009679 | 0.00        | 0.290174, 0.282157, 0.281968, 0.261041, 0.280558 |
| 2       | 0.154893     | 0.000700 | 44.52       | 0.153751, 0.154727, 0.155770, 0.155467, 0.154751 |
| 3       | 0.105203     | 0.000418 | 62.32       | 0.104529, 0.105104, 0.105392, 0.105813, 0.105179 |
| 4       | 0.081005     | 0.000401 | 70.98       | 0.080759, 0.081050, 0.081540, 0.080390, 0.081286 |

This breakdown highlights even more clearly how thread count impacts pure computation time.

---
