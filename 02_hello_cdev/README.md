# 02_hello_cdev

간단한 Character Device Driver 예제

---

## Character Device Driver

문자 디바이스(character device)는 데이터를 **문자 단위로 순차 처리**하는 장치다.  
대표적인 예로 터미널, UART, 센서 등이 있다.  

리눅스에서는 문자 디바이스를 파일처럼 다룬다.  
즉, 사용자 프로그램에서 `open()`, `read()`, `write()`, `ioctl()` 등의 **시스템 콜**을 호출하면,  
커널 내부에서 해당 요청이 드라이버 코드로 전달된다.  

이때 사용자 공간 → 시스템 콜 → **VFS (Virtual File System)** →  
**file_operations 구조체** → **드라이버 함수** 순서로 동작이 이어진다.

---

## file_operations 구조체

`file_operations`는 사용자 공간의 파일 입출력 함수와 커널 내부 드라이버 함수를 연결하는 구조체다.  
`<linux/fs.h>`에 정의되어 있으며, 문자 디바이스 드라이버의 핵심이다.

### 주요 멤버

| 항목 | 설명 |
|------|------|
| **.owner** | 보통 `THIS_MODULE`로 설정. 모듈 참조 카운트 유지 |
| **.open / .release** | `open()` / `close()` 시스템 콜과 연결 |
| **.read / .write** | 사용자 버퍼에서 데이터 송수신 처리 |
| **.llseek** | `lseek()`과 연결되어 파일 오프셋 이동 |
| **.unlocked_ioctl** | `ioctl()`과 연결되어 장치 제어 명령 처리 |
| **.mmap** | 사용자 공간과 디바이스 메모리 매핑 지원 |
| **.poll / .read_iter / .write_iter** | 비동기 I/O 또는 고급 I/O 지원 |

이 구조체 덕분에 커널은 사용자 공간의 파일 입출력 요청을 해당 드라이버 함수로 연결할 수 있다.  

---

## register_chrdev() / unregister_chrdev()

리눅스 커널에 새로운 문자 디바이스 드라이버를 등록하거나 해제하는 함수다.

### register_chrdev()

```c
int register_chrdev(unsigned int major, const char *name,
                    const struct file_operations *fops);
```

- **major**: 장치의 주 번호 (0 전달 시 자동 할당)  
- **name**: 드라이버 이름 (`/proc/devices`에 표시됨)  
- **fops**: 연결할 `file_operations` 구조체  
- **반환값**: 성공 시 major number, 실패 시 음수 에러 코드

### unregister_chrdev()

```c
int unregister_chrdev(unsigned int major, const char *name);
```

- 등록된 드라이버를 해제하며, 일반적으로 `rmmod` 시 호출한다.  

---

## 동작 개요

1. **모듈 로드 시**
   - `register_chrdev()`를 호출해 커널에 문자 디바이스 등록
   - 커널이 **major number**를 할당하고 `/proc/devices`에 이름을 표시함

2. **사용자 공간 접근**
   - `mknod` 명령으로 `/dev/hello0` 같은 장치 파일 생성  
   - `cat`, `echo`, `dd` 등의 명령으로 디바이스에 접근 가능  
   - `read()` 시스템 콜이 실행되면 드라이버의 `my_read()` 함수가 호출됨

3. **모듈 제거 시**
   - `unregister_chrdev()`로 등록된 디바이스 해제  

---

## 실행 방법

1. **모듈 빌드**
   ```bash
   make
   ```

2. **모듈 삽입**
   ```bash
   sudo insmod hello_cdev.ko
   ```

3. **장치 파일 생성**
   ```bash
   sudo mknod /dev/hello0 c <major_number> 0
   ```
   ※ `<major_number>`는 dmesg 또는 `/proc/devices`에서 확인

4. **디바이스 접근**
   ```bash
   cat /dev/hello0
   ```
   → 커널 로그(`dmesg`)에 “Read is called!” 메시지가 표시됨

5. **모듈 제거**
   ```bash
   sudo rmmod hello_cdev
   ```

---

## 실행 예시

```bash
$ sudo insmod hello_cdev.ko
$ dmesg | tail -n 2
[100.100001] hello_cdev - Major Device Number: 236

$ sudo mknod /dev/hello0 c 236 0
$ cat /dev/hello0
$ dmesg | tail -n 1
[101.123456] hello_cdev - Read is called!

$ sudo rmmod hello_cdev
```
