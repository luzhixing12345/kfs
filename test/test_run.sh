
# run test from 000-xxx.sh to 999-xxx.sh

function e4test_run {
    local test=$1
    . $test
}

function e4test_end {
    echo
    echo "Test $1: $TIMING_DIFF_MSECS.$TIMING_DIFF_NSECS ms"
}

function e4test_init {
    echo -n "Test $1: "
}

function e4test_declare_slow {
    if [ -n "$SKIP_SLOW_TESTS" ] ; then
        echo ": SKIPPED"
        exit 0
    fi
}


