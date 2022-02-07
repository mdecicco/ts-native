void main() {
    if (process.raw_argc < 1) {
        print ('Please provide a number argument');
        process.exit(-1);
    }
    string num = process.get_raw_arg(0);

    i32 count = i32.parse(num);
    print('u8: ' + u8.parse(num).toFixed(0));
    print('u16: ' + u16.parse(num).toFixed(0));
    print('u32: ' + u32.parse(num).toFixed(0));
    print('u64: ' + u64.parse(num).toFixed(0));
    print('i8: ' + i8.parse(num).toFixed(0));
    print('i16: ' + i16.parse(num).toFixed(0));
    print('i32: ' + i32.parse(num).toFixed(0));
    print('i64: ' + i64.parse(num).toFixed(0));
    print('f32: ' + f32.parse(num).toFixed(6));
    print('f64: ' + f64.parse(num).toFixed(6));

    /*
    if (count < 0) {
        print('Positive numbers only pls');
        process.exit(-2);
    }

    if (count.isNaN) {
        print('Failed to parse argument');
        process.exit(-3);
    }

    i32[] arr;
    for (u32 i = 0;i < count;i++) arr.push(i);

    i32 len = arr.length;
    for (u32 i = 0;i < len;i++) {
        print('arr[' + i.toFixed(0) + ']: ' + arr[i].toFixed(0));
    }
    
    print(process.env('test'));
    */
}

main();
