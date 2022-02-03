void main() {
    u32 a = 1;
    print(a.toFixed(1));
    array<i32> arr;
    for (u32 i = 0;i < 20;i++) arr.push(i);

    // test

    /*
     * test
     */

    /* test */

    i32 len = arr.length;
    for (u32 i = 0;i < len;i++) {
        print('arr[' + i.toFixed(1) + ']: ' + arr[i].toFixed(2));
    }
}

main();