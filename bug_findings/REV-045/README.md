# REV-045 reproduction

On an x86-64 System V host:

```sh
gcc -no-pie descend_calc/descend_calc.s -o /tmp/rev-045
printf '10\n2\n1\n' | /tmp/rev-045
```

The source declares a Windows x64 ABI entry point but is otherwise assembled as a host object. Even under its intended ABI, the wind value is saved in `%edi` and then overwritten by the direction input; the result uses the direction (0/1), not the supplied wind speed.
