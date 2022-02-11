tmp_file='tmp.txt'
gpio_rst_file='gpio_rst_file.log'
if [ "$1" != "" ]
then
  gpio_rst_file='ref_gpio_rst_file.log'
fi

if [ -e $tmp_file ]
then
  rm $tmp_file
fi

if [ -e *.log ]
then
  rm *.log
fi

arr=$(find /sys/class/gpio/ -mindepth 1 -maxdepth 1 -name 'gpio*' ! -name '*chip*')
for i in $arr; do
  echo $i >> $tmp_file
done

echo $(cat /etc/issue) > $gpio_rst_file
while IFS='' read -r line || [[ -n "$line" ]]; do
  echo -n "$line " >> $gpio_rst_file
  echo $(cat $line/direction) $(cat $line/value) >> $gpio_rst_file
done < "$tmp_file"

if [ -e $tmp_file ]
then
    rm $tmp_file
fi
