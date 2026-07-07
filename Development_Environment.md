# Development Environment Setup

> Yocto 기반 Embedded Linux UDS Diagnostic ECU Simulator 개발 환경 구축 가이드

---

# Development Environment

| Item            | Version            |
| --------------- | ------------------ |
| Host OS         | Windows 11         |
| WSL             | WSL2               |
| Linux           | Ubuntu 24.04 LTS   |
| IDE             | Visual Studio Code |
| Compiler        | GCC / G++          |
| Build System    | CMake              |
| Embedded Linux  | Yocto Project      |
| Emulator        | QEMU               |
| Language        | C++17              |
| Version Control | Git                |

---

# 1. Install WSL2

PowerShell(Admin)

```powershell
wsl --install
```

재부팅

Ubuntu 24.04 설치

확인

```bash
wsl --status
```

---

# 2. Update Ubuntu

```bash
sudo apt update
sudo apt upgrade -y
```

---

# 3. Install Development Packages

```bash
sudo apt install -y \
build-essential \
cmake \
git \
g++ \
gcc \
gdb \
make \
ninja-build \
python3 \
python3-pip \
python3-venv \
wget \
curl \
unzip \
tree
```

---

# 4. Install VSCode

Windows

Visual Studio Code 설치

Extension

* WSL
* C/C++
* CMake Tools
* CMake
* Git Graph
* GitLens

---

# 5. Git Configuration

```bash
git config --global user.name "Your Name"

git config --global user.email "you@example.com"

git config --global init.defaultBranch main
```

확인

```bash
git config --list
```

---

# 6. Create Workspace

```bash
mkdir -p ~/workspace

cd ~/workspace
```

---

# 7. Clone Project

```bash
git clone <Repository URL>

cd embedded-linux-uds
```

---

# 8. CMake Build

```bash
mkdir build

cd build

cmake ..

make -j$(nproc)
```

실행

```bash
./diagnosticd
```

---

# 9. Install Yocto Dependencies

```bash
sudo apt install -y \
gawk \
wget \
git-core \
diffstat \
unzip \
texinfo \
gcc \
build-essential \
chrpath \
socat \
cpio \
python3 \
python3-pip \
python3-pexpect \
xz-utils \
debianutils \
iputils-ping \
python3-git \
python3-jinja2 \
libegl1-mesa \
libsdl1.2-dev \
xterm \
zstd \
liblz4-tool \
file
```

---

# 10. Download Yocto

```bash
git clone git://git.yoctoproject.org/poky
```

---

# 11. Initialize Build Environment

```bash
cd poky

source oe-init-build-env
```

---

# 12. Build Sample Image

```bash
bitbake core-image-minimal
```

---

# 13. Run QEMU

```bash
runqemu qemux86-64
```

Linux Boot 확인

---

# 14. Create Custom Layer

```bash
bitbake-layers create-layer ../meta-diagnostic
```

추가

```bash
bitbake-layers add-layer ../meta-diagnostic
```

확인

```bash
bitbake-layers show-layers
```

---

# 15. Create Application Recipe

예시

```text
meta-diagnostic/

└── recipes-app/

    └── diagnostic/

        ├── diagnostic.bb

        └── files/
```

---

# 16. Build Custom Image

```bash
bitbake diagnostic-image
```

---

# 17. Execute on QEMU

```bash
runqemu
```

부팅 후

```bash
systemctl status diagnostic
```

또는

```bash
journalctl -u diagnostic
```

---

# 18. Development Cycle

```text
Code

↓

Git Commit

↓

CMake Build

↓

Yocto Recipe

↓

BitBake

↓

Image Build

↓

QEMU Boot

↓

Verification

↓

Repeat
```

---

# Directory Structure

```text
embedded-linux-uds/

├── app/
├── include/
├── config/
├── systemd/
├── yocto/
├── docs/
├── scripts/
├── build/
└── README.md
```

---

# Useful Commands

## Build

```bash
cmake ..

make -j
```

---

## Clean

```bash
rm -rf build
```

---

## Git

```bash
git status

git add .

git commit -m "message"

git push
```

---

## Yocto

```bash
source oe-init-build-env

bitbake core-image-minimal

bitbake diagnostic-image
```

---

## QEMU

```bash
runqemu
```

---

## Logs

```bash
journalctl -u diagnostic
```

---

# Learning Goals

* Linux Boot Process
* Yocto Project
* BitBake
* Layer
* Recipe
* CMake
* systemd
* QEMU
* Socket Programming
* UDS Protocol
* Software Architecture
