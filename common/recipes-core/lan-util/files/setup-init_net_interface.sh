#!/bin/sh

INITD="/etc/init.d"
IF_DIR="/mnt/data/etc/network"
IF_CONF="interfaces"

IF_UPDATE_CONF="$IF_DIR/$IF_CONF"
IF_TARGET_CONF="/etc/network/interfaces"
IF_UP="/etc/network/if-up.d"
IF_DOWN="/etc/network/if-down.d"

SV_PREFIX="/etc/sv/dhc6_"
RUN_SV_PREFIX="runsv $SV_PREFIX"

function init_conf()
{
  if [ ! -d $IF_DIR ];
  then
    mkdir -p $IF_DIR
  fi

  if_name=""
  if [ ! -f $IF_UPDATE_CONF ];
  then
    while IFS='' read -r line || [[ -n "$line" ]];
    do
      if ! [[ $line = \#* ]];
      then
        echo "$line" >> $IF_UPDATE_CONF
        if [[ $line = iface* ]];
        then
          if_name=$(echo $line | awk -F' ' '{print $2}')
          if [[ $if_name = eth* ]];
          then
            echo -e "\niface $if_name inet6 dhcp" >> $IF_UPDATE_CONF
          fi
        fi
      fi
    done < "$IF_TARGET_CONF"
  fi
}

function update_rc5()
{
  out="setup-dhc6_$1.sh"
  echo "#!/bin/sh" >> $out
  echo "echo -n \"Setup the dhclient of $1 for IPv6...\"" >> $out
  echo "$RUN_SV_PREFIX$1 &>/dev/null &" >> $out
  echo "echo \"done.\"" >> $out
  chmod 755 $out
  mv $out $INITD
  update-rc.d -f $out remove &>/dev/null
  update-rc.d $out start 2 5 S . &>/dev/null
}

function update_sv()
{
  folder="$SV_PREFIX$1"
  out="$folder/run"

  if [ ! -d "$folder" ];
  then
    mkdir -p $folder
  fi

  cat /dev/null > $out
  echo "#!/bin/sh" >> $out
  echo "pid=\"/var/run/dhclient6.$1.pid\"" >> $out
  echo "exec dhclient -6 -d -D LL -pf \${pid} $1 \"\$@\"" >> $out
  chmod 755 $out
}

function update_ifupdown()
{
  #up
  out="$IF_UP/dhcpv6_$1_up"
  cat /dev/null > $out
  echo "#!/bin/bash" >> $out
  echo "if [ \"\$IFACE\" == \"$1\" ]; then" >> $out
  echo "  sv status dhc6_$1 &>/dev/null &&\\" >> $out
  echo "  sv restart dhc6_$1" >> $out
  echo "fi" >> $out
  echo "exit 0" >> $out
  chmod 755 $out

  #down
  out="$IF_DOWN/dhcpv6_$1_down"
  cat /dev/null > $out
  echo "#!/bin/bash" >> $out
  echo "if [ \"\$IFACE\" == \"$1\" ]; then" >> $out
  echo "  sv stop dhc6_$1 $> /dev/null &" >> $out
  echo "fi" >> $out
  chmod 755 $out
}

function deploy_setting()
{
  update_rc5 $1
  update_sv $1
  update_ifupdown $1
}

function update_conf()
{
  #clear the old content
  cat /dev/null > $IF_TARGET_CONF

  #Update the new content
  i=0
  vlan_id=""
  record_vlan=""
  while IFS='' read -r line || [[ -n "$line" ]];
  do
    interface=""
    if_family=""
    method=""

    if [[ $line = iface* ]];
    then
      interface=$(echo $line | awk -F' ' '{print $2}')
      if_family=$(echo $line | awk -F' ' '{print $3}')
      method=$(echo $line | awk -F' ' '{print $4}')
      if [ "$record_vlan" == "" ];
      then
        record_vlan=$(echo $interface | awk -F'.' '{print $2}')
      fi
      
      if ! [[ $interface = lo* ]];
      then
        /sbin/ifconfig $interface 0 > /dev/null 2>&1
      fi
    fi

    if ! [ "$record_vlan" == "$vlan_id" ] && [[ $interface = eth* ]];
    then
      actual_if=$(echo $interface | awk -F'.' '{print $1}')
      /sbin/vconfig add $actual_if $record_vlan
      vlan_id=$record_vlan
    fi

    if [ "$if_family" == "inet6" ] && [ "$method" == "dhcp" ];
    then
      deploy_setting $interface
    else
      if [ "$line" == "" ];
      then
        ((i++))
      else
        i=0
      fi

      if [ $i -lt 2 ];
      then
        echo "$line" >> $IF_TARGET_CONF
      fi
    fi
  done < "$IF_UPDATE_CONF"
}

function clear_dhc6()
{
  SV_PATH="/etc/sv"
  for file in $SV_PATH/*
  do
    if [[ $file = $SV_PATH/dhc6* ]];
    then
      #stop service
      sv_name=$(echo $file | awk -F'/' '{print $4}')
      /usr/bin/sv stop $sv_name $> /dev/null

      #kill runsv id
      sv_id=$(ps | grep "runsv /etc/sv/$sv_name\$" | head -n 1 | awk -F' ' '{print $1}')
      /bin/kill -9 $sv_id > /dev/null 2>&1

      #rm the dir
      rm -rf $file
    fi
  done

  rm -rf $IF_UP/dhcpv6_*
  rm -rf $IF_DOWN/dhcpv6_*
  rm -rf $INITD/setup-dhc6_*
  rm -rf /etc/rc5.d/S02setup-dhc6_*
}

function clear_vlan()
{
  list=$(ip a | grep @ | cut -d ":" -f 2)
  IFS=' '
  for intf in $list;
  do
    if [ "$intf" == "" ];
    then
      continue
    fi

    intf=$(echo $intf | tr -d '\040\011\012\015')

    vintf=$(echo $intf | cut -d "@" -f 1)
    intf=$(echo $intf | cut -d "@" -f 2 | awk '{print tolower($0)}')

    if [ "$intf" == "none" ];
    then
      continue
    fi

    /sbin/vconfig rem $vintf
  done
}

function run_dhc6()
{
  RC5D="/etc/rc5.d"
  for file in $RC5D/*
  do
    if [[ $file = $RC5D/S02setup-dhc6* ]];
    then
      sh $file $> /dev/null
    fi
  done
}

if [ "$1" == "start" ];
then
  init_conf
  update_conf
elif [ "$1" == "restart" ];
then
  /sbin/ifdown -a
  clear_vlan
  clear_dhc6
  init_conf
  update_conf
  /sbin/ifup -a
  run_dhc6
fi

