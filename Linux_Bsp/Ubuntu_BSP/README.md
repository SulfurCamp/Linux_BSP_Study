# Ubuntu_BSP: Ubuntu Device Driver Development & System Programming

이 폴더는 Ubuntu 환경에서 리눅스 디바이스 드라이버 개발 및 관련 시스템 프로그래밍 예제들을 포함합니다. 주로 리눅스 커널 모듈 개발, 커널과 사용자 공간 간의 통신, 그리고 다양한 하드웨어 제어 기법을 학습하는 데 중점을 둡니다. `p`로 시작하는 폴더 이름은 특정 교재의 페이지 번호나 챕터를 나타낼 가능성이 높습니다.

## 학습 가능한 주요 개념

*   **리눅스 디바이스 드라이버 개발:** 리눅스 커널 모듈(`.ko` 파일) 형태로 디바이스 드라이버를 작성하고 로드하는 방법.
*   **커널-사용자 공간 통신:**
    *   **`ioctl`:** 사용자 공간 애플리케이션이 커널 드라이버에 명령을 전달하고 데이터를 교환하는 표준 방법.
    *   **블로킹/논블로킹 I/O:** 드라이버에서 데이터가 준비될 때까지 대기하거나 즉시 반환하는 I/O 방식.
    *   **폴링 (Poll):** 여러 파일 디스크립터의 이벤트를 동시에 감지하는 메커니즘.
    *   **`/proc` 파일 시스템:** 커널 내부 정보를 사용자 공간에서 접근하고 제어하는 가상 파일 시스템.
*   **하드웨어 제어:** LED, 키패드, FND(Flexible Numeric Display) 등 주변 장치 제어.
*   **커널 타이머:** 커널 내에서 주기적인 작업을 수행하기 위한 타이머 사용.
*   **인터럽트 처리:** 하드웨어 인터럽트를 이용한 비동기 이벤트 처리.
*   **메모리 할당:** 커널 공간에서의 메모리 할당 (`kmalloc`).

## 폴더 구조 및 주요 파일 설명

이 폴더는 다양한 디바이스 드라이버 예제와 이를 테스트하는 사용자 공간 애플리케이션 소스 코드를 포함합니다.

*   **`pXXX_`로 시작하는 폴더들:** 특정 교재의 페이지 번호 또는 챕터에 해당하는 예제들입니다.
    *   **`ledkey` 관련:** LED 및 키패드 제어 드라이버 및 애플리케이션.
        *   `p106_ledkey`, `p106_ledkey_ksh`, `p184_ledkey`, `p184_ledkey_0620`, `p184_ledkey_ksh`, `p238_ledkey_array`: 기본적인 LED/키패드 제어, 배열을 이용한 처리 등.
        *   `p306_ledkey_ioctl_rw`, `p306_ledkey_ioctl_rw_LEDKEY`, `p306_ledkey_ioctl_rw_LEDKEY_ksh`: `ioctl`을 이용한 LED/키패드 제어.
        *   `p369_ledkey_int`, `p369_ledkey_int_kmalloc`, `p369_ledkey_int_kmalloc_hjy`, `p369_ledkey_int_kmalloc_ksh`, `p369_ledkey_int_ksh`: 인터럽트 기반 LED/키패드 제어 및 커널 메모리 할당 (`kmalloc`).
        *   `p399_ledkey_blockio`, `p399_ledkey_blockio_ksh`: 블로킹 I/O를 사용하는 LED/키패드 드라이버.
        *   `p432_ledkey_poll`, `p432_ledkey_poll_builtin`: 폴링(poll) 메커니즘을 사용하는 LED/키패드 드라이버.
        *   `p527_ledkey_proc`: `/proc` 파일 시스템을 이용한 LED/키패드 제어.
    *   **`fnd_4digit`:** 4자리 FND(Flexible Numeric Display) 제어 드라이버.
    *   **`calldev` 관련:** `call_dev` 또는 유사한 이름의 디바이스 드라이버.
        *   `p184_calldev`, `p184_calldev_ledkey`, `p184_calldev_ledkey_backup`: `call_dev` 드라이버 및 LED/키패드와의 연동.
    *   **`kerneltimer` 관련:** 커널 타이머를 활용한 드라이버.
        *   `p335_kerneltimer`, `p335_kerneltimer_dev`.
*   **개별 `.c` 파일:**
    *   `call_app.c`, `call_ledkey_app.c`: 해당 드라이버를 테스트하는 사용자 공간 애플리케이션.
    *   `cmd_kcci_led.c`: KCCI LED 제어 명령어.
    *   `hello.c`: 기본적인 "Hello World" 커널 모듈 또는 사용자 공간 프로그램.
    *   `kerneltimer_led.c`: 커널 타이머와 LED 제어 관련 코드.
    *   `syscall_app.c`, `test_mysyscall_ledkey.c`: 시스템 콜 관련 예제.
*   **`Makefile`**: 커널 모듈 또는 애플리케이션을 빌드하기 위한 스크립트.

## 빌드 및 실행 방법 (일반적인 절차)

1.  **커널 모듈 빌드:** 각 드라이버의 소스 코드(`.c` 파일)를 컴파일하여 `.ko` 파일을 생성합니다. 이를 위해서는 Ubuntu 환경에 리눅스 커널 헤더 파일 및 빌드 도구가 설치되어 있어야 합니다.
    ```bash
    # 드라이버 소스 디렉토리로 이동
    make
    ```
2.  **커널 모듈 로드:** 생성된 `.ko` 파일을 `insmod` 명령어를 사용하여 커널에 로드합니다.
    ```bash
    sudo insmod <드라이버_이름>.ko
    ```
3.  **디바이스 노드 생성:** 드라이버가 `/dev` 디렉토리에 디바이스 노드를 생성하지 않는 경우, `mknod` 명령어를 사용하여 수동으로 생성해야 할 수 있습니다.
    ```bash
    sudo mknod /dev/<디바이스_이름> c <메이저_번호> <마이너_번호>
    ```
4.  **애플리케이션 컴파일 및 실행:** 사용자 공간 애플리케이션(`.c` 파일)을 컴파일하고 실행하여 드라이버의 기능을 테스트합니다.
    ```bash
    gcc -o <애플리케이션_이름> <애플리케이션_소스>.c
    ./<애플리케이션_이름>
    ```
5.  **커널 모듈 언로드:** 테스트가 끝나면 `rmmod` 명령어를 사용하여 드라이버를 언로드합니다.
    ```bash
    sudo rmmod <드라이버_이름>
    ```

## 참고

이 폴더의 예제들은 Ubuntu 환경의 특정 리눅스 커널 버전에 따라 동작 방식이 달라질 수 있습니다. 각 예제에 대한 상세한 설명은 해당 소스 코드 내의 주석이나 별도의 문서에서 찾아볼 수 있습니다.
