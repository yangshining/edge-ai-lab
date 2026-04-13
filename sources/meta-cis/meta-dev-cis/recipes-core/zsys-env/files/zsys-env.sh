
#!/bin/sh

run() {
    $@ > /dev/null 2> /dev/null
    rc=$?
    if [ $rc -ne 0 ]; then
        string="zsys-env failed ($rc): $@"
        echo $string >>/dev/ttyPS0
    fi
}
printer() {
    string="zsys-env: $@"
    echo $string >>/dev/ttyPS0
}

printer "-------init starting...."
exit 0

