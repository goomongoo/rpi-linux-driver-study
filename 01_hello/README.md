# 01_hello

간단한 Hello World 리눅스 커널 모듈 예제

---

## 커널 모듈이란

커널 모듈(kernel module)은 커널의 기능을 동적으로 확장하는 코드 조각이다.  
운영체제를 재부팅하거나 커널 전체를 다시 빌드하지 않고도, 새로운 기능(ex. 디바이스 드라이버)을 추가할 수 있다.  

즉, 커널 모듈은 **필요할 때 로드하고, 필요 없을 때 제거할 수 있는 플러그인 형태의 커널 코드**다.  
확장성과 유지보수성 면에서 유용하며, 드라이버 개발의 기본 단위로 자주 사용된다.

---

## Hello World 커널 모듈

이 예제는 모듈이 **로드될 때**와 **제거될 때** 메시지를 출력하는 단순한 코드다.  
이 두 시점에 호출되는 함수를 `module_init`과 `module_exit` 매크로로 지정한다.

### 초기화 함수 (init)

모듈이 로드될 때 호출된다.

~~~c
static int __init my_init(void)
{
    pr_debug("%s - Hello, Kernel!\n", __func__);
    return 0;
}
~~~

- 반환값이 0이면 성공, 음수이면 로드 실패를 의미한다.  
- `pr_debug()`는 `printk()`의 디버그 버전으로, 메시지를 커널 로그에 출력한다.  
- `__func__`는 현재 함수 이름을 문자열로 나타낸다.

---

### 종료 함수 (exit)

모듈이 제거될 때 호출된다.

~~~c
static void __exit my_exit(void)
{
    pr_debug("%s - Goodbye, Kernel!\n", __func__);
}
~~~

- 반환값과 인자가 없다.  
- 보통 여기서 `init`에서 할당한 자원(메모리, 인터럽트 등)을 해제한다.

---

### 모듈 등록

~~~c
module_init(my_init);
module_exit(my_exit);
~~~

이 두 매크로가 커널에 init과 exit 함수를 등록한다.  
모듈이 삽입될 때 `my_init()`이, 제거될 때 `my_exit()`이 자동 실행된다.

---

### 모듈 메타데이터

~~~c
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("ssafy");
MODULE_DESCRIPTION("A simple Hello World LKM");
~~~

- **MODULE_LICENSE**: GPL v2 라이선스를 사용한다.  
  (일부 배포판은 비공개 라이선스 모듈 로드를 막는다)  
- **MODULE_AUTHOR**: 작성자 정보  
- **MODULE_DESCRIPTION**: 모듈 설명

---

## Makefile

커널 모듈은 일반 GCC 빌드와 달리 커널 빌드 시스템을 이용한다.

~~~makefile
obj-m += hello.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
~~~

- `obj-m += hello.o`: hello.c를 커널 모듈로 빌드  
- `make -C`: 현재 커널 버전의 빌드 환경 사용  
- `M=$(PWD)`: 현재 디렉터리 경로 지정  
- `all`: 모듈 빌드  
- `clean`: 빌드 산출물 삭제

---

## 모듈 관리 명령어

- **커널 로그 확인**
  ```bash
  sudo dmesg -W
  ```

- **모듈 삽입**
  ```bash
  sudo insmod hello.ko
  ```

- **모듈 제거**
  ```bash
  sudo rmmod hello.ko
  ```

삽입하거나 제거할 때, `pr_debug()`로 출력된 메시지가 `dmesg`에서 확인된다.

---

## 실행 예시

```bash
$ sudo insmod hello.ko
$ dmesg | tail -n 1
[12345.678901] my_init - Hello, Kernel!

$ sudo rmmod hello.ko
$ dmesg | tail -n 1
[12346.123456] my_exit - Goodbye, Kernel!
```
