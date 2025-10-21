# 03_open_release_cdev

Character Device Driver의 `open()`과 `release()` 동작을 구현하고 확인하는 예제

---

## 개요

문자 디바이스는 `/dev` 아래의 장치 파일을 통해 접근한다.  
사용자 프로그램이 `open()`, `read()`, `close()` 등의 시스템 콜을 호출하면,  
커널은 해당 요청을 **VFS (Virtual File System)** 를 거쳐 드라이버의 `file_operations` 구조체에 연결된 함수로 전달한다.

이번 예제에서는 다음 두 콜백을 구현한다.

- `open()` : 장치 파일이 열릴 때 호출  
- `release()` : 장치 파일이 닫힐 때 호출  

이를 통해 커널이 장치 접근 시 내부 상태를 어떻게 관리하는지 확인할 수 있다.

---

## open()

```c
int (*open)(struct inode *inode, struct file *filp);
```

### 역할
- 사용자 공간에서 `open("/dev/hello0", O_RDWR)` 같은 시스템 콜이 실행되면  
  커널이 드라이버의 `open()` 함수를 호출한다.
- 장치 파일이 **열릴 때 한 번만 호출**된다.

### 주요 작업
- 디바이스 초기화  
- `inode`와 `file` 구조체를 기반으로 내부 장치 데이터 연결  
- 동시 접근 제어 및 플래그 설정  
- 디버깅 메시지 출력 등  

### 매개변수
| 인자 | 설명 |
|------|------|
| **inode** | 장치 파일의 메타데이터 구조체. `imajor(inode)`와 `iminor(inode)`로 Major/Minor 번호 확인 가능 |
| **filp** | 열린 파일 핸들 정보. `filp->f_pos`, `f_mode`, `f_flags` 등 상태 포함 |

### 반환값
- 성공: `0`  
- 실패: 음수 에러 코드 (`-EBUSY`, `-ENODEV`, `-EACCES` 등)

---

## release()

```c
int (*release)(struct inode *inode, struct file *filp);
```

### 역할
- `close(fd)` 또는 프로세스 종료로 파일이 닫힐 때 VFS가 호출한다.  
- `open()`의 역작용에 해당한다.

### 주요 작업
- 디바이스 사용 플래그 해제  
- `kmalloc`, `vmalloc` 등으로 확보한 자원 해제  
- 하드웨어 리소스 반환 (인터럽트, 전원 관리 등)  
- `open()`에서 초기화한 항목을 정리  

### 반환값
- 성공: `0`  
- 실패: 음수 에러 코드

---

## 동작 개요

1. **모듈 로드**
   - `register_chrdev()`로 문자 디바이스를 등록하고 major number를 획득  
   - `/proc/devices`에 `"hello_cdev"` 항목이 표시됨

2. **사용자 접근**
   - `open()` 호출 시 Major/Minor 번호, 파일 모드, 플래그 등이 로그로 출력  
   - `release()` 호출 시 “File is closed” 메시지 출력  

3. **모듈 언로드**
   - `unregister_chrdev()`로 디바이스 해제  

---

## test program

사용자 프로그램에서 `/dev/hello0` 장치를 여러 모드로 열어본다.  

| 모드 | 설명 |
|------|------|
| `O_RDONLY` | 읽기 전용 |
| `O_RDWR \| O_SYNC` | 읽기·쓰기 동기 모드 |
| `O_WRONLY \| O_NONBLOCK` | 쓰기 전용, 논블로킹 모드 |

각 `open()` 호출 시 커널 로그에 `filp->f_mode`, `f_flags` 값이 달라진다.  
이를 통해 시스템 콜 플래그가 커널 내부 구조체로 어떻게 전달되는지 확인할 수 있다.

---

## 실습 절차

1. **기존 장치 파일 삭제**
   ```bash
   sudo rm /dev/hello*
   ```

2. **모듈 빌드 및 삽입**
   ```bash
   make
   sudo insmod hello_cdev.ko
   ```

3. **Major 번호 확인**
   ```bash
   dmesg | tail -n 1
   ```
   예: `hello_cdev - Major Device Number: 236`

4. **장치 파일 생성**
   ```bash
   sudo mknod /dev/hello0 c 236 0
   sudo mknod /dev/hello10 c 236 10
   ls /dev/hello*
   ```

5. **테스트 실행**
   ```bash
   ./test /dev/hello0
   ./test /dev/hello10
   ```

6. **커널 로그 확인**
   ```bash
   dmesg -T | tail -n 10
   ```
   → 각 장치 접근마다 Major/Minor, f_pos, f_mode, f_flags 값이 다르게 출력됨

7. **모듈 제거**
   ```bash
   sudo rmmod hello_cdev
   ```
