# **Reproduce All Experiment Results(Result Reproduced)**

In this folder, we provide scripts for reproducing figures in our paper. A Standard_L16s_v3 instance is needed to reproduce all the results.

The name of each script corresponds to the number of each figure in our paper. Since some of the scripts in this directory take a long computing time (as we mark below), we strongly recommend you create a tmux session to avoid script interruption due to network instability.

## **Dataset and Benchmark**
> **For artifact evaluation, we strongly recommend the reviewers to use those pre-downloaded and built dataset and benchmark in the ~/data/ folder on the provided Lsv3 instance, since the provided index and dataset requires lots of computation resource and takes about two weeaks to generate these data in a 80-core machine (we build these data offline)**

## **If want to generate these data**

> In our paper, we use SIFT and SPACEV dataset and synthetic benchmark.
these dataset can be download here：
```
http://big-ann-benchmarks.com/neurips21.html
```
> Download Dataset
```bash
wget https://dl.fbaipublicfiles.com/billion-scale-ann-benchmarks/bigann/base.1B.u8bin
wget https://comp21storage.blob.core.windows.net/publiccontainer/comp21/spacev1b/spacev1b_base.i8bin
```
> Download Query
```bash
wget https://dl.fbaipublicfiles.com/billion-scale-ann-benchmarks/bigann/query.public.10K.u8bin
wget https://comp21storage.blob.core.windows.net/publiccontainer/comp21/spacev1b/query.i8bin
```

> To generate dataset for Overall Performance
```bash
# generate dataset for index build
python generate_dataset.py --src source_file_path --dst output_file_path --topk 10000000
# generate dataset for update
python generate_dataset.py --src source_file_path --dst output_file_path --topk 20000000
```
> To generate trace for Overall Performance & generate truth for Overall Performance

This will takes hundreds of hours to generate groundTruth (the nearest K vectors of all queries) without GPU support

```bash
bash generateOverallPerformanceTraceAndTruth.sh
```

> To generate trace for Stress Test

```bash
/home/sosp/SPFresh/Release/usefultool -GenStress true --vectortype UInt8 --VectorPath /home/sosp/data/sift_data/base.1B.u8bin --filetype DEFAULT --UpdateSize 10000000 --BaseNum 1000000000 --TraceFileName bigann1b_update_trace -d 128 --Batch 20 -f DEFAULT
```

> To generate SPANN Index for Overall Peformance

build base SPANN Index

This will takes 1 days to generate SPANN Index of SPACEV100M with a 160 threads machine (we build it offline)

```bash
/home/sosp/SPFresh/Release/ssdserving build_SPANN_spacev100m.ini
```

> To generate DiskANN Index for Overall Performance

This will takes 0.5 days to generate DiskANN Index of SPACEV100M with a 160 threads machine (we build it offline)

```bash
mkdir /home/yuming/data/store_diskann_100m
/home/sosp/DiskANN_Baseline/build/tests/build_disk_index int8 /home/sosp/data/spacev_data/spacev100m_base.i8bin /home/yuming/data/store_diskann_100m/diskann_spacev_100m_ 64 75 128 128 16 l2 0
```

> To generate SPANN Index for Stress Test

This will takes 5 days to generate SPANN Index of SPACEV100M with a 160 threads machine (we build it offline)

```bash
/home/sosp/SPFresh/Release/ssdserving build_SPANN_sift1b.ini
```

## **Additional Setup**
here is additional setup for evaluation

### **How to check current state**
> if we can not see the /dev/nvme0n1 after the following command, it means this disk has been taken over by SPDK
```bash
lsblk
```

### **SPFresh & SPANN+**
> Since we use SPDK to build our storage, we need to bind PCI dev to SPDK
```bash
sudo nvme format /dev/nvme0n1
sudo ./SPFresh/ThirdParty/spdk/scripts/setup.sh
cp bdev.json /home/sosp/SPFresh/
```

### **DiskANN**
> If the device has been binded to SPDK, the following command can reset
```bash
sudo ./SPFresh/ThirdParty/spdk/scripts/setup.sh reset
```

> Prepare DiskANN for evaluation
```bash
sudo mkfs.ext4 /dev/nvme0n1
sudo mount /dev/nvme0n1 /home/sosp/testbed
sudo chmod 777 /home/sosp/testbed
```

## **Start to Run**

### **Motivation (Figure 1)**
> It takes about 22 minutes
```bash
bash motivation.sh
```
> Plot the result
```bash
bash plot_motivation_result.sh
```

### **Overall Performance (Figure 6)**
> It takes about 6 days to reproduce this figure
#### **SPFresh**
> This takes about 43 hours and 50 minutes
```bash
bash overall_spacev_spfresh.sh
```
#### **SPANN+**
> This takes about 43 hours and 30 minutes
```bash
bash overall_spacev_spann.sh
```
#### **DiskANN**
> Before running, move DiskANN Index to the disk (if binded by SPDK, First reset and mkfs (follow the instruction above))
```bash
cp -r /home/sosp/data/store_diskann_100m /home/sosp/testbed
```
> This takes about 35 hours and 44 minutes
```bash
bash overall_spacev_diskann.sh
```
#### **Plot the result of the baselines**
```bash
bash plot_overall_result.sh
```

### **Disk IOPS Limitation (Figure 7)**
> This takes about 5 minutes
```bash
bash iops_limitation.sh
```
> Plot the result
```bash
bash plot_iops_result.sh
```

### **Stress Test (Figure 8)**
> This takes about 35 hours and 26 minutes
```bash
bash stress_spfresh.sh
```
> Plot the result
```bash
bash plot_stress_result.sh
```

### **Data Shifting Micro-benchmark (Figure 9)**
> This takes about 1 hours and 10 minutes
```bash
bash data_shifting.sh
```
> Plot the result
```bash
bash plot_shifting_result.sh
```
### **Parameter Study: Reassign Range (Figure 10)**
> This takes about 1 hours and 30 minutes
```bash
bash parameter_study_range.sh
```
> Plot the result
```bash
bash plot_range_result.sh
```
### **Foreground Scalability (Figure 11)**
> This takes about 11 minutes
```bash
bash parameter_study_balance.sh
```
> Plot the result
```bash
bash plot_balance_result.sh
```


