## **Supported Platform**
We strongly recommend you to run SPFresh using the [Standard_L16s_v3](https://learn.microsoft.com/en-us/azure/virtual-machines/lsv3-series) instances on Azure as the code has been thoroughly tested there. 

### **Important tips for creating Lsv3 VM**
Since we use SPDK to build our storage, we need to disabled Secure boot option before creating Lsv3 VM, since:
```
1. Secure boot by default enables kernel lockdown
2. Kernel lockdown forbids PCI direct access from userspace
3. SPDK requires PCI direct access from userspace to run the NVMe driver
```

## **Source Code (Artifacts Available)**

> Clone the repository and submodules
```bash
git clone git@github.com:SPFresh/SPFresh.git
git submodule update --init --recursive
```

## **Getting Started (Artifacts Functional)**

### SPFresh

#### **Dependency**

> install dependency
```bash
sudo apt install cmake
sudo apt install libjemalloc-dev libsnappy-dev libgflags-dev
sudo apt install pkg-config
sudo apt install swig libboost-all-dev
sudo apt install libtbb-dev
sudo apt install libisal-dev
```

> We have modified rocksdb as an option of storage
```bash
git clone git@github.com:PtilopsisL/rocksdb.git
```

> Compile SPDK
```bash
cd ThirdParty/spdk
./scripts/pkgdep.sh
CC=gcc-9 ./configure
CC=gcc-9 make -j
```
Remember to use higher version of gcc to do **both configure and compile**.

> Compile isal-l_crypto
```bash
cd ThirdParty/isal-l_crypto
./autogen.sh
./configure
make -j
```

> Build RocksDB
```bash
mkdir build && cd build
cmake -DUSE_RTTI=1 -DWITH_JEMALLOC=1 -DWITH_SNAPPY=1 -DCMAKE_C_COMPILER=gcc-9 -DCMAKE_CXX_COMPILER=g++-9 -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-fPIC" ..
make -j
sudo make install
```

> Build SPFresh
```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j
```

### **Usage of build and search**
The detailed usage can be found in [Get started](docs/GettingStart.md), and some of those variables definition can be found in [ParameterDefinitionList.h](AnnService/inc/Core/SPANN/ParameterDefinitionList.h).


### **Source Code of Baselines**
For most figures, there are three lines: SPFresh, SPANN+, and DiskANN. SPFresh stands for our system. and SPANN+ is the base system of SPFresh without LIRE protocol, [DiskANN](https://github.com/microsoft/DiskANN) is a recent system published in NIPS'19 and the [streaming version](https://github.com/microsoft/DiskANN/tree/diskv2) is uploaded in 2022, we use it as a baseline to show the performance bottleneck of out-of-place update. We have modified DiskANN to fit our reproduction scripts. Use the following command to clone the [Forked DiskANN](https://github.com/Yuming-Xu/DiskANN_Baseline.git) and build the baseline systems


### **DiskANN**

#### **Install**
> clone DiskANN
```bash
git clone -b diskv2 https://github.com/Yuming-Xu/DiskANN_Baseline.git
```
> install the dependency of DiskANN
```bash
sudo apt install libgoogle-perftools-dev clang-format
wget https://registrationcenter-download.intel.com/akdlm/irc_nas/18487/l_BaseKit_p_2022.1.2.146_offline.sh
```
#### **Build**
> build then dependency of DiskANN
```bash
sudo sh ./l_BaseKit_p_2022.1.2.146_offline.sh
```
> following commands shows how to configure MKL
```
Accepet & Customize Install --> Intel® oneAPI Math Kernel Library(only select this) --> Skip Eclipse* IDE Configuration --> Begin Installation --> Close
```

> build DiskANN
```
mkdir build && cd build && cmake .. && make -j
```

> if liomp5 can not be found, the following command could be useful
```bash
sudo ln -s /opt/intel/oneapi/compiler/latest/linux/compiler/lib/intel64_lin/libiomp5.so /usr/lib/x86_64-linux-gnu/libiomp5.so
```

## **Reproduce All Experiment Results(Result Reproduced)**

We provide scripts in ./Script_AE folder for reproducing our experiments. For more details, see [./Script_AE/README.md](./Script_AE).
