## Install OPTEE on WSL2
The approach is tested on the following platform:

- Intel(R) Core(TM) i7-8750H
- Windows 10 Pro 21H2 Build 19044.2728
- WSL2
    ```
    WSL version: 1.1.6.0
    Kernel version: 5.15.90.1
    WSLg version: 1.0.50
    MSRDC version: 1.2.3770
    Direct3D version: 1.608.2-61064218
    DXCore version: 10.0.25131.1002-220531-1700.rs-onecore-base2-hyp
    Windows version: 10.0.19044.2728
    ```
- Ubuntu 20.04 LTS running on WSL
    ```
    Linux xxx 5.15.90.1-microsoft-standard-WSL2 #1 SMP Fri Jan 27 02:56:13 UTC 2023 x86_64 x86_64 x86_64 GNU/Linux
    ```
### Install WSL and Ubuntu 20.04
These two can be installed through Microsoft Store.
### Install Dependencies
Install all the softwares listed on [OPTEE doc](https://optee.readthedocs.io/en/latest/building/prerequisites.html), on the tab corresponding to Ubuntu 20.04. Run the following command on Ubuntu terminal.
```
$ sudo apt update
$ sudo apt install \
  android-tools-adb \
  android-tools-fastboot \
  autoconf \
  automake \
  bc \
  bison \
  build-essential \
  ccache \
  cscope \
  curl \
  device-tree-compiler \
  expect \
  flex \
  ftp-upload \
  gdisk \
  iasl \
  libattr1-dev \
  libcap-dev \
  libfdt-dev \
  libftdi-dev \
  libglib2.0-dev \
  libgmp3-dev \
  libhidapi-dev \
  libmpc-dev \
  libncurses5-dev \
  libpixman-1-dev \
  libssl-dev \
  libtool \
  make \
  mtools \
  netcat \
  ninja-build \
  python3-crypto \
  python3-cryptography \
  python3-pip \
  python3-pyelftools \
  python3-serial \
  rsync \
  unzip \
  uuid-dev \
  xdg-utils \
  xterm \
  xz-utils \
  zlib1g-dev
```
An important thing here is that after installing the softwares, the name `python` should be recognized as `python2` by bash. Manually install `python2` may be needed.
### Download the OPTEE source
```
$ mkdir -p ~/bin
$ curl https://storage.googleapis.com/git-repo-downloads/repo > ~/bin/repo && chmod a+x ~/bin/repo
$ export PATH=~/bin:$PATH
$ mkdir optee-qemuv8 && cd optee-qemuv8
$ repo init -q -u https://github.com/OP-TEE/manifest.git -m qemu_v8.xml -b 3.9.0 
```
Modify `.repo/manifests/qemu_v8.xml` to use a specific version we need.
```
- <project path="linux"  name="linaro-swg/linux.git" revision="optee" clone-depth="1" />
+ <project path="linux"  name="linaro-swg/linux.git" revision="refs/tags/optee-3.10.0" clone-depth="1" />
```
Fetch all code
```
$ repo sync -j4 --no-clone-bundle
```
### Build OPTEE for QEMU ARMv8
Before runnig `make`, check your locale settings.
```
$ locale
```
If the output is not xxx.UTF-8 as follows, add `export LOCALE_ALL=en_US.UTF-8` in `~/.bashrc` followed by `source ~/.bashrc`. The locale can't be changed in the way on a regular Linux, e.g. `localectl set-locale`.
```
LANG=C.UTF-8
LANGUAGE=
LC_CTYPE="en_US.UTF-8"
LC_NUMERIC="en_US.UTF-8"
LC_TIME="en_US.UTF-8"
LC_COLLATE="en_US.UTF-8"
LC_MONETARY="en_US.UTF-8"
LC_MESSAGES="en_US.UTF-8"
LC_PAPER="en_US.UTF-8"
LC_NAME="en_US.UTF-8"
LC_ADDRESS="en_US.UTF-8"
LC_TELEPHONE="en_US.UTF-8"
LC_MEASUREMENT="en_US.UTF-8"
LC_IDENTIFICATION="en_US.UTF-8"
LC_ALL=en_US.UTF-8
```
Once locale is fixed, run the running commad to build OPTEE.
```
$ cd build
$ make -j2 toolchains
# clean build: about 30 minutes on a 6-core Windows machine running WSL
$ make QEMU_VIRTFS_ENABLE=y -j `nproc`
```
### Run `QEMU`
```
$ make run-only
```
`QEMU` will launch the normal and secure world consoles that pop out on your Windows machine.
