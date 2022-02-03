void main() {
    string arg_str;
    for (u32 i = 0;i < process.raw_argc;i++) {
        if (i > 0) arg_str += ' ';
        arg_str += process.get_raw_arg(i);
    }
    print('args: ' + arg_str);

    for (u32 i = 0;i < process.argc;i++) {
        string name = process.get_arg(i).name;
        string value = process.get_arg(i).value;
        print((name.length > 0 ? name + (value.length > 0 ? ' : ' : '') : '') + value);
    }
}

main();