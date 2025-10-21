# 05_cdev_class_device

`cdev`, `class`, `device` 구조체를 활용해 문자 디바이스 드라이버를 커널 디바이스 모델에 통합하는 예제.  
이전 단계에서 수동으로 `/dev` 노드를 만들던 방식 대신, **자동으로 `/dev/hello0` 노드를 생성**하도록 개선한다.

---

## 배경

기존의 `register_chrdev()` 방식은 다음과 같은 한계가 있었다:

| 항목 | register_chrdev() 방식 | cdev + class + device 방식 |
|------|------------------------|-----------------------------|
| `/dev` 노드 생성 | ❌ 수동 `mknod` 필요 | ✅ `device_create()`로 자동 생성 |
| sysfs 반영 | ❌ 없음 | ✅ `/sys/class/<class>/<device>` 자동 생성 |
| udev 연동 | ❌ 불가 | ✅ `dev_uevent`, `devnode`로 연동 가능 |
| 여러 minor 관리 | ❌ 직접 분기 처리 | ✅ `cdev_add(..., count)`로 범위 지정 |
| 메모리/수명 관리 | 단순 해제 | 구조적 관리 (`release`, `class_destroy`, `device_destroy`) |
| 드라이버 코어 통합 | 제한적 | 완전 통합 (버스, 클래스, 전원 관리 포함) |

즉, `cdev + class + device`는 **현대 리눅스 커널 드라이버의 표준 구조**이다.

---

## 전체 흐름

1. **장치 번호 할당**
   ```c
   alloc_chrdev_region(&devt, 0, 1, "hello");
   ```
   → 커널이 Major/Minor 번호 자동 할당  

2. **cdev 초기화 및 등록**
   ```c
   cdev_init(&hello_cdev, &fops);
   cdev_add(&hello_cdev, devt, 1);
   ```
   → `file_operations` 연결, 시스템에 등록

3. **클래스 생성**
   ```c
   hello_class = class_create(THIS_MODULE, "hellodev");
   ```
   → `/sys/class/hellodev/` 경로 생성

4. **디바이스 생성**
   ```c
   hello_device = device_create(hello_class, NULL, devt, NULL, "hello%d", 0);
   ```
   → `/dev/hello0` 자동 생성  
   → `/sys/class/hellodev/hello0/` sysfs 노드 생성

5. **모듈 제거 시**
   ```c
   device_destroy(hello_class, devt);
   class_destroy(hello_class);
   cdev_del(&hello_cdev);
   unregister_chrdev_region(devt, 1);
   ```

---

## 주요 구조체 요약

### 1. `struct cdev`
- 커널 내부에서 **문자 디바이스를 표현하는 핵심 구조체**
- `file_operations`와 연결되어 사용자 요청(`read`, `write`, `open`, `ioctl`)을 처리
- 주요 필드  
  - `dev_t dev`: Major/Minor 번호  
  - `const struct file_operations *ops`: 파일 연산 테이블  
  - `struct kobject kobj`: 커널 객체 (sysfs 연계)  
  - `struct module *owner`: 참조 카운트 유지 (`THIS_MODULE`)  

---

### 2. `struct class`
- 관련 장치들을 **논리적으로 묶는 컨테이너**
- `/sys/class/<name>/` 경로에 노출  
- `class_create()`로 생성하며, `device_create()` 호출 시 자동으로 `/dev` 노드 생성
- 주요 필드  
  - `const char *name`: 클래스 이름  
  - `devnode()`: 생성될 `/dev` 노드의 이름·권한 결정  
  - `dev_release()`: 디바이스 해제 시 콜백

---

### 3. `struct device`
- 리눅스 장치 모델의 핵심 객체.  
  **실제 하드웨어나 논리 장치**를 표현하며, `/sys/devices/...` 및 `/sys/class/...`에 나타난다.
- 주요 필드  
  - `dev_t devt`: 장치 번호  
  - `struct class *class`: 속한 클래스  
  - `const char *init_name`: 기본 장치 이름  
  - `void (*release)(struct device *dev)`: 해제 콜백 (필수 구현)
- `device_create()`로 생성되며, **sysfs 노출 + `/dev` 노드 자동 생성** 기능을 수행한다.

---

## 코드 동작 요약

1. **모듈 로드**
   - Major/Minor 번호 할당  
   - `cdev` 등록  
   - `class_create()`로 클래스 생성  
   - `device_create()`로 `/dev/hello0` 자동 생성  
   - 커널 로그에 major/minor 출력  

2. **파일 입출력**
   - `echo` 또는 `cat` 명령을 통해 `read()` / `write()` 테스트 가능  
   - `my_buf[256]`에 데이터 저장 및 반환  

3. **모듈 제거**
   - `device_destroy()`로 `/dev/hello0` 제거  
   - `class_destroy()`로 클래스 해제  
   - `cdev_del()` + `unregister_chrdev_region()`로 정리  

---

## 실행 예시

```bash
$ make
$ sudo insmod hello.ko
$ dmesg | tail -n 1
[1234.567890] hello - created: major=236 minor=0 (/dev/hello0)

$ ls /dev/hello*
/dev/hello0

$ echo "hello world" > /dev/hello0
$ cat /dev/hello0
hello world

$ sudo rmmod hello
$ dmesg | tail -n 1
[1235.123456] hello - unloaded
```

---

## 정리

이번 예제에서는 문자 디바이스 드라이버를 커널 디바이스 모델에 완전히 통합했다.

- `cdev` → 파일 연산과 연결  
- `class` → 논리적 그룹(`/sys/class`) 생성  
- `device` → 실제 `/dev` 노드 자동 생성  

이를 통해 **sysfs, udev, devtmpfs**와의 연계가 가능해지고,  
수동 `mknod` 없이도 장치 노드가 자동 관리된다.  

이 방식이 **현대 리눅스 커널의 표준 구조**.
