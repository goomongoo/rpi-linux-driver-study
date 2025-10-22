# Linux Driver Study

간단한 Linux 커널 모듈 및 Linux 드라이버 예제 모음

## 준비 사항

### 하드웨어
Raspberry Pi 4B 기반

### 초기설정
```bash
sudo apt update && sudo apt upgrade -y
sudo apt install raspberrypi-kernel-headers
sudo apt install build-essential
sudo reboot
```

## 실습 방법
각 예제 디렉토리(ex. `01_hello`)로 이동 후 `make` 실행하여 커녈 모듈(`.ko` 파일) 빌드

```bash
cd 01_hello
make
```

빌드된 모듈은 `insmod`로 로드하고 `rmmod`로 제거

```bash
sudo insmod hello.ko
sudo rmmod hello
```

빌드 결과물 제거는 `make clean` 사용

```bash
make clean
```

## 목차
- **01_hello**: 간단한 "Hello World" 커널 모듈
- **02_hello_cdev**: `read` operation이 포함된 기본 문자 디바이스(`cdev`) 생성
- **03_open_release_cdev**: 문자 디바이스의 `open` 및 `release` operation 구현
- **04_read_write_cdev**: 문자 디바이스의 `read` 및 `write` operation 구현
- **05_cdev_class_device**: `struct class`와 `struct device` 기반으로 `/dev`에 디바이스 파일 자동 생성
- **06_kmalloc**: `kmalloc`과 `file->private_data`를 이용한 커널 동적 메모리 관리

## 참고자료

[Johannes 4GNU_Linux (Youtube)](https://www.youtube.com/@johannes4gnu_linux96)
