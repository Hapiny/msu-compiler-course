# file: my-loop.il
data $fmt = { b "%d\n", b 0 }  # fmt = "%d\n";

export function w $main() {
@start
    %x =w add 0, 5             # x = 5;
@loop
    %x =w mul %x, %x           # x = x * x;
    %cond =w csgew %x, 200     # cond = (x >= 200); // csgew = compare signed greater-equal word
    jnz %cond, @end, @loop     # if (cond) goto @end; else goto @loop;
@end
    %t =w call $printf(l $fmt, w %x)
    ret 0
}
