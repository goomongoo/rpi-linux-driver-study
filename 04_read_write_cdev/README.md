# 04_read_write_cdev

Character Device Driver의 `read()`와 `write()` 기능을 구현하는 예제.  

---

## 개요

사용자 프로그램이 `read()` 또는 `write()` 시스템 콜을 호출하면,  
커널의 VFS(Virtual File System)가 `file_operations` 구조체에 등록된  
`.read()` 또는 `.write()` 함수를 호출한다.

이 함수들은 커널 ↔ 사용자 공간 간의 안전한 데이터 복사를 담당하며,  
문자 디바이스 드라이버에서 실제 데이터 입출력이 일어나는 핵심이다.

---

## read()

```c
ssize_t (*read)(struct file *filp, char __user *buf, size_t count, loff_t *ppos);
```

### 역할
- `read(fd, buf, count)` 호출 시 커널이 드라이버의 `.read()`를 실행한다.  
- 내부 버퍼의 데이터를 사용자 공간으로 복사한다.  
- `*ppos`(파일 오프셋)를 갱신하여 연속적인 읽기가 가능하도록 한다.

### 주요 인자

| 인자 | 설명 |
|------|------|
| **filp** | 열린 파일 핸들 구조체. 내부 컨텍스트 접근 가능 |
| **buf** | 사용자 공간 버퍼 (`__user` 포인터). `copy_to_user()`로 복사해야 함 |
| **count** | 요청된 읽기 크기 |
| **ppos** | 현재 오프셋. 읽은 바이트 수만큼 증가시킴 |

### 반환값
- **양수** → 실제 읽은 바이트 수  
- **0** → EOF (더 이상 읽을 데이터 없음)  
- **음수** → 에러 (`-EFAULT`, `-EINVAL`, `-EIO` 등)

---

## write()

```c
ssize_t (*write)(struct file *filp, const char __user *buf, size_t count, loff_t *ppos);
```

### 역할
- `write(fd, buf, count)` 호출 시 커널이 `.write()`를 실행한다.  
- 사용자 버퍼 데이터를 커널 내부 버퍼로 복사한다.  
- `*ppos`를 갱신해 연속 쓰기가 가능하도록 한다.

### 주요 인자

| 인자 | 설명 |
|------|------|
| **filp** | 열린 파일 핸들 구조체 |
| **buf** | 사용자 버퍼 (`__user` 포인터). `copy_from_user()`로 복사해야 함 |
| **count** | 요청된 쓰기 크기 |
| **ppos** | 현재 오프셋 |

### 반환값
- **양수** → 실제로 쓴 바이트 수  
- **0** → 더 이상 기록할 수 없음  
- **음수** → 에러 코드

---

## 안전한 데이터 복사

커널은 사용자 공간 주소를 직접 접근할 수 없으므로,  
데이터 복사는 반드시 아래의 헬퍼 함수를 사용해야 한다.

| 함수 | 방향 | 설명 |
|------|-------|------|
| `copy_to_user(void __user *to, const void *from, unsigned long n)` | 커널 → 사용자 | read()에서 사용 |
| `copy_from_user(void *to, const void __user *from, unsigned long n)` | 사용자 → 커널 | write()에서 사용 |

두 함수는 실패 시 “복사하지 못한 바이트 수”를 반환하므로,  
성공 시 실제 복사된 크기는 `요청 크기 - 반환값`으로 계산한다.

---

## 동작 개요

1. **모듈 로드**
   - `register_chrdev()`로 문자 디바이스 등록  
   - 내부 버퍼(`my_buf[256]`) 준비

2. **write()**
   - 사용자로부터 문자열 수신 (`echo "Hello World!" > /dev/hello0`)  
   - `copy_from_user()`로 256바이트 버퍼에 저장  
   - 커널 로그에 요청 크기, 실제 기록 크기, 오프셋 출력

3. **read()**
   - 사용자 프로그램(`cat /dev/hello0`)이 내부 데이터를 읽음  
   - `copy_to_user()`로 데이터를 사용자 버퍼에 복사  
   - 파일 오프셋(`*ppos`)을 증가시켜 EOF 조건 판단  

4. **EOF 처리**
   - 버퍼 끝(256바이트)에 도달하면 `read()`는 0을 반환  
   - `cat`은 `read()`가 0을 반환하면 더 이상 읽을 데이터가 없다고 판단하고 종료

---

## cat 명령의 동작

- **왜 131072 bytes를 읽으려 하는가?**  
  `cat`은 내부적으로 **128 KiB(=131072 bytes)** 크기의 버퍼로 읽는다.  
  따라서 VFS가 `.read()` 호출 시 `count=131072`을 전달한다.  
  드라이버는 내부 버퍼 크기(256바이트)까지만 읽고 나머지는 무시한다.

- **왜 한 번 더 read를 시도하는가?**  
  첫 번째 호출에서 `ppos`가 256이 되어 EOF에 도달했다.  
  `cat`은 두 번째로 `read()`를 호출했지만 0바이트 반환(EOL) → 즉시 종료한다.

결과적으로 `cat`은 두 번의 `read()` 호출 후 종료된다.

---

## 실습 예시

```bash
$ make
$ sudo insmod hello_cdev.ko
$ sudo mknod /dev/hello0 c 236 0

$ echo "Hello World!" | sudo tee /dev/hello0
Hello World!

$ sudo cat /dev/hello0
Hello World!
```

### 커널 로그 (dmesg)
```bash
[8708.997372] hello_cdev - Major Device Number: 236
[8726.332168] hello_cdev - Write is called. Requested to write 13 bytes. Actually writing 13 bytes. The offset is 0
[8741.096973] hello_cdev - Read is called. Requested to read 131072 bytes. Actually reading 256 bytes. The offset is 0
[8741.097082] hello_cdev - Read is called. Requested to read 131072 bytes. Actually reading 0 bytes. The offset is 256
```
