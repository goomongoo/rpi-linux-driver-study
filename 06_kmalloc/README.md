# 06_kmalloc

---

## kmalloc 개요

일반 사용자 공간에서는 `malloc()`과 `free()`로 힙 메모리를 할당·해제한다.  
리눅스 커널 모듈에서는 이에 대응하는 **커널 전용 API**인  
`kmalloc()` / `kfree()` (또는 `kzalloc()`)을 사용한다.

### 역할
`kmalloc()`은 **커널 힙 영역에서 물리적으로 연속적인 메모리**를 할당한다.  
SLAB/SLUB 캐시를 통해 빠른 메모리 관리가 이루어지며, 반환된 주소는 커널 가상 주소 공간에 존재한다.

```c
#include <linux/slab.h>

void *kmalloc(size_t size, gfp_t flags);
void *kzalloc(size_t size, gfp_t flags);
void kfree(const void *ptr);
```

- `kmalloc()` : 메모리 블록을 동적으로 할당  
- `kzalloc()` : `kmalloc()` + 메모리 0 초기화  
- `kfree()` : `kmalloc()`으로 확보한 메모리 해제  

> 대용량 버퍼(수 MB 이상)는 `vmalloc()` 사용 권장

---

## GFP 플래그 요약

| 플래그 | 설명 | 사용 상황 |
|--------|------|------------|
| `GFP_KERNEL` | 수면 가능, 일반 메모리 할당 | 대부분의 커널 코드 |
| `GFP_ATOMIC` | 수면 불가, 인터럽트·락 보유 중 | ISR, 스핀락 내부 |
| `GFP_DMA` | DMA용 연속 메모리 | DMA 버퍼 필요 시 |

---

## struct file의 private_data

`struct file`은 열린 파일 디스크립터 하나를 나타내며,  
그 안의 `void *private_data`는 **해당 파일 핸들 전용의 데이터 포인터**로 쓰인다.

```c
struct file {
    ...
    void *private_data;
    ...
};
```

- 같은 디바이스라도 **프로세스마다 별도의 `private_data`를 가짐**  
- 한 파일 디스크립터 내에서는 open → write → read → close 순으로 동일 데이터 사용 가능  
- 여러 프로세스가 같은 `/dev` 장치를 열면, 각각 다른 버퍼를 갖게 됨  

---

## 코드 핵심 요약

```c
static int my_open(struct inode *inode, struct file *filp)
{
    filp->private_data = kmalloc(MEMSIZE, GFP_KERNEL);
    if (!filp->private_data)
        return -ENOMEM;
    return 0;
}

static int my_release(struct inode *inode, struct file *filp)
{
    kfree(filp->private_data);
    return 0;
}
```

- `open()` : 파일 핸들마다 버퍼 동적 생성  
- `release()` : 해당 버퍼 해제  
- `read()/write()` : 항상 `filp->private_data`를 통해 접근

---

## 모듈 로드 및 테스트

```bash
make
sudo insmod hello.ko
```

다음 명령으로 데이터를 써본다:

```bash
echo "Hello World" | sudo tee /dev/hello0
```

출력:
```
Hello World
```

이제 cat으로 읽어본다:

```bash
sudo cat /dev/hello0
```

출력 예시:
```
ELF���������@8@
```

이상한 값(garbage)이 나온다.  

---

## 쓰레기값이 나오는 이유

이건 `cat` 명령의 문제가 아니라 **프로세스 간 버퍼가 독립적이기 때문**이다.

1. `sudo tee /dev/hello0`은 **프로세스 A**에서 장치를 open → write → close 한다.  
   → `open()` 시점에 A 전용 버퍼(`private_data`)가 `kmalloc()`으로 생성된다.  
   → “Hello World”는 이 버퍼에 저장된다.
2. 그다음 `sudo cat /dev/hello0`은 **프로세스 B**에서 장치를 다시 open → read 한다.  
   → B는 새로운 `struct file`을 사용하므로 `open()`이 다시 호출된다.  
   → B 전용 버퍼(`private_data`)가 새로 `kmalloc()` 되어 초기화되지 않은 메모리를 갖는다.  
   → 결국 `read()`는 **미초기화된 버퍼를 사용자 공간으로 복사**한다.

즉, **tee가 쓴 버퍼와 cat이 읽는 버퍼는 전혀 다른 메모리 공간**이다.  
이 예제 구조에서는 프로세스 간 데이터 공유가 이루어지지 않는다.

> 참고: `kzalloc()`을 사용하면 cat 출력이 “빈 문자열”처럼 보이지만, 여전히 데이터 공유는 안 된다.

---

## test 프로그램

이 예제의 설계는 **같은 프로세스 안에서 open → write → read → close** 흐름을 가정한다.  
`cat`은 별도의 프로세스에서 실행되므로 구조 검증에 적합하지 않다.  
이를 위해 별도의 `test.c` 프로그램을 이용한다.

### test.c 동작 순서

1. `/dev/hello0` open()  
2. `"Hello World"` write()  
3. lseek(fd, 0, SEEK_SET)  
4. read()  
5. close()

→ 같은 파일 핸들의 `private_data`를 공유하므로 정상적인 결과를 얻는다.

---

## test.c 실행 결과

```bash
gcc test.c -o test
sudo ./test
```

출력:
```
Hello World
```

### 커널 로그 (dmesg)
```bash
[  11.123456] hello - write: req=12, actual=12, off=0
[  11.123500] hello - read: req=64, actual=12, off=0
```

---

## cat vs test 프로그램 비교

| 항목 | `sudo tee /dev/hello0` → `sudo cat /dev/hello0` | `sudo ./test` |
|------|--------------------------------------------------|----------------|
| 프로세스 | A에서 write, B에서 read (서로 다름) | 한 프로세스 내부에서 write/read |
| 버퍼 | 프로세스별 private_data | 동일 private_data 공유 |
| 초기화 | cat은 새로 kmalloc (미초기화) | write로 데이터 채움 |
| 결과 | garbage(또는 빈 값) | “Hello World” 정상 출력 |

---

## 정리

- 이 예제는 커널의 **동적 메모리 관리(kmalloc/kfree)** 와  
  **파일 핸들 단위 독립 버퍼 구조**를 보여준다.  
- `tee`와 `cat`은 서로 다른 프로세스이므로 **각자 다른 버퍼**를 사용 → garbage 출력.  
- `test.c`는 동일 프로세스 내에서 `private_data`를 공유하므로 정상 동작.  
- 프로세스 간 데이터 공유가 목적이라면 **공유 버퍼 + 락 보호** 구조로 변경해야 한다.
