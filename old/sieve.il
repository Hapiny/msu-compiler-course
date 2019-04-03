export function w $sieve(w %n, l %prime) {
@start
    %cond =w csgtw %n, 0
    jnz %cond, @cond1t, @cond1f
@cond1t
    %prime_iter =l add %prime, 0
    storeb 0, %prime_iter
@cond1f
    %cond =w csgtw %n, 1
    jnz %cond, @cond2t, @cond2f
@cond2t
    %prime_iter =l add %prime, 1
    storeb 0, %prime_iter
@cond2f
    %i =w add 0, 2
@loop1s
    %cond =w csltw %i, %n
    jnz %cond, @loop1_body, @loop1e
@loop1_body
    %i_long =l extsw %i
    %prime_iter =l add %prime, %i_long
    storeb 1, %prime_iter
    %i =w add %i, 1
    jmp @loop1s
@loop1e
    %i =w add 0, 2
@loop2s
    %cond =w csltw %i, %n
    jnz %cond, @loop2_body, @loop2e
@loop2_body
    %i_long =l extsw %i
    %prime_iter =l add %prime, %i_long
    %prime_i =w loadsb %prime_iter
    jnz %prime, @loop2_1, @loop2_inc
@loop2_1
    %j =w mul %i, %i
@loop2_1s
    %cond =w csltw %j, %n
    jnz %cond, @loop2_1_body, @loop2_inc
@loop2_1_body
    %j_long =l extsw %j
    %prime_iter_j =l add %prime, %j_long
    storeb 0, %prime_iter_j
    %j =w add %j, %i
    jmp @loop2_1s
@loop2_inc
    %i =w add %i, 1
    jmp @loop2s
@loop2e
    ret
}