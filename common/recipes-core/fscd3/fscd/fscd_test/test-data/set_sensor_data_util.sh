file_name=$0
pwm_value=$1
pwm_num=$2
echo $pwm_value > ./test-data/test-util-data/fan$pwm_num-set-data
